#!/usr/bin/env python3
import glob
import pickle
import numpy as np
from matplotlib import pyplot as plt

plt.figure()
for fpath in glob.glob('./eval_data/*.pkl'):
    parts = fpath.split('__')
    model_name = "-".join(fpath.replace(".pkl", "").split("_")[2:])
    with open(fpath, 'rb') as f:
        data = pickle.load(f)
        perplexities = data['perplexities']
        perplexities = np.nan_to_num(perplexities, 100)
        perplexities = np.clip(perplexities, 0, 100)
        if 'alpaca' not in fpath:
            identifier = model_name = "-".join(fpath.replace(".pkl", "").split("eval__model-")[1:]) 
            label = 'GPT4all-'
            label += identifier
            
        else:
            label = 'alpaca-lora'
        plt.hist(perplexities, label=label, alpha=.5, bins=50)

plt.xlabel('Perplexity')
plt.ylabel('Frequency')
plt.legend()
plt.savefig('figs/perplexity_hist.png')

