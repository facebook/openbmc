# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

cmake_do_configure:append() {
    # Montblanc use AST2620 Jtag Controller master 2
    JTAG_SYSFS_DIR="/sys/devices/platform/ahb/ahb:apb/1e6e4100.jtag/"
    echo >> ${S}/CMakeLists.txt
    echo "add_compile_definitions(JTAG_SYSFS_DIR=\"${JTAG_SYSFS_DIR}\")" >> ${S}/CMakeLists.txt
    echo >> ${S}/CMakeLists.txt
}
