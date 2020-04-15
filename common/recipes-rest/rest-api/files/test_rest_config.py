import os
import tempfile
import unittest

import rest_config as sut


valid_config = [
    (
        b"""
        """,
        {
            "ports": ["8080"],
            "ssl_ports": [],
            "logfile": "/tmp/rest.log",
            "logformat": "default",
            "loghandler": "file",
            "writable": False,
            "ssl_certificate": None,
            "ssl_key": None,
        },
    ),
    (
        b"""
        [listen]
        port = 8081
        """,
        {
            "ports": ["8081"],
            "ssl_ports": [],
            "logfile": "/tmp/rest.log",
            "logformat": "default",
            "loghandler": "file",
            "writable": False,
            "ssl_certificate": None,
            "ssl_key": None,
        },
    ),
    (
        b"""
        [listen]
        port = 8080,8888
        ssl_port = 8443
        """,
        {
            "ports": ["8080", "8888"],
            "ssl_ports": ["8443"],
            "logfile": "/tmp/rest.log",
            "logformat": "default",
            "loghandler": "file",
            "writable": False,
            "ssl_certificate": None,
            "ssl_key": None,
        },
    ),
    (
        b"""
        [listen]
        port = 8080,8888
        ssl_port = 8443

        [logging]
        filename = /some/other/file.log

        [access]
        write = true
        """,
        {
            "ports": ["8080", "8888"],
            "ssl_ports": ["8443"],
            "logfile": "/some/other/file.log",
            "logformat": "default",
            "loghandler": "file",
            "writable": True,
            "ssl_certificate": None,
            "ssl_key": None,
        },
    ),
    (
        b"""
        [listen]
        port = 8080,8888
        ssl_port = 8443

        [logging]
        format = json
        handler = stdout

        [access]
        write = true
        """,
        {
            "ports": ["8080", "8888"],
            "ssl_ports": ["8443"],
            "logfile": "/tmp/rest.log",
            "logformat": "json",
            "loghandler": "stdout",
            "writable": True,
            "ssl_certificate": None,
            "ssl_key": None,
        },
    ),
    (
        b"""
        [listen]
        port = 8080,8888
        ssl_port = 8443

        [logging]
        format = invalid
        handler = invalid

        [access]
        write = true
        """,
        {
            "ports": ["8080", "8888"],
            "ssl_ports": ["8443"],
            "logfile": "/tmp/rest.log",
            "logformat": "default",
            "loghandler": "file",
            "writable": True,
            "ssl_certificate": None,
            "ssl_key": None,
        },
    ),
]


class TestRestConfig(unittest.TestCase):
    def setUp(self):
        pass

    def test_inputs(self):
        for cfg_file_data, expected in valid_config:
            # FIXME: using temptfile has the potential to cause flaky tests,
            # but for now...
            with tempfile.NamedTemporaryFile(prefix="rest_config", delete=False) as f:
                f.write(cfg_file_data)
                f.close()
                ret = sut.parse_config(f.name)
                self.assertEqual(ret, expected)
                os.unlink(f.name)
