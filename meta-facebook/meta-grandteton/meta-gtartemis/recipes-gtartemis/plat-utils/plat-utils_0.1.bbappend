# Copyright 2023-present Facebook. All Rights Reserved.
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

LOCAL_URI += " \
    file://sol-util \
    file://usb-util \
    file://setup-server-uart.sh \
    file://setup-snr-mon.sh \
    file://setup-i2c-clk.sh \
    file://setup-usb-hub.sh \
    file://setup-reboot-init.sh \
    "

binfiles += " usb-util setup-i2c-clk.sh setup-usb-hub.sh"

do_install:append() {
  install -m 755 setup-usb-hub.sh ${D}${sysconfdir}/init.d/setup-usb-hub.sh
  update-rc.d -r ${D} setup-usb-hub.sh start 65 5 .

  install -m 755 setup-i2c-clk.sh ${D}${sysconfdir}/init.d/setup-i2c-clk.sh
  update-rc.d -r ${D} setup-i2c-clk.sh start 50 5 .
}
