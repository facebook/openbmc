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

# Temporary: show-tech still installs these paths on this layer's platforms.
# Each platform sets "1" in the change that drops show-tech.
SHOWTECH_INSTALL_UTILS = "0"

LOCAL_URI += "\
    file://100_weutil.sh \
    file://101_x86_mTerm.sh \
    file://oob-status.sh \
    "

SHOWTECH_UTILS_FILES:append = " \
    oob-status.sh \
    "

do_install:append() {
    showtech_rules_dir="${D}/etc/showtech/rules/"
    install -d ${showtech_rules_dir}

    install -m 755 100_weutil.sh ${showtech_rules_dir}/100_weutil.sh
    install -m 755 101_x86_mTerm.sh ${showtech_rules_dir}/101_x86_mTerm.sh
}

RDEPENDS:${PN} += "bash"
FILES:${PN} += "/etc/showtech/rules/"
