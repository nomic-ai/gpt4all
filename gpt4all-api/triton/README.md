# To Run Inference Server

docker run --gpus=1 --rm --net=host -v ${PWD}/model_store:/model_store nvcr.io/nvidia/tritonserver:23.01-py3 tritonserver --model-repository=/model_store

python client.py --model=<model_name>


## Dynamic Batching
Need to figure out how to do batching such that we can have dynamic batching
We're getting 1.3 infer/sec which seems slow....

To test,
perf_analyzer -m nomic-ai--gpt4all-j --input-data test_data.json --measurement-interval 25000 --request-rate-range=10 -b 8