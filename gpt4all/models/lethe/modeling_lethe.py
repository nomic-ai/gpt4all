# coding=utf-8
# Copyright 2022 EleutherAI The HuggingFace Inc. team. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
""" PyTorch Lethe model."""

import wandb
import math
import torch.nn.functional as F
import matplotlib.pyplot as plt
from typing import Optional, Tuple, Union

import torch
import torch.utils.checkpoint
from torch import nn
from torch.nn import CrossEntropyLoss

from transformers.activations import ACT2FN
from transformers.modeling_outputs import (
    BaseModelOutputWithPast,
    CausalLMOutputWithPast,
)
from transformers.modeling_utils import PreTrainedModel
from transformers.utils import logging
from gpt4all.models.lethe import LetheConfig
import numpy as np
import faiss
import faiss.contrib.torch_utils


logger = logging.get_logger(__name__)


GPT_NEOX_PRETRAINED_MODEL_ARCHIVE_LIST = [
    "EleutherAI/gpt-neox-20b",
]


class HNSWIndex:
    def __init__(self, max_memories, dimension):
        # num_memories will be batch size * num_neighbors
        # can memmap this too like
        self.index = faiss.IndexHNSWFlat(dimension, 16, faiss.METRIC_INNER_PRODUCT)
        # taking params from: https://www.pinecone.io/learn/vector-indexes/#hnsw-implementation
        # and https://www.pinecone.io/learn/hnsw/#hnsw-performance
        # seems like efConstruction dictates how long the index takes to build
        # and efSearch and M (second arg to faiss.Index) dictates how long it takes to search
        self.index.hnsw.efConstruction = 16
        self.index.hnsw.efSearch = 32   
        self.max_memories = max_memories
        self.dimension = dimension

        # if we want to allow for insertion of len(elements) > max_memories
        # we need to figure out a way to get the most recent memories
        self.idx_offset = 0

    def query(self, query, k=1):
        # hack what should we do here?
        if self.index.ntotal == 0:
            return np.ones((query.shape[0], k), dtype=np.int32)

        _, labels = self.index.search(np.ascontiguousarray(query), k=k)

        return labels

    def add(self, memories):
        return self.index.add(np.ascontiguousarray(memories))

            
    def reset(self):
        self.index.reset()

        
class NumpyKNNIndex:
    def __init__(self, max_memories, dimension):
        # num_memories will be batch size * num_neighbors
        # can memmap this too like
        self.index = np.zeros((max_memories, dimension), dtype=np.float32)
        self.max_memories = max_memories
        self.dimension = dimension

        # if we want to allow for insertion of len(elements) > max_memories
        # we need to figure out a way to get the most recent memories
        self.idx_offset = 0

    def query(self, query, k=1):
        # hack what should we do here?
        if self.index.sum() == 0:
            return np.ones((query.shape[0], k), dtype=np.int32)

        dots = query.dot(self.index[:self.idx_offset].T)
        labels = np.argsort(dots, axis=1)[:, -k:]

        return labels


    def add(self, memories):
        self.index[self.idx_offset:self.idx_offset + memories.shape[0]] = memories
        self.idx_offset += memories.shape[0]

            
    def reset(self):
        self.index.reset()



class MemoryIndex:
    def __init__(self, hidden_dim, num_mems, nheads):
        self.nheads = nheads

        # NOTE: we are storing kv pairs, instead indices for both keys and values
        self.key_indices = [HNSWIndex(num_mems, hidden_dim) for _ in range(nheads)]

        shape = (num_mems, nheads, 2, hidden_dim)
        self.kv_pairs = np.zeros(shape, dtype=np.float32)
        self.idx_offset = 0

    def add(self, keys, values):
        # k/v are (bs, num_attention_heads, seq_len, head_size)
        reshaped_keys = keys.reshape(keys.shape[0] * keys.shape[2], keys.shape[1], keys.shape[3])
        reshaped_values = values.reshape(values.shape[0] * values.shape[2], values.shape[1], values.shape[3])

        for head in range(self.nheads):
            self.key_indices[head].add(reshaped_keys[:, head, :])

        kv_pairs = np.stack((reshaped_keys, reshaped_values), axis=2)

        if self.idx_offset + kv_pairs.shape[0] > self.kv_pairs.shape[0]:
            raise ValueError("Not enough memory!")

        self.kv_pairs[self.idx_offset:self.idx_offset + kv_pairs.shape[0]] = kv_pairs
        self.idx_offset += kv_pairs.shape[0]

    def knn_query(self, query, k=1):
        reshaped_query = query.reshape(query.shape[0] * query.shape[2], query.shape[1], query.shape[3])

        mem_keys = []
        mem_values = []
        mem_indices = []

        # we can prob make this better
        for head in range(self.nheads):
            knn_indices = self.key_indices[head].query(reshaped_query[:, head, :], k=k)
            kv_pairs = self.kv_pairs[:, head, :, :][knn_indices]
            
            mem_keys.append(kv_pairs[:, :, 0, :])
            mem_values.append(kv_pairs[:, :, 1, :])
            mem_indices.append(knn_indices)

        mem_keys = torch.from_numpy(np.stack(mem_keys, axis=1))
        # (bs, num_attention_heads, seq_len, k, head_size)
        mem_keys = mem_keys.view(query.shape[:-1] + (k,) + (query.shape[-1],))

        mem_values = torch.from_numpy(np.stack(mem_values, axis=1))
        # (bs, num_attention_heads, seq_len, k, head_size)
        mem_values = mem_values.view(query.shape[:-1] + (k,) + (query.shape[-1],))

        return mem_keys, mem_values, np.stack(mem_indices, axis=1)
        

    def reset(self):
        for head in range(self.nheads):
            self.key_indices[head].reset()

        self.kv_pairs = np.zeros((self.kv_pairs.shape[0], self.nheads, 2, self.kv_pairs.shape[-1]), dtype=np.float32)


class LethePreTrainedModel(PreTrainedModel):
    """
    An abstract class to handle weights initialization and a simple interface for downloading and loading pretrained
    models.
    """

    config_class = LetheConfig
    base_model_prefix = "gpt_neox"
    supports_gradient_checkpointing = True
    _no_split_modules = ["PythiaSeekLayer"]

    def _init_weights(self, module):
        """Initialize the weights"""
        if isinstance(module, nn.Linear):
            module.weight.data.normal_(mean=0.0, std=self.config.initializer_range)
            if module.bias is not None:
                module.bias.data.zero_()
        elif isinstance(module, nn.Embedding):
            module.weight.data.normal_(mean=0.0, std=self.config.initializer_range)
            if module.padding_idx is not None:
                module.weight.data[module.padding_idx].zero_()
        elif isinstance(module, nn.LayerNorm):
            module.bias.data.zero_()
            module.weight.data.fill_(1.0)

    def _set_gradient_checkpointing(self, module, value=False):
        if isinstance(module, LetheModel):
            module.gradient_checkpointing = value


class LetheAttention(nn.Module):
    def __init__(self, config, memory_attention=False, index=None, tracker=None):
        super().__init__()
        self.num_attention_heads = config.num_attention_heads
        self.hidden_size = config.hidden_size
        self.head_size = self.hidden_size // self.num_attention_heads
        self.rotary_ndims = int(self.head_size * config.rotary_pct)
        max_positions = config.max_position_embeddings
        self.register_buffer(
            "bias",
            torch.tril(torch.ones((max_positions, max_positions), dtype=torch.bool)).view(
                1, 1, max_positions, max_positions
            ),
        )
        self.register_buffer("masked_bias", torch.tensor(-1e9))
        self.rotary_emb = RotaryEmbedding(
            self.rotary_ndims, config.max_position_embeddings, base=config.rotary_emb_base
        )
        if not memory_attention:
            self.register_buffer(
            "norm_factor",
            torch.sqrt(torch.tensor(self.head_size, dtype=torch.float32)).to(torch.get_default_dtype()),
            persistent=False,
        )

        self.query_key_value = nn.Linear(config.hidden_size, 3 * config.hidden_size)
        self.dense = nn.Linear(config.hidden_size, config.hidden_size)
        self.memory = False
        
        if memory_attention:
            self.scale = nn.Parameter(torch.ones(self.num_attention_heads, 1, 1) * math.log(config.attn_scale_init))
            self.memory = True
            self.num_neighbors = config.num_neighbors_to_retrieve
            # for testing, just using np array since it's easy
            self.index = index
            self.tracker = tracker


    def forward(
        self,
        hidden_states: torch.FloatTensor,
        attention_mask: torch.FloatTensor,
        position_ids: torch.LongTensor,
        head_mask: Optional[torch.FloatTensor] = None,
        layer_past: Optional[Tuple[torch.Tensor]] = None,
        use_cache: Optional[bool] = False,
        output_attentions: Optional[bool] = False,
        log_attn_scores: Optional[bool] = False,
        step: Optional[int] = None,
        save_kv: Optional[bool] = True,
    ):
        has_layer_past = layer_past is not None

        # Compute QKV
        # Attention heads [batch, seq_len, hidden_size]
        #   --> [batch, seq_len, (np * 3 * head_size)]
        _, seq_len, _ = hidden_states.size()
        qkv = self.query_key_value(hidden_states)

        # [batch, seq_len, (num_heads * 3 * head_size)]
        #   --> [batch, seq_len, num_heads, 3 * head_size]
        new_qkv_shape = qkv.size()[:-1] + (self.num_attention_heads, 3 * self.head_size)
        qkv = qkv.view(*new_qkv_shape)

        # [batch, seq_len, num_attention_heads, 3 * head_size] --> 3 [batch, num_attention_heads, seq_len, head_size]
        query = qkv[..., : self.head_size].permute(0, 2, 1, 3)
        key = qkv[..., self.head_size : 2 * self.head_size].permute(0, 2, 1, 3)
        value = qkv[..., 2 * self.head_size :].permute(0, 2, 1, 3)

        # if self.memory:
        if self.memory:
            # QKNorm: https://arxiv.org/abs/2010.04245
            query = F.normalize(query, dim=-1)
            key = F.normalize(key, dim=-1)

        # Compute rotary embeddings on rotary_ndims
        query_rot = query[..., : self.rotary_ndims]
        query_pass = query[..., self.rotary_ndims :]
        key_rot = key[..., : self.rotary_ndims]
        key_pass = key[..., self.rotary_ndims :]

        # Compute token offset for rotary embeddings (when decoding)
        seq_len = key.shape[-2]
        if has_layer_past:
            seq_len += layer_past[0].shape[-2]
        cos, sin = self.rotary_emb(value, seq_len=seq_len)
        query, key = apply_rotary_pos_emb(query_rot, key_rot, cos, sin, position_ids)
        query = torch.cat((query, query_pass), dim=-1)
        key = torch.cat((key, key_pass), dim=-1)

        # Cache QKV values
        if has_layer_past:
            past_key = layer_past[0]
            past_value = layer_past[1]
            key = torch.cat((past_key, key), dim=-2)
            value = torch.cat((past_value, value), dim=-2)
        present = (key, value) if use_cache else None

        # memory attention
        if self.memory:
            if save_kv:
                self.index.add(key.cpu().detach().to(torch.float32).numpy(), value.cpu().detach().to(torch.float32).numpy())

            knn_keys, knn_values, knn_labels = self.index.knn_query(query.cpu().detach().to(torch.float32).numpy(), k=self.num_neighbors)
            if log_attn_scores:
                batch_size = query.shape[0]
                seq_len = query.shape[-2]

                key_labels = knn_labels // seq_len
                key_labels = key_labels.reshape(batch_size, seq_len, self.num_attention_heads, -1)
                correct_keys = np.equal(key_labels, np.arange(batch_size)[:, np.newaxis, np.newaxis, np.newaxis])
                # calculate the accuracy 
                key_acc = np.sum(correct_keys) / np.prod(correct_keys.shape)

                self.tracker.log({"retrieved_acc": key_acc}, step=step)

            attn_output = self._mem_attn(query,
                                    knn_keys.to(query.device).to(value.dtype),
                                    knn_values.to(query.device).to(value.dtype),
                                    key,
                                    value,
                                    attention_mask, 
                                    head_mask,
                                    log_attn_scores=log_attn_scores,
                                    step=step,
                                    knn_labels=knn_labels,
                                    )
        else:
            # Normal self-attention
            attn_output, attn_weights = self._attn(query, key, value, attention_mask, head_mask)

        # Reshape outputs
        attn_output = self._merge_heads(attn_output, self.num_attention_heads, self.head_size)
        attn_output = self.dense(attn_output)

        outputs = (attn_output, present)
        if output_attentions:
            outputs += (attn_weights,)

        return outputs

    @classmethod
    def _split_heads(cls, tensor, num_attention_heads, attn_head_size):
        """
        Splits hidden dim into attn_head_size and num_attention_heads
        """
        # tensor: [bs, seq_len, hidden_size]
        new_shape = tensor.size()[:-1] + (num_attention_heads, attn_head_size)
        # -> [bs, seq_len, num_attention_heads, attn_head_size]
        tensor = tensor.view(new_shape)
        # -> [bs, num_attention_heads, seq_len, attn_head_size]
        tensor = tensor.permute(0, 2, 1, 3)
        return tensor

    @classmethod
    def _merge_heads(cls, tensor, num_attention_heads, attn_head_size):
        """
        Merges attn_head_size dim and num_attn_heads dim into hidden dim
        """
        # tensor [bs, num_attention_heads, seq_len, attn_head_size]
        tensor = tensor.permute(0, 2, 1, 3).contiguous()
        # -> [bs, seq_len, num_attention_heads, attn_head_size]
        tensor = tensor.view(tensor.size(0), tensor.size(1), num_attention_heads * attn_head_size)
        # -> [bs, seq_len, hidden_size]
        return tensor

    
    def _mem_attn(self, 
                  query,
                  knn_key, 
                  knn_value, 
                  local_key, 
                  local_value, 
                  attention_mask=None, 
                  head_mask=None, 
                  log_attn_scores=False,
                  step=None,
                  knn_labels=None):
        # local self-attention 
        # q: [bs, num_attention_heads, seq_len, attn_head_size]
        # k,v: [bs, num_attention_heads, seq_len, knn, attn_head_size]
        query_length = query.size(-2)
        key_length = local_key.size(-2)

        causal_mask = self.bias[:, :, key_length - query_length : key_length, :key_length]

        local_attn_scores = torch.matmul(query, local_key.transpose(-1, -2))
        scale = self.scale.exp()

        local_attn_scores = local_attn_scores * scale

        mask_value = torch.finfo(local_attn_scores.dtype).min
        # Need to be a tensor, otherwise we get error: `RuntimeError: expected scalar type float but found double`.
        # Need to be on the same device, otherwise `RuntimeError: ..., x and y to be on the same device`
        mask_value = torch.tensor(mask_value, dtype=local_attn_scores.dtype).to(local_attn_scores.device)
        local_attn_scores = torch.where(causal_mask, local_attn_scores, mask_value)

        if attention_mask is not None:
            # Apply the attention mask
            local_attn_scores = local_attn_scores + attention_mask

        mem_attn_scores = torch.einsum("bhsd, bhsnd-> bhsn", query, knn_key)
        # attn_scores: [bs, seq_len, num_attention_heads, knn]
        mem_attn_scores = mem_attn_scores * scale

        attn_scores = torch.cat((mem_attn_scores, local_attn_scores), dim=-1)

        # softmax over knns
        attn_weights = nn.functional.softmax(attn_scores, dim=-1)
        attn_weights = attn_weights.to(local_value.dtype)

        mem_attn_weights, local_attn_weights = attn_weights.split([self.num_neighbors, local_attn_scores.size(-1)], dim=-1)
        if log_attn_scores:
            # (bs, seq_len, num_attention_heads, knn) probabilities
            # curate (x,y) pairs
            # where x is attention weight, y is accuracy of retrieved token
            bs, seq_len = mem_attn_weights.size(0), mem_attn_weights.size(2)
            key_labels = knn_labels // seq_len
            key_labels = key_labels.reshape(bs, self.num_attention_heads, seq_len, -1)
            correct_keys = np.equal(key_labels, np.arange(bs)[:, np.newaxis, np.newaxis, np.newaxis])

            bin_width = 0.05

            # Calculate the number of bins
            num_bins = int(1 / bin_width)

            # Create empty lists for storing bin probabilities and accuracies
            bin_probabilities = []
            bin_accuracies = []

            probs = mem_attn_weights.clone().detach().cpu().numpy().reshape(-1).tolist()
            correct_keys = correct_keys.reshape(-1).tolist()

            # Iterate over each bin
            for i in range(num_bins):
                bin_lower = i * bin_width
                bin_upper = (i + 1) * bin_width

                # Filter data points within the current bin range
                bin_x_values = [x for x in probs if bin_lower <= x < bin_upper]
                bin_y_values = [y for j, y in enumerate(correct_keys) if bin_lower <= probs[j] < bin_upper]

                # Calculate accuracy for the bin
                total = len(bin_x_values)
                correct = sum(bin_y_values)
                accuracy = correct / total if total > 0 else 0

                # Store the probability and accuracy for the bin
                bin_probabilities.append((bin_lower + bin_upper) / 2)
                bin_accuracies.append(accuracy)

            data = [[x, y] for x, y in zip(bin_probabilities, bin_accuracies)]
            table = wandb.Table(data=data, columns=["attn_prob", "retrieved_acc"])
            self.tracker.log({"attn_vs_acc": wandb.plot.scatter(table, "attn_prob", "retrieved_acc")}, step=step)


        if log_attn_scores:
            # this def won't work well on multi-gpu machines
            num_attention_heads = mem_attn_weights.size(1)
            for head in range(num_attention_heads):
                mem_attn_score_per_head = mem_attn_weights[:, head].reshape(-1)
                mem_flat = mem_attn_score_per_head.clone().detach().cpu()
                mem_hist = torch.histc(mem_flat, bins=20, min=0, max=1)
                mem_bins = torch.linspace(0, 1, steps=20 + 1)
                plt.stairs(mem_hist.tolist(), mem_bins.tolist())
                plt.title(f"mem_attn_score_{head}")
                # set arbitrarily but we want to see those peaks!!
                plt.ylim((0, 1000))
                self.tracker.log({f"mem_attn_score_{head}": wandb.Image(plt)}, step=step)
                plt.close()


                local_attn_scores_per_head = local_attn_weights[:, head].reshape(-1)
                local_flat = local_attn_scores_per_head.clone().detach().cpu()
                local_hist = torch.histc(local_flat, bins=20, min=0, max=1)
                local_bins = torch.linspace(0, 1, steps=20 + 1)
                plt.stairs(local_hist.tolist(), local_bins.tolist())
                plt.title(f"local_attn_score_{head}")
                # set arbitrarily but we want to see those peaks!!
                plt.ylim((0, 1000))
                self.tracker.log({f"local_attn_score_{head}": wandb.Image(plt)}, step=step)
                plt.close()


        # attn_output: [bs, num_attention_heads, seq_len, attn_head_size]
        mem_attn_output = torch.einsum("bhsn, bhsnd-> bhsd", mem_attn_weights, knn_value)
        local_attn_output = torch.matmul(local_attn_weights, local_value)

        # TODO: do we need flamingo style gating 
        # of output_gate.tanh * attn_output
        attn_output = mem_attn_output + local_attn_output

        return attn_output

    def _attn(self, query, key, value, attention_mask=None, head_mask=None):
        # q, k, v: [bs, num_attention_heads, seq_len, attn_head_size]
        # compute causal mask from causal mask buffer
        batch_size, num_attention_heads, query_length, attn_head_size = query.size()
        key_length = key.size(-2)

        causal_mask = self.bias[:, :, key_length - query_length : key_length, :key_length]

        query = query.view(batch_size * num_attention_heads, query_length, attn_head_size)
        key = key.view(batch_size * num_attention_heads, key_length, attn_head_size)
        attn_scores = torch.zeros(
            batch_size * num_attention_heads,
            query_length,
            key_length,
            dtype=query.dtype,
            device=key.device,
        )
        attn_scores = torch.baddbmm(
            attn_scores,
            query,
            key.transpose(1, 2),
            beta=1.0,
            alpha=1.0 if self.memory else (torch.tensor(1.0, dtype=self.norm_factor.dtype, device=self.norm_factor.device) / self.norm_factor)
        )
        attn_scores = attn_scores.view(batch_size, num_attention_heads, query_length, key_length)
        if self.memory:
            attn_scores = attn_scores * self.scale.exp()

        mask_value = torch.finfo(attn_scores.dtype).min
        # Need to be a tensor, otherwise we get error: `RuntimeError: expected scalar type float but found double`.
        # Need to be on the same device, otherwise `RuntimeError: ..., x and y to be on the same device`
        mask_value = torch.tensor(mask_value, dtype=attn_scores.dtype).to(attn_scores.device)
        attn_scores = torch.where(causal_mask, attn_scores, mask_value)

        if attention_mask is not None:
            # Apply the attention mask
            attn_scores = attn_scores + attention_mask

        attn_weights = nn.functional.softmax(attn_scores, dim=-1)
        attn_weights = attn_weights.to(value.dtype)

        # Mask heads if we want to
        if head_mask is not None:
            attn_weights = attn_weights * head_mask

        attn_output = torch.matmul(attn_weights, value)
        return attn_output, attn_weights



class RotaryEmbedding(torch.nn.Module):
    def __init__(self, dim, max_position_embeddings, base=10000, device=None):
        super().__init__()
        inv_freq = 1.0 / (base ** (torch.arange(0, dim, 2).float().to(device) / dim))
        self.register_buffer("inv_freq", inv_freq)

        # Build here to make `torch.jit.trace` work.
        self.max_seq_len_cached = max_position_embeddings
        t = torch.arange(self.max_seq_len_cached, device=self.inv_freq.device, dtype=self.inv_freq.dtype)
        freqs = torch.einsum("i,j->ij", t, self.inv_freq)
        # Different from paper, but it uses a different permutation in order to obtain the same calculation
        emb = torch.cat((freqs, freqs), dim=-1)
        self.cos_cached = emb.cos()[None, None, :, :]
        self.sin_cached = emb.sin()[None, None, :, :]

    def forward(self, x, seq_len=None):
        # x: [bs, num_attention_heads, seq_len, head_size]
        # This `if` block is unlikely to be run after we build sin/cos in `__init__`. Keep the logic here just in case.
        if seq_len > self.max_seq_len_cached:
            self.max_seq_len_cached = seq_len
            t = torch.arange(self.max_seq_len_cached, device=x.device, dtype=self.inv_freq.dtype)
            freqs = torch.einsum("i,j->ij", t, self.inv_freq)
            # Different from paper, but it uses a different permutation in order to obtain the same calculation
            emb = torch.cat((freqs, freqs), dim=-1).to(x.device)
            self.cos_cached = emb.cos()[None, None, :, :]
            self.sin_cached = emb.sin()[None, None, :, :]
        return self.cos_cached[:seq_len, ...].to(x.device).to(x.dtype), self.sin_cached[:seq_len, ...].to(x.device).to(x.dtype)


def rotate_half(x):
    """Rotates half the hidden dims of the input."""
    x1 = x[..., : x.shape[-1] // 2]
    x2 = x[..., x.shape[-1] // 2 :]
    return torch.cat((-x2, x1), dim=-1)


def apply_rotary_pos_emb(q, k, cos, sin, position_ids):
    gather_indices = position_ids[:, None, :, None]  # [bs, 1, seq_len, 1]
    gather_indices = gather_indices.repeat(1, cos.shape[1], 1, cos.shape[3])
    cos = torch.gather(cos.repeat(gather_indices.shape[0], 1, 1, 1), 2, gather_indices)
    sin = torch.gather(sin.repeat(gather_indices.shape[0], 1, 1, 1), 2, gather_indices)
    q_embed = (q * cos) + (rotate_half(q) * sin)
    k_embed = (k * cos) + (rotate_half(k) * sin)
    return q_embed, k_embed


class LetheMLP(nn.Module):
    def __init__(self, config):
        super().__init__()
        self.dense_h_to_4h = nn.Linear(config.hidden_size, config.intermediate_size)
        self.dense_4h_to_h = nn.Linear(config.intermediate_size, config.hidden_size)
        self.act = ACT2FN[config.hidden_act]

    def forward(self, hidden_states):
        hidden_states = self.dense_h_to_4h(hidden_states)
        hidden_states = self.act(hidden_states)
        hidden_states = self.dense_4h_to_h(hidden_states)
        return hidden_states


class LetheLayer(nn.Module):
    def __init__(self, config, memory_attention=False, index=None, tracker=None):
        super().__init__()
        self.use_parallel_residual = config.use_parallel_residual
        self.input_layernorm = nn.LayerNorm(config.hidden_size, eps=config.layer_norm_eps)
        self.post_attention_layernorm = nn.LayerNorm(config.hidden_size, eps=config.layer_norm_eps)
        self.attention = LetheAttention(config, memory_attention=memory_attention, index=index, tracker=tracker)
        self.mlp = LetheMLP(config)

    def forward(
        self,
        hidden_states: Optional[torch.FloatTensor],
        attention_mask: Optional[torch.FloatTensor] = None,
        position_ids: Optional[torch.LongTensor] = None,
        head_mask: Optional[torch.FloatTensor] = None,
        use_cache: Optional[bool] = False,
        layer_past: Optional[Tuple[torch.Tensor]] = None,
        output_attentions: Optional[bool] = False,
        log_attn_scores: Optional[bool] = False,
        step: Optional[int] = None,
        save_kv: Optional[bool] = True
    ):
        ln_hidden_states = self.input_layernorm(hidden_states)
        attention_layer_outputs = self.attention(
            ln_hidden_states,
            attention_mask=attention_mask,
            position_ids=position_ids,
            layer_past=layer_past,
            head_mask=head_mask,
            use_cache=use_cache,
            output_attentions=output_attentions,
            log_attn_scores=log_attn_scores,
            step=step,
            save_kv=save_kv,
        )
        attn_output = attention_layer_outputs[0]  # output_attn: attn_output, present, (attn_weights)
        outputs = attention_layer_outputs[1:]

        if self.use_parallel_residual:
            # pseudocode:
            # x = x + attn(ln1(x)) + mlp(ln2(x))
            mlp_output = self.mlp(self.post_attention_layernorm(hidden_states))
            hidden_states = mlp_output + attn_output + hidden_states
        else:
            # pseudocode:
            # x = x + attn(ln1(x))
            # x = x + mlp(ln2(x))
            attn_output = attn_output + hidden_states
            mlp_output = self.mlp(self.post_attention_layernorm(attn_output))
            hidden_states = mlp_output + attn_output

        if use_cache:
            outputs = (hidden_states,) + outputs
        else:
            outputs = (hidden_states,) + outputs[1:]

        return outputs  # hidden_states, present, (attentions)
        

class LetheModel(LethePreTrainedModel):
    def __init__(self, config, index, tracker=None):
        super().__init__(config)
        self.config = config

        self.embed_in = nn.Embedding(config.vocab_size, config.hidden_size)

        self.layers = nn.ModuleList([LetheLayer(config,
                                                memory_attention=i+1 == config.memory_attn_layer,
                                                index=index if i+1 == config.memory_attn_layer else None,
                                                tracker=tracker) 
                                     for i in range(config.num_hidden_layers)])
        self.final_layer_norm = nn.LayerNorm(config.hidden_size, eps=config.layer_norm_eps)

        self.gradient_checkpointing = False

        # Initialize weights and apply final processing
        self.post_init()

    def get_input_embeddings(self):
        return self.embed_in

    def set_input_embeddings(self, value):
        self.embed_in = value

    def forward(
        self,
        input_ids: Optional[torch.LongTensor] = None,
        attention_mask: Optional[torch.FloatTensor] = None,
        position_ids: Optional[torch.LongTensor] = None,
        head_mask: Optional[torch.FloatTensor] = None,
        inputs_embeds: Optional[torch.FloatTensor] = None,
        past_key_values: Optional[Tuple[Tuple[torch.FloatTensor]]] = None,
        use_cache: Optional[bool] = None,
        output_attentions: Optional[bool] = None,
        output_hidden_states: Optional[bool] = None,
        return_dict: Optional[bool] = None,
        log_attn_scores: Optional[bool] = False,
        step: Optional[int] = None,
        save_kv: Optional[bool] = True,
    ) -> Union[Tuple, BaseModelOutputWithPast]:
        r"""
        past_key_values (`tuple(tuple(torch.FloatTensor))` of length `config.n_layers` with each tuple having 4 tensors of shape `(batch_size, num_heads, sequence_length - 1, embed_size_per_head)`):
            Contains precomputed key and value hidden states of the attention blocks. Can be used to speed up decoding.
            If `past_key_values` are used, the user can optionally input only the last `decoder_input_ids` (those that
            don't have their past key value states given to this model) of shape `(batch_size, 1)` instead of all
            `decoder_input_ids` of shape `(batch_size, sequence_length)`.
        use_cache (`bool`, *optional*):
            If set to `True`, `past_key_values` key value states are returned and can be used to speed up decoding (see
            `past_key_values`).
        """
        output_attentions = output_attentions if output_attentions is not None else self.config.output_attentions
        output_hidden_states = (
            output_hidden_states if output_hidden_states is not None else self.config.output_hidden_states
        )
        return_dict = return_dict if return_dict is not None else self.config.use_return_dict
        use_cache = use_cache if use_cache is not None else self.config.use_cache

        if input_ids is not None and inputs_embeds is not None:
            raise ValueError("You cannot specify both input_ids and inputs_embeds at the same time")
        elif input_ids is not None:
            input_shape = input_ids.size()
        elif inputs_embeds is not None:
            input_shape = inputs_embeds.size()[:-1]
        else:
            raise ValueError("You have to specify either input_ids or inputs_embeds")

        batch_size, seq_length = input_shape

        if past_key_values is None:
            past_length = 0
            past_key_values = tuple([None] * self.config.num_hidden_layers)
        else:
            past_length = past_key_values[0][0].size(-2)

        if position_ids is None:
            device = input_ids.device if input_ids is not None else inputs_embeds.device
            position_ids = torch.arange(past_length, seq_length + past_length, dtype=torch.long, device=device)
            position_ids = position_ids.unsqueeze(0).view(-1, seq_length)
        else:
            position_ids = position_ids.view(-1, seq_length).long()

        # Attention mask.
        if attention_mask is not None:
            assert batch_size > 0, "batch_size has to be defined and > 0"
            attention_mask = attention_mask.view(batch_size, -1)
            # We create a 3D attention mask from a 2D tensor mask.
            # Sizes are [batch_size, 1, 1, to_seq_length]
            # So we can broadcast to [batch_size, num_heads, from_seq_length, to_seq_length]
            # this attention mask is more simple than the triangular masking of causal attention
            # used in OpenAI GPT, we just need to prepare the broadcast dimension here.
            attention_mask = attention_mask[:, None, None, :]

            # Since attention_mask is 1.0 for positions we want to attend and 0.0 for
            # masked positions, this operation will create a tensor which is 0.0 for
            # positions we want to attend and the dtype's smallest value for masked positions.
            # Since we are adding it to the raw scores before the softmax, this is
            # effectively the same as removing these entirely.
            attention_mask = attention_mask.to(dtype=self.dtype)  # fp16 compatibility
            attention_mask = (1.0 - attention_mask) * torch.finfo(self.dtype).min

        # Prepare head mask if needed
        # 1.0 in head_mask indicate we keep the head
        # attention_probs has shape bsz x n_heads x N x N
        # input head_mask has shape [num_heads] or [num_hidden_layers x num_heads]
        # and head_mask is converted to shape [num_hidden_layers x batch x num_heads x seq_length x seq_length]
        head_mask = self.get_head_mask(head_mask, self.config.num_hidden_layers)

        if inputs_embeds is None:
            inputs_embeds = self.embed_in(input_ids)

        hidden_states = inputs_embeds

        if self.gradient_checkpointing and self.training:
            if use_cache:
                logger.warning(
                    "`use_cache=True` is incompatible with gradient checkpointing. Setting `use_cache=False`..."
                )
                use_cache = False

        presents = () if use_cache else None
        all_attentions = () if output_attentions else None
        all_hidden_states = () if output_hidden_states else None
        for i, (layer, layer_past) in enumerate(zip(self.layers, past_key_values)):
            if output_hidden_states:
                all_hidden_states = all_hidden_states + (hidden_states,)

            if self.gradient_checkpointing and self.training:

                def create_custom_forward(module):
                    def custom_forward(*inputs):
                        # None for layer_past
                        return module(*inputs, use_cache, None, output_attentions, log_attn_scores, step, save_kv)

                    return custom_forward

                outputs = torch.utils.checkpoint.checkpoint(
                    create_custom_forward(layer),
                    hidden_states,
                    attention_mask,
                    position_ids,
                    head_mask[i],
                )
            else:
                outputs = layer(
                    hidden_states=hidden_states,
                    attention_mask=attention_mask,
                    position_ids=position_ids,
                    head_mask=head_mask[i],
                    layer_past=layer_past,
                    use_cache=use_cache,
                    output_attentions=output_attentions,
                    log_attn_scores=log_attn_scores,
                    step=step,
                    save_kv=save_kv,
                )
            hidden_states = outputs[0]
            if use_cache is True:
                presents = presents + (outputs[1],)
            if output_attentions:
                all_attentions = all_attentions + (outputs[2 if use_cache else 1],)

        hidden_states = self.final_layer_norm(hidden_states)
        # Add last hidden state
        if output_hidden_states:
            all_hidden_states = all_hidden_states + (hidden_states,)

        if not return_dict:
            return tuple(v for v in [hidden_states, presents, all_hidden_states, all_attentions] if v is not None)

        return BaseModelOutputWithPast(
            last_hidden_state=hidden_states,
            past_key_values=presents,
            hidden_states=all_hidden_states,
            attentions=all_attentions,
        )


class LetheForCausalLM(LethePreTrainedModel):
    _keys_to_ignore_on_load_missing = [r"position_ids", r"predictions.decoder.bias"]

    def __init__(self, config, index, tracker=None):
        super().__init__(config)

        self.gpt_neox = LetheModel(config, index, tracker=tracker)
        self.embed_out = nn.Linear(config.hidden_size, config.vocab_size, bias=False)

        self.hidden_size = config.hidden_size


        # Initialize weights and apply final processing
        self.post_init()

    def get_output_embeddings(self):
        return self.embed_out

    def set_output_embeddings(self, new_embeddings):
        self.embed_out = new_embeddings

    def forward(
        self,
        input_ids: torch.LongTensor,
        attention_mask: Optional[torch.FloatTensor] = None,
        past_key_values: Optional[Tuple[Tuple[torch.Tensor]]] = None,
        position_ids: Optional[torch.LongTensor] = None,
        head_mask: Optional[torch.FloatTensor] = None,
        inputs_embeds: Optional[torch.FloatTensor] = None,
        labels: Optional[torch.LongTensor] = None,
        use_cache: Optional[bool] = None,
        output_attentions: Optional[bool] = None,
        output_hidden_states: Optional[bool] = None,
        return_dict: Optional[bool] = None,
        log_attn_scores: Optional[bool] = None,
        step: Optional[int] = None,
        save_kv: Optional[bool] = True,
    ) -> Union[Tuple, CausalLMOutputWithPast]:
        r"""
        past_key_values (`tuple(tuple(torch.FloatTensor))`, *optional*, returned when `use_cache=True` is passed or when `config.use_cache=True`):
            Tuple of `tuple(torch.FloatTensor)` of length `config.n_layers`, with each tuple having 2 tensors of shape
            `(batch_size, num_heads, sequence_length, embed_size_per_head)`) and 2 additional tensors of shape
            `(batch_size, num_heads, encoder_sequence_length, embed_size_per_head)`. The two additional tensors are
            only required when the model is used as a decoder in a Sequence to Sequence model.

            Contains pre-computed hidden-states (key and values in the self-attention blocks that can be used (see
            `past_key_values` input) to speed up sequential decoding.

            If `past_key_values` are used, the user can optionally input only the last `decoder_input_ids` (those that
            don't have their past key value states given to this model) of shape `(batch_size, 1)` instead of all
            `decoder_input_ids` of shape `(batch_size, sequence_length)`.
        labels (`torch.LongTensor` of shape `(batch_size, sequence_length)`, *optional*):
            Labels for computing the left-to-right language modeling loss (next word prediction). Indices should be in
            `[-100, 0, ..., config.vocab_size]` (see `input_ids` docstring) Tokens with indices set to `-100` are
            ignored (masked), the loss is only computed for the tokens with labels n `[0, ..., config.vocab_size]`.
        use_cache (`bool`, *optional*):
            If set to `True`, `past_key_values` key value states are returned and can be used to speed up decoding (see
            `past_key_values`).

        Returns:

        Example:

        ```python
        >>> from transformers import AutoTokenizer, PythiaSeekForCausalLM, PythiaSeekConfig
        >>> import torch

        >>> tokenizer = AutoTokenizer.from_pretrained("EleutherAI/gpt-neox-20b")
        >>> config = PythiaSeekConfig.from_pretrained("EleutherAI/gpt-neox-20b")
        >>> config.is_decoder = True
        >>> model = PythiaSeekForCausalLM.from_pretrained("EleutherAI/gpt-neox-20b", config=config)

        >>> inputs = tokenizer("Hello, my dog is cute", return_tensors="pt")
        >>> outputs = model(**inputs)

        >>> prediction_logits = outputs.logits
        ```"""
        return_dict = return_dict if return_dict is not None else self.config.use_return_dict

        outputs = self.gpt_neox(
            input_ids,
            attention_mask=attention_mask,
            position_ids=position_ids,
            head_mask=head_mask,
            inputs_embeds=inputs_embeds,
            past_key_values=past_key_values,
            use_cache=use_cache,
            output_attentions=output_attentions,
            output_hidden_states=output_hidden_states,
            return_dict=return_dict,
            log_attn_scores=log_attn_scores,
            step=step,
            save_kv=save_kv,
        )

        hidden_states = outputs[0]
        lm_logits = self.embed_out(hidden_states)

        lm_loss = None
        if labels is not None:
            # move labels to correct device to enable model parallelism
            labels = labels.to(lm_logits.device)
            # we are doing next-token prediction; shift prediction scores and input ids by one
            shift_logits = lm_logits[:, :-1, :].contiguous()
            labels = labels[:, 1:].contiguous()
            loss_fct = CrossEntropyLoss()
            lm_loss = loss_fct(shift_logits.view(-1, shift_logits.size(-1)), labels.view(-1))

        if not return_dict:
            output = (lm_logits,) + outputs[1:]
            return ((lm_loss,) + output) if lm_loss is not None else output

        return CausalLMOutputWithPast(
            loss=lm_loss,
            logits=lm_logits,
            past_key_values=outputs.past_key_values,
            hidden_states=outputs.hidden_states,
            attentions=outputs.attentions,
        )

    def prepare_inputs_for_generation(
        self, input_ids, past_key_values=None, attention_mask=None, inputs_embeds=None, **kwargs
    ):
        input_shape = input_ids.shape

        # cut decoder_input_ids if past is used
        if past_key_values and past_key_values[0] is not None:
            input_ids = input_ids[:, -1:]

        position_ids = kwargs.get("position_ids", None)
        if attention_mask is not None and position_ids is None:
            # create position_ids on the fly for batch generation
            position_ids = attention_mask.long().cumsum(-1) - 1
            position_ids.masked_fill_(attention_mask == 0, 1)
            if past_key_values:
                position_ids = position_ids[:, -1].unsqueeze(-1)

        # if model is used as a decoder in encoder-decoder model, the decoder attention mask is created on the fly
        if attention_mask is None:
            attention_mask = input_ids.new_ones(input_shape)

        # if `inputs_embeds` are passed, we only want to use them in the 1st generation step
        if inputs_embeds is not None and past_key_values is None:
            model_inputs = {"inputs_embeds": inputs_embeds}
        else:
            model_inputs = {"input_ids": input_ids}

        model_inputs.update(
            {
                "attention_mask": attention_mask,
                "past_key_values": past_key_values,
                "position_ids": position_ids,
            }
        )

        return model_inputs

    def _reorder_cache(self, past_key_values, beam_idx):
        reordered_past = ()
        for layer_past in past_key_values:
            reordered_past += (
                tuple(past_state.index_select(0, beam_idx) for past_state in layer_past[:2]) + layer_past[2:],
            )
        return reordered_past
