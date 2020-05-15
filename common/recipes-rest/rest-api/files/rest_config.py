#!/usr/bin/env python3
#
# Copyright 2014-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

import configparser
import importlib
import syslog
import typing as t

from acl_providers import common_acl_provider_base


VALID_LOG_HANDLERS = ["stdout", "syslog", "file"]
VALID_LOG_FORMATS = ["default", "json"]


def parse_config(configpath):
    RestConfig = configparser.ConfigParser()
    RestConfig.read(configpath)

    writable = RestConfig.getboolean("access", "write", fallback=False)
    if writable:
        syslog.syslog(syslog.LOG_INFO, "REST: Launched with Read/Write Mode")
    else:
        syslog.syslog(syslog.LOG_INFO, "REST: Launched with Read Only Mode")
    log_handler = RestConfig.get("logging", "handler", fallback="file")
    if log_handler not in VALID_LOG_HANDLERS:
        # There is no logger yet, so we syslog the fallback warning ¯\_(ツ)_/¯
        syslog.syslog(
            syslog.LOG_WARNING,
            "Invalid logging handler ("
            + repr(log_handler)
            + ") detected, falling back to 'file'",
        )
        log_handler = "file"
    log_format = RestConfig.get("logging", "format", fallback="text")
    if log_format not in VALID_LOG_FORMATS:
        # There is no logger yet, so we syslog the fallback warning ¯\_(ツ)_/¯
        syslog.syslog(
            syslog.LOG_WARNING,
            "Invalid logging format ("
            + repr(log_format)
            + ") detected, falling back to 'default'",
        )
        log_format = "default"
    try:
        acl_settings = dict(RestConfig.items("acl"))
        acl_settings.pop("provider", None)
    except configparser.NoSectionError:
        acl_settings = {}
    return {
        "ports": RestConfig.get("listen", "port", fallback="8080").split(","),
        "ssl_ports": list(
            filter(None, RestConfig.get("listen", "ssl_port", fallback="").split(","))
        ),
        "logformat": log_format,
        "loghandler": log_handler,
        "logfile": RestConfig.get("logging", "filename", fallback="/tmp/rest.log"),
        "writable": writable,
        "ssl_certificate": RestConfig.get("ssl", "certificate", fallback=None),
        "ssl_key": RestConfig.get("ssl", "key", fallback=None),
        "ssl_ca_certificate": RestConfig.get("ssl", "ca_certificate", fallback=None),
        "acl_provider": RestConfig.get(
            "acl",
            "provider",
            fallback="acl_providers.dummy_acl_provider.DummyAclProvider",
        ),
        "acl_settings": acl_settings,
    }


def load_acl_provider(config: t.Dict) -> common_acl_provider_base.AclProviderBase:
    acl_provider = config["acl_provider"].split(".")
    if acl_provider[0] != "acl_providers":
        raise ValueError(
            "Invalid ACL Provider %s, Please use acl providers from the acl_providers package"
            % config["acl_provider"]
        )
    mod = importlib.import_module(".".join(acl_provider[:-1]))
    klass = getattr(mod, acl_provider[-1])
    if not issubclass(klass, common_acl_provider_base.AclProviderBase):
        raise ValueError("%s  is not sublass of AclProviderBase" % repr(klass))
    return klass(**config["acl_settings"])
