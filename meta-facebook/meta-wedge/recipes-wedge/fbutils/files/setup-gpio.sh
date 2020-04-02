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
#

### BEGIN INIT INFO
# Provides:          gpio-setup
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description:  Set up GPIO pins as appropriate
### END INIT INFO

# This file contains definitions for the GPIO pins that were not otherwise
# defined in other files.  We should probably move some more of the
# definitions to this file at some point.

# The commented-out sections are generally already defined elsewhere,
# and defining them twice generates errors.

# The exception to this is the definition of the GPIO H0, H1, and H2
# pins, which seem to adversely affect the rebooting of the system.
# When defined, the system doesn't reboot cleanly.  We're still
# investigating this.

. /usr/local/bin/openbmc-utils.sh

if uname -r | grep "4\.1\.*" > /dev/null 2>&1; then
    LEGACY_KERNEL="legacy-4.1"
fi

devmem_cond_set() {
    local addr=$1
    local bit=$2

    if [ -n "${LEGACY_KERNEL}" ]; then
        devmem_set_bit "${addr}" "${bit}"
    fi
}

devmem_cond_clear() {
    local addr=$1
    local bit=$2

    if [ -n "${LEGACY_KERNEL}" ]; then
        devmem_clear_bit "${addr}" "${bit}"
    fi
}

setup_gpio_fc_evt() {
    local board_type=$1

    # FAB_SLOT_ID is GPIOU0
    # PEER_FAB_PRSNT is GPIOU1
    devmem_cond_set "$(scu_addr a0)" 8
    devmem_cond_set "$(scu_addr a0)" 9
    gpio_export_by_name "${ASPEED_GPIO}" GPIOU0 FAB_SLOT_ID
    gpio_export_by_name "${ASPEED_GPIO}" GPIOU1 PEER_FAB_PRSNT

    # T2_POWER_UP is GPIOT6
    devmem_cond_set "$(scu_addr a0)" 6
    gpio_export_by_name "${ASPEED_GPIO}" GPIOT6 T2_POWER_UP

    # HS_FAULT_N is GPIOT7
    devmem_cond_set "$(scu_addr a0)" 7
    gpio_export_by_name "${ASPEED_GPIO}" GPIOT7 HS_FAULT_N

    if [ "$board_type" = "FC-LEFT" ]; then
        # GPIOE2 is CPU_EEPROM_SEL, on FC-LEFT
        devmem_cond_clear "$(scu_addr 80)" 18
        devmem_cond_clear "$(scu_addr 8c)" 13
        devmem_cond_clear "$(scu_addr 70)" 22
        gpio_export_by_name "${ASPEED_GPIO}" GPIOE2 CPU_EEPROM_SEL

        # GPIOA6 and GPIOA7 are MAC2 MDIO pins, we use them as
        # GPIO for bitbang driver
        devmem_cond_clear "$(scu_addr 90)" 2
        devmem_cond_clear "$(scu_addr 80)" 6
        devmem_cond_clear "$(scu_addr 80)" 7
        gpio_export_by_name "${ASPEED_GPIO}" GPIOA6 BMC_MAC2_MDC
        gpio_export_by_name "${ASPEED_GPIO}" GPIOA7 BMC_MAC2_MDIO
    fi
}

setup_gpio_fc_dvt() {
    local board_type=$1

    if [ "$board_type" = "FC-LEFT" ]; then # Left FC
        # BMC_SW_RST is GPIOL0, 16p switch
        # SCU84[16] must be 0
        devmem_cond_clear "$(scu_addr 84)" 16
        gpio_export_by_name "${ASPEED_GPIO}" GPIOL0 BMC_SW_RST
        gpio_set_value BMC_SW_RST 1

        # MDC|MDIO_CONT are GPIOR6 and GPIOR7, 16p switch
        # SCU88[30:31] must be 0
        devmem_cond_clear "$(scu_addr 88)" 30
        devmem_cond_clear "$(scu_addr 88)" 31
        gpio_export_by_name "${ASPEED_GPIO}" GPIOR6 BMC_MAC1_MDC
        gpio_export_by_name "${ASPEED_GPIO}" GPIOR7 BMC_MAC1_MDIO
        gpio_set_value BMC_MAC1_MDC 1
        gpio_set_value BMC_MAC1_MDIO 1

        # SWITCH_EEPROM1_WRT is GPIOE2, 16p switch EEPROM (U61)
        # SCU80[18], SCU8C[13], and SCU70[22] must be 0
        devmem_cond_clear "$(scu_addr 80)" 18
        devmem_cond_clear "$(scu_addr 8C)" 13
        devmem_cond_clear "$(scu_addr 70)" 22
        gpio_export_by_name "${ASPEED_GPIO}" GPIOE2 SWITCH_EEPROM1_WRT

        # SPI bus to 16p switch EEPROM
        # GPIOI4 <--> BMC_EEPROM1_SPI_SS
        # GPIOI5 <--> BMC_EEPROM1_SPI_SCK
        # GPIOI6 <--> BMC_EEPROM1_SPI_MOSI
        # GPIOI7 <--> BMC_EEPROM1_SPI_MISO
        # The EEPROM SPI clk does not match with the BMC SPI master.
        # Have to configure these pins as GPIO to use with
        # SPI bitbang driver.
        # SCU70[13:12,5] must be 0
        devmem_cond_clear "$(scu_addr 70)" 5
        devmem_cond_clear "$(scu_addr 70)" 12
        devmem_cond_clear "$(scu_addr 70)" 13
        gpio_export_by_name "${ASPEED_GPIO}" GPIOI4 BMC_EEPROM1_SPI_SS
        gpio_export_by_name "${ASPEED_GPIO}" GPIOI5 BMC_EEPROM1_SPI_SCK
        gpio_export_by_name "${ASPEED_GPIO}" GPIOI6 BMC_EEPROM1_SPI_MOSI
        gpio_export_by_name "${ASPEED_GPIO}" GPIOI7 BMC_EEPROM1_SPI_MISO

        # BMC_PHY_RST is GPIOT0, Front Panel Port PHY on the 16p switch
        # SCUA0[0] must be 1
        devmem_cond_set "$(scu_addr a0)" 0
        gpio_export_by_name "${ASPEED_GPIO}" GPIOT0 BMC_PHY_RST
        gpio_set_value BMC_PHY_RST 1

        # BMC_5PORTSW_RST is GPIOT1, 5p switch
        # SCUA0[1] must be 1
        devmem_cond_set "$(scu_addr a0)" 1
        gpio_export_by_name "${ASPEED_GPIO}" GPIOT1 BMC_5PORTSW_RST
        gpio_set_value BMC_5PORTSW_RST 1

        # ISO_SWITCH1_MDC|MDIO are GPIOT4 and GPIOT5, 5p switch
        # SCUA0[4:5] must be 1
        devmem_cond_set "$(scu_addr a0)" 4
        devmem_cond_set "$(scu_addr a0)" 5
        gpio_export_by_name "${ASPEED_GPIO}" GPIOT4 ISO_SWITCH1_MDC
        gpio_export_by_name "${ASPEED_GPIO}" GPIOT5 ISO_SWITCH1_MDIO
        gpio_set_value ISO_SWITCH1_MDC 1
        gpio_set_value ISO_SWITCH1_MDIO 1

        # ISO_SWITCH_EEPROM2_WRT is GPIOV0, 5p switch EEPROM (U114)
        # SCUA0[16] must be 1
        devmem_cond_set "$(scu_addr a0)" 16
        gpio_export_by_name "${ASPEED_GPIO}" GPIOV0 ISO_SWITCH_EEPROM2_WRT

        # SPI bus to 5p switch EEPROM (U114)
        # GPIOI0 <--> ISO_BMC_EEPROM2_SPI_SS
        # GPIOI1 <--> ISO_BMC_EEPROM2_SPI_SCK
        # GPIOI2 <--> ISO_BMC_EEPROM2_SPI_MOSI
        # GPIOI3 <--> ISO_BMC_EEPROM2_SPI_MISO
        # The EEPROM SPI clk does not match with the BMC SPI master.
        # Have to configure these pins as GPIO to use with
        # SPI bitbang driver.
        # SCU70[13] must be 0, has already been set when
        # preparing for GPIOI4-GPIOI7
        gpio_export_by_name "${ASPEED_GPIO}" GPIOI0 ISO_BMC_EEPROM2_SPI_SS
        gpio_export_by_name "${ASPEED_GPIO}" GPIOI1 ISO_BMC_EEPROM2_SPI_SCK
        gpio_export_by_name "${ASPEED_GPIO}" GPIOI2 ISO_BMC_EEPROM2_SPI_MOSI
        gpio_export_by_name "${ASPEED_GPIO}" GPIOI3 ISO_BMC_EEPROM2_SPI_MISO

        # BMC_PHYL_RST is GPIOF0, Left BMC PHY
        # SCU80[24] must be 0
        devmem_cond_clear "$(scu_addr 80)" 24
        gpio_export_by_name "${ASPEED_GPIO}" GPIOF0 BMC_PHYL_RST
        gpio_set_value BMC_PHYL_RST 1
    else # Right FC
        # BMC_PHYR_RST is GPIOL1, Right BMC PHY
        # SCU84[17] must be 0
        devmem_cond_clear "$(scu_addr 84)" 17
        gpio_export_by_name "${ASPEED_GPIO}" GPIOL1 BMC_PHYR_RST
        gpio_set_value BMC_PHYR_RST 1
    fi

    # T2_POWER_UP is GPIOU4
    # SCUA0[12] must be 1
    devmem_cond_set "$(scu_addr a0)" 12
    gpio_export_by_name "${ASPEED_GPIO}" GPIOU4 T2_POWER_UP

    # HS_FAULT_N is GPIOU5
    # SCUA0[13] must be 1
    devmem_cond_set "$(scu_addr a0)" 13
    gpio_export_by_name "${ASPEED_GPIO}" GPIOU5 HS_FAULT_N

    # FAB_SLOT_ID is GPIOU6
    # SCUA0[14] must be 1
    devmem_cond_set "$(scu_addr a0)" 14
    gpio_export_by_name "${ASPEED_GPIO}" GPIOU6 FAB_SLOT_ID

    # PEER_FAB_PRSNT is GPIOU7
    # SCUA0[15] must be 1
    devmem_cond_set "$(scu_addr a0)" 15
    gpio_export_by_name "${ASPEED_GPIO}" GPIOU7 PEER_FAB_PRSNT
}

setup_gpio_lc_wedge() {
    local board_rev=$1

    # Set up to watch for FC presence, and switch between interfaces.
    # GPIOC0..C7, interested in C4, C5
    devmem_cond_clear "$(scu_addr 90)" 0
    devmem_cond_clear "$(scu_addr 90)" 25

    if [ "$board_rev" -lt 3 ]; then
        # Prior to DVTC
        # BP_SLOT_ID GPIO pins are U0, U1, U2, U3
        devmem_cond_set "$(scu_addr a0)" 8
        devmem_cond_set "$(scu_addr a0)" 9
        devmem_cond_set "$(scu_addr a0)" 10
        devmem_cond_set "$(scu_addr a0)" 11
        gpio_export_by_name "${ASPEED_GPIO}" GPIOU0 BP_SLOT_ID0
        gpio_export_by_name "${ASPEED_GPIO}" GPIOU1 BP_SLOT_ID1
        gpio_export_by_name "${ASPEED_GPIO}" GPIOU2 BP_SLOT_ID2
        gpio_export_by_name "${ASPEED_GPIO}" GPIOU3 BP_SLOT_ID3

        # T2_POWER_UP is GPIOT6
        devmem_cond_set "$(scu_addr a0)" 6
        gpio_export_by_name "${ASPEED_GPIO}" GPIOT6 T2_POWER_UP

        # HS_FAULT_N is GPIOT7
        devmem_cond_set "$(scu_addr a0)" 7
        gpio_export_by_name "${ASPEED_GPIO}" GPIOT7 HS_FAULT_N
    else
        # Starting from DVTC
        # BP_SLOT_ID GPIO pins are U6, U7, V0, V1
        devmem_cond_set "$(scu_addr 70)" 6
        devmem_cond_set "$(scu_addr a0)" 14
        devmem_cond_set "$(scu_addr a0)" 15
        devmem_cond_set "$(scu_addr a0)" 16
        devmem_cond_set "$(scu_addr a0)" 17
        gpio_export_by_name "${ASPEED_GPIO}" GPIOU6 BP_SLOT_ID0
        gpio_export_by_name "${ASPEED_GPIO}" GPIOU7 BP_SLOT_ID1
        gpio_export_by_name "${ASPEED_GPIO}" GPIOV0 BP_SLOT_ID2
        gpio_export_by_name "${ASPEED_GPIO}" GPIOV1 BP_SLOT_ID3

        # T2_POWER_UP is GPIOU4
        devmem_cond_set "$(scu_addr a0)" 12
        gpio_export_by_name "${ASPEED_GPIO}" GPIOU4 T2_POWER_UP

        # HS_FAULT_N is GPIOU5
        devmem_cond_set "$(scu_addr a0)" 13
        gpio_export_by_name "${ASPEED_GPIO}" GPIOU5 HS_FAULT_N
    fi
}

# Set up to read the board revision pins, Y0, Y1, Y2
devmem_cond_set "$(scu_addr 70)" 19
devmem_cond_clear "$(scu_addr a4)" 8
devmem_cond_clear "$(scu_addr a4)" 9
devmem_cond_clear "$(scu_addr a4)" 10
gpio_export_by_name "${ASPEED_GPIO}" GPIOY0 BOARD_REV_ID0
gpio_export_by_name "${ASPEED_GPIO}" GPIOY1 BOARD_REV_ID1
gpio_export_by_name "${ASPEED_GPIO}" GPIOY2 BOARD_REV_ID2

# enabled Y0, Y1, Y2, we can use wedge_board_rev() now
board_rev=$(wedge_board_rev)

# Set up ISO_SVR_ID[0-3], GPION[2-5]
# On wedge, these 4 GPIOs are not connected. And the corresponding
# 4 pins from uS are strapped to low.
# On fabic, these 4 pins are connected to uS SVR_ID pins,
# which are used to set the uS FPGA i2c address.
# Force all pins to low to have the same uS FPGA i2c address on wedge
# and fabric
# To use GPION[2-5], SCU90[4:5] must be 0, and SCU88[2-5] must be 0 also
devmem_cond_clear "$(scu_addr 90)" 4
devmem_cond_clear "$(scu_addr 90)" 5
devmem_cond_clear "$(scu_addr 88)" 2
devmem_cond_clear "$(scu_addr 88)" 3
devmem_cond_clear "$(scu_addr 88)" 4
devmem_cond_clear "$(scu_addr 88)" 5
gpio_export_by_name "${ASPEED_GPIO}" GPION2 ISO_SVR_ID0
gpio_export_by_name "${ASPEED_GPIO}" GPION3 ISO_SVR_ID1
gpio_export_by_name "${ASPEED_GPIO}" GPION4 ISO_SVR_ID2
gpio_export_by_name "${ASPEED_GPIO}" GPION5 ISO_SVR_ID3
for i in 0 1 2 3; do
    gpio_set_value ISO_SVR_ID${i} 0
done

## CARD_EN, GPIO C3
#devmem_cond_clear "$(scu_addr 90)" 0
#devmem_cond_clear "$(scu_addr 90)" 24
#echo 18 > /sys/class/gpio/export

# T2_RESET_N, RESET_SEQ0, RESET_SEQ1, on GPIO C0, F3, and F2
devmem_cond_clear "$(scu_addr 90)" 0
devmem_cond_clear "$(scu_addr 90)" 23
devmem_cond_clear "$(scu_addr 80)" 26
devmem_cond_clear "$(scu_addr a4)" 13
devmem_cond_clear "$(scu_addr 80)" 27
devmem_cond_clear "$(scu_addr a4)" 14
devmem_cond_set "$(scu_addr 70)" 19
gpio_export_by_name "${ASPEED_GPIO}" GPIOC0 T2_RESET_N
gpio_export_by_name "${ASPEED_GPIO}" GPIOF2 RESET_SEQ1
gpio_export_by_name "${ASPEED_GPIO}" GPIOF3 RESET_SEQ0

# PANTHER_PRSNT_N, uServer presence, on GPIO E4
devmem_cond_clear "$(scu_addr 80)" 20
devmem_cond_clear "$(scu_addr 8c)" 14
devmem_cond_clear "$(scu_addr 70)" 22
gpio_export_by_name "${ASPEED_GPIO}" GPIOE4 PANTHER_PRSNT_N

# MRSRVR_SYS_RST, reset the uServer, on GPIO C1
devmem_cond_clear "$(scu_addr 90)" 0
devmem_cond_clear "$(scu_addr 90)" 23
gpio_export_by_name "${ASPEED_GPIO}" GPIOC1 MRSRVR_SYS_RST
# output

# BMC_PWR_BTN_IN_N, uServer power button in, on GPIO D0
# BMC_PWR_BTN_OUT_N, uServer power button out, on GPIO D1
devmem_cond_clear "$(scu_addr 90)" 1
devmem_cond_clear "$(scu_addr 8c)" 8
devmem_cond_clear "$(scu_addr 70)" 21
gpio_export_by_name "${ASPEED_GPIO}" GPIOD0 BMC_PWR_BTN_IN_N
gpio_export_by_name "${ASPEED_GPIO}" GPIOD1 BMC_PWR_BTN_OUT_N
# we have to ensure that BMC_PWR_BTN_OUT_N is high so that
# when we enable the isolation buffer, uS will not be powered down
gpio_set_value BMC_PWR_BTN_OUT_N 1

## BMC_READY_IN, BMC signal that it's up, on GPIO P7
# To use GPIOP7 (127), SCU88[23] must be 0
devmem_cond_clear "$(scu_addr 88)" 23
# Put GPIOP7 (127) to low so that we can control uS power now
# This must be after 'gpio_set 25 1'
gpio_export_by_name "${ASPEED_GPIO}" GPIOP7 BMC_READY_IN
gpio_set_value BMC_READY_IN 0

# PANTHER_I2C_ALERT_N, alert for uServer I2C, GPIO B0
devmem_cond_clear "$(scu_addr 80)" 8
gpio_export_by_name "${ASPEED_GPIO}" GPIOB0 PANTHER_I2C_ALERT_N

# MNSERV_NIC_SMBUS_ALRT, alert for uServer NIC, GPIO B1
devmem_cond_clear "$(scu_addr 80)" 9
gpio_export_by_name "${ASPEED_GPIO}" GPIOB1 MNSERV_NIC_SMBUS_ALRT

# LED_PWR_BLUE, blue power light, GPIO E5
devmem_cond_clear "$(scu_addr 80)" 21
devmem_cond_clear "$(scu_addr 8c)" 14
devmem_cond_clear "$(scu_addr 70)" 22
gpio_export_by_name "${ASPEED_GPIO}" GPIOE5 LED_PWR_BLUE
# output

# BMC_HEARTBEAT_N, heartbeat LED, GPIO Q7
devmem_cond_clear "$(scu_addr 90)" 28
gpio_export_by_name "${ASPEED_GPIO}" GPIOQ7 BMC_HEARTBEAT_N
# output

# XXX:  setting those causes the system to lock up on reboot
## T2 ROV1, ROV2, ROV3 -- voltage reading, GPIOs H0, H1, and H2
#devmem_cond_clear "$(scu_addr 90)" 6
#devmem_cond_clear "$(scu_addr 90)" 7
#devmem_cond_clear "$(scu_addr 70)" 4
## Do I need to set 70:1 and 70:0 to 1?
gpio_export_by_name "${ASPEED_GPIO}" GPIOH0 T2_ROV1
gpio_export_by_name "${ASPEED_GPIO}" GPIOH1 T2_ROV2
gpio_export_by_name "${ASPEED_GPIO}" GPIOH2 T2_ROV3

# HOTSWAP_PG, hotswap issues, GPIO L3
devmem_cond_clear "$(scu_addr 90)" 5
devmem_cond_clear "$(scu_addr 90)" 4
devmem_cond_clear "$(scu_addr 84)" 19
gpio_export_by_name "${ASPEED_GPIO}" GPIOL3 HOTSWAP_PG

# XXX:  These interfere with i2c bus 11 (on Linux, it's 12 on the hardware)
# which we need to talk to the power supplies on certain hardware.
## Hardware presence pins C4 and C5
#devmem_cond_clear "$(scu_addr 90)" 0
#devmem_cond_clear "$(scu_addr 90)" 24
#echo 20 > /sys/class/gpio/export
#echo 21 > /sys/class/gpio/export

# FAB_GE_SEL, uServer GE connection, GPIO A0
devmem_cond_clear "$(scu_addr 80)" 0
gpio_export_by_name "${ASPEED_GPIO}" GPIOA0 FAB_GE_SEL
# output

# USB_OCS_N1, resettable fuse tripped, GPIO Q6
devmem_cond_clear "$(scu_addr 90)" 28
gpio_export_by_name "${ASPEED_GPIO}" GPIOQ6 USB_OCS_N1

# RX loss signal?

# System SPI
# Strap 12 must be 0 and Strape 13 must be 1
devmem_cond_clear "$(scu_addr 70)" 12
devmem_cond_set "$(scu_addr 70)" 13
# GPIOQ4 is ISO_FLASH_WP, must be 1 to avoid write protection
# GPIOQ5 is ISO_FLASH_HOLD, must be 1 to be out of reset
# To use GPIOQ4 and GPIOQ5, SCU90[27] must be 0
devmem_cond_clear "$(scu_addr 90)" 27
gpio_export_by_name "${ASPEED_GPIO}" GPIOQ4 ISO_FLASH_WP
gpio_export_by_name "${ASPEED_GPIO}" GPIOQ5 ISO_FLASH_HOLD
gpio_set_value ISO_FLASH_WP 1
gpio_set_value ISO_FLASH_HOLD 1
# GPIOD6 is ISO_FL_PRG_SEL, set it to 0 so that BMC does not have control
# on the EEPROM by default.
# To use GPIOD6, SCU90[1] must be 0, SCU8C[21] must be 0, and Strap[21] must be 0
devmem_cond_clear "$(scu_addr 90)" 1
devmem_cond_clear "$(scu_addr 8c)" 8
devmem_cond_clear "$(scu_addr 70)" 21
gpio_export_by_name "${ASPEED_GPIO}" GPIOD6 ISO_FL_PRG_SEL
gpio_set_value ISO_FL_PRG_SEL 0

# DEBUG_RST_BTN_N, Debug Reset button on front panel, GPIO R2
devmem_cond_clear "$(scu_addr 88)" 26
gpio_export_by_name "${ASPEED_GPIO}" GPIOR2 DEBUG_RST_BTN_N

# DEBUG_PORT_UART_SEL_N, Debug Select button, GPIO B2
devmem_cond_clear "$(scu_addr 80)" 10
gpio_export_by_name "${ASPEED_GPIO}" GPIOB2 DEBUG_PORT_UART_SEL_N

# DEBUG_UART_SEL_0, select uServer UART to the debug header, GPIO E0
devmem_cond_clear "$(scu_addr 80)" 16
devmem_cond_clear "$(scu_addr 8c)" 12
devmem_cond_clear "$(scu_addr 70)" 22
gpio_export_by_name "${ASPEED_GPIO}" GPIOE0 DEBUG_UART_SEL_0
# output

# USB_BRDG_RST , GPIO D4
devmem_cond_clear "$(scu_addr 90)" 1
devmem_cond_clear "$(scu_addr 8c)" 10
devmem_cond_clear "$(scu_addr 70)" 21
gpio_export_by_name "${ASPEED_GPIO}" GPIOD4 USB_BRDG_RST

# Bloodhound GPIOs, P0-6, G4, J1-3, Y3
# Make sure GPIOP0,1,2,3,6 are enabled.
for i in {16..19} 22; do
  devmem_cond_clear "$(scu_addr 88)" $i
done
# Enable GPIOY3
devmem_cond_clear "$(scu_addr a4)" 11
# GPIOG4
devmem_cond_clear "$(scu_addr 2c)" 1
# GPIOJ1
devmem_cond_clear "$(scu_addr 84)" 9
# GPIOJ2
devmem_cond_clear "$(scu_addr 84)" 10
# GPIOJ11
devmem_cond_clear "$(scu_addr 84)" 11

# Export all the GPIOs
for i in {0..6}; do
    gpio_export_by_name "${ASPEED_GPIO}" GPIOP${i} BLOODHOUND_GPIOP${i}
done
for i in {1..3}; do
    gpio_export_by_name "${ASPEED_GPIO}" GPIOJ${i} BLOODHOUND_GPIOJ${i}
done
gpio_export_by_name "${ASPEED_GPIO}" GPIOG4 BLOODHOUND_GPIOG4
gpio_export_by_name "${ASPEED_GPIO}" GPIOY3 BLOODHOUND_GPIOY3

# ISO_BUF_EN, GPIOC2, SCU90[0] and SCU90[24] must be 0
devmem_cond_clear "$(scu_addr 90)" 0
devmem_cond_clear "$(scu_addr 90)" 24
gpio_export_by_name "${ASPEED_GPIO}" GPIOC2 ISO_BUF_EN

# Enable the isolation buffer
wedge_iso_buf_enable

# load the EEPROM
i2c_device_add 6 0x50 24c64

# Get the board type by parsing the EEPROM.
# This must be after enabling isolation buffer as the i2c bus
# is isolated by the buffer
board_type=$(wedge_board_type)

case "$board_type" in
    FC-LEFT|FC-RIGHT)
        # On FC
        if [ "$board_rev" -lt 2 ]; then
            # EVT board
            setup_gpio_fc_evt "$board_type"
        else
            # DVT board
            setup_gpio_fc_dvt "$board_type"
        fi
        ;;
    *)
        setup_gpio_lc_wedge "$board_rev"
        ;;
esac

# Make it possible to turn off T2 if fand sees overheating via GPIOF1
# Do not change the GPIO direction here as the default value of this GPIO
# is low, which causes a Non-maskable interrupt to the uS.
devmem_cond_clear "$(scu_addr 80)" 25
devmem_cond_clear "$(scu_addr a4)" 12
gpio_export_by_name "${ASPEED_GPIO}" GPIOF1 BMC_GPIOF1

# Allow us to set the fan LEDs boards.
# This is GPIO G5, G6, G7, and J0

devmem_cond_clear "$(scu_addr 70)" 23
devmem_cond_clear "$(scu_addr 84)" 5
devmem_cond_clear "$(scu_addr 84)" 6
devmem_cond_clear "$(scu_addr 84)" 7
devmem_cond_clear "$(scu_addr 84)" 8
gpio_export_by_name "${ASPEED_GPIO}" GPIOG5 FAN_LED_GPIOG5
gpio_export_by_name "${ASPEED_GPIO}" GPIOG6 FAN_LED_GPIOG6
gpio_export_by_name "${ASPEED_GPIO}" GPIOG7 FAN_LED_GPIOG7
gpio_export_by_name "${ASPEED_GPIO}" GPIOJ0 FAN_LED_GPIOJ0

gpio_set_direction FAN_LED_GPIOG5 out
gpio_set_direction FAN_LED_GPIOG6 out
gpio_set_direction FAN_LED_GPIOG7 out
gpio_set_direction FAN_LED_GPIOJ0 out

# Once we set "out", output values will be random unless we set them
# to something
gpio_set_value FAN_LED_GPIOG5 0
gpio_set_value FAN_LED_GPIOG6 0
gpio_set_value FAN_LED_GPIOG7 0
gpio_set_value FAN_LED_GPIOJ0 0

#
# Setup LED_POSTCODE_[0..3], connected to GPIOG[0..3]:
#   - GPIOG[0..3]: SCU84[0..3] = 0
#
devmem_cond_clear $(scu_addr 84) 0
devmem_cond_clear $(scu_addr 84) 1
devmem_cond_clear $(scu_addr 84) 2
devmem_cond_clear $(scu_addr 84) 3
gpio_export_by_name "${ASPEED_GPIO}" GPIOG0 LED_POSTCODE_0
gpio_export_by_name "${ASPEED_GPIO}" GPIOG1 LED_POSTCODE_1
gpio_export_by_name "${ASPEED_GPIO}" GPIOG2 LED_POSTCODE_2
gpio_export_by_name "${ASPEED_GPIO}" GPIOG3 LED_POSTCODE_3

#
# Setup LED_POSTCODE_[4..7], connected to GPIOB[4..7]:
#   - GPIOB4: SCU80[12] = 0 and Strap[14] = 0
#   - GPIOB[5..7]: SCU80[13..15] = 0
#
devmem_cond_clear $(scu_addr 70) 14
devmem_cond_clear $(scu_addr 80) 12
devmem_cond_clear $(scu_addr 80) 13
devmem_cond_clear $(scu_addr 80) 14
devmem_cond_clear $(scu_addr 80) 15
gpio_export_by_name "${ASPEED_GPIO}" GPIOB4 LED_POSTCODE_4
gpio_export_by_name "${ASPEED_GPIO}" GPIOB5 LED_POSTCODE_5
gpio_export_by_name "${ASPEED_GPIO}" GPIOB6 LED_POSTCODE_6
gpio_export_by_name "${ASPEED_GPIO}" GPIOB7 LED_POSTCODE_7

#
# Setup BMC_USB_RESET_N, connected to GPIOD7
#   - GPIOD7: SCU90[1], SCU8C[11], and SCU70[21] must be 0
#
devmem_cond_clear $(scu_addr 8C) 11
devmem_cond_clear $(scu_addr 70) 21
devmem_cond_clear $(scu_addr 90) 1
gpio_export_by_name "${ASPEED_GPIO}" GPIOD7 BMC_USB_RESET_N
