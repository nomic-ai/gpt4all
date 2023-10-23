#!/usr/bin/env python3
import numpy as np
import glob
import os
import json
import jsonlines
import pandas as pd


prompt_generation_dir = "raw_data_sanity_cleaned_without_p3/"
for file in glob.glob(os.path.join(prompt_generation_dir, "*.jsonl")):
    if "clean.jsonl" in file:
        continue
    data = []
    print(file)
    with open(file) as f:
        for line in f:
            try:
                contents = json.loads(line)
                data.append(contents)
            except BaseException:
                pass

    processed = []

    for item in data:
        if 'source' not in item:
            item['source'] = 'unspecified'
        if 'model_settings' in item:
            item.pop('model_settings', None)
        
        for key in list(item.keys()):
            if key not in ['source', 'prompt', 'response']:
                #print(item[key])
                item.pop(key, None)
        
        if isinstance(item['prompt'], dict):
            if "value" in item["prompt"]:
                item["prompt"] = item["prompt"]["value"]
            elif "description" in item["prompt"]:
                item["prompt"] = item["prompt"]["description"]
            else:
                continue
                
        elif not isinstance(item['prompt'], str):
            continue
        
        if isinstance(item['response'], dict):
            if "value" in item["response"]:
                item["response"] = item["response"]["value"]
            elif "description" in item["response"]:
                item["response"] = item["response"]["description"]
            else:
                continue 
        elif not isinstance(item['response'], str):
            continue

        if item:
            processed.append(item)

    df = pd.DataFrame(processed)
    prev_len = len(df)

    # drop empty or null string
    df = df.dropna(subset=['prompt', 'response'])
    df = df[df['prompt'] != '']
    df = df[df['response'] != '']
    df = df[df["prompt"].str.len() > 1]
    curr_len = len(df)

    print(f"Removed {prev_len - curr_len} rows")

    clean_name = file.split(".jsonl")[0] + "_clean.jsonl"
    print(f"writing to {curr_len} rows to {clean_name}")
    df.to_json(clean_name, orient="records", lines=True)
