#!/bin/bash

# shellcheck source=meta-facebook/recipes-fb/obmc_functions/files/fb-common-functions
source /usr/libexec/fb-common-functions

marvell_switch_init() {
	# Drive PWRGD signals high to release the reset pin of the Marvell 88E6393X
	set_gpio FPGA_PWRGD_P1V5_AUX_R 1
	set_gpio FPGA_PWRGD_P1V05_AUX_R 1
	set_gpio FPGA_PWRGD_P5V_AUX_R2 1
	set_gpio FPGA_PWRGD_P12V_AUX_R2 1

	# Clear PHY detect on Port 0
	ftdi-mdio write -d 6 -i 1 -p 0x00 -r 0x00 -v 0x04

	# Configure Port 9 and Port 10 for SGMII mode
	ftdi-mdio write -d 6 -i 1 -p 0x09 -r 0x00 -v 0x0a
	ftdi-mdio write -d 6 -i 1 -p 0x0a -r 0x00 -v 0x0a

	# Configure the external PHY on Port 9 and Port 10 for SGMII mode
	ftdi-mdio write -d 6 -i 1 -p 0x1c -r 0x19 -v 18
	ftdi-mdio write -d 6 -i 1 -p 0x1c -r 0x18 -v $((0xB400 | (0 << 5) | 22))

	ftdi-mdio write -d 6 -i 1 -p 0x1c -r 0x19 -v 0x8001
	ftdi-mdio write -d 6 -i 1 -p 0x1c -r 0x18 -v $((0xB400 | (0 << 5) | 20))

	ftdi-mdio write -d 6 -i 1 -p 0x1c -r 0x19 -v 18
	ftdi-mdio write -d 6 -i 1 -p 0x1c -r 0x18 -v $((0xB400 | (1 << 5) | 22))

	ftdi-mdio write -d 6 -i 1 -p 0x1c -r 0x19 -v 0x8001
	ftdi-mdio write -d 6 -i 1 -p 0x1c -r 0x18 -v $((0xB400 | (1 << 5) | 20))
}

marvell_switch_init
