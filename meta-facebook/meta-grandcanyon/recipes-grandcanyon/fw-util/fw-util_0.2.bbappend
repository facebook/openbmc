# Copyright 2020-present Facebook. All Rights Reserved.
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

SRC_URI += "\
        file://nic_ext.cpp \
        file://nic_ext.h \
        file://bmc_fpga.cpp \
        file://bmc_fpga.h \
        file://platform.cpp \
        file://server.h \
        file://bic_cpld.cpp \
        file://bic_vr.cpp \
        file://bic_vr.h \
        file://scc_exp.cpp \
        file://scc_exp.h \
        file://grandcanyon.json \
        "

do_unpack_append () {
    bb.build.exec_func('do_replace_image_parts_json', d)
}

do_replace_image_parts_json() {
        bbnote "replace common image_parts.json with grandcanyon.json"
        mv -vf ${S}/image_parts.json ${S}/image_parts-generic.json
        mv -vf ${S}/grandcanyon.json ${S}/image_parts.json
}

CXXFLAGS += " -DBIC_SUPPORT "
DEPENDS += " libpal libbic libncsi libnl-wrapper libkv libfpga libfbgc-common libexp "
RDEPENDS_${PN} += " libpal libbic libncsi libnl-wrapper libkv libfpga libfbgc-common libexp "
LDFLAGS += " -lpal -lbic -lnl-wrapper -lkv -lfpga -lexp "
