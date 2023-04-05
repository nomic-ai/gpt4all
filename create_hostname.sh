#!/bin/bash

export WORKER_IP=$1
N_GPUS=8
# create dir if doesn't exist
sudo mkdir -p /job
printf "localhost slots=$N_GPUS\n$WORKER_IP slots=$N_GPUS" | sudo tee /job/hostfile
echo /job/hostfile