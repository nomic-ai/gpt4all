import os
import shutil


def copy_folder():
    script_folder = os.path.dirname(os.path.abspath(__file__))
    source_folder = os.path.normpath(os.path.join(script_folder, "../../../gpt4all-backend"))
    destination_folder = os.path.normpath(os.path.join(script_folder, "../gpt4all-backend"))

    # Check if the destination folder already exists
    if os.path.exists(destination_folder):
        print(f"The destination folder '{destination_folder}' already exists. Aborting.")
        return

    # Copy the contents of the source folder to the destination folder
    shutil.copytree(source_folder, destination_folder, ignore=shutil.ignore_patterns('build'))


def clean_project():
    # remove possible build folder
    script_folder = os.path.dirname(os.path.abspath(__file__))
    destination_folder = os.path.normpath(os.path.join(script_folder, "../gpt4all-backend"))
    build_folder_destination = os.path.join(destination_folder, "build")
    if os.path.exists(build_folder_destination):
        shutil.rmtree(build_folder_destination)

    # TODO: remove all unnecessary files
    # List of files and folders to remove
    items_to_remove = [
        "./llama.cpp-mainline/models",
        "./llama.cpp-mainline/docs",
        "./llama.cpp-mainline/examples",
        "./llama.cpp-mainline/prompts",
        "./llama.cpp-mainline/.devops",
        "./llama.cpp-mainline/.github",
        "./llama.cpp-mainline/ci",
        "./llama.cpp-mainline/LICENSE",
        "./llama.cpp-mainline/media",
        "./llama.cpp-mainline/kompute/.github",
        "./llama.cpp-mainline/kompute/docker-builders",
        "./llama.cpp-mainline/kompute/docs",
        "./llama.cpp-mainline/kompute/examples",
        "./llama.cpp-mainline/README",
        "./llama.cpp-mainline/gguf-py",
        "./llama.cpp-mainline/tests",
        "./llama.cpp-mainline/kompute/test",
        "./llama.cpp-mainline/kompute/SECURITY.md",
        "./llama.cpp-mainline/kompute/CHANGELOG.md",
        "./llama.cpp-mainline/kompute/CODE_OF_CONDUCT.md",
        "./llama.cpp-mainline/kompute/GOVERNANCE.md"
    ]

    for item in items_to_remove:
        item_path = os.path.join(destination_folder, item)
        if os.path.exists(item_path):
            if os.path.isdir(item_path):
                shutil.rmtree(item_path)
            else:
                os.remove(item_path)


def main():
    copy_folder()
    clean_project()
    print("Folder 'gpt4all-backend' copied.")


if __name__ == "__main__":
    main()
