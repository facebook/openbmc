# Copyright 2015-present Facebook. All Rights Reserved.
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

SUMMARY = "OpenBMC Utilities"
DESCRIPTION = "Various OpenBMC utilities"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=eb723b61539feef013de476e68b5c50a"

SRC_URI = " \
    file://COPYING \
    file://mount_data0.sh \
    file://openbmc-utils.sh \
    file://rc.early \
    file://rc.local \
    file://dhclient-exit-hooks \
    file://rm_poweroff_cmd.sh \
    file://revise_inittab \
    "

OPENBMC_UTILS_FILES = " \
    mount_data0.sh \
    openbmc-utils.sh \
    rc.early \
    rc.local \
    rm_poweroff_cmd.sh \
    revise_inittab \
    "

S = "${WORKDIR}"

DEPENDS = "update-rc.d-native"
RDEPENDS_${PN} += "bash"

do_install() {
    pkgdir="/usr/local/packages/utils"
    dstdir="${D}${pkgdir}"
    install -d $dstdir
    localbindir="${D}/usr/local/bin"
    install -d ${localbindir}
    for f in ${OPENBMC_UTILS_FILES}; do
        install -m 755 $f ${dstdir}/${f}
        ln -s ${pkgdir}/${f} ${localbindir}
    done

    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d

    # the script to process dhcp options
    install -m 0755 ${WORKDIR}/dhclient-exit-hooks ${D}${sysconfdir}/dhclient-exit-hooks

    # the script to mount /mnt/data, after udev (level 4) is started
    install -m 0755 ${WORKDIR}/mount_data0.sh ${D}${sysconfdir}/init.d/mount_data0.sh
    update-rc.d -r ${D} mount_data0.sh start 05 S .

    install -m 0755 ${WORKDIR}/rc.early ${D}${sysconfdir}/init.d/rc.early
    update-rc.d -r ${D} rc.early start 06 S .

    install -m 0755 ${WORKDIR}/rc.local ${D}${sysconfdir}/init.d/rc.local
    update-rc.d -r ${D} rc.local start 99 2 3 4 5 .

    install -m 0755 ${WORKDIR}/rm_poweroff_cmd.sh ${D}${sysconfdir}/init.d/rm_poweroff_cmd.sh
    update-rc.d -r ${D} rm_poweroff_cmd.sh start 99 S .
}

FILES_${PN} += "/usr/local"
