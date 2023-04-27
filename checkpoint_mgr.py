from os.path import basename, abspath, join, isdir
from typing import List, Optional, Tuple, TypedDict
import sys
from glob import glob

from accelerate import Accelerator
from accelerate.data_loader import DataLoader


class CheckpointManagerException(Exception):
    pass


class Checkpoint(TypedDict):
    """Lightweight type that represents a saved checkpoint"""

    epoch: int
    step: int
    path: str


class CheckpointManager:
    """Encapsulates training loop checkpointing logic"""

    def __init__(
        self,
        base_dir: str,
        accelerator: Accelerator,
        epochs: int,
        max_steps_between_checkpoints=2000,
        prefix="ckpt",
        max_runs=sys.maxsize,
        max_epochs=sys.maxsize,
        max_steps=sys.maxsize,
    ) -> None:
        self.base_dir = base_dir
        self.accelerator = accelerator
        self.epochs = epochs
        self.max_steps_between_checkpoints = max_steps_between_checkpoints
        self.prefix = prefix
        self.max_runs = max_runs
        self.max_epochs = max_epochs
        self.max_steps = max_steps

        # Calculated when start() is invoked
        self._epoch_end: Optional[int] = None

    @staticmethod
    def parse_path(checkpoint_path: str) -> Checkpoint:
        checkpoint_base_name = basename(checkpoint_path)
        parts = checkpoint_base_name.split("_")

        if len(parts) != 3:
            raise CheckpointManagerException(
                f"The following checkpoint path name cannot be parsed: '{checkpoint_base_name}', "
                "checkpoint names must be in the following format: PREFIX_EPOCH_STEP."
            )

        try:
            derived_epoch = int(parts[1])
        except ValueError as ex:
            raise CheckpointManagerException(
                f"Unable to parse the epoch portion of '{checkpoint_base_name}' to an integer"
            ) from ex

        try:
            derived_step = int(parts[2])
        except ValueError as ex:
            raise CheckpointManagerException(
                f"Unable to parse the step portion of '{checkpoint_base_name}' to an integer"
            ) from ex

        return {
            "epoch": derived_epoch,
            "step": derived_step,
            "path": abspath(checkpoint_path),
        }

    def gen_path(self, epoch: int, step: int) -> str:
        cur_epoch_str = str(epoch).zfill(len(str(self.max_epochs)))
        cur_step_str = str(step).zfill(len(str(self.max_steps)))

        return join(
            self.base_dir,
            f"{self.prefix}_{cur_epoch_str}_{cur_step_str}",
        )

    def ls(self) -> List[Checkpoint]:
        unsorted = []
        for checkpoint_path in glob(join(self.base_dir, f"{self.prefix}*")):
            if isdir(checkpoint_path):
                unsorted.append(CheckpointManager.parse_path(checkpoint_path))

        return sorted(unsorted, key=lambda x: x["path"])

    def latest(self) -> Optional[Checkpoint]:
        checkpoints = self.ls()
        return checkpoints[-1] if len(checkpoints) > 0 else None

    def _save_checkpoint(self, epoch: int, step: int) -> None:
        new_checkpoint_path = self.gen_path(epoch=epoch, step=step)
        self.accelerator.save_state(new_checkpoint_path)

    def start(
        self, train_dataloader: DataLoader, force_checkpoint_path: Optional[str] = None
    ) -> Tuple[int, int, int]:
        if self._epoch_end is not None:
            raise CheckpointManagerException(
                f"CheckpointManager session already started: self._epoch_end={self._epoch_end}"
            )

        last_checkpoint: Optional[Checkpoint] = None

        if force_checkpoint_path:
            self.accelerator.print(
                "[CheckpointManager] Loading checkpoint specified in config file"
            )
            last_checkpoint = CheckpointManager.parse_path(
                checkpoint_path=force_checkpoint_path
            )

        else:
            # Else, if the auto-resume setting is enabled in the configuration then look for the most
            # recent checkpoint in the configured output directory
            self.accelerator.print(
                "[CheckpointManager] Determining auto-resume checkpoint"
            )
            last_checkpoint = self.latest()

        # If it was determined that a checkpoint needs to be loaded from the previous block, configure
        # Accelerate accordingly
        if last_checkpoint:
            self.accelerator.print(
                f"[CheckpointManager] Resuming from {last_checkpoint['path']}"
            )
            self.accelerator.load_state(last_checkpoint["path"])

            if (last_checkpoint["step"] + 1) % len(train_dataloader) == 0:
                # Start a the beginning of the next epoch
                step_offset = 0
                epoch_start = last_checkpoint["epoch"] + 1

                # This logic will determine whether we are resuming from an in-process training session
                # or starting a new one.
                if (last_checkpoint["epoch"] + 1) % self.epochs == 0:
                    self.accelerator.print(
                        "[CheckpointManager] Previous session completed, starting new session"
                    )

                    # Starting a new session
                    epoch_end = epoch_start + self.epochs
                else:
                    # TODO: Make sure we test this...
                    self.accelerator.print(
                        "[CheckpointManager] Previous session did not complete, resuming"
                    )

                    # Resuming an existing session
                    epoch_end = self.epochs - epoch_start

            else:
                # Otherwise, we haven't reached the end of the current epoch, make the appropriate
                # adjustments to resume where we left off
                step_offset = last_checkpoint["step"]
                epoch_start = last_checkpoint["epoch"]
                epoch_end = (self.epochs * (epoch_start // self.epochs)) + (
                    self.epochs - 1
                )

        else:
            # When we are not resuming from a checkpoint, set the step and epoch offsets to 0
            self.accelerator.print(
                "[CheckpointManager] No checkpoints found, this is an initial run"
            )
            epoch_start = 0
            epoch_end = self.epochs
            step_offset = 0

        # Fast-forward the dataloader if a step offset was detected. The +1 to the step_offset will simplify the training loop
        if step_offset > 0:
            self.accelerator.skip_first_batches(train_dataloader, step_offset)
            step_offset = step_offset + 1

        # Set the ending epochs in the object state. This will be used to determine if the final
        # checkpoint needs to be saved at the end of the training session
        self._epoch_end = epoch_end

        # Output a helpful message summarizing the output of the checkpoint resuming logic
        checkpoint_name = (
            basename(last_checkpoint["path"]) if last_checkpoint else "N/A"
        )
        self.accelerator.print(
            f"[CheckpointManager] checkpoint={checkpoint_name}, epoch_start={epoch_start}, epoch_end={epoch_end}, step_offset={step_offset}"
        )

        return (epoch_start, epoch_end, step_offset)

    def step_complete(self, epoch: int, step: int):
        # Save checkpoint when our current step has hit max_steps_between_checkpoints
        if step > 0 and step % self.max_steps_between_checkpoints == 0:
            self._save_checkpoint(epoch=epoch, step=step)

    def epoch_complete(self, epoch: int, step: int):
        # Always save checkpoint on the final epoch. Note that the self.epochs passed into the constructor
        # is a count of epochs starting at 1 that the training process is configured to run however we
        # need to do a +1 when comparing with an index
        if (epoch + 1) == self._epoch_end:
            self._save_checkpoint(epoch=epoch, step=step)

    def end(self):
        self._epoch_end = None
