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

LOCAL_URI += "\
    file://900_dump_gpio.sh \
    "

do_install:append() {
    showtech_rules_dir="${D}/etc/showtech/rules/"
    install -d ${showtech_rules_dir}

    install -m 755 900_dump_gpio.sh ${showtech_rules_dir}/900_dump_gpio.sh
}

RDEPENDS:${PN} += "bash"
FILES:${PN} += "/etc/showtech/rules/"
