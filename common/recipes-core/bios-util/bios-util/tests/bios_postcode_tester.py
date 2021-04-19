# Copyright 2004-present Facebook. All Rights Reserved.

import os.path
import unittest

from bios_base_tester import captured_output
from bios_postcode import postcode


TEST_POST_CODE_FILE = "./test-data/test-post_code_buffer.bin"
TEST_POST_CODE_RESULT = "./test-data/test-post_code_result.bin"


class BiosUtilPostCodeUnitTest(unittest.TestCase):
    # Following is BIOS POST code related test:

    def test_bios_postcode(self):
        """
        Test Boot POST code response data is match in expectation
        """
        if os.path.isfile(TEST_POST_CODE_RESULT):
            postcode_result = open(TEST_POST_CODE_RESULT, "r")
        else:
            postcode_result = "\n"

        with captured_output() as (out, err):
            self.test_postcode = postcode("server", TEST_POST_CODE_FILE)
            self.assertTrue(self.test_postcode)
        output = out.getvalue().strip()
        self.assertEqual(output, postcode_result.read().strip())
