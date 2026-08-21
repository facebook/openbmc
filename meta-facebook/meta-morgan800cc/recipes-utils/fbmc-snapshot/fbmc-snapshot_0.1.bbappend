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
    file://901_dump_mcb_fpga.sh \
    file://902_dump_scm_fpga.sh \
    file://903_collect_bios_info.sh \
    file://904_display_otp_bits.sh \
    "

do_install:append() {
    showtech_rules_dir="${D}/etc/showtech/rules/"
    install -d ${showtech_rules_dir}

    install -m 755 900_dump_gpio.sh ${showtech_rules_dir}/900_dump_gpio.sh
    install -m 755 901_dump_mcb_fpga.sh ${showtech_rules_dir}/901_dump_mcb_fpga.sh
    install -m 755 902_dump_scm_fpga.sh ${showtech_rules_dir}/902_dump_scm_fpga.sh
    install -m 755 903_collect_bios_info.sh ${showtech_rules_dir}/903_collect_bios_info.sh
    install -m 755 904_display_otp_bits.sh ${showtech_rules_dir}/904_display_otp_bits.sh
}

RDEPENDS:${PN} += "bash"
FILES:${PN} += "/etc/showtech/rules/"
