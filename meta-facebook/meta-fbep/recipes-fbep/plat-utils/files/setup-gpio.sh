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

. /usr/local/fbpackages/utils/ast-functions

for i in {0..7};
do
  for j in {0..4};
  do
    gpioexp_export 7-002${i} OAM${i}_LINK_CONFIG${j} ${j}
  done
  gpio_set OAM${i}_LINK_CONFIG0 0
  gpio_set OAM${i}_LINK_CONFIG2 0
  gpio_set OAM${i}_LINK_CONFIG4 0
done

for i in {0..3};
do
  gpioexp_export 18-0067 FAN${i}_PRESENT ${i}
  gpioexp_export 18-0067 FAN${i}_PWR_GOOD $((i+4))
  gpioexp_export 18-0067 FAN${i}_OK $((i+8))
  gpioexp_export 18-0067 FAN${i}_FAIL $((i+12))
  gpio_set FAN${i}_OK 1
  gpio_set FAN${i}_FAIL 0
done

# To enable GPIOA
#devmem_clear_bit $(scu_addr 80) 0
#devmem_clear_bit $(scu_addr 80) 1
#devmem_clear_bit $(scu_addr 80) 3
#devmem_clear_bit $(scu_addr 80) 6
#devmem_clear_bit $(scu_addr 80) 7
#devmem_clear_bit $(scu_addr 90) 2

# Reserved
gpio_export BMC_GPIOA1 GPIOA1
gpio_export BMC_GPIOA2 GPIOA2
gpio_export BMC_GPIOA3 GPIOA3

# OAM debug mode enable
gpio_export SCALE_DEBUG_EN_N_ASIC0 GPIOA7
gpio_export SCALE_DEBUG_EN_N_ASIC1 GPIOA6

# To enable GPIOB
#devmem_clear_scu70_bit 23
#devmem_clear_bit $(scu_addr 80) 13
#devmem_clear_bit $(scu_addr 80) 14

# PCIe switch GPIO (preserved)
gpio_export PAX2_SKU_ID GPIOB0
gpio_export PAX2_ALERT GPIOB1
gpio_export PAX3_SKU_ID GPIOB4
gpio_export PAX3_ALERT GPIOB5
gpio_export PAX0_INT2 GPIOB6
gpio_export PAX1_INT2 GPIOB7

# BMC ready
gpio_export BMC_READY_N GPIOB2

# System power good
gpio_export SYS_PWR_READY GPIOB3

# To enable GPIOD
#devmem_clear_scu70_bit 21
#devmem_clear_bit $(scu_addr 8c) 8
#devmem_clear_bit $(scu_addr 8c) 9
#devmem_clear_bit $(scu_addr 8c) 10
#devmem_clear_bit $(scu_addr 8c) 11
#devmem_clear_bit $(scu_addr 90) 1

# To enable GPIOE
#devmem_clear_scu70_bit 22
#devmem_clear_bit $(scu_addr 80) 16
#devmem_clear_bit $(scu_addr 80) 17
#devmem_clear_bit $(scu_addr 80) 18
#devmem_clear_bit $(scu_addr 80) 19
#devmem_clear_bit $(scu_addr 80) 20
#devmem_clear_bit $(scu_addr 80) 21
#devmem_clear_bit $(scu_addr 80) 22
#devmem_clear_bit $(scu_addr 80) 23
#devmem_clear_bit $(scu_addr 8c) 12
#devmem_clear_bit $(scu_addr 8c) 13
#devmem_clear_bit $(scu_addr 8c) 14
#devmem_clear_bit $(scu_addr 8c) 15

# BMC power button (input)
gpio_export BMC_PWR_BTN_IN_N GPIOE0

# PDB 12V POWER (output)
gpio_export BMC_IPMI_PWR_ON GPIOE1
gpio_set BMC_IPMI_PWR_ON 1

# ASIC warm reset
gpio_export WARMRST_BMC_N_ASIC0 GPIOE7
gpio_set WARMRST_BMC_N_ASIC0 1
gpio_export WARMRST_BMC_N_ASIC1 GPIOE6
gpio_set WARMRST_BMC_N_ASIC1 1
gpio_export WARMRST_BMC_N_ASIC2 GPIOE5
gpio_set WARMRST_BMC_N_ASIC2 1
gpio_export WARMRST_BMC_N_ASIC3 GPIOE4
gpio_set WARMRST_BMC_N_ASIC3 1
gpio_export WARMRST_BMC_N_ASIC4 GPIOE3
gpio_set WARMRST_BMC_N_ASIC4 1
gpio_export WARMRST_BMC_N_ASIC5 GPIOE2
gpio_set WARMRST_BMC_N_ASIC5 1

# To enable GPIOF
#devmem_clear_bit $(scu_addr 80) 24
#devmem_clear_bit $(scu_addr 80) 25
#devmem_clear_bit $(scu_addr 80) 26
#devmem_clear_bit $(scu_addr 80) 27
#devmem_clear_bit $(scu_addr 80) 28
#devmem_clear_bit $(scu_addr 80) 29
#devmem_clear_bit $(scu_addr 80) 30
#devmem_clear_bit $(scu_addr 80) 31
#devmem_clear_bit $(scu_addr 90) 30

# POST LED 
gpio_export LED_POSTCODE_0 GPIOF0
gpio_set LED_POSTCODE_0 0
gpio_export LED_POSTCODE_1 GPIOF1
gpio_set LED_POSTCODE_1 0
gpio_export LED_POSTCODE_2 GPIOF2
gpio_set LED_POSTCODE_2 0
gpio_export LED_POSTCODE_3 GPIOF3
gpio_set LED_POSTCODE_3 0
gpio_export LED_POSTCODE_4 GPIOF4
gpio_set LED_POSTCODE_4 0
gpio_export LED_POSTCODE_5 GPIOF5
gpio_set LED_POSTCODE_5 0
gpio_export LED_POSTCODE_6 GPIOF6
gpio_set LED_POSTCODE_6 0
gpio_export LED_POSTCODE_7 GPIOF7
gpio_set LED_POSTCODE_7 0

# To enable GPIOG
#devmem_clear_bit $(scu_addr 84) 0
#devmem_clear_bit $(scu_addr 84) 1
#devmem_clear_bit $(scu_addr 84) 2
#devmem_clear_bit $(scu_addr 84) 3
#devmem_clear_bit $(scu_addr 84) 4
#devmem_clear_bit $(scu_addr 84) 5
#devmem_clear_bit $(scu_addr 84) 6
#devmem_clear_bit $(scu_addr 84) 7
#devmem_clear_bit $(scu_addr 94) 12

# JTAG mux, 1 = Altera PFR, 0 = Lattice
gpio_export SEL_CPLD GPIOG0
gpio_set SEL_CPLD 0

# TPM presence
gpio_export FM_BMC_TPM_PRES_N GPIOG1

# Enable JTAG mux, 1 = Disbale, 0 = Enable
gpio_export JTAG_ENABLE GPIOG2
gpio_set JTAG_ENABLE 0

# OAM test pin (defined by OAM)
gpio_export BMC_OAM_TEST7 GPIOG3
gpio_export BMC_OAM_TEST8 GPIOG7

# Board revision ID
gpio_export REV_ID0 GPIOG4
gpio_export REV_ID1 GPIOG5
gpio_export REV_ID2 GPIOG6

# To enable GPIOH
#devmem_clear_bit $(scu_addr 90) 7
#devmem_clear_bit $(scu_addr 94) 5
#devmem_clear_bit $(scu_addr 94) 6
#devmem_clear_bit $(scu_addr 94) 7

# ASIC presence
gpio_export PRSNT0_N_ASIC7 GPIOH0
gpio_export PRSNT0_N_ASIC6 GPIOH1
gpio_export PRSNT0_N_ASIC5 GPIOH2
gpio_export PRSNT0_N_ASIC4 GPIOH3
gpio_export PRSNT0_N_ASIC3 GPIOH4
gpio_export PRSNT0_N_ASIC2 GPIOH5
gpio_export PRSNT0_N_ASIC1 GPIOH6
gpio_export PRSNT0_N_ASIC0 GPIOH7

# To enable GPIOI
#devmem_clear_scu70_bit 13

# OAM debug pin (defined by OAM)
gpio_export BMC_OAM_TEST13 GPIOI0
gpio_export BMC_OAM_TEST14 GPIOI1

# OAM debug pin (defined by OAM)
gpio_export BMC_OAM_TEST3 GPIOJ0
gpio_export BMC_OAM_TEST4 GPIOJ1
gpio_export BMC_OAM_TEST5 GPIOJ2
gpio_export BMC_OAM_TEST6 GPIOJ3
gpio_export BMC_OAM_TEST0 GPIOJ6
gpio_export BMC_OAM_TEST1 GPIOJ7

# OAM debug mode enable
gpio_export SCALE_DEBUG_EN_N_ASIC5 GPIOJ4
gpio_export SCALE_DEBUG_EN_N_ASIC4 GPIOJ5

# To enable GPIOL
#devmem_clear_bit $(scu_addr 84) 16
#devmem_clear_bit $(scu_addr 84) 17
#devmem_clear_bit $(scu_addr 84) 18
#devmem_clear_bit $(scu_addr 84) 19
#devmem_clear_bit $(scu_addr 84) 20
#devmem_clear_bit $(scu_addr 84) 21
#devmem_clear_bit $(scu_addr 90) 5

# Select clock source from MB
# 0:0 = MB#1
# 1:0 = MB#2
# 0:1 = MB#3
# 1:1 = MB#4
gpio_export SEL0_CLK_MUX GPIOL0
gpio_set SEL0_CLK_MUX 0
gpio_export SEL1_CLK_MUX GPIOL1
gpio_set SEL1_CLK_MUX 0

# Output control for clock source from MB
# 0 = enable, 1 = high-impedance state
gpio_export OEA_CLK_MUX_N GPIOL2
gpio_set OEA_CLK_MUX_N 0
gpio_export OEB_CLK_MUX_N GPIOL3
gpio_set OEB_CLK_MUX_N 0

# ASIC warm reset
gpio_export WARMRST_BMC_N_ASIC7 GPIOL4
gpio_set WARMRST_BMC_N_ASIC7 1
gpio_export WARMRST_BMC_N_ASIC6 GPIOL5
gpio_set WARMRST_BMC_N_ASIC6 1

# To enable GPIOM
#devmem_clear_bit $(scu_addr 84) 24
#devmem_clear_bit $(scu_addr 84) 25
#devmem_clear_bit $(scu_addr 84) 26
#devmem_clear_bit $(scu_addr 84) 27
#devmem_clear_bit $(scu_addr 84) 28
#devmem_clear_bit $(scu_addr 84) 29

# Slave i2c alert (defined by OAM)
gpio_export SMB_ALERT_ASIC01 GPIOM2
gpio_export SMB_ALERT_ASIC23 GPIOM3
gpio_export SMB_ALERT_ASIC45 GPIOM4
gpio_export SMB_ALERT_ASIC67 GPIOM5

# PCIe switch GPIO (preserved)
gpio_export PAX2_INT2 GPIOM0
gpio_export PAX3_INT2 GPIOM6

# Power control with debug jumper J34
# 0 = controlled by BMC
# 1 = controlled by CPLD
gpio_export PWR_CTRL GPIOM1
gpio_set PWR_CTRL 0

# RTC interrupt (PCF85263AT)
gpio_export BMC_RTC_INT GPIOM7

# To enable GPION
#devmem_clear_bit $(scu_addr 88) 2
#devmem_clear_bit $(scu_addr 88) 3
#devmem_clear_bit $(scu_addr 88) 4
#devmem_clear_bit $(scu_addr 88) 5
#devmem_clear_bit $(scu_addr 88) 6
#devmem_clear_bit $(scu_addr 88) 7

# OAM debug pin (defined by OAM)
gpio_export BMC_OAM_TEST2 GPION4

# Board ID
gpio_export BOARD_ID0 GPION5
gpio_export BOARD_ID1 GPION6
gpio_export BOARD_ID2 GPION7

# To enable GPIOO
#devmem_clear_bit $(scu_addr 88) 8
#devmem_clear_bit $(scu_addr 88) 9
#devmem_clear_bit $(scu_addr 88) 10
#devmem_clear_bit $(scu_addr 88) 11
#devmem_clear_bit $(scu_addr 88) 12
#devmem_clear_bit $(scu_addr 88) 13
#devmem_clear_bit $(scu_addr 88) 14
#devmem_clear_bit $(scu_addr 88) 15

# To enable GPIOP
#devmem_clear_bit $(scu_addr 88) 16
#devmem_clear_bit $(scu_addr 88) 17
#devmem_clear_bit $(scu_addr 88) 18
#devmem_clear_bit $(scu_addr 88) 19
#devmem_clear_bit $(scu_addr 88) 20
#devmem_clear_bit $(scu_addr 88) 21
#devmem_clear_bit $(scu_addr 88) 22
#devmem_clear_bit $(scu_addr 88) 23

# OAM debug pin (defined by OAM)
gpio_export BMC_OAM_TEST9 GPIOP0
gpio_export BMC_OAM_TEST10 GPIOP1
gpio_export BMC_OAM_TEST11 GPIOP2
gpio_export BMC_OAM_TEST12 GPIOP3

# ASIC presence
gpio_export PRSNT1_N_ASIC3 GPIOP4
gpio_export PRSNT1_N_ASIC2 GPIOP5
gpio_export PRSNT1_N_ASIC1 GPIOP6
gpio_export PRSNT1_N_ASIC0 GPIOP7

# To enable GPIOQ
#devmem_clear_bit $(scu_addr 2c) 1
#devmem_clear_bit $(scu_addr 2c) 29

# Battery voltage detect control
gpio_export BATTERY_DETECT GPIOQ6
gpio_set BATTERY_DETECT 0

# Reserved
gpio_export BMC_GPIOQ7 GPIOQ7

# To enable GPIOR
#devmem_clear_bit $(scu_addr 88) 25
#devmem_clear_bit $(scu_addr 88) 26
#devmem_clear_bit $(scu_addr 88) 27
#devmem_clear_bit $(scu_addr 88) 28
#devmem_clear_bit $(scu_addr 88) 29
#devmem_clear_bit $(scu_addr 88) 30
#devmem_clear_bit $(scu_addr 88) 31

# OAM debug mode enable
gpio_export SCALE_DEBUG_EN_N_ASIC3 GPIOR6
gpio_export SCALE_DEBUG_EN_N_ASIC2 GPIOR7

# To enable GPIOS
#devmem_clear_bit $(scu_addr 8C) 0
#devmem_clear_bit $(scu_addr 8C) 1
#devmem_clear_bit $(scu_addr 8C) 2
#devmem_clear_bit $(scu_addr 8C) 3
#devmem_clear_bit $(scu_addr 8C) 4
#devmem_clear_bit $(scu_addr 8C) 5
#devmem_clear_bit $(scu_addr 8C) 6
#devmem_clear_bit $(scu_addr 8C) 7

# HSC throttle
gpio_export HSC1_THROT_N GPIOS0
gpio_export HSC2_THROT_N GPIOS1

# Set target OAM for test
gpio_export CPLD_MUX_ID3 GPIOS2
gpio_set CPLD_MUX_ID3 0

# PMBUS alert from HSC
gpio_export PMBUS_BMC_1_ALERT_N GPIOS3

# To enable GPIOT
#devmem_clear_bit $(scu_addr 48) 29
#devmem_clear_bit $(scu_addr 48) 30
#devmem_set_bit $(scu_addr a0) 0
#devmem_set_bit $(scu_addr a0) 4
#devmem_set_bit $(scu_addr a0) 5
#devmem_set_bit $(scu_addr a0) 6

# OAM debug mode enable
gpio_export SCALE_DEBUG_EN_N_ASIC6 GPIOT0
gpio_export SCALE_DEBUG_EN_N_ASIC7 GPIOT6

# To enable GPIOU
#devmem_set_bit $(scu_addr a0) 10
#devmem_set_bit $(scu_addr a0) 11
#devmem_set_bit $(scu_addr a0) 13

# PCIe switch firmware recovery pin
gpio_export BMC_BOOT_RCVR_B0_PAX0 GPIOU4
gpio_set BMC_BOOT_RCVR_B0_PAX0 0
gpio_export BMC_BOOT_RCVR_B0_PAX1 GPIOU5
gpio_set BMC_BOOT_RCVR_B0_PAX1 0
gpio_export BMC_BOOT_RCVR_B0_PAX2 GPIOU6
gpio_set BMC_BOOT_RCVR_B0_PAX2 0
gpio_export BMC_BOOT_RCVR_B0_PAX3 GPIOU7
gpio_set BMC_BOOT_RCVR_B0_PAX3 0

# To enable GPIOV
#devmem_set_bit $(scu_addr a0) 19

# BSM module presence
# 0 = present, 1 = not present
gpio_export BSM_EMMC_PRSNT_R_N GPIOV0

# Debug connector presence
# 1 = no debug port
gpio_export JTAG_0_SCR_PRESENT GPIOV1

# OAM JTAG SEL
gpio_export BMC2OAM_JTAG_PT_EN GPIOV2
gpio_set BMC2OAM_JTAG_PT_EN 0
gpio_export BMC2OAM_JTAG_PT_SEL GPIOV3
gpio_set BMC2OAM_JTAG_PT_SEL 0

# Reset eMMC on BSM
# 0 = reset, 1 = not reset
gpio_export BMC_EMMC_RST_R0_N GPIOV4
gpio_set BMC_EMMC_RST_R0_N 1

# PMBUS alert from HSC
gpio_export PMBUS_BMC_3_ALERT_N GPIOV5
gpio_export PMBUS_BMC_2_ALERT_N GPIOV6

# i2c alert from CPLD
gpio_export CPLD_SMB_ALERT_N GPIOV7

# To enable GPIOY
#devmem_clear_scu70_bit 19
#devmem_clear_bit $(scu_addr 94) 10
#devmem_clear_bit $(scu_addr 94) 11
#devmem_clear_bit $(scu_addr a4) 8
#devmem_clear_bit $(scu_addr a4) 9
#devmem_clear_bit $(scu_addr a4) 10
#devmem_clear_bit $(scu_addr a4) 11

# ASIC presence
gpio_export PRSNT1_N_ASIC7 GPIOY0
gpio_export PRSNT1_N_ASIC6 GPIOY1
gpio_export PRSNT1_N_ASIC5 GPIOY2
gpio_export PRSNT1_N_ASIC4 GPIOY3

# To enable GPIOZ
#devmem_clear_bit $(scu_addr 90) 31
#devmem_clear_bit $(scu_addr a4) 16
#devmem_clear_bit $(scu_addr a4) 17
#devmem_clear_bit $(scu_addr a4) 18
#devmem_clear_bit $(scu_addr a4) 19
#devmem_clear_bit $(scu_addr a4) 20
#devmem_clear_bit $(scu_addr a4) 21
#devmem_clear_bit $(scu_addr a4) 22
#devmem_clear_bit $(scu_addr a4) 23

# Select USB from MB
# [S1:S0]
# 1:1 = MB#0
# 1:0 = MB#1
# 0:1 = MB#2
# 0:0 = MB#3
# PCH
gpio_export USB2_SEL0_U42 GPIOZ0
gpio_set USB2_SEL0_U42 1
gpio_export USB2_SEL1_U42 GPIOZ1
gpio_set USB2_SEL1_U42 1
# BMC
gpio_export USB2_SEL0_U43 GPIOZ2
gpio_set USB2_SEL0_U43 1
gpio_export USB2_SEL1_U43 GPIOZ3
gpio_set USB2_SEL1_U43 1

# To enable GPIOAA
#devmem_clear_bit $(scu_addr a4) 24
#devmem_clear_bit $(scu_addr a4) 25
#devmem_clear_bit $(scu_addr a4) 26
#devmem_clear_bit $(scu_addr a4) 27
#devmem_clear_bit $(scu_addr a4) 28
#devmem_clear_bit $(scu_addr a4) 29
#devmem_clear_bit $(scu_addr a4) 30
#devmem_clear_bit $(scu_addr a4) 31

# SPI flash select of PCIe switch
# 0: access by PCIe switch
# 1: access by BMC
gpio_export SEL_FLASH_PAX0 GPIOAA0
gpio_set SEL_FLASH_PAX0 0
gpio_export SEL_FLASH_PAX1 GPIOAA1
gpio_set SEL_FLASH_PAX1 0
gpio_export SEL_FLASH_PAX2 GPIOAA5
gpio_set SEL_FLASH_PAX2 0
gpio_export SEL_FLASH_PAX3 GPIOAA6
gpio_set SEL_FLASH_PAX3 0

# System log LED
gpio_export SYSTEM_LOG_LED GPIOAA2
gpio_set SYSTEM_LOG_LED 0

# Select USB from PCH/BMC of MB
# 0: BMC
# 1: PCH
gpio_export SEL_USB_MUX GPIOAA3
gpio_set SEL_USB_MUX 0

# SPI BMC write protection
gpio_export SPI_BMC_BT_WP0_N GPIOAA4

# FRU on BSM write protection
# 0 = writable
gpio_export FRU_WP GPIOAA7

# To enable GPIOAB
#devmem_clear_bit $(scu_addr a8) 0
#devmem_clear_bit $(scu_addr a8) 1
#devmem_clear_bit $(scu_addr a8) 3

# Set target OAM for test
gpio_export CPLD_MUX_ID0 GPIOAB1
gpio_set CPLD_MUX_ID0 0
gpio_export CPLD_MUX_ID1 GPIOAB0
gpio_set CPLD_MUX_ID1 0
gpio_export CPLD_MUX_ID2 GPIOAB3
gpio_set CPLD_MUX_ID2 0

# To enable GPIOAC
devmem_clear_bit $(scu_addr ac) 0
devmem_clear_bit $(scu_addr ac) 1
devmem_clear_bit $(scu_addr ac) 2
devmem_clear_bit $(scu_addr ac) 3
devmem_clear_bit $(scu_addr ac) 5
devmem_clear_bit $(scu_addr ac) 6
devmem_clear_bit $(scu_addr ac) 7

# PCIe switch GPIO (preserved)
gpio_export PAX0_SKU_ID GPIOAC0
gpio_export PAX0_ALERT GPIOAC1
gpio_export PAX1_SKU_ID GPIOAC2
gpio_export PAX1_ALERT GPIOAC3

# Reserved
gpio_export BMC_GPIOAC5 GPIOAC5
gpio_export BMC_GPIOAC6 GPIOAC6
gpio_export BMC_GPIOAC7 GPIOAC7

echo -n "Setup PCIe switch config "
# 8S by default
gpio_set PAX0_SKU_ID 0
gpio_set PAX1_SKU_ID 0
gpio_set PAX2_SKU_ID 0
gpio_set PAX3_SKU_ID 0

if [[ -f "/mnt/data/kv_store/server_type" ]]; then
  # If KV had existed
  server_type=$(cat /mnt/data/kv_store/server_type)
  if [[ "$server_type" == "2" ]]; then
    gpio_set PAX0_SKU_ID 1
    gpio_set PAX1_SKU_ID 1
    gpio_set PAX2_SKU_ID 1
    gpio_set PAX3_SKU_ID 1
  fi
else
  # Get config from MB0's BMC
  for retry in {1..30};
  do
    server_type=$(/usr/local/bin/ipmb-util 2 0x20 0xE8 0x0)
    server_type=${server_type:1:1}
    if [[ "$server_type" == "0" ]]; then
      /usr/local/bin/cfg-util server_type 8
      break
    elif [[ "$server_type" == "1" ]]; then
      /usr/local/bin/cfg-util server_type 2
      gpio_set PAX0_SKU_ID 1
      gpio_set PAX1_SKU_ID 1
      gpio_set PAX2_SKU_ID 1
      gpio_set PAX3_SKU_ID 1
      break
    else
      echo -n "."
      sleep 1
    fi
  done
fi
echo "done"

gpio_set BMC_READY_N 0
