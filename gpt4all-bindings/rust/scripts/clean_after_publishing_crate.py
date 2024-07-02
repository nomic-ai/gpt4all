import os
import shutil


def remove_published_backend():
    script_folder = os.path.dirname(os.path.abspath(__file__))
    destination_folder = os.path.normpath(os.path.join(script_folder, "../gpt4all-backend"))
    if os.path.exists(destination_folder):
        shutil.rmtree(destination_folder)


def main():
    remove_published_backend()
    print("Folder 'gpt4all-backend' removed.")


if __name__ == "__main__":
    main()
