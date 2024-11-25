from setuptools import setup, find_packages
import os
import pathlib
import platform
import shutil

package_name = "gpt4all"

# Define the location of your prebuilt C library files
SRC_CLIB_DIRECTORY = os.path.join("..", "..", "gpt4all-backend")
SRC_CLIB_BUILD_DIRECTORY = os.path.join("..", "..", "gpt4all-backend", "build") 

LIB_NAME = "llmodel"

DEST_CLIB_DIRECTORY = os.path.join(package_name, f"{LIB_NAME}_DO_NOT_MODIFY")
DEST_CLIB_BUILD_DIRECTORY = os.path.join(DEST_CLIB_DIRECTORY, "build")

system = platform.system()

def get_c_shared_lib_extension():
    
    if system == "Darwin":
        return "dylib"
    elif system == "Linux":
        return "so"
    elif system == "Windows":
        return "dll"
    else:
        raise Exception("Operating System not supported")
    
lib_ext = get_c_shared_lib_extension()

def copy_prebuilt_C_lib(src_dir, dest_dir, dest_build_dir):
    files_copied = 0

    if not os.path.exists(dest_dir):
        os.mkdir(dest_dir)
        os.mkdir(dest_build_dir)

    for dirpath, _, filenames in os.walk(src_dir):
        for item in filenames:
            # copy over header files to dest dir
            s = os.path.join(dirpath, item)
            if item.endswith(".h"):
                d = os.path.join(dest_dir, item)
                shutil.copy2(s, d)
                files_copied += 1
            if item.endswith(lib_ext) or item.endswith('.metallib'):
                s = os.path.join(dirpath, item)
                d = os.path.join(dest_build_dir, item)
                shutil.copy2(s, d)
                files_copied += 1
    
    return files_copied


# NOTE: You must provide correct path to the prebuilt llmodel C library. 
# Specifically, the llmodel.h and C shared library are needed.
copy_prebuilt_C_lib(SRC_CLIB_DIRECTORY,
                    DEST_CLIB_DIRECTORY,
                    DEST_CLIB_BUILD_DIRECTORY)


def get_long_description():
    with open(pathlib.Path(__file__).parent / "README.md", encoding="utf-8") as fp:
        return fp.read()


setup(
    name=package_name,
    version="2.8.3.dev0",
    description="Python bindings for GPT4All",
    long_description=get_long_description(),
    long_description_content_type="text/markdown",
    author="Nomic and the Open Source Community",
    author_email="support@nomic.ai",
    url="https://www.nomic.ai/gpt4all",
    project_urls={
        "Documentation": "https://docs.gpt4all.io/gpt4all_python.html",
        "Source code": "https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/python",
        "Changelog": "https://github.com/nomic-ai/gpt4all/blob/main/gpt4all-bindings/python/CHANGELOG.md",
    },
    classifiers = [
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.8',
    packages=find_packages(),
    install_requires=[
        'importlib_resources; python_version < "3.9"',
        'jinja2~=3.1',
        'requests',
        'tqdm',
        'typing-extensions>=4.3.0; python_version >= "3.9" and python_version < "3.11"',
    ],
    extras_require={
        'cuda': [
            'nvidia-cuda-runtime-cu11',
            'nvidia-cublas-cu11',
        ],
        'all': [
            'gpt4all[cuda]; platform_system == "Windows" or platform_system == "Linux"',
        ],
        'dev': [
            'gpt4all[all]',
            'pytest',
            'twine',
            'wheel',
            'setuptools',
            'mkdocs-material',
            'mkdocs-material[imaging]',
            'mkautodoc',
            'mkdocstrings[python]',
            'mkdocs-jupyter',
            'black',
            'isort',
            'typing-extensions>=3.10',
        ]
    },
    package_data={'llmodel': [os.path.join(DEST_CLIB_DIRECTORY, "*")]},
    include_package_data=True
)
