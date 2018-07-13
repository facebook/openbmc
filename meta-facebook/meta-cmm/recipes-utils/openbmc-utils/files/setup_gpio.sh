#!/bin/sh
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

source /usr/local/bin/openbmc-utils.sh

#
# Setup gpios managed by the given i2c IO Expander.
# $1 - sysfs path of the i2c io expander. For example, 30-0020
# $2 - offset (within the gpio chip)
# $3 - list of symbolic links for the gpio pins
#
setup_i2c_gpios() {
    local i2c_path=${1}
    local offset=${2}
    shift 2
    local symlinks="$*"
    local gpio_chip=$(gpiochip_lookup_by_i2c_path ${i2c_path})
    local gpio_base=$(gpiochip_get_base ${gpio_chip})
    local pin link_name

    for link_name in ${symlinks}; do
        pin=$((gpio_base + offset))
        gpio_export ${pin} ${link_name}
        offset=$((offset + 1))
    done
}

# The gpio pins managed by aspeed gpio controller always start
# from 0 in kernel 4.1, but it's dynamically allocated in kernel
# 4.17, thus we need to read the base from sysfs.
KERNEL_VERSION=`uname -r`
if [[ ${KERNEL_VERSION} == 4.1.* ]]; then
    ASPEED_GPIO_BASE=0
else
    ASPEED_GPIOCHIP=$(gpiochip_lookup_by_label 1e780000.gpio)
    ASPEED_GPIO_BASE=$(gpiochip_get_base ${ASPEED_GPIOCHIP})
fi

# GPIOM0: BMC_CPLD_TMS
# GPIOM1: BMC_CPLD_TDI
# GPIOM2: BMC_CPLD_TCK
# GPIOM3: BMC_CPLD_TDO
# SCU84[24|25|26|27] must be 0
devmem_clear_bit $(scu_addr 84) 24
devmem_clear_bit $(scu_addr 84) 25
devmem_clear_bit $(scu_addr 84) 26
devmem_clear_bit $(scu_addr 84) 27
IDX=0
SYMLINKS="BMC_CPLD_TMS BMC_CPLD_TDI BMC_CPLD_TCK BMC_CPLD_TDO"
for LINK_NAME in ${SYMLINKS}; do
    OFFSET=$(gpio_name2value M${IDX})
    PIN=$((ASPEED_GPIO_BASE + OFFSET))
    gpio_export ${PIN} ${LINK_NAME}
    IDX=$((IDX + 1))
done

# GPIOF0: CPLD_JTAG_SEL (needs to be low)
# SCU90[30] must 0 adn SCU80[24] must be 0
devmem_clear_bit $(scu_addr 90) 30
devmem_clear_bit $(scu_addr 80) 24
OFFSET=$(gpio_name2value F0)
PIN=$((ASPEED_GPIO_BASE + OFFSET))
gpio_export ${PIN} CPLD_JTAG_SEL

# SYSTEM LED GPIOs
SYSLED_SYMLINKS="SYS_LED_RED SYS_LED_GREEN SYS_LED_BLUE
                 FAN_LED_RED FAN_LED_GREEN FAN_LED_BLUE
                 PSU_LED_RED PSU_LED_GREEN PSU_LED_BLUE
                 FAB_LED_RED FAB_LED_GREEN FAB_LED_BLUE"
setup_i2c_gpios "30-0021" 0 ${SYSLED_SYMLINKS}

# PSU PRESENCE GPIOs
PSU_SYMLINKS="PSU4_PRESENT PSU3_PRESENT PSU2_PRESENT PSU1_PRESENT"
setup_i2c_gpios "29-0020" 8 ${PSU_SYMLINKS}

# FCB1 GPIOs
FCB1_SYMLINKS="FCB1_CPLD_TMS FCB1_CPLD_TCK FCB1_CPLD_TDI FCB1_CPLD_TDO"
setup_i2c_gpios "172-0022" 0 ${FCB1_SYMLINKS}

# FCB2 GPIOs
FCB2_SYMLINKS="FCB2_CPLD_TMS FCB2_CPLD_TCK FCB2_CPLD_TDI FCB2_CPLD_TDO"
setup_i2c_gpios "180-0022" 0 ${FCB2_SYMLINKS}

# FCB3 GPIOs
FCB3_SYMLINKS="FCB3_CPLD_TMS FCB3_CPLD_TCK FCB3_CPLD_TDI FCB3_CPLD_TDO"
setup_i2c_gpios "188-0022" 0 ${FCB3_SYMLINKS}

# FCB4 GPIOs
FCB4_SYMLINKS="FCB4_CPLD_TMS FCB4_CPLD_TCK FCB4_CPLD_TDI FCB4_CPLD_TDO"
setup_i2c_gpios "196-0022" 0 ${FCB4_SYMLINKS}
