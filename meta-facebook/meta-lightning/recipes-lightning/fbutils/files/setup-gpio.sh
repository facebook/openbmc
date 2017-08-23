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

. /usr/local/fbpackages/utils/ast-functions

# GPIOA
# To use GPIOA0(PEER_TRAY_R), SCU80[0] must be 0
# To use GPIOA1(BMC_JTAG_L_EN), SCU80[1] must be 0
# To use GPIOA2(BMC_USB_EN_1), SCU80[2] must be 0
# To use SCL9(BMC_I2C9_CLKBUF2_SCL), SCU90[22] must be 1
# To use SDA9(BMC_I2C9_CLKBUF2_SDA), SCU90[22] must be 1
devmem_clear_bit $(scu_addr 80) 0
devmem_clear_bit $(scu_addr 80) 1
devmem_clear_bit $(scu_addr 80) 2
devmem_set_bit $(scu_addr 90) 22

gpio_get A0
gpio_set A1 0
gpio_set A2 0

# GPIOB
# To use GPIOB0(PE1_PERST#), SCU80[8] must be 0
# To use GPIOB1(BMC_RESET#_DDR3), SCU80[9] must be 0
# To use GPIOB3(LAN_SMB_ALERT_N), SCU80[11] must be 0
# To use GPIOB4(FM_PCH_LAN1_DISABLE_N), SCU80[12] must be 0 and STRAP[14] must be 0
# To use EXTRST#(RST_BMC_EXTRST_N), SCU80[15] must be 1, SCU90[31] must be 0, and SCU3C[3] must be 1
devmem_clear_bit $(scu_addr 80) 8
devmem_clear_bit $(scu_addr 80) 9
devmem_clear_bit $(scu_addr 80) 11
devmem_clear_bit $(scu_addr 80) 12
devmem_clear_bit $(scu_addr 70) 14
devmem_set_bit $(scu_addr 80) 15
devmem_clear_bit $(scu_addr 90) 31
devmem_set_bit $(scu_addr 3c) 3

gpio_set B0 1
gpio_set B1 1
gpio_set B3 1
gpio_set B4 1

# GPIOC
# To use SCL10(BMC_I2C10_8536_SCL), SCU90[0] must be 0 and SCU90[23] must be 1
# To use SDA10(BMC_I2C10_8536_SDA), SCU90[0] must be 0 and SCU90[23] must be 1
# To use SCL11(BMC_I2C11_MINI5_SCL_S), SCU90[0] must be 0 and SCU90[24] must be 1
# To use SDA11(BMC_I2C11_MINI5_SDA_S), SCU90[0] must be 0 and SCU90[24] must be 1
# To use SCL12(BMC_I2C12_MINI6_SCL_S), SCU90[0] must be 0 and SCU90[25] must be 1
# To use SDA12(BMC_I2C12_MINI6_SDA_S), SCU90[0] must be 0 and SCU90[25] must be 1
# To use SCL13(BMC_I2C13_MINI7_SCL_S), SCU90[0] must be 0 and SCU90[26] must be 1
# To use SDA13(BMC_I2C13_MINI7_SDA_S), SCU90[0] must be 0 and SCU90[26] must be 1
devmem_clear_bit $(scu_addr 90) 0
devmem_set_bit $(scu_addr 90) 23
devmem_set_bit $(scu_addr 90) 24
devmem_set_bit $(scu_addr 90) 25
devmem_set_bit $(scu_addr 90) 26

# GPIOD
# To use GPIOD6(PMC_PLX_BOARD), SCU90[1] must be 0, SCU8C[11] must be 0, and STRAP[21] must be 0
devmem_clear_bit $(scu_addr 90) 1
devmem_clear_bit $(scu_addr 8c) 11
devmem_clear_bit $(scu_addr 70) 21

gpio_get D6

# GPIOE
# To use GPIOE0(FLA1_WPN), SCU80[16] must be 0, SCU8C[12] must be 0, and STRAP[22] must be 0 
# To use GPIOE4(BMC_ADD1), SCU80[20] must be 0, SCU8C[14] must be 0, and STRAP[22] must be 0
# To use GPIOE5(BMC_ADD2), SCU80[21] must be 0, SCU8C[14] must be 0, and STRAP[22] must be 0
devmem_clear_bit $(scu_addr 80) 16
devmem_clear_bit $(scu_addr 80) 20
devmem_clear_bit $(scu_addr 80) 21
devmem_clear_bit $(scu_addr 8c) 12
devmem_clear_bit $(scu_addr 8c) 14
devmem_clear_bit $(scu_addr 70) 22

gpio_tolerance_fun E0
gpio_get E4
gpio_get E5

# GPIOF
# To use GPIOF0(BMC_SSD_RST#7), SCU80[24] must be 0
# To use GPIOF1(BMC_SSD_RST#8), SCU80[25] must be 0, SCUA4[12] must be 0, and STRAP[19] must be 1
# To use GPIOF2(BMC_SSD_RST#9), SCU80[26] must be 0, SCUA4[13] must be 0, and STRAP[19] must be 1
# To use GPIOF3(BMC_SSD_RST#10), SCU80[27] must be 0, SCUA4[14] must be 0, and STRAP[19] must be 1
# To use GPIOF4(BMC_SSD_RST#11), SCU80[28] must be 0
# To use GPIOF5(BMC_SSD_RST#12), SCU80[29] must be 0, SCUA4[15] must be 0, and STRAP[19] must be 1
# To use GPIOF6(BMC_SSD_RST#13), SCU80[30] must be 0
# To use GPIOF7(BMC_SSD_RST#14), SCU80[31] must be 0
devmem_clear_bit $(scu_addr 80) 24
devmem_clear_bit $(scu_addr 80) 25
devmem_clear_bit $(scu_addr 80) 26
devmem_clear_bit $(scu_addr 80) 27
devmem_clear_bit $(scu_addr 80) 28
devmem_clear_bit $(scu_addr 80) 29
devmem_clear_bit $(scu_addr 80) 30
devmem_clear_bit $(scu_addr 80) 31
devmem_clear_bit $(scu_addr A4) 12
devmem_clear_bit $(scu_addr A4) 13
devmem_clear_bit $(scu_addr A4) 14
devmem_clear_bit $(scu_addr A4) 15
devmem_set_bit $(scu_addr 70) 19

gpio_set F0 1
gpio_set F1 1
gpio_set F2 1
gpio_set F3 1
gpio_set F4 1
gpio_set F5 1
gpio_set F6 1
gpio_set F7 1

# GPIOG
# To use GPIOG0(HIGH_HEX_0), SCU84[0] must be 0
# To use GPIOG1(HIGH_HEX_1), SCU84[1] must be 0
# To use GPIOG2(HIGH_HEX_2), SCU84[2] must be 0
# To use GPIOG3(HIGH_HEX_3), SCU84[3] must be 0
# To use GPIOG4(FM_TPM_PRES_N), SCU84[4] must be 0 and SCU2C[1] must be 0
# To use GPIOG5(BMC_SELF_HW_RST), SCU84[5] must be 0 and STRAP[23] must be 0
# To use GPIOG6(DP_RST_N), SCU84[6] must be 0
# To use GPIOG7(EXP_TRAY_ID), SCU84[7] must be 0
devmem_clear_bit $(scu_addr 84) 0
devmem_clear_bit $(scu_addr 84) 1
devmem_clear_bit $(scu_addr 84) 2
devmem_clear_bit $(scu_addr 84) 3
devmem_clear_bit $(scu_addr 84) 4
devmem_clear_bit $(scu_addr 84) 5
devmem_clear_bit $(scu_addr 84) 6
devmem_clear_bit $(scu_addr 84) 7
devmem_clear_bit $(scu_addr 2c) 1
devmem_clear_bit $(scu_addr 70) 23

gpio_tolerance_fun G0
gpio_tolerance_fun G1
gpio_tolerance_fun G2
gpio_tolerance_fun G3
gpio_get G4
gpio_set G5 0
gpio_get G6
gpio_get G7

# GPIOH
# NONE

# GPIOI
# To use GPIOI3(P0V9_E_PG), STRAP[13] must be 0
# To use SPICS0#(SPICS0_N), STRAP[12] must be 1
# To use SPICK, STRAP[12] must be 1
# To use SPIDO, STRAP[12] must be 1
# To use SPIDI, STRAP[12] must be 1
devmem_clear_bit $(scu_addr 70) 13
devmem_set_bit $(scu_addr 70) 12

gpio_get I3

# GPIOJ
# To use GPIOJ0(LOW_HEX_0), SCU84[8] must be 0
# To use GPIOJ1(LOW_HEX_1), SCU84[9] must be 0
# To use GPIOJ2(LOW_HEX_2), SCU84[10] must be 0
# To use GPIOJ3(LOW_HEX_3), SCU84[10] must be 0
devmem_clear_bit $(scu_addr 84) 8
devmem_clear_bit $(scu_addr 84) 9
devmem_clear_bit $(scu_addr 84) 10
devmem_clear_bit $(scu_addr 84) 11

gpio_tolerance_fun J0
gpio_tolerance_fun J1
gpio_tolerance_fun J2
gpio_tolerance_fun J3

# GPIOK
# To use SCL5(BMC_I2C5_D_PEB_DEVICE_SCL), SCU90[18] must be 1
# To use SDA5(BMC_I2C5_D_PEB_DEVICE_SDA), SCU90[18] must be 1
# To use SCL6(BMC_I2C6_C_FCB_SCL), SCU90[19] must be 1
# To use SDA6(BMC_I2C6_C_FCB_SDA), SCU90[19] must be 1
# To use SCL7(BMC_I2C7_B_DBP_SCL), SCU90[20] must be 1
# To use SDA7(BMC_I2C7_B_DPB_SDA), SCU90[20] must be 1
# To use SCL8(BMC_I2C8_CLKBUF1_SCL), SCU90[21] must be 1
# To use SDA8(BMC_I2C8_CLKBUF1_SDA), SCU90[21] must be 1
devmem_set_bit $(scu_addr 90) 18
devmem_set_bit $(scu_addr 90) 19
devmem_set_bit $(scu_addr 90) 20
devmem_set_bit $(scu_addr 90) 21

# GPIOL
# To use GPIOL0(CBL_PRSNT_N_1), SCU84[16] must be 0
# To use GPIOL1(CBL_PRSNT_N_2), SCU84[17] must be 0
# To use GPIOL2(CBL_PRSNT_N_3), SCU84[18] must be 0
# To use GPIOL3(CBL_PRSNT_N_4), SCU84[19] must be 0
# To use GPIOL4(CBL_PRSNT_N_5), SCU84[20] must be 0
# To use GPIOL5(CBL_PRSNT_N_6), SCU84[21] must be 0
# To use GPIOL6(CBL_PRSNT_N_7), SCU84[22] must be 0
# To use GPIOL7(CBL_PRSNT_N_8), SCU84[23] must be 0
devmem_clear_bit $(scu_addr 84) 16
devmem_clear_bit $(scu_addr 84) 17
devmem_clear_bit $(scu_addr 84) 18
devmem_clear_bit $(scu_addr 84) 19
devmem_clear_bit $(scu_addr 84) 20
devmem_clear_bit $(scu_addr 84) 21
devmem_clear_bit $(scu_addr 84) 22
devmem_clear_bit $(scu_addr 84) 23

gpio_get L0
gpio_get L1
gpio_get L2
gpio_get L3
gpio_get L4
gpio_get L5
gpio_get L6
gpio_get L7

# GPIOM
# To use GPIOM0(BMC_I210_PERST_N), SCU84[24] must be 0
# To use GPIOM1(BMC_SSD_RST#0), SCU84[25] must be 0
# To use GPIOM2(BMC_SSD_RST#1), SCU84[26] must be 0
# To use GPIOM3(BMC_SSD_RST#2), SCU84[27] must be 0
# To use GPIOM4(BMC_SSD_RST#3), SCU84[28] must be 0
# To use GPIOM5(BMC_SSD_RST#4), SCU84[29] must be 0
# To use GPIOM6(BMC_SSD_RST#5), SCU84[30] must be 0
# To use GPIOM7(BMC_SSD_RST#6), SCU84[31] must be 0
devmem_clear_bit $(scu_addr 84) 24
devmem_clear_bit $(scu_addr 84) 25
devmem_clear_bit $(scu_addr 84) 26
devmem_clear_bit $(scu_addr 84) 27
devmem_clear_bit $(scu_addr 84) 28
devmem_clear_bit $(scu_addr 84) 29
devmem_clear_bit $(scu_addr 84) 30
devmem_clear_bit $(scu_addr 84) 31

gpio_set M0 1
gpio_set M1 1
gpio_set M2 1
gpio_set M3 1
gpio_set M4 1
gpio_set M5 1
gpio_set M6 1
gpio_set M7 1

# GPION
# To use PWM0(PWM_EXP_A), SCU90[4] must be 0, SCU90[5] must be 0, and SCU88[0] must be 1
# To use GPION1(BMC_FW_DET_BOARD_VER_0), SCU88[1] must be 0
# To use GPION2(LED_DR_LED1), SCU88[2] must be 0
# To use GPION4(BMC_SELF_TRAY), SCU88[4] must be 0
# To use GPION6(BMC_FW_DET_BOARD_VER_1), SCU88[6] must be 0
# To use GPION7(BMC_FW_DET_BOARD_VER_2), SCU88[7] must be 0
devmem_clear_bit $(scu_addr 90) 4
devmem_clear_bit $(scu_addr 90) 5
devmem_set_bit $(scu_addr 88) 0
devmem_clear_bit $(scu_addr 88) 1
devmem_clear_bit $(scu_addr 88) 2
devmem_clear_bit $(scu_addr 88) 4
devmem_clear_bit $(scu_addr 88) 6
devmem_clear_bit $(scu_addr 88) 7

gpio_get N1
gpio_set N2 1
gpio_get N4
gpio_get N6
gpio_get N7

# GPIOO
# To use GPIOO3(HB_OUT_BMC), SCU88[11] must be 0
# To use GPIOO5(PEER_BMC_HB), SCU88[13] must be 0
devmem_clear_bit $(scu_addr 88) 11
devmem_clear_bit $(scu_addr 88) 13

gpio_set O3 0
gpio_get O5

# GPIOP
# To use GPIOP3(BMC_UART_SWITCH_1), SCU88[19] must be 0
# To use GPIOP5(UART_COUNT), SCU88[21] must be 0
# To use GPIOP7(BMC_ENCLOSURE_LED), SCU88[23] must be 0
devmem_clear_bit $(scu_addr 88) 19
devmem_clear_bit $(scu_addr 88) 21
devmem_clear_bit $(scu_addr 88) 23

gpio_tolerance_fun P3
gpio_get P5
gpio_set P7 1

# GPIOQ
# To use SCL3(BMC_I2C3_MINI3_SCL_S), SCU90[16] must be 1
# To use SDA3(BMC_I2C3_MINI3_SDA_S), SCU90[16] must be 1
# To use SCL4(BMC_I2C4_MIN4_SCL_S), SCU90[17] must be 1
# To use SDA4(BMC_I2C4_MIN4_SDA_S), SCU90[17] must be 1
# To use SCL14(BMC_I2C14_MINI8_SCL_S), SCU90[27] must be 1
# To use SDA14(BMC_I2C14_MINI8_SDA_S), SCU90[27] must be 1
# To use GPIOQ6(I2C_MUX_RESET_PEB), SCU90[28] must be 0
# To use GPIOQ7(TCA9554_INT_N), SCU90[28] must be 0
devmem_set_bit $(scu_addr 90) 16
devmem_set_bit $(scu_addr 90) 17
devmem_set_bit $(scu_addr 90) 27
devmem_clear_bit $(scu_addr 90) 28

gpio_set Q6 1
gpio_get Q7

# GPIOR
# To use ROMCS1#(FLA_CS1_R), SCU88[24] must be 1
# To use GPIOR1(TRAY_ID1), SCU88[25] must be 0
# To use GPIOR2(TRAY_ID2), SCU88[26] must be 0
# To use GPIOR3(BP_PRSNT_N), SCU88[27] must be 0
# To use GPIOR4(BMC_INT_N), SCU88[28] must be 0
# To use GPIOR5(TRAY_POWER_CRIT_N), SCU88[29] must be 0
devmem_set_bit $(scu_addr 88) 24
devmem_clear_bit $(scu_addr 88) 25
devmem_clear_bit $(scu_addr 88) 26
devmem_clear_bit $(scu_addr 88) 27
devmem_clear_bit $(scu_addr 88) 28
devmem_clear_bit $(scu_addr 88) 29

gpio_get R1
gpio_get R2
gpio_get R3
gpio_set R4 1
gpio_get R5

# GPIOS
# To use ROMD4, SCU8C[0] must be 1, SCU94[0] must be 0, and SCU94[1] must be 0
# To use ROMD5, SCU8C[1] must be 1, SCU94[0] must be 0, and SCU94[1] must be 0
# To use ROMD6, SCU8C[2] must be 1, SCU94[0] must be 0, and SCU94[1] must be 0
# To use ROMD7, SCU8C[3] must be 1, SCU94[0] must be 0, and SCU94[1] must be 0
# To use GPIOS5(TRAY_ID0), SCU8C[5] must be 0
# To use ROMA22, SCU8C[6] must be 1 and SCU94[1] must be 0
# To use ROMA23, SCU8C[7] must be 1 and SCU94[1] must be 0
devmem_set_bit $(scu_addr 8c) 0
devmem_set_bit $(scu_addr 8c) 1
devmem_set_bit $(scu_addr 8c) 2
devmem_set_bit $(scu_addr 8c) 3
devmem_clear_bit $(scu_addr 8c) 5
devmem_set_bit $(scu_addr 8c) 6
devmem_set_bit $(scu_addr 8c) 7
devmem_clear_bit $(scu_addr 94) 0
devmem_clear_bit $(scu_addr 94) 1

gpio_get S5

# GPIOT
# To use RMII1TXEN(RMII_BMC_TX_EN), SCUA0[0] must be 0 and STRAP[6] must be 0
# To use RMII1TXD0(RMII_BMC_PHY_TXD0), SCUA0[2] must be 0 and STRAP[6] must be 0
# To use RMII1TXD1(RMII_BMC_PHY_TXD1), SCUA0[3] must be 0 and STRAP[6] must be 0
devmem_clear_bit $(scu_addr a0) 0
devmem_clear_bit $(scu_addr a0) 2
devmem_clear_bit $(scu_addr a0) 3
devmem_clear_bit $(scu_addr 70) 6

# GPIOU
# To use RMII1RCLK(CLK_50M_BMC0), SCUA0[12] must be 0 and STRAP[6] must be 0
# To use RMII1RXD0(RMII_PHY_BMC_RXD0), SCUA0[14] must be 0 and STRAP[6] must be 0
# To use RMII1RXD1(RMII_PHY_BMC_RXD1), SCUA0[15] must be 0 and STRAP[6] must be 0
devmem_clear_bit $(scu_addr a0) 12
devmem_clear_bit $(scu_addr a0) 14
devmem_clear_bit $(scu_addr a0) 15
devmem_clear_bit $(scu_addr 70) 6

# GPIOV
# To use RMII1CRSDV(RMII_BMC_CRS_DV), SCUA0[16] must be 0 and STRAP[6] must be 0
# To use RMII1RXER(RMII_BMC_RX_ER), SCUA0[17] must be 0 and STRAP[6] must be 0
# To use RMII2RCLK(CLK_50M_BMC_NCSI), SCUA0[18] must be 0 and STRAP[7] must be 0
devmem_clear_bit $(scu_addr a0) 16
devmem_clear_bit $(scu_addr a0) 17
devmem_clear_bit $(scu_addr a0) 18
devmem_clear_bit $(scu_addr 70) 7

# GPIOW
# To use ADC0(ADC_12V), SCUA0[24] must be 0
# To use ADC1(ADC_P5V_STBY), SCUA0[25] must be 0
# To use ADC2(ADC_P3V3_STBY), SCUA0[26] must be 0
# To use ADC3(ADC_P1V8_STBY), SCUA0[27] must be 0
# To use ADC4(ADC_P1V53V), SCUA0[28] must be 0
# To use ADC5(ADC_P0V9), SCUA0[29] must be 0
# To use ADC6(ADC_P0V9_E), SCUA0[30] must be 0
# To use ADC7(ADC_P1V26), SCUA0[31] must be 0
devmem_clear_bit $(scu_addr a0) 24
devmem_clear_bit $(scu_addr a0) 25
devmem_clear_bit $(scu_addr a0) 26
devmem_clear_bit $(scu_addr a0) 27
devmem_clear_bit $(scu_addr a0) 28
devmem_clear_bit $(scu_addr a0) 29
devmem_clear_bit $(scu_addr a0) 30
devmem_clear_bit $(scu_addr a0) 31

# GPIOX
# NONE

# GPIOY
# To use GPIOY0(SPI_FLASH_SELECT), SCUA4[8] must be 0 and STRAP[19] must be 1
# To use GPIOY1(FCB_HW_REVISION), SCUA4[9] must be 0 and STRAP[19] must be 1
# To use GPIOY2(DPB_HW_REVISION), SCUA4[10] must be 0 and STRAP[19] must be 1
devmem_clear_bit $(scu_addr a4) 8
devmem_clear_bit $(scu_addr a4) 9
devmem_clear_bit $(scu_addr a4) 10
devmem_set_bit $(scu_addr 70) 19

gpio_set Y0 1
gpio_set Y1 0
gpio_set Y2 0

# GPIOZ
# To use ROMA2, SCUA4[16] must be 1, SCU94[0] must be 0, and SCU94[1] must be 0
# To use ROMA3, SCUA4[17] must be 1, SCU94[0] must be 0, and SCU94[1] must be 0
# To use ROMA4, SCUA4[18] must be 1, SCU94[0] must be 0, and SCU94[1] must be 0
# To use ROMA5, SCUA4[19] must be 1, SCU94[0] must be 0, and SCU94[1] must be 0
# To use ROMA6, SCUA4[20] must be 1, SCU94[0] must be 0, and SCU94[1] must be 0
# To use ROMA7, SCUA4[21] must be 1, SCU94[0] must be 0, and SCU94[1] must be 0
# To use ROMA8, SCUA4[22] must be 1, SCU94[0] must be 0, and SCU94[1] must be 0
# To use ROMA9, SCUA4[23] must be 1, SCU94[0] must be 0, and SCU94[1] must be 0
devmem_set_bit $(scu_addr a4) 16
devmem_set_bit $(scu_addr a4) 17
devmem_set_bit $(scu_addr a4) 18
devmem_set_bit $(scu_addr a4) 19
devmem_set_bit $(scu_addr a4) 20
devmem_set_bit $(scu_addr a4) 21
devmem_set_bit $(scu_addr a4) 22
devmem_set_bit $(scu_addr a4) 23
devmem_clear_bit $(scu_addr 94) 0
devmem_clear_bit $(scu_addr 94) 1

# GPIOAA
# To use ROMA10, SCUA4[24] must be 1, SCU94[0] must be 0, and SCU94[1] must be 0
# To use ROMA11, SCUA4[25] must be 1, SCU94[0] must be 0, and SCU94[1] must be 0
# To use ROMA12, SCUA4[26] must be 1, SCU94[0] must be 0, and SCU94[1] must be 0
# To use ROMA13, SCUA4[27] must be 1, SCU94[0] must be 0, and SCU94[1] must be 0
# To use ROMA14, SCUA4[28] must be 1 and SCU94[1] must be 0
# To use ROMA15, SCUA4[29] must be 1 and SCU94[1] must be 0
# To use ROMA16, SCUA4[30] must be 1 and SCU94[1] must be 0
# To use ROMA17, SCUA4[31] must be 1 and SCU94[1] must be 0
devmem_set_bit $(scu_addr a4) 24
devmem_set_bit $(scu_addr a4) 25
devmem_set_bit $(scu_addr a4) 26
devmem_set_bit $(scu_addr a4) 27
devmem_set_bit $(scu_addr a4) 28
devmem_set_bit $(scu_addr a4) 29
devmem_set_bit $(scu_addr a4) 30
devmem_set_bit $(scu_addr a4) 31
devmem_clear_bit $(scu_addr 94) 0
devmem_clear_bit $(scu_addr 94) 1

# GPIOAB
# To use ROMA18, SCUA8[0] must be 1 and SCU94[1] must be 0
# To use ROMA19, SCUA8[1] must be 1 and SCU94[1] must be 0
# To use ROMA20, SCUA8[2] must be 1 and SCU94[1] must be 0
# To use ROMA21, SCUA8[3] must be 1 and SCU94[1] must be 0
devmem_set_bit $(scu_addr a8) 0
devmem_set_bit $(scu_addr a8) 1
devmem_set_bit $(scu_addr a8) 2
devmem_set_bit $(scu_addr a8) 3
devmem_clear_bit $(scu_addr 94) 1


exit 0
