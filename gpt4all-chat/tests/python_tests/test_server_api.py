import os
import pytest

# Test the chat environment variable
def test_chat_environment():

    chat_executable = os.getenv('CHAT_EXECUTABLE')
    # not none and not empty
    assert chat_executable
    assert os.path.exists(chat_executable)
