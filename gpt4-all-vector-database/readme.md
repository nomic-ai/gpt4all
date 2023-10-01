<div align="center">
  <h1>🚀 Supercharge your GPT4All with a Vector Database!</h1>
</div>
A vector database lets you convert your documents to vectors and search through them using natural language.  For example, you can search with something like "Where in this 1000 page PDF does it discuss something," and the vector database easily pulls the results.  The vector database is connected to the large langauge model in GPt4All, which is responsible for providing a nice summary of the results from the vector database.

<div align="center">
  <h4>⚡GPU Acceleration Support⚡
  <table>
    <thead>
      <tr>
        <th>GPU</th>
        <th>Windows</th>
        <th>Linux</th>
        <th>Requirements</th>
      </tr>
    </thead>
    <tbody>
      <tr>
        <td>Nvidia</td>
        <td>✅</td>
        <td>✅</td>
        <td>CUDA</td>
      </tr>
      <tr>
        <td>AMD</td>
        <td>❌</td>
        <td>✅</td>
        <td>ROCm 5.4.2</td>
      </tr>
      <tr>
        <td>Apple/Metal</td>
        <td colspan="3" align="center"> ✅ </td>
      </tr>
    </tbody>
  </table></h4>
</div>

# Installation

> First, make sure you're running [Python 3.10+](https://www.python.org/downloads/release/python-31011/).  Also, you must have both [Git](https://git-scm.com/downloads) and [git-lfs](https://git-lfs.com/) installed.<br>
<details>
  <summary>🪟 Windows</summary>
  
### Step 1 - GPU Acceleration Software
* Nvidia GPUs ➜ install [CUDA 11.8](https://developer.nvidia.com/cuda-11-8-0-download-archive)
* AMD GPUs
    > Unfortuantely, PyTorch does not currently support AMD GPUs on Windows (only Linux).

### Step 2 - Obtain Repository
* Download the ZIP file containing the latest "release" and unzip and place anywhere on your computer.

### Step 3 - Virtual Environment
* Open the folder containing my repository files.  Create a command prompt and create a virtual environment with this command:
```
python -m venv .
```
* Then "activate" the virtual environment:
```
.\Scripts\activate
```

### Step 4 - Upgrade pip
```
python -m pip install --upgrade pip
```

### Step 5 - Install PyTorch
* Nvidia GPUs:
```
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu118
```
> PyTorch does not currently support AMD GPUs on Windows (only Linux).
* If you're NOT using GPU acceleration:
```
pip install torch torchvision torchaudio
```

### Step 6 - Doublecheck GPU-Acceleration
```
python check_gpu.py
```

### Step 7 - Install Dependencies
```
pip install -r requirements.txt
```
</details>

<details>
  <summary>🐧 Linux</summary>

### Step 1 - GPU Acceleration Software
  * Nvidia GPUs ➜ install [CUDA 11.8](https://developer.nvidia.com/cuda-11-8-0-download-archive)
  * AMD GPUs ➜ install ROCm version 5.4.2 according to the instructions [HERE](https://rocmdocs.amd.com/en/latest/deploy/linux/quick_start.html) and [HERE](https://rocmdocs.amd.com/en/latest/deploy/linux/index.html)
  * Additionally, [this repo](https://github.com/nktice/AMD-AI) might help but I can't verify since I don't have an AMD GPU nor Linux.

### Step 2 - Obtain Repository
* Download the ZIP file containing the latest release for my repository.  Inside the ZIP file is a folder holding my repository.  Unzip and place this folder anywhere you want on your computer.

### Step 3 - Virtual Environment
* Open the folder containing my repository files.  Create a command prompt and create a virtual environment with this command:
```
python -m venv .
```
* Then activate the virtual environment:
```
.\Scripts\activate
```

### Step 4 - Update Pip
```
python -m pip install --upgrade pip
```
### Step 5 - Install PyTorch
* For Nvidia GPUs:
```
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu118
```
* For AMD GPUs:
```
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/rocm5.4.2
```
* If you're NOT using GPU acceleration:
```
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cpu
```

### Step 6 - Doublecheck GPU-acceleration
```
python check_gpu.py
```
### Step 7 - Install Dependencies
```
pip install -r requirements.txt
```
</details>

<details>
  <summary>🍎 Apple</summary>

### Step 1 - GPU Acceleration Software
* All Macs with MacOS 12.3+ come with Metal/MPS support, which is the equivalent of CUDA and ROCm for Nvidia and AMD, respectively.  However, you do need to have [Xcode Command Line Tools](https://www.makeuseof.com/install-xcode-command-line-tools/).

### Step 2 - Obtain Repository
* Download the ZIP file containing the latest release for my repository.  Inside the ZIP file is a folder holding my repository.  Unzip and place this folder anywhere you want on your computer.

### Step 3 - Virtual Environment
* Open the folder containing my repository files.  Create a terminal window and create a virtual environment with this command:
```
python3 -m venv .
```
* Then activate the virtual environment:
```
source bin/activate
```
### Step 5 - Update Pip
```
python3 -m pip install --upgrade pip
```
### Step 6 - Install PyTorch
```
pip3 install torch torchvision torchaudio
```
### Step 7 - Doublecheck Metal/MPS-acceleration
```
python3 check_gpu.py
```
### Step 8 - Install Dependencies
```
pip3 install -r requirements.txt
```
</details>

# Usage
<details>
  <summary>Instructions</summary>

### Step 1 - Virtual Environment
> Open a command prompt within my repository folder and activate the virtual environment:<br>
> NOTE: For Macs the preferred command is ```source bin/activate```
```
.\Scripts\activate
```

### Step 2 - Run Program
```
python gui.py
```
* NOTE: Only systems running Windows with an Nvidia GPU will display metrics in the GUI.  Will fix later so hold tight.

### Step 3 - "Download Embedding Model"
You must wait until the download is complete AND unpacked before trying to create the database.

### Step 4 - "Select Embedding Model Directory"
Selects the directory containing the model you want to use.

### Step 5 - "Choose Documents for Database"
Select one or more files. Currently supports pdf, docx, txt, json, enex, eml, msg, csv, xls, xlsx.  NOTE: You can always remove files from the "Docs_for_DB" folder, but you must recreate the vector database if you don't want them included in the new database.

### Step 6 - "Create Vector Database."
You should see GPU usage spike.  After the spike, the program still needs to "persist" the database to disk .  You must wait for this to complete before trying to query the database.  You can look at the command prompt window to see exactly when the database is "persisted."

### Step 7 - Submit Query to GPT4All
> Remember, my program is currently only tailured to interact with LLama2-based models that use the standard Llama2 prompt style.

Open GPT4All.  Click "Server Chat" in the left pane.<br><br>
Within my program, type your question in the box and submit.  Please be aware, you will likely need to change the "Settings" for the specific model you're using within GPT4All.  The vector database produces a lot of "context," which it prepends to a user's query.  Therefore, you must adjust the context size within GPT4All to make sure that it doesn't exceed the limit!
</details>

# Contact

All suggestions (positive and negative) are welcome.  "bbc@chintellalaw.com"

<div align="center">
  <img src="https://github.com/BBC-Esq/ChromaDB-Plugin-for-LM-Studio/raw/main/example.png" alt="Example Image">
</div>
