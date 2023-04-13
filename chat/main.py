import sys
import time
import gpt
from PyQt5 import QtCore, QtGui, QtQml


class WorkerThread(QtCore.QThread):
    resultReady = QtCore.pyqtSignal(str)
    first_time_question = False

    def __init__(self, parent=None):
        super(WorkerThread, self).__init__(parent)
        self.inputString = ""

    def setInputString(self, inputString):
        self.inputString = inputString

    def run(self):
        if not self.first_time_question:
            self.resultReady.emit("Starting work on the answer. You’ll have to be patient for a little while…")
            self.first_time_question = True
        result = "Result: " + gpt.ask(self.inputString)

        self.resultReady.emit(result)


class Backend(QtCore.QObject):

    workerResultChanged = QtCore.pyqtSignal(str)

    def __init__(self, parent=None):
        super(Backend, self).__init__(parent)
        self._workerResult = ""
        self.workerThread = WorkerThread()
        self.workerThread.resultReady.connect(self.handleResultReady)

    @QtCore.pyqtProperty(str, notify=workerResultChanged)
    def workerResult(self):
        return self._workerResult

    @workerResult.setter
    def workerResult(self, value):
        if self._workerResult != value:
            self._workerResult = value
            self.workerResultChanged.emit(self._workerResult)

    @QtCore.pyqtSlot(str)
    def runWorker(self, inputString):
        self.workerThread.setInputString(inputString)
        self.workerThread.start()

    def handleResultReady(self, result):
        self.workerResult = result


if __name__ == "__main__":
    app = QtGui.QGuiApplication(sys.argv)

    backend = Backend()

    engine = QtQml.QQmlApplicationEngine()
    engine.rootContext().setContextProperty("backend", backend)
    engine.load(QtCore.QUrl.fromLocalFile("main.qml"))

    if not engine.rootObjects():
        sys.exit(-1)

    sys.exit(app.exec_())

