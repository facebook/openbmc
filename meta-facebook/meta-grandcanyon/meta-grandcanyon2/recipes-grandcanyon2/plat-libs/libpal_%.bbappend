# Copyright 2020-present Facebook. All Rights Reserved.
#
#
# GrandCanyon 2.0 Project
FILESEXTRAPATHS:prepend := "${THISDIR}/files/pal:"
## Because bb file have UNPACKDIR define, don't need subdir
LOCAL_URI += " \
    file://pal_sensors.c \
    file://pal_sensors.h \
    file://pal_power.c \
    file://pal_power.h \
    file://pal.c \
    file://pal.h \
    "

