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

SRC_URI += "file://mTerm1/run \
            file://mTerm2/run \
            file://mTerm3/run \
            file://mTerm4/run \
            file://mTerm5/run \
            file://mTerm6/run \
            file://mTerm7/run \
            file://mTerm-service-setup.sh \
           "

S = "${WORKDIR}"

# launch 7 mTerm services, 1 for each server
MTERM_SERVICES = "mTerm1 \
                  mTerm2 \
                  mTerm3 \
                  mTerm4 \
                  mTerm5 \
                  mTerm6 \
                  mTerm7 \
                 "


do_install_append() {
 #remove the old setting
 update-rc.d -f -r ${D} mTerm-service-setup.sh remove
  
 #install the new setting
 update-rc.d -r ${D} mTerm-service-setup.sh start 61 5 .
}
