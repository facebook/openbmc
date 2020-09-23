import unittest

import rest_presence


EXAMPLE_PRESENCE_UTIL_OUTPUT = """
CARDS
scm : 1
FANS
fan1 : 1
fan2 : 1
fan3 : 1
fan4 : 1
POWER SUPPLIES
psu1 : 0
psu2 : 1
DEBUG CARD
debug_card : 0
"""


class TestRestConfig(unittest.TestCase):
    def test_parse_presence_info_data(self):
        parsed = rest_presence._parse_presence_info_data(EXAMPLE_PRESENCE_UTIL_OUTPUT)
        self.assertEqual(
            parsed,
            {
                "scm": "1",
                "fan1": "1",
                "fan2": "1",
                "fan3": "1",
                "fan4": "1",
                "psu1": "0",
                "psu2": "1",
                "debug_card": "0",
            },
        )
