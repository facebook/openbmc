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

FILESEXTRAPATHS_append := "${THISDIR}/files:"
SRC_URI += "file://pal_peci-util.c \
            file://crashdump_p0_coreid \
            file://crashdump_p0_msr \
            file://crashdump_p1_coreid \
            file://crashdump_p1_msr \
            file://crashdump_pcie \
            file://crashdump_pcie_bus \
            file://autodump.sh \
           "

binfiles += "autodump.sh \
            "

do_install_append() {
  install -d ${D}${sysconfdir}/peci
  install -m 644 crashdump_p0_coreid ${D}${sysconfdir}/peci/crashdump_p0_coreid
  install -m 644 crashdump_p0_msr ${D}${sysconfdir}/peci/crashdump_p0_msr
  install -m 644 crashdump_p1_coreid ${D}${sysconfdir}/peci/crashdump_p1_coreid
  install -m 644 crashdump_p1_msr ${D}${sysconfdir}/peci/crashdump_p1_msr
  install -m 644 crashdump_pcie ${D}${sysconfdir}/peci/crashdump_pcie
  install -m 644 crashdump_pcie_bus ${D}${sysconfdir}/peci/crashdump_pcie_bus
  
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  for f in ${binfiles}; do
    install -m 755 $f ${dst}/$f
    ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done
}

DEPENDS += " libgpio"

FILES_${PN} += "${sysconfdir} "

