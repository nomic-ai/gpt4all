import torch.distributed as dist


def rank0_print(msg):
    if dist.get_rank() == 0:
        print(msg)
