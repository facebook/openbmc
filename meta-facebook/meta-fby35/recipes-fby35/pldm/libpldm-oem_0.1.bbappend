# Copyright 2024-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"

LOCAL_URI += " \
        file://plat/meson.build \
        file://platform_event.cpp \
        file://event_manager.hpp \
        file://event_manager.cpp \
        "

DEPENDS += " nlohmann-json "