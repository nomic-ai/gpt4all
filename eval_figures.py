import glob
import pickle
import numpy as np
from matplotlib import pyplot as plt

plt.figure()
for fpath in glob.glob('./eval_data/*.pkl'):
    parts = fpath.split('_')
    model_name = "-".join(fpath.replace(".pkl", "").split("_")[2:])
    lora_name = parts[2].replace('lora-', '').replace('.pkl', '')
    with open(fpath, 'rb') as f:
        data = pickle.load(f)
        perplexities = data['perplexities']
        perplexities = np.nan_to_num(perplexities, 100)
        perplexities = np.clip(perplexities, 0, 100)
        if 'nomic' in fpath:
            label = f'GPT4all-{model_name}'
        else:
            label = 'alpaca-lora'
        plt.hist(perplexities, label=label, alpha=.5)

plt.xlabel('Perplexity')
plt.ylabel('Frequency')
plt.legend()
plt.savefig('figs/perplexity_hist.png')

