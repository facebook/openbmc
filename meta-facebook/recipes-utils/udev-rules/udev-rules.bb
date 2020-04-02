# Copyright 2019-present Facebook. All Rights Reserved.
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

SUMMARY = "udev rules to give predictable names to flash and persistent storage"
DESCRIPTION = "UDEV rules to unify device names across all platforms"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=12f884d2ae1ff87c09e5b7ccc2c4ca7e"

SRC_URI = "file://persistent_devices.rules \
           file://COPYING \
          "

S = "${WORKDIR}"

do_install() {
    dst=${D}${sysconfdir}/udev/
    install -d ${dst}/rules.d

    install -m 0644 persistent_devices.rules ${dst}/rules.d/80-persistent-devices.rules
}

FILES_${PN} += "${sysconfdir}"
