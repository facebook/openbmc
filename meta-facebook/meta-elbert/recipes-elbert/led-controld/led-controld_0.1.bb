# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "ELBERT LED Control Daemon"
DESCRIPTION = "Daemon to change ELBERT System LEDs based on system condition"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://led-controld.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"


DEPENDS_append = "update-rc.d-native"

# ELBERTTODO 447407 LED CONTROL DAEMON

S = "${WORKDIR}"
