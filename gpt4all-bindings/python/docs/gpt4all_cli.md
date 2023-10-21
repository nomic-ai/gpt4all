# GPT4All CLI

The GPT4All command-line interface (CLI) is a Python script which is built on top of the
[Python bindings][docs-bindings-python] ([repository][repo-bindings-python]) and the [typer]
package. The source code, README, and local build instructions can be found
[here][repo-bindings-cli].

[docs-bindings-python]: gpt4all_python.html
[repo-bindings-python]: https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/python
[repo-bindings-cli]: https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/cli
[typer]: https://typer.tiangolo.com/

## Installation
### The Short Version

The CLI is a Python script called [app.py]. If you're already familiar with Python best practices,
the short version is to [download app.py][app.py-download] into a folder of your choice, install
the two required dependencies with some variant of:
```shell
pip install gpt4all typer
```

Then run it with a variant of:
```shell
python app.py repl
```
In case you're wondering, _REPL_ is an acronym for [read-eval-print loop][wiki-repl].

[app.py]: https://github.com/nomic-ai/gpt4all/blob/main/gpt4all-bindings/cli/app.py
[app.py-download]: https://raw.githubusercontent.com/nomic-ai/gpt4all/main/gpt4all-bindings/cli/app.py
[wiki-repl]: https://en.wikipedia.org/wiki/Read%E2%80%93eval%E2%80%93print_loop

### Recommendations & The Long Version

Especially if you have several applications/libraries which depend on Python, to avoid descending
into dependency hell at some point, you should:
- Consider to always install into some kind of [_virtual environment_][venv].
- On a _Unix-like_ system, don't use `sudo` for anything other than packages provided by the system
  package manager, i.e. never with `pip`.

[venv]: https://docs.python.org/3/library/venv.html

There are several ways and tools available to do this, so below are descriptions on how to install
with a _virtual environment_ (recommended) or a user installation on all three main platforms.

Different platforms can have slightly different ways to start the Python interpreter itself.

Note: _Typer_ has an optional dependency for more fanciful output. If you want that, replace `typer`
with `typer[all]` in the pip-install instructions below.

#### Virtual Environment Installation
You can name your _virtual environment_ folder for the CLI whatever you like. In the following,
`gpt4all-cli` is used throughout.

##### macOS

There are at least three ways to have a Python installation on _macOS_, and possibly not all of them
provide a full installation of Python and its tools. When in doubt, try the following:
```shell
python3 -m venv --help
python3 -m pip --help
```
Both should print the help for the `venv` and `pip` commands, respectively. If they don't, consult
the documentation of your Python installation on how to enable them, or download a separate Python
variant, for example try an [unified installer package from python.org][python.org-downloads].

[python.org-downloads]: https://www.python.org/downloads/

Once ready, do:
```shell
python3 -m venv gpt4all-cli
. gpt4all-cli/bin/activate
python3 -m pip install gpt4all typer
```

##### Windows

Download the [official installer from python.org][python.org-downloads] if Python isn't already
present on your system.

A _Windows_ installation should already provide all the components for a _virtual environment_. Run:
```shell
py -3 -m venv gpt4all-cli
gpt4all-cli\Scripts\activate
py -m pip install gpt4all typer
```

##### Linux

On Linux, a Python installation is often split into several packages and not all are necessarily
installed by default. For example, on Debian/Ubuntu and derived distros, you will want to ensure
their presence with the following:
```shell
sudo apt-get install python3-venv python3-pip
```
The next steps are similar to the other platforms:
```shell
python3 -m venv gpt4all-cli
. gpt4all-cli/bin/activate
python3 -m pip install gpt4all typer
```
On other distros, the situation might be different. Especially the package names can vary a lot.
You'll have to look it up in the documentation, software directory, or package search.

#### User Installation
##### macOS

There are at least three ways to have a Python installation on _macOS_, and possibly not all of them
provide a full installation of Python and its tools. When in doubt, try the following:
```shell
python3 -m pip --help
```
That should print the help for the `pip` command. If it doesn't, consult the documentation of your
Python installation on how to enable them, or download a separate Python variant, for example try an
[unified installer package from python.org][python.org-downloads].

Once ready, do:
```shell
python3 -m pip install --user --upgrade gpt4all typer
```

##### Windows

Download the [official installer from python.org][python.org-downloads] if Python isn't already
present on your system. It includes all the necessary components. Run:
```shell
py -3 -m pip install --user --upgrade gpt4all typer
```

##### Linux

On Linux, a Python installation is often split into several packages and not all are necessarily
installed by default. For example, on Debian/Ubuntu and derived distros, you will want to ensure
their presence with the following:
```shell
sudo apt-get install python3-pip
```
The next steps are similar to the other platforms:
```shell
python3 -m pip install --user --upgrade gpt4all typer
```
On other distros, the situation might be different. Especially the package names can vary a lot.
You'll have to look it up in the documentation, software directory, or package search.

## Running the CLI

The CLI is a self-contained script called [app.py]. As such, you can [download][app.py-download]
and save it anywhere you like, as long as the Python interpreter has access to the mentioned
dependencies.

Note: different platforms can have slightly different ways to start Python. Whereas below the
interpreter command is written as `python` you typically want to type instead:
- On _Unix-like_ systems: `python3`
- On _Windows_: `py -3`

The simplest way to start the CLI is:
```shell
python app.py repl
```
This automatically selects the [groovy] model and downloads it into the `.cache/gpt4all/` folder
of your home directory, if not already present.

[groovy]: https://huggingface.co/nomic-ai/gpt4all-j#model-details

If you want to use a different model, you can do so with the `-m`/`--model` parameter. If only a
model file name is provided, it will again check in `.cache/gpt4all/` and might start downloading.
If instead given a path to an existing model, the command could for example look like this:
```shell
python app.py repl --model /home/user/my-gpt4all-models/gpt4all-13b-snoozy-q4_0.gguf
```

When you're done and want to end a session, simply type `/exit`.

To get help and information on all the available commands and options on the command-line, run:
```shell
python app.py --help
```
And while inside the running _REPL_, write `/help`.

Note that if you've installed the required packages into a _virtual environment_, you don't need
to activate that every time you want to run the CLI. Instead, you can just start it with the Python
interpreter in the folder `gpt4all-cli/bin/` (_Unix-like_) or `gpt4all-cli/Script/` (_Windows_).

That also makes it easy to set an alias e.g. in [Bash][bash-aliases] or [PowerShell][posh-aliases]:
- Bash: `alias gpt4all="'/full/path/to/gpt4all-cli/bin/python' '/full/path/to/app.py' repl"`
- PowerShell:
  ```posh
  Function GPT4All-Venv-CLI {"C:\full\path\to\gpt4all-cli\Scripts\python.exe" "C:\full\path\to\app.py" repl}
  Set-Alias -Name gpt4all -Value GPT4All-Venv-CLI
  ```

Don't forget to save these in the start-up file of your shell.

[bash-aliases]: https://www.gnu.org/software/bash/manual/html_node/Aliases.html
[posh-aliases]: https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.utility/set-alias

Finally, if on _Windows_ you see a box instead of an arrow `⇢` as the prompt character, you should
change the console font to one which offers better Unicode support.
