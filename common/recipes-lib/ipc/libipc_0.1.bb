# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "IPC Helper Library"
DESCRIPTION = "library to abstract away IPC from user (Underlying implementation could be socket or dbus)"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ipc.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

BBCLASSEXTEND = "native"

SRC_URI = "\
    file://meson.build \
    file://ipc.c \
    file://ipc.h \
    "

S = "${WORKDIR}"

inherit meson
