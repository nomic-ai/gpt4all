import os

import pytest

# test that the chat executable exists
def test_chat_environment():
    assert os.path.exists(os.environ['CHAT_EXECUTABLE'])
