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
SUMMARY = "Crashdump utility"
DESCRIPTION = "Util for generating crashdumps"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://COPYING;md5=eb723b61539feef013de476e68b5c50a"

LOCAL_URI = " \
    file://dump.sh \
    file://crashdump_coreid \
    file://crashdump_pcu \
    file://crashdump_ubox \
    file://crashdump_pcie \
    file://crashdump_pcie_pch \
    file://crashdump_iio \
    file://crashdump_imc \
    file://crashdump_mesh \
    file://crashdump_upi \
    file://crashdump_uncore \
    file://crashdump_msr \
    file://autodump.sh \
    file://COPYING \
    "

binfiles += "dump.sh \
             autodump.sh \
            "

pkgdir = "crashdump"
RDEPENDS:${PN} += "bash"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 644 crashdump_coreid ${dst}/crashdump_coreid
  install -m 644 crashdump_pcu ${dst}/crashdump_pcu
  install -m 644 crashdump_ubox ${dst}/crashdump_ubox
  install -m 644 crashdump_pcie ${dst}/crashdump_pcie
  install -m 644 crashdump_pcie_pch ${dst}/crashdump_pcie_pch
  install -m 644 crashdump_iio ${dst}/crashdump_iio
  install -m 644 crashdump_imc ${dst}/crashdump_imc
  install -m 644 crashdump_mesh ${dst}/crashdump_mesh
  install -m 644 crashdump_upi ${dst}/crashdump_upi
  install -m 644 crashdump_uncore ${dst}/crashdump_uncore
  install -m 644 crashdump_msr ${dst}/crashdump_msr
  for f in ${binfiles}; do
    install -m 755 $f ${dst}/$f
    ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/crashdump ${prefix}/local/bin "
