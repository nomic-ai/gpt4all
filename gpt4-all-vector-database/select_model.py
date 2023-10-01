import os
import yaml
from PySide6.QtWidgets import QApplication, QFileDialog

def load_config():
    with open("config.yaml", 'r') as stream:
        return yaml.safe_load(stream)

def select_embedding_model_directory():
    initial_dir = 'Embedding_Models' if os.path.exists('Embedding_Models') else os.path.expanduser("~")
    chosen_directory = QFileDialog.getExistingDirectory(None, "Select Embedding Model Directory", initial_dir)
    
    if chosen_directory:
        config_data = load_config()
        config_data["EMBEDDING_MODEL_NAME"] = chosen_directory
        with open("config.yaml", 'w') as file:
            yaml.dump(config_data, file)

        print(f"Selected directory: {chosen_directory}")

if __name__ == '__main__':
    app = QApplication([])
    select_embedding_model_directory()
    app.exec()
