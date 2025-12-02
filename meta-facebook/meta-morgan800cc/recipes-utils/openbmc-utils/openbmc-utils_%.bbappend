# Copyright (c) Meta Platforms, Inc. and affiliates.
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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

LOCAL_URI += "\
    file://board-utils.sh \
    file://setup_i2c.sh \
    file://setup-gpio.sh \
    file://bios-util.sh \
    file://sys-fs-pstore.mount \
    file://mount_flash1_data0.sh \
    "

OPENBMC_UTILS_FILES += "mount_flash1_data0.sh"

do_install:append() {
    install -d ${D}/usr/local/bin
    install -m 755 ${UNPACKDIR}/bios-util.sh ${D}/usr/local/bin
}

do_work_systemd:append() {
    install -m 0644 sys-fs-pstore.mount ${D}${systemd_system_unitdir}
}

SYSTEMD_SERVICE:${PN} += "sys-fs-pstore.mount"

#Not needed for morgan800cc
SYSTEMD_SERVICE:${PN}:remove = "mount_data1.service"

