import os
import tempfile
import unittest

import rest_config as sut
from acl_providers import common_acl_provider_base


valid_config = [
    (
        b"""
        """,
        {
            "acl_provider": "acl_providers.dummy_acl_provider.DummyAclProvider",
            "acl_settings": {},
            "ports": ["8080"],
            "ssl_ports": [],
            "logfile": "/tmp/rest.log",
            "logformat": "default",
            "loghandler": "file",
            "writable": False,
            "ssl_certificate": None,
            "ssl_key": None,
            "ssl_ca_certificate": None,
        },
    ),
    (
        b"""
        [listen]
        port = 8081
        """,
        {
            "acl_provider": "acl_providers.dummy_acl_provider.DummyAclProvider",
            "acl_settings": {},
            "ports": ["8081"],
            "ssl_ports": [],
            "logfile": "/tmp/rest.log",
            "logformat": "default",
            "loghandler": "file",
            "writable": False,
            "ssl_certificate": None,
            "ssl_key": None,
            "ssl_ca_certificate": None,
        },
    ),
    (
        b"""
        [listen]
        port = 8080,8888
        ssl_port = 8443
        """,
        {
            "acl_provider": "acl_providers.dummy_acl_provider.DummyAclProvider",
            "acl_settings": {},
            "ports": ["8080", "8888"],
            "ssl_ports": ["8443"],
            "logfile": "/tmp/rest.log",
            "logformat": "default",
            "loghandler": "file",
            "writable": False,
            "ssl_certificate": None,
            "ssl_key": None,
            "ssl_ca_certificate": None,
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
            "acl_provider": "acl_providers.dummy_acl_provider.DummyAclProvider",
            "acl_settings": {},
            "ports": ["8080", "8888"],
            "ssl_ports": ["8443"],
            "logfile": "/some/other/file.log",
            "logformat": "default",
            "loghandler": "file",
            "writable": True,
            "ssl_certificate": None,
            "ssl_key": None,
            "ssl_ca_certificate": None,
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
            "acl_provider": "acl_providers.dummy_acl_provider.DummyAclProvider",
            "acl_settings": {},
            "ports": ["8080", "8888"],
            "ssl_ports": ["8443"],
            "logfile": "/tmp/rest.log",
            "logformat": "json",
            "loghandler": "stdout",
            "writable": True,
            "ssl_certificate": None,
            "ssl_key": None,
            "ssl_ca_certificate": None,
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
            "acl_provider": "acl_providers.dummy_acl_provider.DummyAclProvider",
            "acl_settings": {},
            "ports": ["8080", "8888"],
            "ssl_ports": ["8443"],
            "logfile": "/tmp/rest.log",
            "logformat": "default",
            "loghandler": "file",
            "writable": True,
            "ssl_certificate": None,
            "ssl_key": None,
            "ssl_ca_certificate": None,
        },
    ),
    (
        b"""
        [acl]
        provider = acl_providers.test
        firstparam = a
        secondparam = b
        """,
        {
            "acl_provider": "acl_providers.test",
            "acl_settings": {"firstparam": "a", "secondparam": "b"},
            "ports": ["8080"],
            "ssl_ports": [],
            "logfile": "/tmp/rest.log",
            "logformat": "default",
            "loghandler": "file",
            "writable": False,
            "ssl_certificate": None,
            "ssl_key": None,
            "ssl_ca_certificate": None,
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
                self.maxDiff = None
                self.assertDictEqual(ret, expected)
                os.unlink(f.name)

    def test_acl_provider_loader(self):
        loaded_provider = sut.load_acl_provider(
            {
                "acl_provider": "acl_providers.dummy_acl_provider.DummyAclProvider",
                "acl_settings": {},
            }
        )
        self.assertIsInstance(loaded_provider, common_acl_provider_base.AclProviderBase)
        with self.assertRaises(
            ValueError,
            msg="Invalid ACL Provider antigravity.browser, Please use acl providers from the acl_providers package",
        ):
            loaded_provider = sut.load_acl_provider(
                {"acl_provider": "antigravity.browser"}
            )
