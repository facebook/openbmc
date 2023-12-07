# Copyright 2023-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRCREV = "df5c868a14eb0c7e7aa0eb4b44587013c95997b9"

SRC_URI:remove = "file://0002-Fix-GCC-13-issue.patch \
                  file://0002-Add-PECI-Telemetry-command.patch \
                 "

SRC_URI += "file://0001-Add-PECI-Telemetry-command.patch \
           "
