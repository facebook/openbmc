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

# Set up to read the board revision pins, Y0, Y1, Y2
devmem_set_bit $(scu_addr 70) 19
devmem_clear_bit $(scu_addr a4) 8
devmem_clear_bit $(scu_addr a4) 9
devmem_clear_bit $(scu_addr a4) 10
echo 192 > /sys/class/gpio/export
echo 193 > /sys/class/gpio/export
echo 194 > /sys/class/gpio/export

# enabled Y0, Y1, Y2, we can use wedge_board_rev() now
board_rev=$(wedge_board_rev)

# load the EEPROM
i2c_device_add 6 0x50 24c64

# Set up ISO_SVR_ID[0-3], GPION[2-5]
# On wedge, these 4 GPIOs are not connected. And the corresponding
# 4 pins from uS are strapped to low.
# On fabic, these 4 pins are connected to uS SVR_ID pins,
# which are used to set the uS FPGA i2c address.
# Force all pins to low to have the same uS FPGA i2c address on wedge
# and fabric
# To use GPION[2-5], SCU90[4:5] must be 0, and SCU88[2-5] must be 0 also
devmem_clear_bit $(scu_addr 90) 4
devmem_clear_bit $(scu_addr 90) 5
devmem_clear_bit $(scu_addr 88) 2
devmem_clear_bit $(scu_addr 88) 3
devmem_clear_bit $(scu_addr 88) 4
devmem_clear_bit $(scu_addr 88) 5
gpio_set 106 0
gpio_set 107 0
gpio_set 108 0
gpio_set 109 0

## CARD_EN, GPIO C3
#devmem_clear_bit $(scu_addr 90) 0
#devmem_clear_bit $(scu_addr 90) 24
#echo 18 > /sys/class/gpio/export

# T2_RESET_N, RESET_SEQ0, RESET_SEQ1, on GPIO C0, F2, and F3
devmem_clear_bit $(scu_addr 90) 0
devmem_clear_bit $(scu_addr 90) 23
devmem_clear_bit $(scu_addr 80) 26
devmem_clear_bit $(scu_addr a4) 13
devmem_clear_bit $(scu_addr 80) 27
devmem_clear_bit $(scu_addr a4) 14
devmem_set_bit $(scu_addr 70) 19
gpio_export C0 T2_RESET_N
gpio_export F2 RESET_SEQ1
gpio_export F3 RESET_SEQ0

# PANTHER_PRSNT_N, uServer presence, on GPIO E4
devmem_clear_bit $(scu_addr 80) 20
devmem_clear_bit $(scu_addr 8c) 14
devmem_clear_bit $(scu_addr 70) 22
echo 36 > /sys/class/gpio/export

# MRSRVR_SYS_RST, reset the uServer, on GPIO C1
devmem_clear_bit $(scu_addr 90) 0
devmem_clear_bit $(scu_addr 90) 23
echo 17 > /sys/class/gpio/export
# output

# BMC_PWR_BTN_IN_N, uServer power button in, on GPIO D0
# BMC_PWR_BTN_OUT_N, uServer power button out, on GPIO D1
devmem_clear_bit $(scu_addr 90) 1
devmem_clear_bit $(scu_addr 8c) 8
devmem_clear_bit $(scu_addr 70) 21
echo 24 > /sys/class/gpio/export
# we have to ensure that BMC_PWR_BTN_OUT_N is high so that
# when we enable the isolation buffer, uS will not be powered down
gpio_set 25 1

## BMC_READY_IN, BMC signal that it's up, on GPIO P7
# To use GPIOP7 (127), SCU88[23] must be 0
devmem_clear_bit $(scu_addr 88) 23
# Put GPIOP7 (127) to low so that we can control uS power now
# This must be after 'gpio_set 25 1'
gpio_set 127 0

# PANTHER_I2C_ALERT_N, alert for uServer I2C, GPIO B0
devmem_clear_bit $(scu_addr 80) 8
echo 8 > /sys/class/gpio/export

# MNSERV_NIC_SMBUS_ALRT, alert for uServer NIC, GPIO B1
devmem_clear_bit $(scu_addr 80) 9
echo 9 > /sys/class/gpio/export

# LED_PWR_BLUE, blue power light, GPIO E5
devmem_clear_bit $(scu_addr 80) 21
devmem_clear_bit $(scu_addr 8c) 14
devmem_clear_bit $(scu_addr 70) 22
echo 37 > /sys/class/gpio/export
# output

# BMC_HEARTBEAT_N, heartbeat LED, GPIO Q7
devmem_clear_bit $(scu_addr 90) 28
echo 135 > /sys/class/gpio/export
# output

# XXX:  setting those causes the system to lock up on reboot
## T2 ROV1, ROV2, ROV3 -- voltage reading, GPIOs H0, H1, and H2
#devmem_clear_bit $(scu_addr 90) 6
#devmem_clear_bit $(scu_addr 90) 7
#devmem_clear_bit $(scu_addr 70) 4
## Do I need to set 70:1 and 70:0 to 1?
echo 56 > /sys/class/gpio/export
echo 57 > /sys/class/gpio/export
echo 58 > /sys/class/gpio/export

# HOTSWAP_PG, hotswap issues, GPIO L3
devmem_clear_bit $(scu_addr 90) 5
devmem_clear_bit $(scu_addr 90) 4
devmem_clear_bit $(scu_addr 84) 19
echo 99 > /sys/class/gpio/export

# XXX:  These interfere with i2c bus 11 (on Linux, it's 12 on the hardware)
# which we need to talk to the power supplies on certain hardware.
## Hardware presence pins C4 and C5
#devmem_clear_bit $(scu_addr 90) 0
#devmem_clear_bit $(scu_addr 90) 24
#echo 20 > /sys/class/gpio/export
#echo 21 > /sys/class/gpio/export

# FAB_GE_SEL, uServer GE connection, GPIO A0
devmem_clear_bit $(scu_addr 80) 0
echo 0 > /sys/class/gpio/export
# output

# USB_OCS_N1, resettable fuse tripped, GPIO Q6
devmem_clear_bit $(scu_addr 90) 28
echo 136 > /sys/class/gpio/export

# RX loss signal?

# System SPI
# Strap 12 must be 0 and Strape 13 must be 1
devmem_clear_bit $(scu_addr 70) 12
devmem_set_bit $(scu_addr 70) 13
# GPIOQ4 is ISO_FLASH_WP, must be 1 to avoid write protection
# GPIOQ5 is ISO_FLASH_HOLD, must be 1 to be out of reset
# To use GPIOQ4 and GPIOQ5, SCU90[27] must be 0
devmem_clear_bit $(scu_addr 90) 27
gpio_set Q4 1
gpio_set Q5 1
# GPIOD6 is ISO_FL_PRG_SEL, set it to 0 so that BMC does not have control
# on the EEPROM by default.
# To use GPIOD6, SCU90[1] must be 0, SCU8C[21] must be 0, and Strap[21] must be 0
devmem_clear_bit $(scu_addr 90) 1
devmem_clear_bit $(scu_addr 8c) 8
devmem_clear_bit $(scu_addr 70) 21
gpio_set 30 0

# DEBUG_RST_BTN_N, Debug Reset button on front panel, GPIO R2
devmem_clear_bit $(scu_addr 88) 26
echo 138 > /sys/class/gpio/export

# DEBUG_PORT_UART_SEL_N, Debug Select button, GPIO B2
devmem_clear_bit $(scu_addr 80) 10
echo 10 > /sys/class/gpio/export

# DEBUG_UART_SEL_0, select uServer UART to the debug header, GPIO E0
devmem_clear_bit $(scu_addr 80) 16
devmem_clear_bit $(scu_addr 8c) 12
devmem_clear_bit $(scu_addr 70) 22
echo 32 > /sys/class/gpio/export
# output

# USB_BRDG_RST , GPIO D4
devmem_clear_bit $(scu_addr 90 ) 1
devmem_clear_bit $(scu_addr 8c ) 10
devmem_clear_bit $(scu_addr 70 ) 21
gpio_export D4 USB_BRDG_RST

# Bloodhound GPIOs, P0-6, G4, J1-3, Y3
# Make sure GPIOP0,1,2,3,6 are enabled.
for i in {16..19} 22; do
  devmem_clear_bit $(scu_addr 88) $i
done
# Enable GPIOY3
devmem_clear_bit $(scu_addr a4) 11
# GPIOG4
devmem_clear_bit $(scu_addr 2c) 1
# GPIOJ1
devmem_clear_bit $(scu_addr 84) 9
# GPIOJ2
devmem_clear_bit $(scu_addr 84) 10
# GPIOJ11
devmem_clear_bit $(scu_addr 84) 11

# Export all the GPIOs
for i in {120..126} 52 {73..75} 195; do
  echo $i > /sys/class/gpio/export
done

# Enable the isolation buffer
wedge_iso_buf_enable

# Get the board type by parsing the EEPROM.
# This must be after enabling isolation buffer as the i2c bus
# is isolated by the buffer
board_type=$(wedge_board_type)

case "$board_type" in
    FC-LEFT|FC-RIGHT)
        # On FC
        if [ $board_rev -lt 2 ]; then
            # EVT board
            # FAB_SLOT_ID is GPIOU0
            # PEER_FAB_PRSNT is GPIOU1
            devmem_set_bit $(scu_addr a0) 8
            devmem_set_bit $(scu_addr a0) 9
            gpio_export U0
            gpio_export U1
            # T2_POWER_UP is GPIOT6
            devmem_set_bit $(scu_addr a0) 6
            gpio_export T6 T2_POWER_UP
            # HS_FAULT_N is GPIOT7
            devmem_set_bit $(scu_addr a0) 7
            gpio_export T7
            if [ "$board_type" = "FC-LEFT" ]; then
                # GPIOE2 is CPU_EEPROM_SEL, on FC-LEFT
                devmem_clear_bit $(scu_addr 80) 18
                devmem_clear_bit $(scu_addr 8c) 13
                devmem_clear_bit $(scu_addr 70) 22
                gpio_export E2
                # GPIOA6 and GPIOA7 are MAC2 MDIO pins, we use them as
                # GPIO for bitbang driver
                devmem_clear_bit $(scu_addr 90) 2
                devmem_clear_bit $(scu_addr 80) 6
                devmem_clear_bit $(scu_addr 80) 7
                gpio_export A6
                gpio_export A7
            fi
        else
            # DVT board
            if [ "$board_type" = "FC-LEFT" ]; then # Left FC
                # BMC_SW_RST is GPIOL0, 16p switch
                # SCU84[16] must be 0
                devmem_clear_bit $(scu_addr 84) 16
                gpio_set L0 1

                # MDC|MDIO_CONT are GPIOR6 and GPIOR7, 16p switch
                # SCU88[30:31] must be 0
                devmem_clear_bit $(scu_addr 88) 30
                devmem_clear_bit $(scu_addr 88) 31
                gpio_set R6 1
                gpio_set R7 1

                # SWITCH_EEPROM1_WRT is GPIOE2, 16p switch EEPROM (U61)
                # SCU80[18], SCU8C[13], and SCU70[22] must be 0
                devmem_clear_bit $(scu_addr 80) 18
                devmem_clear_bit $(scu_addr 8C) 13
                devmem_clear_bit $(scu_addr 70) 22
                gpio_export E2

                # SPI bus to 16p switch EEPROM
                # GPIOI4 <--> BMC_EEPROM1_SPI_SS
                # GPIOI5 <--> BMC_EEPROM1_SPI_SCK
                # GPIOI6 <--> BMC_EEPROM1_SPI_MOSI
                # GPIOI7 <--> BMC_EEPROM1_SPI_MISO
                # The EEPROM SPI clk does not match with the BMC SPI master.
                # Have to configure these pins as GPIO to use with
                # SPI bitbang driver.
                # SCU70[13:12,5] must be 0
                devmem_clear_bit $(scu_addr 70) 5
                devmem_clear_bit $(scu_addr 70) 12
                devmem_clear_bit $(scu_addr 70) 13
                gpio_export I4
                gpio_export I5
                gpio_export I6
                gpio_export I7

                # BMC_PHY_RST is GPIOT0, Front Panel Port PHY on the 16p switch
                # SCUA0[0] must be 1
                devmem_set_bit $(scu_addr a0) 0
                gpio_set T0 1

                # BMC_5PORTSW_RST is GPIOT1, 5p switch
                # SCUA0[1] must be 1
                devmem_set_bit $(scu_addr a0) 1
                gpio_set T1 1

                # ISO_SWITCH1_MDC|MDIO are GPIOT4 and GPIOT5, 5p switch
                # SCUA0[4:5] must be 1
                devmem_set_bit $(scu_addr a0) 4
                devmem_set_bit $(scu_addr a0) 5
                gpio_set T4 1
                gpio_set T5 1

                # ISO_SWITCH_EEPROM2_WRT is GPIOV0, 5p switch EEPROM (U114)
                # SCUA0[16] must be 1
                devmem_set_bit $(scu_addr a0) 16
                gpio_export V0

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
                gpio_export I0
                gpio_export I1
                gpio_export I2
                gpio_export I3

                # BMC_PHYL_RST is GPIOF0, Left BMC PHY
                # SCU80[24] must be 0
                devmem_clear_bit $(scu_addr 80) 24
                gpio_set F0 1
            else               # Right FC
                # BMC_PHYR_RST is GPIOL1, Right BMC PHY
                # SCU84[17] must be 0
                devmem_clear_bit $(scu_addr 84) 17
                gpio_set L1 1
            fi
            # T2_POWER_UP is GPIOU4
            # SCUA0[12] must be 1
            devmem_set_bit $(scu_addr a0) 12
            gpio_export U4 T2_POWER_UP

            # HS_FAULT_N is GPIOU5
            # SCUA0[13] must be 1
            devmem_set_bit $(scu_addr a0) 13
            gpio_export U5

            # FAB_SLOT_ID is GPIOU6
            # SCUA0[14] must be 1
            devmem_set_bit $(scu_addr a0) 14
            gpio_export U6

            # PEER_FAB_PRSNT is GPIOU7
            # SCUA0[15] must be 1
            devmem_set_bit $(scu_addr a0) 15
            gpio_export U7
        fi
        ;;
    *)
        # Set up to watch for FC presence, and switch between interfaces.
        # GPIOC0..C7, interested in C4, C5
        devmem_clear_bit $(scu_addr 90) 0
        devmem_clear_bit $(scu_addr 90) 25
        if [ $board_rev -lt 3 ]; then
            # Prior to DVTC
            # BP_SLOT_ID GPIO pins are U0, U1, U2, U3
            devmem_set_bit $(scu_addr a0) 8
            devmem_set_bit $(scu_addr a0) 9
            devmem_set_bit $(scu_addr a0) 10
            devmem_set_bit $(scu_addr a0) 11
            gpio_export U0
            gpio_export U1
            gpio_export U2
            gpio_export U3
            # T2_POWER_UP is GPIOT6
            devmem_set_bit $(scu_addr a0) 6
            gpio_export T6 T2_POWER_UP
            # HS_FAULT_N is GPIOT7
            devmem_set_bit $(scu_addr a0) 7
            gpio_export T7
        else
            # Starting from DVTC
            # BP_SLOT_ID GPIO pins are U6, U7, V0, V1
            devmem_set_bit $(scu_addr 70) 6
            devmem_set_bit $(scu_addr a0) 14
            devmem_set_bit $(scu_addr a0) 15
            devmem_set_bit $(scu_addr a0) 16
            devmem_set_bit $(scu_addr a0) 17
            gpio_export U6
            gpio_export U7
            gpio_export V0
            gpio_export V1
            # T2_POWER_UP is GPIOU4
            devmem_set_bit $(scu_addr a0) 12
            gpio_export U4 T2_POWER_UP
            # HS_FAULT_N is GPIOU5
            devmem_set_bit $(scu_addr a0) 13
            gpio_export U5
        fi
        ;;
esac

# Make it possible to turn off T2 if fand sees overheating via GPIOF1
# Do not change the GPIO direction here as the default value of this GPIO
# is low, which causes a Non-maskable interrupt to the uS.
devmem_clear_bit $(scu_addr 80) 25
devmem_clear_bit $(scu_addr a4) 12
echo 41 > /sys/class/gpio/export

# Allow us to set the fan LEDs boards.
# This is GPIO G5, G6, G7, and J0

devmem_clear_bit $(scu_addr 70) 23
devmem_clear_bit $(scu_addr 84) 5
devmem_clear_bit $(scu_addr 84) 6
devmem_clear_bit $(scu_addr 84) 7
devmem_clear_bit $(scu_addr 84) 8

echo 53 > /sys/class/gpio/export
echo 54 > /sys/class/gpio/export
echo 55 > /sys/class/gpio/export
echo 72 > /sys/class/gpio/export
echo "out" > /sys/class/gpio/gpio53/direction
echo "out" > /sys/class/gpio/gpio54/direction
echo "out" > /sys/class/gpio/gpio55/direction
echo "out" > /sys/class/gpio/gpio72/direction

# Once we set "out", output values will be random unless we set them
# to something

echo "0" > /sys/class/gpio/gpio53/value
echo "0" > /sys/class/gpio/gpio54/value
echo "0" > /sys/class/gpio/gpio55/value
echo "0" > /sys/class/gpio/gpio72/value
