import torch.distributed as dist
from contextlib import contextmanager


def rank0_print(msg):
    if dist.is_initialized():
        if dist.get_rank() == 0:
            print(msg)
    else:
        print(msg)


@contextmanager
def main_process_first(is_main):
    yield from _goes_first(is_main)
    

        
def _goes_first(is_main):
    if not is_main:
        dist.barrier()

    yield

    if is_main:
        dist.barrier()
    