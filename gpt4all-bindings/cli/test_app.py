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
        self.LLModel_mock = LLModel

    def tearDown(self):
        self.rsm = None
        self.gpt4all = None

    def test_response_handler_patching(self):
        self.assertFalse(self.rsm._is_response_callback_patched)  # no need to patch on creation
        result = self.rsm._patch_response_callback()
        self.assertTrue(result)
        self.assertTrue(self.rsm._is_response_callback_patched)
        self.assertEqual(self.rsm._response_callback, self.gpt4all.model._response_callback)
        self.rsm._revert_response_callback()
        self.assertFalse(self.rsm._is_response_callback_patched)
        self.assertNotEqual(self.rsm._response_callback, self.gpt4all.model._response_callback)
        result = self.rsm._patch_response_callback()
        self.assertTrue(result)
        self.assertTrue(self.rsm._is_response_callback_patched)
        result = self.rsm._patch_response_callback()  # 2x should not cause inconsistent state
        self.assertFalse(result)
        self.assertTrue(self.rsm._is_response_callback_patched)
        self.assertEqual(self.rsm._response_callback, self.gpt4all.model._response_callback)
        self.rsm.__del__()  # calling once or even more times is safe
        self.assertFalse(self.rsm._is_response_callback_patched)
        self.assertNotEqual(self.rsm._response_callback, self.gpt4all.model._response_callback)
        self.rsm.__del__()
        self.assertFalse(self.rsm._is_response_callback_patched)
        self.assertNotEqual(self.rsm._response_callback, self.gpt4all.model._response_callback)
        self.assertRaises(RuntimeError, self.rsm.__enter__)  # can no longer manage a context, however

    def test_sigint_handling_activation(self):
        # TODO assert sigint manipulation and consistency
        self.assertFalse(self.rsm.is_managing_sigint)  # don't manage SIGINT initially
        result = self.rsm._patch_response_callback()
        self.assertTrue(result)
        result = self.rsm._activate_response_sigint_handler()
        self.assertTrue(result)
        self.assertTrue(self.rsm._is_response_callback_patched)
        self.assertTrue(self.rsm.is_managing_sigint)
        self.assertIsNotNone(self.rsm._old_sigint_handler)
        self.rsm._deactivate_response_sigint_handler()
        self.assertFalse(self.rsm.is_managing_sigint)
        self.assertIsNone(self.rsm._old_sigint_handler)
        result = self.rsm._activate_response_sigint_handler()
        self.assertTrue(result)
        result = self.rsm._activate_response_sigint_handler()  # 2x should not cause inconsistent state
        self.assertFalse(result)
        self.assertTrue(self.rsm.is_managing_sigint)
        self.rsm.__del__()  # calling once or even more times is safe
        self.assertFalse(self.rsm.is_managing_sigint)
        self.rsm.__del__()
        self.assertFalse(self.rsm.is_managing_sigint)
        self.assertRaises(RuntimeError, self.rsm.__enter__)  # can no longer manage a context, however

    def test_dont_handle_sigint_when_unable_to_patch(self):
        self.assertFalse(self.rsm.is_managing_sigint)  # don't manage SIGINT initially
        result = self.rsm._activate_response_sigint_handler()
        self.assertFalse(result)
        self.assertFalse(self.rsm._is_response_callback_patched)
        self.assertFalse(self.rsm.is_managing_sigint)  # will only manage SIGINT if patch in place
        result = self.rsm._activate_response_sigint_handler()
        self.assertFalse(result)
        self.assertFalse(self.rsm._is_response_callback_patched)
        self.assertFalse(self.rsm.is_managing_sigint)  # 2x should not cause inconsistent state
        self.rsm.__del__()
        self.assertFalse(self.rsm.is_managing_sigint)
        self.assertRaises(RuntimeError, self.rsm.__enter__)  # can no longer manage a context

    def test_context_manager_sigint_handling(self):
        self.assertFalse(self.rsm.is_managing_sigint)  # don't manage SIGINT initially
        self.assertTrue(self.rsm.keep_generating_response)  # by default, keep generating
        with self.rsm:
            self.assertTrue(self.rsm.is_managing_sigint)
            self.assertTrue(self.rsm.keep_generating_response)
            signal.raise_signal(signal.SIGINT)
            self.assertTrue(self.rsm.is_managing_sigint)
            self.assertFalse(self.rsm.keep_generating_response)  # stop generating
        self.assertFalse(self.rsm.is_managing_sigint)  # don't manage SIGINT afterwards
        self.assertTrue(self.rsm.keep_generating_response)  # 'keep generating' reverted to default


if __name__ == '__main__':
    unittest.main()
