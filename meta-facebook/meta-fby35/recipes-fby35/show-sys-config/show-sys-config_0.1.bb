# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Show System Configuration Utility"
DESCRIPTION = "Util for showing system configuration"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://show_sys_config.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

LOCAL_URI = " \
    file://show_sys_config.c \
    file://meson.build\
    "

inherit meson pkgconfig
inherit legacy-packages

pkgdir = "show_sys_config"

DEPENDS = "libbic libpal libipmi libipmb libfby35-common jansson"
