import sys
from PySide6.QtWidgets import QApplication, QMessageBox
import torch

def display_info():
    app = QApplication(sys.argv)
    info_message = ""

    if torch.cuda.is_available():
        info_message += "CUDA is available!\n"
        info_message += "CUDA version: {}\n\n".format(torch.version.cuda)
    else:
        info_message += "CUDA is not available.\n\n"

    if torch.backends.mps.is_available():
        info_message += "Metal/MPS is available!\n\n"
    else:
        info_message += "Metal/MPS is not available.\n\n"

    info_message += "If you want to check the version of Metal and MPS on your macOS device, you can go to \"About This Mac\" -> \"System Report\" -> \"Graphics/Displays\" and look for information related to Metal and MPS.\n\n"

    if torch.version.hip is not None:
        info_message += "ROCm is available!\n"
        info_message += "ROCm version: {}\n".format(torch.version.hip)
    else:
        info_message += "ROCm is not available.\n"

    msg_box = QMessageBox(QMessageBox.Information, "GPU Acceleration Available?", info_message)
    msg_box.exec()

if __name__ == "__main__":
    display_info()
