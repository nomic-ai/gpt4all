from os.path import basename, abspath, join, isdir
from typing import List, Optional, Tuple, TypedDict
import sys
from glob import glob
import logging
import shutil

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
    """A training loop checkpoint manager for HuggingFace Accelerate.

    Example training loop:

    ```
    checkpoint_manager = CheckpointManager(
        base_dir=base_dir,
        accelerator=accelerator,
        epochs=epochs_per_run,
        max_steps_between_checkpoints=max_steps_between_checkpoints,
    )

    epoch_start, epoch_end, step_offset = checkpoint_manager.start(
        train_dataloader=train_dataloader
    )

    for epoch in range(epoch_start, epoch_end):

        # Step offsets returned by CheckpointManager only apply to the
        # first epoch so we need to adjust accordingly
        derived_step = step_offset if epoch == epoch_start else 0

        with tqdm(
            initial=derived_step,
            total=len(train_dataloader),
            desc=f"Epoch {epoch} of {epoch_end-1}",
        ) as pbar:
            for step in range(derived_step, len(train_dataloader)):
                #
                # Training logic goes here...
                #

                # Run step complete hook and update progress bar
                checkpoint_manager.step_complete(epoch=epoch, step=step)
                pbar.update(1)

        # Run epoch complete hook
        checkpoint_manager.epoch_complete(epoch=epoch, step=step)

    # Run session end hook
    checkpoint_manager.end()
    ```


    """

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
        max_checkpoints_per_epoch: Optional[int] = None,
        log: logging.Logger = logging.getLogger(__name__),
    ) -> None:
        self.base_dir = base_dir
        self.accelerator = accelerator
        self.epochs = epochs
        self.max_steps_between_checkpoints = max_steps_between_checkpoints
        self.prefix = prefix
        self.max_runs = max_runs
        self.max_epochs = max_epochs
        self.max_steps = max_steps

        if max_checkpoints_per_epoch is not None and max_checkpoints_per_epoch < 1:
            raise CheckpointManagerException(f"max_checkpoints_per_epoch must be >= 1")

        self.max_checkpoints_per_epoch = max_checkpoints_per_epoch

        # Calculated when start() is invoked
        self._epoch_end: Optional[int] = None

        # Add a default logging handler if the root logger hasn't been configured and there are
        # no specific handlers attached to the Logger instance
        self.log = log
        if not logging.root.handlers and not self.log.handlers:
            console_handler = logging.StreamHandler()
            console_handler.propagate = False
            console_formatter = logging.Formatter(
                "%(asctime)s %(levelname)s: %(message)s"
            )
            console_handler.setFormatter(console_formatter)
            self.log.addHandler(console_handler)
            self.log.setLevel(logging.INFO)

    @staticmethod
    def parse_path(checkpoint_path: str) -> Checkpoint:
        """Parses checkpoint path into corresponding typed dictionary

        Parameters
        ----------
        checkpoint_path : str
            Path to checkpoint directory

        Returns
        -------
        Checkpoint
            Parsed checkpoint dictionary

        Raises
        ------
        CheckpointManagerException
            Raised when parsing fails
        """
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
        """Generates a checkpoint directory path. This method will not actually create the directory.

        Parameters
        ----------
        epoch : int
            Epoch number to encode in the path
        step : int
            Step number to encode in the path

        Returns
        -------
        str
            Path to the generated checkpoint directory
        """
        cur_epoch_str = str(epoch).zfill(len(str(self.max_epochs)))
        cur_step_str = str(step).zfill(len(str(self.max_steps)))

        return join(
            self.base_dir,
            f"{self.prefix}_{cur_epoch_str}_{cur_step_str}",
        )

    def ls(self) -> List[Checkpoint]:
        """List all checkpoints under the checkpoint base directory

        Returns
        -------
        List[Checkpoint]
            List of checkpoint objects found under the configured checkpoint
            base. Will be an empty list of no checkpoints were found.
        """
        unsorted = []
        for checkpoint_path in glob(join(self.base_dir, f"{self.prefix}*")):
            if isdir(checkpoint_path):
                unsorted.append(CheckpointManager.parse_path(checkpoint_path))

        return sorted(unsorted, key=lambda x: x["path"])

    def latest(self) -> Optional[Checkpoint]:
        """Utility method to get the latest checkpoint directory

        Returns
        -------
        Optional[Checkpoint]
            If a checkpoint directory exists return a Checkpoint instance, else return None
        """
        checkpoints = self.ls()
        return checkpoints[-1] if len(checkpoints) > 0 else None

    def _save_checkpoint(self, epoch: int, step: int) -> None:
        """Saves a checkpoint

        Parameters
        ----------
        epoch : int
            Epoch number
        step : int
            Step number
        """
        new_checkpoint_path = self.gen_path(epoch=epoch, step=step)
        self.accelerator.save_state(new_checkpoint_path)

        # Optionally purge after checkpoint is saved
        if (
            self.accelerator.is_main_process
            and self.max_checkpoints_per_epoch is not None
            and self.max_checkpoints_per_epoch > 1
        ):
            all_checkpoints = self.ls()

            for cur_epoch in {x["epoch"] for x in all_checkpoints}:
                purge_files = list(
                    filter(lambda x: x["epoch"] == cur_epoch, all_checkpoints)
                )[: self.max_checkpoints_per_epoch * -1]
                for purge_file in purge_files:
                    self.log.info("Purging: %s", purge_file["path"])
                    shutil.rmtree(purge_file["path"])

    def start(
        self, train_dataloader: DataLoader, force_checkpoint_path: Optional[str] = None
    ) -> Tuple[int, int, int]:
        """After a CheckpointManager instance has bene created call start() before the training
        loop. This method analyzes the configured checkpoint base directory, restores the last
        checkpoint, then determines the correct step_offset, epoch_start, and epoch_end.

        Parameters
        ----------
        train_dataloader : DataLoader
            Dataloader instance, used to determine the step offset
        force_checkpoint_path : Optional[str], optional
            When set, this method will resume from the specified checkpoint vs the latest, by default None

        Returns
        -------
        Tuple[int, int, int]
            Tuple containing (epoch_start, epoch_end, step_offset)

        Raises
        ------
        CheckpointManagerException
            Raised if start() is called twice
        """
        if self._epoch_end is not None:
            raise CheckpointManagerException(
                f"CheckpointManager session already started: self._epoch_end={self._epoch_end}"
            )

        self.log.info("Looking for last checkpoint")
        last_checkpoint: Optional[Checkpoint] = self.latest()

        # If it was determined that a checkpoint needs to be loaded from the previous block, configure
        # Accelerate accordingly
        if last_checkpoint:
            if force_checkpoint_path:
                # If we force resume from a specified checkpoint, load it here. Note that we don't want to replace
                # last_checkpoint since we want the step/epoch numbering to be consistent.
                resume_checkpoint = CheckpointManager.parse_path(
                    checkpoint_path=force_checkpoint_path
                )
            else:
                resume_checkpoint = last_checkpoint

            self.log.info("Resuming from checkpoint: %s", resume_checkpoint["path"])
            self.accelerator.load_state(resume_checkpoint["path"])

            if (last_checkpoint["step"] + 1) >= len(train_dataloader):
                # Start a the beginning of the next epoch if the saved checkpoint is either
                step_offset = 0
                epoch_start = last_checkpoint["epoch"] + 1
                epoch_end = epoch_start + self.epochs

            else:
                # Otherwise, we haven't reached the end of the current epoch, make the appropriate
                # adjustments to resume where we left off
                step_offset = last_checkpoint["step"]
                epoch_start = last_checkpoint["epoch"]
                epoch_end = (self.epochs * (epoch_start // self.epochs)) + self.epochs

        else:
            # When we are not resuming from a checkpoint, set the step and epoch offsets to 0
            self.log.info("No checkpoints found, this is an initial run")
            epoch_start = 0
            epoch_end = self.epochs
            step_offset = 0

        # Fast-forward the dataloader if a step offset was detected. The +1 to the step_offset will
        # simplify the training loop
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
        self.log.info(
            "Started: checkpoint=%s, epoch_start=%i, epoch_end=%i, step_offset=%i",
            checkpoint_name,
            epoch_start,
            epoch_end,
            step_offset,
        )

        return (epoch_start, epoch_end, step_offset)

    def step_complete(self, epoch: int, step: int):
        """Hook to call when a step is completed. Encapsulates checkpoint save logic.

        Parameters
        ----------
        epoch : int
            Current epoch
        step : int
            Current Step
        """
        # Save checkpoint when our current step has hit max_steps_between_checkpoints
        if step > 0 and step % self.max_steps_between_checkpoints == 0:
            self._save_checkpoint(epoch=epoch, step=step)

    def epoch_complete(self, epoch: int, step: int):
        """Hook to call when an epoch is complete. A checkpoint will always be saved at the end
        of a the last epoch.

        Parameters
        ----------
        epoch : int
            Current epoch
        step : int
            Current step
        """

        # Always save checkpoint on the final epoch. Note that the self.epochs passed into the constructor
        # is a count of epochs starting at 1 that the training process is configured to run however we
        # need to do a +1 when comparing with an index
        if (epoch + 1) == self._epoch_end:
            self._save_checkpoint(epoch=epoch, step=step)

    def end(self):
        """Hook to call a the end of a training cycle."""

        self._epoch_end = None
