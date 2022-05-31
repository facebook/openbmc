# Copyright (c) 2022 Cisco Systems Inc.
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

PACKAGECONFIG += "disable-watchdog"
PACKAGECONFIG += "boot-info"

LOCAL_URI += "file://setup_i2c.sh \
            file://mount_data1.service \
            file://eth0_mac_fixup.sh \
            file://power-on.sh \
            file://sol.sh \
            file://wedge_power.sh \
            file://board-utils.sh \
            file://oob_switch_utils.sh \
	    "

OPENBMC_UTILS_FILES += " \
    eth0_mac_fixup.sh \
    sol.sh \
    wedge_power.sh \
    board-utils.sh \
    oob_switch_utils.sh \
    "

inherit systemd

do_work_systemd() {
  install -d ${D}/usr/local/bin
  install -d ${D}${systemd_system_unitdir}

  # mount secondary storage (emmc) to /mnt/data1
  install -m 0644 ${S}/mount_data1.service ${D}${systemd_system_unitdir}

  # Install mount_data0.sh for /mnt/data (SPI Flash)
  install -m 755 ${S}/mount_data0.sh ${D}/usr/local/bin/mount_data0.sh

  # instantiate peripherals like IDPROM, temp sensor and Switch
  install -m 755 ${S}/setup_i2c.sh ${D}/usr/local/bin/setup_i2c.sh

  # fix mac address in eth0 and u-boot env by reading from IDPROM
  install -m 755 ${S}/eth0_mac_fixup.sh ${D}/usr/local/bin/eth0_mac_fixup.sh

  # power-on script that will be used by power-on service
  install -m 755 power-on.sh ${D}/usr/local/bin/power-on.sh

}

do_install_board() {
  # Taken from reference platforms:
  # for backward compatible, create /usr/local/fbpackages/utils/ast-functions
  olddir="/usr/local/fbpackages/utils"
  install -d ${D}${olddir}
  ln -s "/usr/local/bin/openbmc-utils.sh" "${D}${olddir}/ast-functions"

  do_work_systemd
}

do_install:append() {
  do_install_board
}

FILES:${PN} += "${sysconfdir} ${datadir}"

SYSTEMD_SERVICE:${PN} += "mount_data1.service"
SYSTEMD_SERVICE:${PN} += "setup_i2c.service"

# Feature not required (error if enabled)
SYSTEMD_SERVICE:${PN}:remove = "enable_watchdog_ext_signal.service"

