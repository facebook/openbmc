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

do_configure:prepend:mf-mtd-ubifs() {
    #
    # append "ubi.mtd=data0" to bootargs if "mtd-ubifs" is enabled.
    #
    sed -i '/CONFIG_BOOTARGS/ s/"\s*$/ ubi.mtd=data0"/' ${S}/include/configs/fbwedge400.h
}
