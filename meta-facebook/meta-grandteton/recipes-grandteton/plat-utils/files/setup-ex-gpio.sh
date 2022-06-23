#!/bin/bash

. /usr/local/fbpackages/utils/ast-functions

# I/O Expander PCA95552 0xEE FAN Present
gpio_export_ioexp 40-0021 FAN0_PRESENT   4
gpio_export_ioexp 40-0021 FAN4_PRESENT   5
gpio_export_ioexp 40-0021 FAN8_PRESENT   6
gpio_export_ioexp 40-0021 FAN12_PRESENT  7
gpio_export_ioexp 40-0021 FAN1_PRESENT   0
gpio_export_ioexp 40-0021 FAN5_PRESENT   1
gpio_export_ioexp 40-0021 FAN9_PRESENT   2
gpio_export_ioexp 40-0021 FAN13_PRESENT  3

gpio_export_ioexp 41-0021 FAN2_PRESENT   4
gpio_export_ioexp 41-0021 FAN6_PRESENT   5
gpio_export_ioexp 41-0021 FAN10_PRESENT  6
gpio_export_ioexp 41-0021 FAN14_PRESENT  7
gpio_export_ioexp 41-0021 FAN3_PRESENT   0
gpio_export_ioexp 41-0021 FAN7_PRESENT   1
gpio_export_ioexp 41-0021 FAN11_PRESENT  2
gpio_export_ioexp 41-0021 FAN15_PRESENT  3

# I/O Expander PCA9539 0xE8 BIC and GPU
gpio_export_ioexp 29-0074 RST_USB_HUB     0
gpio_export_ioexp 29-0074 RST_SWB_BIC_N   1
gpio_export_ioexp 29-0074 GPU_SMB1_ALERT  8
gpio_export_ioexp 29-0074 GPU_SMB2_ALERT  9
gpio_export_ioexp 29-0074 GPU_BASE_ID0    10
gpio_export_ioexp 29-0074 GPU_BASE_ID1    11
gpio_export_ioexp 29-0074 GPU_PEX_STRAP0  12
gpio_export_ioexp 29-0074 GPU_PEX_STRAP1  13

# I/O Expander PCA9539 0xEC BIC
gpio_export_ioexp 32-0076 BIC_FWSPICK        0
gpio_export_ioexp 32-0076 BIC_UART_BMC_SEL   1


#Set IO Expender
gpio_set BIC_FWSPICK 0
gpio_set RST_SWB_BIC_N 1


# I/O Expander PCA955X FAN LED
gpio_export_ioexp 40-0062  FAN1_LED_GOOD   0
gpio_export_ioexp 40-0062  FAN1_LED_FAIL   1
gpio_export_ioexp 40-0062  FAN5_LED_GOOD   2
gpio_export_ioexp 40-0062  FAN5_LED_FAIL   3
gpio_export_ioexp 40-0062  FAN9_LED_GOOD   4
gpio_export_ioexp 40-0062  FAN9_LED_FAIL   5
gpio_export_ioexp 40-0062  FAN13_LED_GOOD  6
gpio_export_ioexp 40-0062  FAN13_LED_FAIL  7
gpio_export_ioexp 40-0062  FAN12_LED_GOOD  8
gpio_export_ioexp 40-0062  FAN12_LED_FAIL  9
gpio_export_ioexp 40-0062  FAN8_LED_GOOD   10
gpio_export_ioexp 40-0062  FAN8_LED_FAIL   11
gpio_export_ioexp 40-0062  FAN4_LED_GOOD   12
gpio_export_ioexp 40-0062  FAN4_LED_FAIL   13
gpio_export_ioexp 40-0062  FAN0_LED_GOOD   14
gpio_export_ioexp 40-0062  FAN0_LED_FAIL   15

gpio_export_ioexp 41-0062  FAN3_LED_GOOD   0
gpio_export_ioexp 41-0062  FAN3_LED_FAIL   1
gpio_export_ioexp 41-0062  FAN7_LED_GOOD   2
gpio_export_ioexp 41-0062  FAN7_LED_FAIL   3
gpio_export_ioexp 41-0062  FAN11_LED_GOOD  4
gpio_export_ioexp 41-0062  FAN11_LED_FAIL  5
gpio_export_ioexp 41-0062  FAN15_LED_GOOD  6
gpio_export_ioexp 41-0062  FAN15_LED_FAIL  7
gpio_export_ioexp 41-0062  FAN14_LED_GOOD  8
gpio_export_ioexp 40-0062  FAN14_LED_FAIL  9
gpio_export_ioexp 41-0062  FAN10_LED_GOOD  10
gpio_export_ioexp 41-0062  FAN10_LED_FAIL  11
gpio_export_ioexp 41-0062  FAN6_LED_GOOD   12
gpio_export_ioexp 41-0062  FAN6_LED_FAIL   13
gpio_export_ioexp 41-0062  FAN2_LED_GOOD   14
gpio_export_ioexp 41-0062  FAN2_LED_FAIL   15


gpio_set  FAN0_LED_GOOD  1
gpio_set  FAN1_LED_GOOD  1
gpio_set  FAN2_LED_GOOD  1
gpio_set  FAN3_LED_GOOD  1
gpio_set  FAN4_LED_GOOD  1
gpio_set  FAN5_LED_GOOD  1
gpio_set  FAN6_LED_GOOD  1
gpio_set  FAN7_LED_GOOD  1
gpio_set  FAN8_LED_GOOD  1
gpio_set  FAN9_LED_GOOD  1
gpio_set  FAN10_LED_GOOD 1
gpio_set  FAN11_LED_GOOD 1
gpio_set  FAN12_LED_GOOD 1
gpio_set  FAN13_LED_GOOD 1
gpio_set  FAN14_LED_GOOD 1
gpio_set  FAN15_LED_GOOD 1

gpio_set  FAN0_LED_FAIL  0
gpio_set  FAN1_LED_FAIL  0
gpio_set  FAN2_LED_FAIL  0
gpio_set  FAN3_LED_FAIL  0
gpio_set  FAN4_LED_FAIL  0
gpio_set  FAN5_LED_FAIL  0
gpio_set  FAN6_LED_FAIL  0
gpio_set  FAN7_LED_FAIL  0
gpio_set  FAN8_LED_FAIL  0
gpio_set  FAN9_LED_FAIL  0
gpio_set  FAN10_LED_FAIL 0
gpio_set  FAN11_LED_FAIL 0
gpio_set  FAN12_LED_FAIL 0
gpio_set  FAN13_LED_FAIL 0
gpio_set  FAN14_LED_FAIL 0
gpio_set  FAN15_LED_FAIL 0
