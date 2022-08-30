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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

LOCAL_URI += " \
    file://mTerm1/run \
    file://mTerm2/run \
    file://mTerm-service-setup.sh \
    file://mTerm_server1.service \
    file://mTerm_server2.service \
    "

# launch 2 mTerm services
MTERM_SERVICES = "mTerm1 \
                  mTerm2 \
                 "

MTERM_SYSTEMD_SERVICES = "mTerm_server1.service \
                          mTerm_server2.service \
                         "

sysv_install:append() {
  #remove the old setting
  update-rc.d -f -r ${D} mTerm-service-setup.sh remove

  #install the new setting
  update-rc.d -r ${D} mTerm-service-setup.sh start 61 5 .
}
