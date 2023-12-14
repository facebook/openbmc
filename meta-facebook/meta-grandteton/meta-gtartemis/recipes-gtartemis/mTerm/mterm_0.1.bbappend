# Copyright 2023-present Facebook. All Rights Reserved.
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
    file://mTerm_server2.service \
    file://mTerm2/run \
    file://mTerm_server1_1.service \
    file://mTerm1_1/run \
    file://mTerm_server1_2.service \
    file://mTerm1_2/run \
    file://mTerm_server2_1.service \
    file://mTerm2_1/run \
    file://mTerm_server2_2.service \
    file://mTerm2_2/run \
    file://mTerm_server3_1.service \
    file://mTerm3_1/run \
    file://mTerm_server3_2.service \
    file://mTerm3_2/run \
    file://mTerm_server4_1.service \
    file://mTerm4_1/run \
    file://mTerm_server4_2.service \
    file://mTerm4_2/run \
    file://mTerm_server5_1.service \
    file://mTerm5_1/run \
    file://mTerm_server5_2.service \
    file://mTerm5_2/run \
    file://mTerm_server6_1.service \
    file://mTerm6_1/run \
    file://mTerm_server6_2.service \
    file://mTerm6_2/run \
    file://mTerm_server7_1.service \
    file://mTerm7_1/run \
    file://mTerm_server7_2.service \
    file://mTerm7_2/run \
    file://mTerm_server8_1.service \
    file://mTerm8_1/run \
    file://mTerm_server8_2.service \
    file://mTerm8_2/run \
    file://mTerm_server9_1.service \
    file://mTerm9_1/run \
    file://mTerm_server9_2.service \
    file://mTerm9_2/run \
    file://mTerm_server10_1.service \
    file://mTerm10_1/run \
    file://mTerm_server10_2.service \
    file://mTerm10_2/run \
    file://mTerm_server11_1.service \
    file://mTerm11_1/run \
    file://mTerm_server11_2.service \
    file://mTerm11_2/run \
    file://mTerm_server12_1.service \
    file://mTerm12_1/run \
    file://mTerm_server12_2.service \
    file://mTerm12_2/run \
    file://mTerm-service-setup.sh \
    "

MTERM_SERVICES += "mTerm1_1 \
                   mTerm1_2 \
                   mTerm2_1 \
                   mTerm2_2 \
                   mTerm3_1 \
                   mTerm3_2 \
                   mTerm4_1 \
                   mTerm4_2 \
                   mTerm5_1 \
                   mTerm5_2 \
                   mTerm6_1 \
                   mTerm6_2 \
                   mTerm7_1 \
                   mTerm7_2 \
                   mTerm8_1 \
                   mTerm8_2 \
                   mTerm9_1 \
                   mTerm9_2 \
                   mTerm10_1 \
                   mTerm10_2 \
                   mTerm11_1 \
                   mTerm11_2 \
                   mTerm12_1 \
                   mTerm12_2 \
                  "

MTERM_SYSTEMD_SERVICES += "mTerm_server1_1.service \
                           mTerm_server1_2.service \
                           mTerm_server2_1.service \
                           mTerm_server2_2.service \
                           mTerm_server3_1.service \
                           mTerm_server3_2.service \
                           mTerm_server4_1.service \
                           mTerm_server4_2.service \
                           mTerm_server5_1.service \
                           mTerm_server5_2.service \
                           mTerm_server6_1.service \
                           mTerm_server6_2.service \
                           mTerm_server7_1.service \
                           mTerm_server7_2.service \
                           mTerm_server8_1.service \
                           mTerm_server8_2.service \
                           mTerm_server9_1.service \
                           mTerm_server9_2.service \
                           mTerm_server10_1.service \
                           mTerm_server10_2.service \
                           mTerm_server11_1.service \
                           mTerm_server11_2.service \
                           mTerm_server12_1.service \
                           mTerm_server12_2.service \
                          "
