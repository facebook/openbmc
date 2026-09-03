#!/bin/bash
#
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
#

echo -e "\n################################"
echo "######## BMC BOOT INFO  ########"
echo "################################"
/usr/local/bin/boot_info.sh bmc

echo -e "\n##### FLASH0 META_INFO #####"
/usr/local/bin/meta_info.sh flash0

echo -e "\n##### FLASH1 META_INFO #####"
/usr/local/bin/meta_info.sh flash1
