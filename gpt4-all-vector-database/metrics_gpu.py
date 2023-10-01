from pynvml import *
from multiprocessing import Process, Pipe, Event
import time
from PySide6.QtCore import QTimer

def monitor_nvml(pipe, stop_event):
    nvmlInit()
    handle = nvmlDeviceGetHandleByIndex(0)

    while not stop_event.is_set():
        memory_info = nvmlDeviceGetMemoryInfo(handle)
        utilization_rates = nvmlDeviceGetUtilizationRates(handle)

        memory_used_str = f"{memory_info.used / (1024 * 1024):.2f} MiB"
        gpu_utilization = f"{utilization_rates.gpu}%"

        data = (memory_used_str, gpu_utilization)
        pipe.send(data)

        time.sleep(0.5)

    nvmlShutdown()

def start_monitoring_gpu():
    stop_event = Event()
    parent_conn, child_conn = Pipe()
    p = Process(target=monitor_nvml, args=(child_conn, stop_event))
    p.start()
    return parent_conn, p, stop_event

def stop_monitoring_gpu(p, stop_event):
    stop_event.set()
    p.join()

class GPU_Monitor:
    def __init__(self, vram_label, gpu_label, root):
        self.vram_label = vram_label
        self.gpu_label = gpu_label
        self.root = root
        self.parent_conn, self.process, self.stop_event = start_monitoring_gpu()
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_gpu_info)
        self.timer.start(500)

    def update_gpu_info(self):
        if self.parent_conn.poll():
            memory_used_str, gpu_utilization = self.parent_conn.recv()
            self.vram_label.setText(f"VRAM: {memory_used_str}")
            self.gpu_label.setText(f"GPU: {gpu_utilization}")

    def stop_and_exit_gpu_monitor(self):
        self.timer.stop()
        stop_monitoring_gpu(self.process, self.stop_event)
        self.root.close()