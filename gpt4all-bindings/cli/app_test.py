import unittest
from unittest import TestCase
from unittest.mock import patch

import signal
from gpt4all import GPT4All

from app import ResponseSigintManager


DEFAULT_MODEL_NAME = 'ggml-gpt4all-j-v1.3-groovy'


class TestCreateResponseManager(TestCase):

    @patch('gpt4all.pyllmodel.LLModel')
    def setUp(self, LLModel):
        self.gpt4all = GPT4All(DEFAULT_MODEL_NAME)

    def tearDown(self):
        self.gpt4all = None

    def test_creation(self):
        self.assertRaises(TypeError, lambda: ResponseSigintManager(None))
        rsm = ResponseSigintManager(self.gpt4all)
        self.assertIsInstance(rsm, ResponseSigintManager)
        self.assertIsNotNone(rsm.gpt4all)


class TestResponseManager(TestCase):

    @patch('gpt4all.pyllmodel.LLModel')
    def setUp(self, LLModel):
        self.gpt4all = GPT4All(DEFAULT_MODEL_NAME)
        self.rsm = ResponseSigintManager(self.gpt4all)
        
    def tearDown(self):
        self.rsm = None
        self.gpt4all = None
    
    def test_response_handler_patching(self):
        # TODO check return values of calls
        # TODO assert response callback monkey patching and consistency
        self.assertFalse(self.rsm._is_response_handler_patched)  # no need to patch on creation
        self.rsm._patch_response_handler()
        self.assertTrue(self.rsm._is_response_handler_patched)
        self.rsm._revert_response_handler()
        self.assertFalse(self.rsm._is_response_handler_patched)
        self.rsm._patch_response_handler()
        self.rsm._patch_response_handler()  # 2x should not cause inconsistent state
        self.assertTrue(self.rsm._is_response_handler_patched)
        self.rsm.__del__()  # calling once or even more times is safe
        self.assertFalse(self.rsm._is_response_handler_patched)
        self.rsm.__del__()
        self.assertFalse(self.rsm._is_response_handler_patched)
        self.assertRaises(RuntimeError, self.rsm.__enter__)  # can no longer manage a context, however

    def test_sigint_handling(self):
        # TODO check return values of calls
        # TODO assert sigint manipulation and consistency
        self.assertFalse(self.rsm.is_managing_sigint)  # don't manage SIGINT on creation
        self.rsm._activate_response_sigint_handler()
        self.assertFalse(self.rsm._is_response_handler_patched)
        self.assertFalse(self.rsm.is_managing_sigint)  # will only manage SIGINT if patch in place
        self.rsm._patch_response_handler()
        self.rsm._activate_response_sigint_handler()
        self.assertTrue(self.rsm._is_response_handler_patched)
        self.assertTrue(self.rsm.is_managing_sigint)
        self.assertIsNotNone(self.rsm._old_sigint_handler)
        self.rsm._deactivate_response_sigint_handler()
        self.assertFalse(self.rsm.is_managing_sigint)
        self.assertIsNone(self.rsm._old_sigint_handler)
        self.rsm._activate_response_sigint_handler()
        self.rsm._activate_response_sigint_handler()  # 2x should not cause inconsistent state
        self.assertTrue(self.rsm.is_managing_sigint)
        self.rsm.__del__()  # calling once or even more times is safe
        self.assertFalse(self.rsm.is_managing_sigint)
        self.rsm.__del__()
        self.assertFalse(self.rsm.is_managing_sigint)
        self.assertRaises(RuntimeError, self.rsm.__enter__)  # can no longer manage a context, however



if __name__ == '__main__':
    unittest.main()
