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
    # Export Group B GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOB2 GPO1

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

    # Export Group H GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOH0 GPIO5_SGPMCK
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOH1 GPIO0_SGPMLD
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOH2 GPIO3_SGPMO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOH3 GPIO1_SGPMI

    # Export Group I GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOI1 JTAG_TDI
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOI2 JTAG_TCK
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOI3 JTAG_TMS
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOI4 JTAG_TDO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOI5 FPGA_BMC_CONFIG_DONE
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOI7 FPGA_NSTATUS

    # Export Group L GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOL6 VGAHS_GPIO2
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOL7 VGAVS_GPIO4

    # Export Group M GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOM0 BMC_UART1_NCTS
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOM2 BMC_UART1_NDSR
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOM3 BMC_UART_SEL_1
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOM4 BMC_UART1_NDTR
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOM5 BMC_UART1_NRTS

    # Export Group N GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPION0 TEMP_SENSOR_ALERT
    gpio_export_by_name  "${ASPEED_GPIO}" GPION1 RESERVED_GPIO67
    gpio_export_by_name  "${ASPEED_GPIO}" GPION2 CPU_RST#
   
    # Export Group P GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOP0 BMC_UART_SEL_2
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOP2 GPIO85_PASSTHRU1_IN
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOP3 GPIO45_PASSTHRU1_OUT
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOP4 GPIO109_PASSTHRU2_IN
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOP5 GPIO21_PASSTHRU2_OUT
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOP6 BMC_GPIO61

    # Export Group V GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV0 GPIO37_INDICATOR#
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV1 UCD90160A_2_GPI1
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV2 UCD90160A_1_GPI1
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV3 PWRMONITOR_BMC
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV4 BMC_PWRGD
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV5 FPGA_DEV_CLR_N
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV6 FPGA_DEV_OE
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV7 FPGA_CONFIG_SEL

    # Export Group X GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOX0 BMC_TPM_SPI_CS_N
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOX1 SPI2CS1#_GPIO86
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOX2 SPI2CS2#_GPIO84
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOX3 SPI2CK_GPIO78
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOX4 SPI2MOSI_GPIO82
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOX5 SPI2MISO_GPIO80
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOX6 BMC_FPGA_JTAG_EN
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOX7 BMC_TPM_SPI_PIRQ_N

    # Export Group Y GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOY0 WDTRST1_GPIO59
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOY1 WDTRST2_GPIO51
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOY2 BMC_GPIO57
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOY3 BMC_GPIO55
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOY4 FWSPI_IO2_GPIO41
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOY5 FWSPI_IO3_GPIO43
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOY7 GPIO35_FWSPIWP#

    # Export Group Z GPIO
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOZ0 SPI1CS1#_GPIO124
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOZ1 I2C_TPM_PP
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOZ2 I2C_TPM_INT_N
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOZ3 SPI1CK_GPIO122
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOZ4 SPI1MOSI_IO0_GPO3
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOZ5 SPI1MISO_IO1_GPO4
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOZ6 GPO5_SPI1_IO2
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOZ7 GPIO120_SPI1_IO3
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
gpio_set_direction BMC_UART1_NCTS in
gpio_set_direction BMC_UART1_NDSR in
gpio_set_direction BMC_UART_SEL_1 out
gpio_set_direction SYS_CPLD_JTAG_EN_N out
gpio_set_direction BMC_UART1_NDTR out
gpio_set_direction BMC_UART1_NRTS out
gpio_set_direction CPU_RST# in
gpio_set_direction BMC_UART_SEL_2 out
gpio_set_direction BMC_GPIO61 out
gpio_set_direction UCD90160A_2_GPI1 out
gpio_set_direction UCD90160A_1_GPI1 out
gpio_set_direction PWRMONITOR_BMC in
gpio_set_direction BMC_PWRGD in
gpio_set_direction FPGA_DEV_CLR_N out
gpio_set_direction FPGA_DEV_OE out
gpio_set_direction FPGA_CONFIG_SEL out
gpio_set_direction BMC_TPM_SPI_CS_N out
gpio_set_direction BMC_FPGA_JTAG_EN out
gpio_set_direction BMC_TPM_SPI_PIRQ_N in
gpio_set_direction WDTRST1_GPIO59 out
gpio_set_direction WDTRST2_GPIO51 out
gpio_set_direction BMC_GPIO57 out
gpio_set_direction BMC_GPIO55 out
gpio_set_direction FWSPI_IO2_GPIO41 out
gpio_set_direction FWSPI_IO3_GPIO43 out
gpio_set_direction I2C_TPM_PP out
gpio_set_direction I2C_TPM_INT_N in
gpio_set_direction GPO5_SPI1_IO2 out
gpio_set_direction JTAG_TDI out
gpio_set_direction JTAG_TDO in
gpio_set_direction JTAG_TCK out
gpio_set_direction JTAG_TMS out

# Once we set "out", output values will be random unless we set them
# to something
# Workaround: Set BMC_JTAG_MUX_IN high to prevnet RunBMC test board MAC3 RMII reset
gpio_set_value BMC_JTAG_MUX_IN 1
# Set UCD90160A_CNTRL high to provide the SMB power
gpio_set_value UCD90160A_CNTRL 1

