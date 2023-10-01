from PySide6.QtWidgets import QDialog, QVBoxLayout, QRadioButton, QPushButton, QButtonGroup
import os
import yaml
import subprocess
import threading

class DownloadModelDialog(QDialog):
    def __init__(self, parent=None):
        super(DownloadModelDialog, self).__init__(parent)
        self.setWindowTitle('Download Model')
        self.setLayout(QVBoxLayout())

        try:
            with open('config.yaml', 'r') as file:
                config = yaml.safe_load(file)
                self.available_models = config.get('AVAILABLE_MODELS', [])
        except Exception as e:
            print(f"Error loading config.yaml: {e}")
            self.available_models = []

        self.button_group = QButtonGroup(self)
        self.selected_model = None

        downloaded_models = [f for f in os.listdir('Embedding_Models') if os.path.isdir(os.path.join('Embedding_Models', f))]

        for model in self.available_models:
            radiobutton = QRadioButton(model)
            radiobutton.setEnabled(model not in downloaded_models)
            self.layout().addWidget(radiobutton)
            self.button_group.addButton(radiobutton)

        download_button = QPushButton('Download', self)
        download_button.clicked.connect(self.accept)
        self.layout().addWidget(download_button)

    def accept(self):
        for button in self.button_group.buttons():
            if button.isChecked():
                self.selected_model = button.text()
                break
        if self.selected_model:
            super().accept()

def download_embedding_model(parent):
    if not os.path.exists('Embedding_Models'):
        os.makedirs('Embedding_Models')

    dialog = DownloadModelDialog(parent)
    if dialog.exec_():
        selected_model = dialog.selected_model

        if selected_model:
            model_url = f"https://huggingface.co/{selected_model}"
            target_directory = os.path.join("Embedding_Models", selected_model.replace("/", "--"))

            def download_model():
                subprocess.run(["git", "clone", model_url, target_directory])
                print(f"{selected_model} has been downloaded and is ready to use!")

            download_thread = threading.Thread(target=download_model)
            download_thread.start()
