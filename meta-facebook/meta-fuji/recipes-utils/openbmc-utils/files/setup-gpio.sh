#!/bin/bash
#
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
# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

setup_gpio_fuji_evt() {
    # Export Group F GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOF0 BMC_JTAG_MUX_IN
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOF1 BMC_UART_SEL_0
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOF2 BMC_FCM_1_SEL
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOF3 FCM_2_CARD_PRESENT
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOF4 FCM_1_CARD_PRESENT
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOF5 BMC_SCM_CPLD_JTAG_EN_N
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOF6 FCM_2_CPLD_JTAG_EN_N
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOF7 BMC_FCM_2_SEL

    # Export Group G GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOG0 FCM_1_CPLD_JTAG_EN_N
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOG1 BMC_GPIO63
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOG2 UCD90160A_CNTRL
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOG3 BMC_UART_SEL_3
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOG4 SYS_CPLD_JTAG_EN_N
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOG6 BMC_I2C_SEL
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOG7 BMC_GPIO53

    # Export Group I GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOI5 FPGA_BMC_CONFIG_DONE
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOI7 FPGA_NSTATUS

    # Export Group M GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOM3 BMC_UART_SEL_1

    # Export Group N GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPION0 LM75_ALERT
    gpio_export_by_name  "${ASPEED_GPIO}" GPION2 CPU_RST#
    gpio_export_by_name  "${ASPEED_GPIO}" GPION3 GPIO123_USB2BVBUSSNS
    gpio_export_by_name  "${ASPEED_GPIO}" GPION4 GPIO126_USB2APWREN
    gpio_export_by_name  "${ASPEED_GPIO}" GPION5 GPIO125_USB2AVBUSSNS
   
    # Export Group P GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOP0 BMC_UART_SEL_2
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOP6 BMC_GPIO61

    # Export Group V GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV1 UCD90160A_2_GPI1
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV2 UCD90160A_1_GPI1
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV3 PWRMONITOR_BMC
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV4 BMC_PWRGD
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV5 FPGA_DEV_CLR_N
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV6 FPGA_DEV_OE
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV7 FPGA_CONFIG_SEL

    gpio_export_by_name  "${ASPEED_GPIO}" GPIOW0 LPCD0
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOW1 LPCD1
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOW2 LPCD2
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOW3 LPCD3
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOW4 LPCCLK
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOW5 LPCFRAME#
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOW6 LPCIRQ#
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOW7 LPCRST#

    # Export Group X GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOX6 BMC_FPGA_JTAG_EN
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOX7 BMC_TPM_SPI_PIRQ_N

    # Export Group Y GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOY2 BMC_GPIO57
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOY3 BMC_GPIO55
}

setup_gpio_fuji_evt

gpio_set_direction BMC_JTAG_MUX_IN out
gpio_set_direction BMC_UART_SEL_0 out
gpio_set_direction BMC_FCM_1_SEL out
gpio_set_direction FCM_2_CARD_PRESENT in
gpio_set_direction FCM_1_CARD_PRESENT in
gpio_set_direction BMC_SCM_CPLD_JTAG_EN_N out
gpio_set_direction FCM_2_CPLD_JTAG_EN_N out
gpio_set_direction BMC_FCM_2_SEL out
gpio_set_direction FCM_1_CPLD_JTAG_EN_N out
gpio_set_direction BMC_GPIO63 out
gpio_set_direction UCD90160A_CNTRL out
gpio_set_direction BMC_UART_SEL_3 out
gpio_set_direction BMC_I2C_SEL out
gpio_set_direction BMC_GPIO53 out
gpio_set_direction FPGA_BMC_CONFIG_DONE in
gpio_set_direction FPGA_NSTATUS in
gpio_set_direction BMC_UART_SEL_1 out
gpio_set_direction SYS_CPLD_JTAG_EN_N out
gpio_set_direction LM75_ALERT in
gpio_set_direction CPU_RST# in
gpio_set_direction GPIO123_USB2BVBUSSNS in
gpio_set_direction GPIO125_USB2AVBUSSNS in
gpio_set_direction GPIO126_USB2APWREN out
gpio_set_direction BMC_UART_SEL_2 out
gpio_set_direction BMC_GPIO61 out
gpio_set_direction UCD90160A_2_GPI1 out
gpio_set_direction UCD90160A_1_GPI1 out
gpio_set_direction PWRMONITOR_BMC in
gpio_set_direction BMC_PWRGD in
gpio_set_direction FPGA_DEV_CLR_N out
gpio_set_direction FPGA_DEV_OE out
gpio_set_direction FPGA_CONFIG_SEL out
gpio_set_direction LPCD0 in
gpio_set_direction LPCD1 in
gpio_set_direction LPCD2 in
gpio_set_direction LPCD3 in
gpio_set_direction LPCCLK in
gpio_set_direction LPCFRAME# in
gpio_set_direction LPCIRQ# in
gpio_set_direction LPCRST# in
gpio_set_direction BMC_FPGA_JTAG_EN out
gpio_set_direction BMC_TPM_SPI_PIRQ_N in
gpio_set_direction BMC_GPIO57 out
gpio_set_direction BMC_GPIO55 out

# Once we set "out", output values will be random unless we set them
# to something
# Set UCD90160A_CNTRL high to provide the SMB power
gpio_set_value UCD90160A_CNTRL 1
# Set JTAG to Header (0:Header 1:BMC)
gpio_set_value BMC_JTAG_MUX_IN 0
# Set FPGA JTAG to Header (0:BMC 1:Header)
gpio_set_value BMC_FPGA_JTAG_EN 1
# Set PFR FPGA Image,Current Not Used,set Image1 as default (0:Image0 1:Image1)
gpio_set_value FPGA_CONFIG_SEL 1
# FPGA all register clear enable (0:enable 1:disable)
gpio_set_value FPGA_DEV_CLR_N 1
# FPGA all I/O pins tri-stated enable (0:enable 1:disbale)
gpio_set_value FPGA_DEV_OE 1
# USB Power Out enable(0:disable 1:enable)
gpio_set_value GPIO126_USB2APWREN 1
# UCD90160A set reserved
gpio_set_value UCD90160A_2_GPI1 0
gpio_set_value UCD90160A_1_GPI1 0
# BMC UART SEL set reserved
gpio_set_value BMC_UART_SEL_0 0
gpio_set_value BMC_UART_SEL_1 0
gpio_set_value BMC_UART_SEL_2 0
gpio_set_value BMC_UART_SEL_3 0
# BMC GPIO set reserved
gpio_set_value BMC_GPIO53 0
# set BMC_GPIO55 High to prevent EMMC disable
gpio_set_value BMC_GPIO55 1
gpio_set_value BMC_GPIO57 0
gpio_set_value BMC_GPIO61 0
gpio_set_value BMC_GPIO63 0
# CPLD JTAG Enable (0:enable 1:disable)
gpio_set_value SYS_CPLD_JTAG_EN_N 1
gpio_set_value FCM_2_CPLD_JTAG_EN_N 1
gpio_set_value BMC_SCM_CPLD_JTAG_EN_N 1
gpio_set_value FCM_1_CPLD_JTAG_EN_N 1
# JTAG Connected (0:connected to JTAG 1:disconnected to JTAG)
gpio_set_value BMC_FCM_1_SEL 1
gpio_set_value BMC_FCM_2_SEL 1
# PFR status by i2c bus (0:disconnect this bus 1:connect this bus)
gpio_set_value BMC_I2C_SEL 0
