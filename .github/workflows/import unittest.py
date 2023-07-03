import unittest

def is_even(num):
  return num % 2 == 0

class TestIsEven(unittest.TestCase):

  def test_is_even_with_even_number(self):
    self.assertTrue(is_even(4))

  def test_is_even_with_odd_number(self):
    self.assertFalse(is_even(5))

if __name__ == '__main__':
  unittest.main()