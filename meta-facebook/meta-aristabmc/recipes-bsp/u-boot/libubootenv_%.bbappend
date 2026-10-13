# Copyright 2026-present Facebook. All Rights Reserved.
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

# Aristabmc exposes its U-Boot environment through the named MTD partition.
# The generic NOR configuration uses a legacy /dev/mtd/<name> path, which is
# not created on this platform.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append:aristabmc = " \
    file://fw_env_flash_nor.config \
    file://u-boot-env-ast2700-spi.txt \
"

# Keep the userspace environment-reset input aligned with the SPI-NOR
# environment generated for Aristabmc U-Boot.
do_install:append:aristabmc() {
    install -d ${D}${sysconfdir}
    install -m 0644 ${UNPACKDIR}/fw_env_flash_nor.config \
        ${D}${sysconfdir}/fw_env.config
    install -m 0644 ${UNPACKDIR}/u-boot-env-ast2700-spi.txt \
        ${D}${sysconfdir}/u-boot-initial-env
}
