#!/usr/bin/env python3

import unittest


class SimpleTests(unittest.TestCase):
    def setUp(self):
        print("I run before each test.")

    def tearDown(self):
        print("I run after each test.")

    def test_simple(self):
        self.assertEqual(1 + 1, 2)
