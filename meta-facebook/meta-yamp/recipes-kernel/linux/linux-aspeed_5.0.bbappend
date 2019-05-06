# Copyright 2019-present Facebook. All Rights Reserved.
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

#
# XXX this is not the "official" way to update Kconfig file: we need to
# add some logic to merge common Kconfig file (under meta-aspeed layer)
# and platform specific Kconfig file by "merge_config.sh".
#
do_configure_prepend() {
    if [ -f "${WORKDIR}/defconfig" ]; then
        sed -i 's/NET_NCSI_MC_MAC_OFFSET=1/NET_NCSI_MC_MAC_OFFSET=2/g' "${WORKDIR}/defconfig"
    fi
}
