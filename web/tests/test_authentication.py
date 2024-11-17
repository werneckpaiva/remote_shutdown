import unittest
from unittest.mock import patch

import settings
from remote_shutdown.authentication import lcg_random, generate_current_token


class TestLCGRandom(unittest.TestCase):

    def test_lcg_random_basic(self):
        self.assertEqual(lcg_random(seed=12345, salt=10, size=6), 288923)

    def test_lcg_zero_seed(self):
        self.assertEqual(lcg_random(seed=0, salt=5, size=4), 3923)

    def test_lcg_random_zero_salt(self):
        self.assertEqual(lcg_random(seed=543, salt=0, size=3), 543)  # Should return the seed

    def test_lcg_random_zero_salt_seed_bigger_than_size(self):
        self.assertEqual(lcg_random(seed=54321, salt=0, size=3), 321)  # Should return the seed truncated

    def test_lcg_zero_size(self):
        self.assertEqual(lcg_random(seed=100, salt=5, size=0), 0)


class TestGenerateCurrentToken(unittest.TestCase):

    @patch('remote_shutdown.authentication.get_current_minute_timestamp')  # Replace your_module
    def test_generate_current_token_no_seed(self, mock_get_timestamp):
        mock_get_timestamp.return_value = 12345
        settings.AUTHENTICATION_SALT = 20
        settings.TOKEN_SIZE = 10
        self.assertEqual(generate_current_token(), "6801820173")  # Based on lcg_random test

    @patch('remote_shutdown.authentication.lcg_random')  #
    def test_generate_current_token_with_seed(self, mock_lcg_random):
        mock_lcg_random.return_value = 54321
        settings.TOKEN_SIZE = 8
        self.assertEqual(generate_current_token(seed=98765), "00054321")  # Should pad with zeros



if __name__ == '__main__':
    unittest.main()
