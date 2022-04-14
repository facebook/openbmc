# Copyright 2020-present Facebook. All Rights Reserved.
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

PACKAGECONFIG += "disable-watchdog"
PACKAGECONFIG += "boot-info"

DEPENDS:append = " update-rc.d-native"

do_install_board() {
    # init
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d

    # the script to mount /mnt/data
    install -m 0755 ${S}/mount_data0.sh ${D}${sysconfdir}/init.d/mount_data0.sh
    update-rc.d -r ${D} mount_data0.sh start 03 S .

    install -m 0755 ${S}/rc.early ${D}${sysconfdir}/init.d/rc.early
    update-rc.d -r ${D} rc.early start 04 S .

    # create VLAN intf automatically
    install -d ${D}/${sysconfdir}/network/if-up.d
    install -m 755 create_vlan_intf ${D}${sysconfdir}/network/if-up.d/create_vlan_intf

    install -m 0755 ${S}/rc.local ${D}${sysconfdir}/init.d/rc.local
    update-rc.d -r ${D} rc.local start 99 2 3 4 5 .
}

do_install:append() {
  do_install_board
}

FILES:${PN} += "${sysconfdir}"
