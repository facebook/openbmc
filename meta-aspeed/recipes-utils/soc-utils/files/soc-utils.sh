#!/bin/bash
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

scu_addr() {
    echo $((0x1E6E2000 + 0x$1))
}

ast_config_adc() {
    local ADC_PATH="/sys/devices/platform/ast_adc.0"
    local channel=$1
    local r1=$2
    local r2=$3
    local v2=$4
    echo "$r1" > "${ADC_PATH}/adc${channel}_r1"
    echo "$r2" > "${ADC_PATH}/adc${channel}_r2"
    echo "$v2" > "${ADC_PATH}/adc${channel}_v2"
    echo 1 > "${ADC_PATH}/adc${channel}_en"
}

aspeed_g6_chip_ver() {
        chip_ver=$(devmem 0x1e6e2014 | awk '{print substr($0,6,1)}')
        case $chip_ver in
                0)
                        echo "A0"
                        ;;
                1)
                        echo "A1"
                        ;;
                2)
                        echo "A2"
                        ;;
                3)
                        echo "A3"
                        ;;
        esac
}

aspeed_soc_chip_ver() {
	soc_chip_ver=$(awk -F, 'NR==2 {print $NF}' /proc/device-tree/compatible)
	if [ "$soc_chip_ver" = "ast2600" ]; then
		echo "ATS2600-$(aspeed_g6_chip_ver)"
	fi
}
