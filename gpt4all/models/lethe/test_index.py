import time
import numpy as np
from gpt4all.models.lethe.modeling_lethe import MemoryIndex


index = MemoryIndex(256,
                    575000,
                    8 
)

keys = np.random.randn(32, 8, 1024, 256)
values = np.random.randn(32, 8, 1024, 256)
start = time.time()
index.add(keys, values)
print(f"index.add time: {time.time() - start}")

print(index.key_indices[0].index.ntotal)
queries = np.random.randn(32, 8, 1024, 256)
start = time.time()
index.knn_query(queries, k=32)
print(f"index.knn_query time: {time.time() - start}")