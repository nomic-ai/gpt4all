import sys
import random
import threading
import gpt
from PyQt5.QtCore import pyqtSignal, QObject, Qt
from PyQt5.QtGui import QTextCursor, QColor, QPalette, QTextCharFormat, QPen, QTextFormat
from PyQt5.QtWidgets import QApplication, QMainWindow, QTextBrowser, QTextEdit, QPushButton, QVBoxLayout, QHBoxLayout, \
    QWidget


class LibraryExecutor(QObject):
    resultReady = pyqtSignal(str)

    def __init__(self):
        super().__init__()

    def processString(self, input_string):
        if input_string:
            result = gpt.ask(input_string)
            self.resultReady.emit("GPT4ALL: \n"+result)


class Message:
    def __init__(self, text, isSentByMe):
        self.text = text
        self.isSentByMe = isSentByMe


class ChatWindow(QMainWindow):

    def __init__(self):
        super().__init__()
        self.setWindowTitle("GPT4ALL_GUI")
        self.setGeometry(100, 100, 360, 550)
        self.setAutoFillBackground(True)
        # Set up the GUI for the chat application
        self.messageHistory = QTextBrowser()
        self.messageHistory.setReadOnly(True)
        self.messageHistory.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.messageHistory.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOn)
        self.messageEditor = QTextEdit()
        self.messageEditor.setMaximumHeight(50)
        self.sendButton = QPushButton("Send")

        # Set up layouts for the GUI
        messageLayout = QHBoxLayout()
        messageLayout.addWidget(self.messageEditor)
        messageLayout.addWidget(self.sendButton)

        centralWidget = QWidget()
        mainLayout = QVBoxLayout()
        mainLayout.addWidget(self.messageHistory)
        mainLayout.addLayout(messageLayout)
        centralWidget.setLayout(mainLayout)
        self.setCentralWidget(centralWidget)

        # Set up the LibraryExecutor thread and connect to its signal when result is ready
        self.executor = LibraryExecutor()
        self.executorThread = threading.Thread(target=self.executor.processString, args=("",))
        self.executorThread.start()
        self.executor.resultReady.connect(self.displayResult)

        # Connect the send button to the sendAndDisplayMessage method
        self.sendButton.clicked.connect(self.sendAndDisplayMessage)

    def sendAndDisplayMessage(self):
        messageText = self.messageEditor.toPlainText().strip()
        if messageText:
            self.addSentMessage("You: \n"+messageText)

            # Send the message to the LibraryExecutor thread
            self.executorThread = threading.Thread(target=self.executor.processString, args=(messageText,))
            self.executorThread.start()

        # Clear the message editor
        self.messageEditor.clear()

    def addReceivedMessage(self, messageText):
        message = Message(messageText, False)
        self.addMessageToHistory(message)

    def addSentMessage(self, messageText):
        message = Message(messageText, True)
        self.addMessageToHistory(message)

    def addMessageToHistory(self, message):
        # Add the message to the message history
        cursor = self.messageHistory.textCursor()
        cursor.movePosition(QTextCursor.End)
        self.messageHistory.setTextCursor(cursor)
        messageFormat = QTextCharFormat()
        messageFormat.setBackground(QColor(240, 240, 240))

        if message.isSentByMe:
            messageFormat.setForeground(Qt.blue)
            #alignment = Qt.AlignRight
        else:
            messageFormat.setForeground(Qt.darkGreen)
            #alignment = Qt.AlignLeft

        cursor.insertText(' ' + message.text + '\n', messageFormat)
        cursor.movePosition(QTextCursor.PreviousBlock)
        cursor.select(QTextCursor.BlockUnderCursor)
        #cursor.setAlignment(alignment)
        cursor.clearSelection()

    def displayResult(self, result):
        # Add the result to the message history
        self.addReceivedMessage(result)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    chatWindow = ChatWindow()
    chatWindow.show()
    sys.exit(app.exec_())
