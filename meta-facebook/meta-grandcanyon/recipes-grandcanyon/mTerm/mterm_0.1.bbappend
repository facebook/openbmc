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

SRC_URI += "file://mTerm-scc_exp_smart/run \
            file://mTerm-scc_exp_sdb/run \
            file://mTerm-scc_ioc_smart/run \
            file://mTerm-server/run \
            file://mTerm-bic/run \
            file://mTerm-iocm_ioc_smart/run \
            file://mTerm-service-setup.sh \
           "

S = "${WORKDIR}"

# launch 6 mTerm services, 1 for each server
MTERM_SERVICES = "mTerm-scc_exp_smart \
                  mTerm-scc_exp_sdb \
                  mTerm-scc_ioc_smart \
                  mTerm-server \
                  mTerm-bic \
                  mTerm-iocm_ioc_smart \
                 "
