/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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
#include <syslog.h>
#include <unistd.h>
#include <openbmc/libgpio.h>

enum GPIO_VALUE {
	GPIO_LOW = 0,
	GPIO_HIGH,
};

typedef struct gpio_info {
	char *shadow_name;
	char *pin_name;
	gpio_direction_t direction;
	gpio_value_t value;
} gpio_cfg;

/* expander gpio table */
gpio_cfg exp_gpio_cfg[] = {
	/* shadow_name, pin_name, direction, value */

	// COMP_PRSNT_N: P00 (748)
	{"COMP_PRSNT_N",         NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// FAN_0_INS_N: P01 (749)
	{"FAN_0_INS_N",          NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// FAN_1_INS_N: P02 (750)
	{"FAN_1_INS_N",          NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// FAN_2_INS_N: P03 (751)
	{"FAN_2_INS_N",          NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// FAN_3_INS_N: P04 (752)
	{"FAN_3_INS_N",          NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// UIC_RMT_INS_N: P05 (753)
	{"UIC_RMT_INS_N",        NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// SCC_LOC_INS_N: P06 (754)
	{"SCC_LOC_INS_N",        NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// SCC_RMT_INS_N: P07 (755)
	{"SCC_RMT_INS_N",        NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// SCC_LOC_TYPE_0: P10 (756)
	{"SCC_LOC_TYPE_0",       NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// SCC_RMT_TYPE_0: P11 (757)
	{"SCC_RMT_TYPE_0",       NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// SCC_STBY_PGOOD: P12 (758)
	{"SCC_STBY_PGOOD",       NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// SCC_FULL_PGOOD: P13 (759)
	{"SCC_FULL_PGOOD",       NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// COMP_PGOOD: P14 (760)
	{"COMP_PGOOD",           NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// DRAWER_CLOSED_N: P15 (761)
	{"DRAWER_CLOSED_N",      NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// E1S_1_PRSNT_N: P16 (762)
	{"E1S_1_PRSNT_N",        NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// E1S_2_PRSNT_N: P17 (763)
	{"E1S_2_PRSNT_N",        NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// I2C_E1S_1_RST_N: P00 (764)
	{"I2C_E1S_1_RST_N",      NULL, GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
	// I2C_E1S_2_RST_N: P01 (765)
	{"I2C_E1S_2_RST_N",      NULL, GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
	// E1S_1_LED_ACT: P02 (766)
	{"E1S_1_LED_ACT",        NULL, GPIO_DIRECTION_OUT, GPIO_VALUE_LOW},
	// E1S_2_LED_ACT: P03 (767)
	{"E1S_2_LED_ACT",        NULL, GPIO_DIRECTION_OUT, GPIO_VALUE_LOW},
	// SCC_STBY_PWR_EN: P04 (768)
	{"SCC_STBY_PWR_EN",      NULL, GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
	// SCC_FULL_PWR_EN: P05 (769)
	{"SCC_FULL_PWR_EN",      NULL, GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
	// BMC_EXP_SOFT_RST_N: P06 (770)
	{"BMC_EXP_SOFT_RST_N",   NULL, GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
	// UIC_COMP_BIC_RST_N: P07 (771)
	{"UIC_COMP_BIC_RST_N",   NULL, GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
	// E1S_1_3V3EFUSE_PGOOD: P10 (772)
	{"E1S_1_3V3EFUSE_PGOOD", NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// E1S_2_3V3EFUSE_PGOOD: P11 (773)
	{"E1S_2_3V3EFUSE_PGOOD", NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// P12V_NIC_FAULT_N: P12 (774)
	{"P12V_NIC_FAULT_N",     NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// P3V3_NIC_FAULT_N: P13 (775)
	{"P3V3_NIC_FAULT_N",     NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// SCC_POR_RST_N: P14 (776)
	{"SCC_POR_RST_N",        NULL, GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
	// IOC_T7_SYS_PGOOD: P15 (777)
	{"IOC_T7_SYS_PGOOD",     NULL, GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
	// BMC_COMP_BLED: P16 (778)
	{"BMC_COMP_BLED",        NULL, GPIO_DIRECTION_OUT, GPIO_VALUE_LOW},
	// BMC_COMP_YLED: P17 (779)
	{"BMC_COMP_YLED",        NULL, GPIO_DIRECTION_OUT, GPIO_VALUE_LOW},
	{NULL, NULL, GPIO_DIRECTION_INVALID, GPIO_VALUE_INVALID}
};

int setup_gpio_with_value(const char *chip_name, const char *shadow_name, const char *pin_name, int offset, gpio_direction_t direction, gpio_value_t value)
{
	int ret = 0;
	if (gpio_is_exported(shadow_name) == false) {
		if (strncmp(chip_name, GPIO_CHIP_ASPEED, GPIO_CHIP_MAX) == 0) {
			ret = gpio_export_by_name(GPIO_CHIP_ASPEED, pin_name, shadow_name);
		}
		else if (strncmp(chip_name, GPIO_CHIP_I2C_IO_EXP, GPIO_CHIP_MAX) == 0) {
			ret = gpio_export_by_offset(GPIO_CHIP_I2C_IO_EXP, offset, shadow_name);
		}
		else {
			printf("failed to recognize chip name: %s\n", chip_name);
			syslog(LOG_ERR, "failed to recognize chip name: %s\n", chip_name);
			return -1;
		}

		if (ret != 0) {
			printf("failed to export %s\n", shadow_name);
			syslog(LOG_ERR, "failed to export %s\n", shadow_name);
			return ret;
		}

		if (direction == GPIO_DIRECTION_OUT) {
			ret = gpio_set_init_value_by_shadow(shadow_name, value);

			if (ret != 0) {
				printf("failed to set initial value to %s\n", shadow_name);
				syslog(LOG_ERR, "failed to set initial value to %s\n", shadow_name);
				return ret;
			}
		}
	}

	return ret;
}

void setup_gpios_by_table(const char *chip_name, gpio_cfg *gpio_config) {
	int ret = 0, offset = 0;

	while (gpio_config[offset].shadow_name != NULL) {
		ret = setup_gpio_with_value(chip_name,
			gpio_config[offset].shadow_name,
			gpio_config[offset].pin_name,
			offset,
			gpio_config[offset].direction,
			gpio_config[offset].value);

		if (ret != 0) {
			printf("failed to setup %s\n", gpio_config[offset].shadow_name);
			syslog(LOG_ERR, "failed to setup %s\n", gpio_config[offset].shadow_name);
		}
		offset += 1;
	}
}

int set_gpio_init_value_after_export(const char * name, const char *shadow, gpio_value_t value)
{
	int ret = 0;
	if (gpio_is_exported(shadow) == false) {
		gpio_export_by_name(GPIO_CHIP_ASPEED, name, shadow);
	}
	ret = gpio_set_init_value_by_shadow(shadow, value);
	return ret;
}

int
main(int argc, char **argv) {

	printf("Set up GPIO pins...\n");

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


	printf("Set up expander GPIO pins...\n");
	setup_gpios_by_table(GPIO_CHIP_I2C_IO_EXP, exp_gpio_cfg);

	printf("done.\n");

	return 0;
}
