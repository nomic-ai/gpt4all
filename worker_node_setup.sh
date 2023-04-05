#!/bin/sh
HEAD_IP=$1

sudo apt install -y nfs-common
sudo mkdir -p ./data_multiplus
sudo mount ${HEAD_IP}:${PWD}/data_multiplus ./data_multiplus