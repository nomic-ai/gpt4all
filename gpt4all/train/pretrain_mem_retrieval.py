import os
import torch.nn.functional as F
from transformers import AutoTokenizer, get_scheduler, AutoConfig
import torch
from torch.optim import AdamW
from argparse import ArgumentParser
from gpt4all.utils.read import read_config
from accelerate import Accelerator
from accelerate.utils import DummyScheduler, DummyOptim, set_seed
from gpt4all.data.enwik8 import load_enwik8_dataloader
from torchmetrics import MeanMetric
from tqdm import tqdm
from gpt4all.models import LetheForCausalLM, LetheConfig
from gpt4all.models.lethe.modeling_lethe import BatchedMemory
import wandb

torch.backends.cuda.matmul.allow_tf32 = True

def format_metrics(metrics, split, prefix=""):
    log = f"[{split}]" + prefix
    log += " ".join([f"{key}: {value:.4f}" for key, value in metrics.items()])

    return log


def evaluate(model, index, pad_token_id, config, val_dataloader, main_process=False):
    model.eval()
    val_loss = MeanMetric(nan_strategy="error").to(model.device)

    chunk_size = config["seq_len"]
    with torch.no_grad():
        for batch in tqdm(val_dataloader, disable=not main_process):
            seq_len = batch.shape[1]
            for chunk_start in range(0, seq_len, chunk_size):
                chunk_end = min(seq_len, chunk_start + chunk_size)
                inputs = batch[:, chunk_start:chunk_end].to(model.device)
                labels = inputs.clone()
                outputs = model(input_ids=inputs, 
                                attention_mask=inputs.ne(pad_token_id),
                                labels=labels, 
                                log_attn_scores=False,
                                step=None,
                                save_kv=True,
                )
                loss = outputs.loss / config["segments"]
                loss_values = accelerator.gather_for_metrics({"loss": loss.item()})
                val_loss.update(loss_values["loss"])

            index.reset()

    return val_loss


def train(accelerator, config):
    set_seed(config['seed'])

    accelerator.print(config)
    accelerator.print(f"Using {accelerator.num_processes} GPUs")

    tokenizer = AutoTokenizer.from_pretrained(config['tokenizer_name'], model_max_length=config['max_length'])
    # if no pad token, set it to eos
    if tokenizer.pad_token is None:
        tokenizer.pad_token = tokenizer.eos_token

        
    with accelerator.main_process_first():
        train_dataloader, val_dataloader = load_enwik8_dataloader(config, tokenizer)


    if accelerator.state.deepspeed_plugin is not None:
        gradient_accumulation_steps = accelerator.state.deepspeed_plugin.deepspeed_config[
            "gradient_accumulation_steps"
        ]

    accelerator.print(f"Len of train_dataloader: {len(train_dataloader)}")
    total_num_steps = (len(train_dataloader) / gradient_accumulation_steps) * config["num_epochs"]
    # instead of decaying to zero, decay to ratio of min_lr / lr
    accelerator.print(f"Total training steps: {total_num_steps}")

    checkpoint = config["gradient_checkpointing"]
    
    model_config = LetheConfig.from_pretrained(config["model_name"])
    model_config.memory_attn_layer = config["memory_attn_layer"]
    model_config.num_neighbors_to_retrieve = config["num_neighbors_to_retrieve"]
    model_config.use_cache = False if checkpoint else True

    head_size = model_config.hidden_size // model_config.num_attention_heads
    index = BatchedMemory(config["batch_size"],
                        head_size,
                        config["num_memories_per_index"],
                        model_config.num_attention_heads,
    )

    model = LetheForCausalLM(model_config, 
                             index=index,
                             tracker=accelerator.get_tracker("wandb"))


    accelerator.print(f"Training a {model.num_parameters():,} parameter model")
    if checkpoint:
        model.gradient_checkpointing_enable()

    optimizer_cls = (
        AdamW
        if accelerator.state.deepspeed_plugin is None
        or "optimizer" not in accelerator.state.deepspeed_plugin.deepspeed_config
        else DummyOptim
    )

    # karpathy doesn't decay embeddding, maybe we should exclude
    # https://github.com/karpathy/minGPT/commit/bbbdac74fa9b2e55574d70056163ffbae42310c1#diff-2075fa9c224b395be5bda85544dd36572b59c76c54562819eadadbf268602834R157s
    optimizer = optimizer_cls(model.parameters(), lr=config["lr"], weight_decay=config["weight_decay"])


    # Creates Dummy Scheduler if `scheduler` was spcified in the config file else creates `args.lr_scheduler_type` Scheduler
    if config["scheduler"] or "scheduler" in accelerator.state.deepspeed_plugin.deepspeed_config:
        if (
            accelerator.state.deepspeed_plugin is None
            or "scheduler" not in accelerator.state.deepspeed_plugin.deepspeed_config
        ):
            scheduler = get_scheduler(
                name="cosine",
                optimizer=optimizer,
                num_warmup_steps=config["warmup_steps"] * accelerator.num_processes,
                num_training_steps=total_num_steps,
            )
        else:
            scheduler = DummyScheduler(
                optimizer, total_num_steps=total_num_steps, warmup_num_steps=config["warmup_steps"]
            )
        model, optimizer, scheduler, train_dataloader, val_dataloader = accelerator.prepare(
                model, optimizer, scheduler, train_dataloader, val_dataloader
        )
        use_scheduler = True
    else:
        model, optimizer, train_dataloader, val_dataloader = accelerator.prepare(
                model, optimizer, train_dataloader, val_dataloader
        )
        use_scheduler = False

    #  setup for saving training states in case preemption
    if use_scheduler:
        accelerator.register_for_checkpointing(scheduler)

    if config["checkpoint"]:
        accelerator.load_state(config["checkpoint"])
        accelerator.print(f"Resumed from checkpoint: {config['checkpoint']}")
        path = os.path.basename(config["train_args"]["resume_from_checkpoint"])
        training_difference = os.path.splitext(path)[0]
        resume_step = int(training_difference.replace("step_", ""))
        accelerator.skip_first_batches(train_dataloader, resume_step)
        accelerator.print(f"Resuming from step {resume_step}")

    # log gradients
    if accelerator.is_main_process and config["wandb"]:
        wandb.watch(model, log_freq=config["log_grads_every"], log="all")

    main_process = accelerator.is_main_process

    chunk_size = config["seq_len"]
    for epoch in range(config["num_epochs"]):
        train_loss = MeanMetric(nan_strategy="error").to(model.device)
        for step, batch in enumerate(tqdm(train_dataloader, disable=not main_process)):
            epoch_step = epoch * len(train_dataloader) + step * config["segments"]
            seq_len = batch["input_ids"].shape[1]
            model.train()
            for i, chunk_start in enumerate(range(0, seq_len, chunk_size)):
                curr_step = epoch_step + i
                chunk_end = min(seq_len, chunk_start + chunk_size)
                inputs = batch["input_ids"][:, chunk_start:chunk_end]
                labels = inputs.clone()
                labels[labels == tokenizer.pad_token_id] = -100
                outputs = model(input_ids=inputs, 
                                attention_mask=inputs.ne(tokenizer.pad_token_id),
                                labels=labels, 
                                log_attn_scores=True,
                                step=curr_step,
                                save_kv=True,
                )
                loss = outputs.loss / config["segments"]

                if config["wandb"]:
                    accelerator.log({"loss": loss}, step=curr_step)

                # gather loss before backprop in case of gradient accumulation
                loss_values = accelerator.gather_for_metrics({"loss": loss.detach().float()})
                train_loss.update(loss_values["loss"])

                loss = loss / gradient_accumulation_steps
                accelerator.backward(loss)

                # log LR in case something weird happens 
                if config["wandb"]:
                    if step > 0 and step % (config["log_lr_every"] ) == 0:
                        lr = optimizer.param_groups[0]["lr"]
                        accelerator.log({"lr": lr}, step=curr_step)

                optimizer.step()
                if use_scheduler:
                    scheduler.step()
                optimizer.zero_grad()
            
            # reset index on batch end
            index.reset()

            if step > 0 and config["save_every"] > 0 and step % config["save_every"] == 0:
                # accelerator.save_state(f"{config['output_dir']}/step_{curr_step}")
                unwrapped_model = accelerator.unwrap_model(model)
                unwrapped_model.save_pretrained(
                        f"{config['output_dir']}/step_{step}",
                        is_main_process=accelerator.is_main_process,
                        save_function=accelerator.save,
                        state_dict=accelerator.get_state_dict(model),
                )

            if step > 0 and (step % config["eval_every"] == 0 or step == len(train_dataloader) - 1):
                val_loss = evaluate(model, index, tokenizer.pad_token_id, config, val_dataloader, main_process=main_process)

                log_train = {
                        "train_loss": train_loss.compute()
                    }
                log_val = {
                    "val_loss": val_loss.compute(),
                }

                if config["wandb"]:
                    curr_step = step + epoch * len(train_dataloader)
                    accelerator.log({**log_train, **log_val}, step=curr_step)

                accelerator.print(f"Current LR: {optimizer.param_groups[0]['lr']}")
                accelerator.print(format_metrics(log_train, "train", f" step {step} "))
                accelerator.print(format_metrics(log_val, "val", f" step {step} "))

                train_loss.reset()

        accelerator.print(f"Epoch {epoch} finished")
        accelerator.wait_for_everyone()
        unwrapped_model = accelerator.unwrap_model(model)
        if config["push_to_hub"]:
            accelerator.print(f"Pushing to HF hub")
            try:
                if accelerator.is_main_process:
                    unwrapped_model.push_to_hub(config["save_name"] + f"-epoch_{epoch}", private=True)

            except Exception as e:
                accelerator.print(e)
                accelerator.print(f"Failed to push to hub")

        unwrapped_model.save_pretrained(
            f"{config['output_dir']}/epoch_{epoch}",
            is_main_process=accelerator.is_main_process,
            save_function=accelerator.save,
            state_dict=accelerator.get_state_dict(model),
        )
            
    accelerator.wait_for_everyone()
    unwrapped_model = accelerator.unwrap_model(model)
    unwrapped_model.save_pretrained(
        f"{config['output_dir']}/final",
        is_main_process=accelerator.is_main_process,
        save_function=accelerator.save,
        state_dict=accelerator.get_state_dict(model),
    )

    accelerator.end_training()

    

if __name__ == "__main__":
    # parse arguments by reading in a config
    parser = ArgumentParser()
    parser.add_argument("--config", type=str, default="config.yaml")

    args = parser.parse_args()

    config = read_config(args.config)

    if config["wandb"]:
        accelerator = Accelerator(log_with="wandb")
        accelerator.init_trackers(
            project_name=config["wandb_project_name"],
            config=config,
            init_kwargs={"wandb": {"entity": config["wandb_entity"]}},
        )
    else:
        accelerator = Accelerator()

    train(accelerator, config=config)
