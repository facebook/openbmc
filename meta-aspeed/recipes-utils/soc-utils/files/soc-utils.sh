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

# Function to set RGMII delay values for MAC#3 and MAC#4 interfaces
# Usage: ast2600_setup_RGMII34_clock_delay MAC4_TX=<value> MAC4_RX=<value> MAC3_TX=<value> MAC3_RX=<value>
# Each parameter is optional. Provide only the delay values you want to set
# Support only for AST G6
ast2600_setup_RGMII34_clock_delay() {
        chip_ver=$(aspeed_soc_chip_ver)
        if [[ $chip_ver != *"ATS2600"* ]]; then
                echo "Error: This function is only compatible with AST G6 models." 1>&2
                return 1
        fi
        SCU=0x1E6E2000
        SCU350=$((SCU + 0x350))
        # SCU350 MAC34 Interface Clock Delay Setting
        #   31     R/W     RGMII 125MHz clock source selection
        #                   0: PAD RGMIICK
        #                   1: Internal PLL
        #   30     R/W     RMII4 50MHz RCLK output enable
        #                   0: Disable
        #                   1: Enable
        #   29     R/W     RMII3 50MHz RCLK output enable
        #                   0: Disable
        #                   1: Enable
        #   28:26  ---     Reserved
        #   25     R/W     MAC#4 RMII transmit data at clock falling edge
        #   24     R/W     MAC#3 RMII transmit data at clock falling edge
        #   23:18  R/W     MAC#4 RMII RCLK/RGMII RXCLK (1G) clock input delay
        #   17:12  R/W     MAC#3 RMII RCLK/RGMII RXCLK (1G) clock input delay
        #   11:6   R/W     MAC#4 RGMII TXCLK (1G) clock output delay
        #   5:0    R/W     MAC#3 RGMII TXCLK (1G) clock output delay

        value=$(devmem "$SCU350")

        # Parse arguments
        for arg in "$@"; do
                case "$arg" in
                        MAC4_TX=*)
                                MAC4_TX="${arg#*=}"
                                value=$((value & 0xFFFFF03F )) # Clear previous MAC4 TX delay value
                                value=$((value | (MAC4_TX << 6)))
                                ;;
                        MAC4_RX=*)
                                MAC4_RX="${arg#*=}"
                                value=$((value & 0xFF03FFFF )) # Clear previous MAC4 RX delay value
                                value=$((value | (MAC4_RX << 18)))
                                ;;
                        MAC3_TX=*)
                                MAC3_TX="${arg#*=}"
                                value=$((value & 0xFFFFFFC0 )) # Clear previous MAC3 TX delay value
                                value=$((value | (MAC3_TX << 0)))
                                ;;
                        MAC3_RX=*)
                                MAC3_RX="${arg#*=}"
                                value=$((value & 0xFFFC0FFF )) # Clear previous MAC3 RX delay value
                                value=$((value | (MAC3_RX << 12)))
                                ;;
                        *)
                                echo "Invalid argument: $arg"
                                echo "Usage: ast2600_setup_RGMII34_clock_delay MAC4_TX=<value> MAC4_RX=<value> MAC3_TX=<value> MAC3_RX=<value>"
                                exit 1
                                ;;
                esac
        done
        # Write the modified value back to the register
        devmem "$SCU350" 32 $value
}
