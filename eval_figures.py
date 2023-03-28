import glob
import pickle
import numpy as np
from matplotlib import pyplot as plt

plt.figure()
for fpath in glob.glob('./eval_data/*multi*.pkl'):
    parts = fpath.split('__')
    model_name = parts[1].replace('model-', '').replace('.pkl', '')
    lora_name = parts[2].replace('lora-', '').replace('.pkl', '')
    with open(fpath, 'rb') as f:
        data = pickle.load(f)
        perplexities = data['perplexities']
        perplexities = np.nan_to_num(perplexities, 100)
        perplexities = np.clip(perplexities, 0, 100)
        plt.hist(perplexities, label='{}-{}'.format(model_name, lora_name), alpha=.5)

plt.xlabel('Perplexity')
plt.ylabel('Frequency')
plt.legend()
plt.savefig('figs/perplexity_hist.png')

