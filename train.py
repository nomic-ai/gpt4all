import os
from transformers import AutoModelForCausalLM, AutoTokenizer 
from transformers.trainer_pt_utils import get_parameter_names
import torch
import torch.nn as nn
from argparse import ArgumentParser
from read import read_config
from accelerate import Accelerator
from accelerate.utils import DummyScheduler, DummyOptim, set_seed
from peft import get_peft_model, LoraConfig, TaskType
from data import load_data
from torchmetrics import MeanMetric
from tqdm import tqdm


def format_metrics(metrics, split, prefix=""):
    log = f"[{split}]" + prefix
    log += " ".join([f"{key}: {value:.4f}" for key, value in metrics.items()])

    return log


def evaluate(config, model, val_dataloader):
    model.eval()
    val_loss = MeanMetric().to(model.device)

    with torch.no_grad():
        for i, batch in enumerate(
            tqdm(val_dataloader),
        ):
            if i == config["eval_steps"]:
                break
                
            loss = model(**batch).loss

            loss_values = accelerator.gather_for_metrics({"loss": loss.detach()})

            val_loss.update(loss_values["loss"])

    return val_loss


def train(accelerator, config):
    set_seed(config['seed'])

    accelerator.print(config)
    accelerator.print(f"Using {accelerator.num_processes} GPUs")

    tokenizer = AutoTokenizer.from_pretrained(config['tokenizer_name'])
    # llama has no pad token, set it to new token
    if tokenizer.pad_token is None:
        # these tokens are already in the vocab, just not mapped correctly
        added_tokens = tokenizer.add_special_tokens({"bos_token": "<s>", "eos_token": "</s>", "pad_token": "<pad>"})

        
    with accelerator.main_process_first():
        train_dataloader, val_dataloader = load_data(config, tokenizer) 
        

    checkpoint = config["gradient_checkpointing"]
    model = AutoModelForCausalLM.from_pretrained(config["model_name"], 
                                                    use_cache=False if checkpoint else True,
                                                    trust_remote_code=True) 

    if added_tokens > 0:
        model.resize_token_embeddings(len(tokenizer))
    
    if checkpoint:
        model.gradient_checkpointing_enable()

    if config["lora"]:
        peft_config = LoraConfig(
            # should R be configurable?
            task_type=TaskType.CAUSAL_LM, inference_mode=False, r=8, lora_alpha=32, lora_dropout=0.1
        )
        model = get_peft_model(model, peft_config)
        model.print_trainable_parameters()

    optimizer_cls = (
        torch.optim.AdamW
        if accelerator.state.deepspeed_plugin is None
        or "optimizer" not in accelerator.state.deepspeed_plugin.deepspeed_config
        else DummyOptim
    )

    # karpathy doesn't decay embeddding, maybe we should exclude
    # https://github.com/karpathy/minGPT/commit/bbbdac74fa9b2e55574d70056163ffbae42310c1#diff-2075fa9c224b395be5bda85544dd36572b59c76c54562819eadadbf268602834R157s
    optimizer = optimizer_cls(model.parameters(), lr=config["lr"])

    # scheduler defined in Deepspeed config
    scheduler = DummyScheduler(
            optimizer,  warmup_num_steps=config["warmup_steps"],
        )

    model, optimizer, train_dataloader, val_dataloader, scheduler = accelerator.prepare(
            model, optimizer, train_dataloader, val_dataloader, scheduler
    )

    # setup for saving training states in case preemption
    accelerator.register_for_checkpointing(scheduler)

    if config["checkpoint"]:
        accelerator.load_state(config["checkpoint"])
        accelerator.print(f"Resumed from checkpoint: {config['checkpoint']}")
        path = os.path.basename(config["train_args"]["resume_from_checkpoint"])
        training_difference = os.path.splitext(path)[0]
        resume_step = int(training_difference.replace("step_", ""))
        accelerator.skip_first_batches(train_dataloader, resume_step)
        accelerator.print(f"Resuming from step {resume_step}")

    train_loss = MeanMetric().to(model.device)

    if accelerator.state.deepspeed_plugin is not None:
        gradient_accumulation_steps = accelerator.state.deepspeed_plugin.deepspeed_config[
            "gradient_accumulation_steps"
        ]

    for epoch in range(config["num_epochs"]):
        for step, batch in enumerate(tqdm(train_dataloader)):
            model.train()
            outputs = model(**batch)
            loss = outputs.loss
            loss = loss / gradient_accumulation_steps

            accelerator.backward(loss)

            # log LR in case something weird happens 
            if step > 0 and step % (config["eval_every"] // 10) == 0:
                if config["wandb"]:
                    curr_step = step + epoch * len(train_dataloader)
                    accelerator.log({"lr": scheduler.get_last_lr()[0]}, step=curr_step)

            if (step + 1) % gradient_accumulation_steps == 0 or step == len(train_dataloader) - 1:
                optimizer.step()
                scheduler.step()
                optimizer.zero_grad()

            loss_values = accelerator.gather_for_metrics({"loss": loss.detach()})
            train_loss.update(loss_values["loss"])

            if step > 0 and step % config["save_every"] == 0:
                accelerator.save_state(f"{config['output_dir']}/step_{step}")

            if step > 0 and step % config["eval_every"] == 0:
                val_loss = evaluate(config, model, val_dataloader)

                log_train = {
                        "train_loss": train_loss.compute()
                    }
                log_val = {
                    "val_loss": val_loss.compute()
                }

                if config["wandb"]:
                    curr_step = step + epoch * len(train_dataloader)
                    accelerator.log({**log_train, **log_val}, step=curr_step)

                accelerator.print(f"Current LR: {scheduler.get_last_lr()[0]}")
                accelerator.print(format_metrics(log_train, "train", f" step {step} "))
                accelerator.print(format_metrics(log_val, "val", f" step {step} "))

                train_loss.reset()

        accelerator.print(f"Epoch {epoch} finished")
        accelerator.print(f"Pushing to HF hub")
        accelerator.wait_for_everyone()
        unwrapped_model = accelerator.unwrap_model(model)
        if accelerator.is_main_process:
            unwrapped_model.push_to_hub(config["save_name"] + "_first_epoch", private=True)

            
    accelerator.wait_for_everyone()
    unwrapped_model = accelerator.unwrap_model(model)
    unwrapped_model.save_pretrained(
        f"{config['output_dir']}/final",
        is_main_process=accelerator.is_main_process,
        save_function=accelerator.save,
        state_dict=accelerator.get_state_dict(model),
    )

    if accelerator.is_main_process:
        unwrapped_model.push_to_hub(config["save_name"], private=True)

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
