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

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://eth0_mac_fixup.sh \
    file://fix_fru_eeprom.py \
    file://setup_adc.sh \
    file://setup_i2c.sh \
    "

DEPENDS_append = " update-rc.d-native"

RDEPENDS_${PN} += "python"

do_install_append() {
    install -m 755 setup_i2c.sh ${D}${sysconfdir}/init.d/setup_i2c.sh
    update-rc.d -r ${D} setup_i2c.sh start 80  S .

    # EEPROM is loaded in setup_i2c.sh
    install -m 755 fix_fru_eeprom.py ${D}${sysconfdir}/init.d/fix_fru_eeprom.py
    update-rc.d -r ${D} fix_fru_eeprom.py start 81 S .


    # networking is started after rcS, any start level within rcS after loading
    # EEPROM should work
    install -m 755 eth0_mac_fixup.sh ${D}${sysconfdir}/init.d/eth0_mac_fixup.sh
    update-rc.d -r ${D} eth0_mac_fixup.sh start 99 S .

    install -m 755 setup_adc.sh ${D}${sysconfdir}/init.d/setup_adc.sh
    update-rc.d -r ${D} setup_adc.sh start 80 2 3 5 .
}
