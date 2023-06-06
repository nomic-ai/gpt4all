# To Run Inference Server

docker run --gpus=1 --rm --net=host -v ${PWD}/model_store:/model_store nvcr.io/nvidia/tritonserver:23.01-py3 tritonserver --model-repository=/model_store

python client.py --model=<model_name>