#!/bin/sh
WORKER_IP=$1
N_GPUS=$2

sudo apt install -y nfs-kernel-server
sudo mkdir -p ./data_multiplus
sudo chmod 777 ./data_multiplus
printf "${PWD}/data_multiplus ${WORKER_IP}(rw,sync,no_subtree_check)" | sudo tee -a /etc/exports
sudo systemctl restart nfs-kernel-server

sudo apt-get install -y pdsh
export DSHPATH=$PATH
export PDSH_RCMD_TYPE=ssh

ssh-keygen -t rsa -N ''
cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys

sudo mkdir -p /job
printf "localhost slots=$N_GPUS\n$WORKER_IP slots=$N_GPUS" | sudo tee /job/hostfile