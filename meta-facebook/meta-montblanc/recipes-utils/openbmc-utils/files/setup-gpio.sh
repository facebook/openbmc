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

### BEGIN INIT INFO
# Provides:          gpio-setup
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description:  Set up GPIO pins as appropriate
### END INIT INFO

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

# arg: NAME PIN DIR [DEFAULT VALUE]
setup_gpio() {
    NAME="$1"
    PIN="$2"
    DIR="$3"

    gpio_export_by_name "${ASPEED_GPIO}" "$PIN" "$NAME"
    if [ "$DIR" = "out" ]; then
        gpio_set_direction "$NAME" out
        if [ $# -ge 4 ]; then
            VALUE="$4"
            gpio_set_value "$NAME" "$VALUE"
        else
            echo "ERROR: setup-gpio $NAME output default value not defined" > /dev/kmsg
        fi
    elif [ "$DIR" = "in" ]; then
        gpio_set_direction "$NAME" in
    else
        echo "ERROR: setup-gpio $NAME invalid direction [$DIR] not (in,out)" > /dev/kmsg
    fi
}

# To debug jumper
setup_gpio BMC_DEBUG_JUMPER1                GPIOG5 in
setup_gpio BMC_DEBUG_JUMPER2                GPIOB1 in
setup_gpio BMC_DEBUG_JUMPER3                GPIOB2 in
# OCP Debug Card present event, high active
setup_gpio DEBUG_PRSNT                      GPIOG2 in

# Indicate the PCH power good on the Netlake. From COME D83
setup_gpio PWRGD_PCH_PWROK                  GPIOF4 in
# Reserved for power button event input, also pass through to BMC Pin40
setup_gpio PWR_BTN_L                        GPIOP4 in
# Reserved for reset button event input, also pass through to BMC Pin76
setup_gpio RST_BTN_L                        GPIOP2 in

# BMC Power Good, output to CPLD 
setup_gpio BMC_PWRGD_R                      GPIOV4 out 1
# BMC ready event to COME (COME A85)
setup_gpio FM_BMC_READY_R_L                 GPIOP0 out 0
# Reserved for Power button event output, which pass through from BMC Pin160
setup_gpio BMC_PTH_PWR_BTN_L                GPIOP5 out 1
# Reserved for reset button event output, which pass through from BMC Pin121
setup_gpio BMC_PTH_RST_BTN_L                GPIOP3 out 1

setup_gpio BMC_I2C1_EN                      GPIOG0 out 0
setup_gpio BMC_I2C2_EN                      GPIOF1 out 0
# JTAG MUX
setup_gpio MUX_JTAG_SEL_0                   GPIOF0 out 0
setup_gpio MUX_JTAG_SEL_1                   GPIOF5 out 0
# Mux select signal for BMC to program IOB flash (SPI)
setup_gpio IOB_FLASH_SEL                    GPIOG4 out 0
# SPI Mux select to COME (B89) and PROT module
setup_gpio SPI_MUX_SEL                      GPIOQ6 out 0

# BMC CPU_RST#
setup_gpio BMC_CPU_RST                      GPION2 in
# To SCM CPLD, reserved.
setup_gpio BMC_GPIO126_USB2APWREN           GPION4 in
# CPU power good event indicates to BMC
setup_gpio COME_ISO_PWRGD_CPU_LVC3          GPIOO2 in
# CATERR/MSMI event from SOC
setup_gpio FM_CPU_MSMI_CATERR_LVT3_BUF_N    GPIOM3 in
# Non-maskable Interrupt to BMC, from COMe B95
setup_gpio ISO_FM_BMC_NMI_L                 GPIOO3 in
# Reserve for BMC to enable Netlake power, to COMe A93
setup_gpio ISO_FM_BMC_ONCTL_L               GPIOG6 out 0
# COMe FPGA CRC error event indication, from COMe C69
setup_gpio ISO_FPGA_CRC_ERROR               GPIOQ0 in
# Netlake FPGA asset it to inform BMC for PCIe hot plug (From COME A16)
setup_gpio ISO_SMB_FPGA_VPP_ALERT_L         GPIOG3 in