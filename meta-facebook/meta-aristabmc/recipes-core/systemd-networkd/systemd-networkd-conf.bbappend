# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

SRC_URI += "file://10-eth0.network \
            file://40-eth0.4092.netdev \
            file://40-eth0.4092.network \
           "
do_install:append() {
    install -d ${D}${sysconfdir}/systemd/network
    install -m 0644 40-eth0.4092.network ${D}${sysconfdir}/systemd/network
    install -m 0644 40-eth0.4092.netdev ${D}${sysconfdir}/systemd/network
}

CONFFILES:${PN} += "${sysconfdir}/systemd/network/40-eth0.4092.network \
                 ${sysconfdir}/systemd/network/40-eth0.4092.netdev \
                 "
