/*
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <openbmc/libgpio.h>

enum GPIO_VALUE {
	GPIO_LOW = 0,
	GPIO_HIGH,
};


int set_gpio_init_value_after_export(const char * name, const char *shadow, gpio_value_t value)
{
	int ret;
	if (gpio_is_exported(shadow) == false) {
		gpio_export_by_name(GPIO_CHIP_ASPEED, name, shadow);
	}
	ret = gpio_set_init_value_by_shadow(shadow, value);
	return ret;
}

int
main(int argc, char **argv) {

	printf("Set up GPIO pins.....\n");

	// FPGA_CRCERROR, GPIOB0 (824)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOB0", "FPGA_CRCERROR");

	// FPGA_NCONFIG, GPIOB1 (825)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOB1", "FPGA_NCONFIG");

	// BMC_NIC_PWRBRK_R, GPIOC1 (833)
	set_gpio_init_value_after_export("GPIOC1", "BMC_NIC_PWRBRK_R", GPIO_LOW);

	// BMC_NIC_SMRST_N_R, GPIOC2 (834)
	set_gpio_init_value_after_export("GPIOC2", "BMC_NIC_SMRST_N_R", GPIO_HIGH);

	// NIC_WAKE_BMC_N, GPIOC3 (835)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOC3", "NIC_WAKE_BMC_N");

	// BMC_NIC_FULL_PWR_EN_R, GPIOC4 (836)
	set_gpio_init_value_after_export("GPIOC4", "BMC_NIC_FULL_PWR_EN_R", GPIO_HIGH);

	// NIC_FULL_PWR_PG, GPIOC5 (837)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOC5", "NIC_FULL_PWR_PG");

	// BOARD_REV_ID0, GPIOH0 (872)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOH0", "BOARD_REV_ID0");

	// BOARD_REV_ID1, GPIOH1 (873)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOH1", "BOARD_REV_ID1");

	// BOARD_REV_ID2, GPIOH2 (874)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOH2", "BOARD_REV_ID2");

	// EN_ASD_DEBUG, GPIOI5 (885)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOI5", "EN_ASD_DEBUG");

	// DEBUG_RST_BTN_N, GPIOI7 (887)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOI7", "DEBUG_RST_BTN_N");

	// E1S_1_P3V3_PG_R, GPIOL6 (910)
	set_gpio_init_value_after_export("GPIOL6", "E1S_1_P3V3_PG_R", GPIO_LOW);

	// E1S_2_P3V3_PG_R, GPIOL7 (911)
	set_gpio_init_value_after_export("GPIOL7", "E1S_2_P3V3_PG_R", GPIO_LOW);

	// BMC_FPGA_UART_SEL0_R, GPIOM0 (912)
	set_gpio_init_value_after_export("GPIOM0", "BMC_FPGA_UART_SEL0_R", GPIO_LOW);

	// BMC_FPGA_UART_SEL1_R, GPIOM1 (913)
	set_gpio_init_value_after_export("GPIOM1", "BMC_FPGA_UART_SEL1_R", GPIO_LOW);

	// BMC_FPGA_UART_SEL2_R, GPIOM2 (914)
	set_gpio_init_value_after_export("GPIOM2", "BMC_FPGA_UART_SEL2_R", GPIO_LOW);

	// BMC_FPGA_UART_SEL3_R, GPIOM3 (915)
	set_gpio_init_value_after_export("GPIOM3", "BMC_FPGA_UART_SEL3_R", GPIO_LOW);

	// DEBUG_BMC_UART_SEL_R, GPIOM4 (916)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOM4", "DEBUG_BMC_UART_SEL_R");

	// DEBUG_GPIO_BMC_1, GPION0 (920)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPION0", "DEBUG_GPIO_BMC_1");

	// DEBUG_GPIO_BMC_2, GPION1 (921)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPION1", "DEBUG_GPIO_BMC_2");

	// DEBUG_GPIO_BMC_3, GPION2 (922)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPION2", "DEBUG_GPIO_BMC_3");

	// DEBUG_GPIO_BMC_4, GPION3 (923)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPION3", "DEBUG_GPIO_BMC_4");

	// DEBUG_GPIO_BMC_5, GPION4 (924)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPION4", "DEBUG_GPIO_BMC_5");

	// DEBUG_GPIO_BMC_6, GPION5 (925)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPION5", "DEBUG_GPIO_BMC_6");

	// USB_OC_N1, GPIOO3 (931)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOO3", "USB_OC_N1");

	// SCC_I2C_EN_R, GPIOO4 (932)
	set_gpio_init_value_after_export("GPIOO4", "SCC_I2C_EN_R", GPIO_LOW);

	// BMC_READY_R, GPIOO5 (933)
	set_gpio_init_value_after_export("GPIOO5", "BMC_READY_R", GPIO_LOW);

	// LED_POSTCODE_0, GPIOO7 (935)
	set_gpio_init_value_after_export("GPIOO7", "LED_POSTCODE_0", GPIO_HIGH);

	// LED_POSTCODE_1, GPIOP0 (936)
	set_gpio_init_value_after_export("GPIOP0", "LED_POSTCODE_1", GPIO_HIGH);

	// LED_POSTCODE_2, GPIOP1 (937)
	set_gpio_init_value_after_export("GPIOP1", "LED_POSTCODE_2", GPIO_HIGH);

	// LED_POSTCODE_3, GPIOP2 (938)
	set_gpio_init_value_after_export("GPIOP2", "LED_POSTCODE_3", GPIO_HIGH);

	// LED_POSTCODE_4, GPIOP3 (939)
	set_gpio_init_value_after_export("GPIOP3", "LED_POSTCODE_4", GPIO_HIGH);

	// LED_POSTCODE_5, GPIOP4 (940)
	set_gpio_init_value_after_export("GPIOP4", "LED_POSTCODE_5", GPIO_HIGH);

	// LED_POSTCODE_6, GPIOP5 (941)
	set_gpio_init_value_after_export("GPIOP5", "LED_POSTCODE_6", GPIO_HIGH);

	// LED_POSTCODE_7, GPIOP6 (942)
	set_gpio_init_value_after_export("GPIOP6", "LED_POSTCODE_7", GPIO_HIGH);

	// BMC_LOC_HEARTBEAT_R, GPIOQ1 (945)
	set_gpio_init_value_after_export("GPIOQ1", "BMC_LOC_HEARTBEAT_R", GPIO_HIGH);

	// BIC_READY_IN, GPIOQ6 (950)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOQ6", "BIC_READY_IN");

	// COMP_STBY_PG_IN, GPIOQ7 (951)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOQ7", "COMP_STBY_PG_IN");

	// UIC_LOC_ID_IN, GPIOR1 (953)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOR1", "UIC_LOC_ID_IN");

	// UIC_RMT_ID_IN, GPIOR2 (954)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOR2", "UIC_RMT_ID_IN");

	// BMC_COMP_PWR_EN_R, GPIOR4 (956)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOR4", "BMC_COMP_PWR_EN_R");

	// EXT_MINISAS_INS_OR_N_IN, GPIOR6 (958)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOR6", "EXT_MINISAS_INS_OR_N_IN");

	// NIC_PRSNTB3_N, GPIOS0 (960)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOS0", "NIC_PRSNTB3_N");

	// FM_BMC_TPM_PRSNT_N, GPIOS2 (962)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOS2", "FM_BMC_TPM_PRSNT_N");

	// DEBUG_CARD_PRSNT_N, GPIOS3 (963)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOS3", "DEBUG_CARD_PRSNT_N");

	// BMC_RST_BTN_N_R, GPIOV0 (984)
	set_gpio_init_value_after_export("GPIOV0", "BMC_RST_BTN_N_R", GPIO_HIGH);

	// PCIE_COMP_UIC_RST_N, GPIOV1 (985)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOV1", "PCIE_COMP_UIC_RST_N");

	// BMC_COMP_SYS_RST_N_R, GPIOV2 (986)
	set_gpio_init_value_after_export("GPIOV2", "BMC_COMP_SYS_RST_N_R", GPIO_HIGH);

	// BMC_LED_STATUS_BLUE_EN_R, GPIOV4 (988)
	set_gpio_init_value_after_export("GPIOV4", "BMC_LED_STATUS_BLUE_EN_R", GPIO_LOW);

	// BMC_LED_STATUS_YELLOW_EN_R, GPIOV5 (989)
	set_gpio_init_value_after_export("GPIOV5", "BMC_LED_STATUS_YELLOW_EN_R", GPIO_HIGH);

	// BMC_LED_PWR_BTN_EN_R, GPIOV6 (990)
	set_gpio_init_value_after_export("GPIOV6", "BMC_LED_PWR_BTN_EN_R", GPIO_HIGH);

	// UIC_STBY_PG, GPIOW1 (993)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOW1", "UIC_STBY_PG");

	// PWRGD_NIC_BMC, GPIOW2 (994)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOW2", "PWRGD_NIC_BMC");

	// EMMC_PRESENT_N, GPIOW3 (995)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOW3", "EMMC_PRESENT_N");

	// HSC_P12V_DPB_FAULT_N_IN, GPIOW5 (997)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOW5", "HSC_P12V_DPB_FAULT_N_IN");

	// HSC_COMP_FAULT_N_IN, GPIOW6 (998)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOW6", "HSC_COMP_FAULT_N_IN");

	// SCC_FAULT_N_IN, GPIOW7 (999)
	gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOW7", "SCC_FAULT_N_IN");

	// EMMC_RST_N, GPIOY3 (1011)
	set_gpio_init_value_after_export("GPIOY3", "EMMC_RST_N", GPIO_HIGH);

	return 0;
}
