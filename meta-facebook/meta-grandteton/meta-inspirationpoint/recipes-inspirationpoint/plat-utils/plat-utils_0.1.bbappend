# Copyright 2022-present Facebook. All Rights Reserved.
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
    file://setup-gpio-cover.sh \
    file://setup-cover-dev.sh \
    file://setup-pwon-snr.sh \
    file://sync_date.sh \
    file://setup-server-uart.sh \
    "

do_install:append() {
# setup-pwon-snr.sh
  install -m 755 setup-pwon-snr.sh  ${D}${sysconfdir}/init.d/setup-pwon-snr.sh
  update-rc.d -r ${D} setup-pwon-snr.sh start 68 5 .
}

