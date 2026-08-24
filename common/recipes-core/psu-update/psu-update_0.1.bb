# Copyright 2014-present Facebook. All Rights Reserved.
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
SUMMARY = "PSU Upgrade Functionality"
DESCRIPTION = "PSU Upgrade Functionality"
SECTION = "base"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

RDEPENDS:${PN} = "python3-core bash rackmon"
RDEPENDS:${PN}:append:ventura2 = " python3-minimalmodbus"

S = "${UNPACKDIR}"
LOCAL_URI = " \
    file://systemd_util.py \
    file://modbus_common.py \
    file://modbus_monitor.py \
    file://modbus_impl_pyrmd.py \
    file://modbus_impl_minimalmodbus.py \
    file://phosphor_modbus.py \
    file://manufacturers.py \
    file://modbus_update_helper.py \
    file://psu-update-delta.py \
    file://psu_update_delta_orv3.py \
    file://psu-update-bel.py \
    file://psu-update-artesyn.py \
    file://psu_update_aei.py \
    file://orv3_device_update_mailbox.py \
    file://rpu_update_delta_plc.py \
    file://rpu_update_delta_hex.py \
    file://rpu_update_coolermaster.py \
    file://psu-update-delta-orv3.py \
    file://psu-update-aei.py \
    file://orv3-device-update-mailbox.py \
    file://rpu-update-delta-plc.py \
    file://rpu-update-delta-hex.py \
    file://rpu-update-coolermaster.py \
    file://delta_key.py \
    file://srec.py \
    file://hexfile.py \
    "

pkgdir = "psu"

do_install() {
    dst="${D}/usr/local/fbpackages/${pkgdir}"
    bin="${D}/usr/local/bin"
    install -d $dst
    install -d $bin
    for f in ${LOCAL_URI}; do
        f=${f#file://}
        install -m 755 ${UNPACKDIR}/$f ${dst}/$f
        ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
    done
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"
FILES:${PN} = "${FBPACKAGEDIR}/psu ${prefix}/local/bin "
