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

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://start_mTerm_server.sh \
            file://mTerm_client.sh \
            file://mTerm_server.sh \
           "

S = "${WORKDIR}"

CONS_BIN_FILES += " \
    start_mTerm_server.sh \
    mTerm_client.sh \
    mTerm_server.sh \
    "

#do_install_append() {
#  install -d ${D}${sysconfdir}/init.d
#  install -d ${D}${sysconfdir}/rcS.d
#  install -m 755 start_mTerm_server.sh ${D}${sysconfdir}/init.d/start_mTerm_server.sh
#  update-rc.d -r ${D} start_mTerm_server.sh start 83 S .
#}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/mTerm ${prefix}/local/bin ${sysconfdir} "

# Inhibit complaints about .debug directories for the fand binary:

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
