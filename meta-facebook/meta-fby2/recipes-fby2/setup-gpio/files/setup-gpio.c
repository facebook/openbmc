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
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <sys/mman.h>
#include <openbmc/pal.h>
#include <openbmc/libgpio.h>

#define SCU_BASE    	0x1E6E2000
#define REG_SCU7C   	0x7C
#define REG_SCU80   	0x80
#define REG_SCU84   	0x84
#define REG_SCU88   	0x88
#define REG_SCU8C   	0x8C
#define REG_SCU90   	0x90
#define REG_SCU94   	0x94
#define REG_SCU9C   	0x9C
#define REG_SCU2C   	0x2C
#define REG_SCUA0   	0xA0
#define REG_SCUA4   	0xA4
#define REG_SCUA8   	0xA8

#define WDT_BASE		0x1E785000
#define REG_WDT1_RESET	0x1C

#define GPIO_BASE		0x1E780000

#define PAGE_SIZE   	0x1000

#define ASPPED_CHIP   "aspeed-gpio"
#define GPIO_TMP_PATH "/tmp/gpionames/%s"

// Registers to set
enum {
	SCU80 = 0,
	SCU84,
	SCU88,
	SCU8C,
	SCU90,
	SCU94,
	SCU2C,
	SCUA0,
	SCUA4,
	SCUA8,
	LASTEST_REG,
};

int
get_register(uint32_t base, uint32_t offset, uint32_t *value) {
	void *base_addr, *read_addr;
	int ret = 0;
	int fd = 0;

	fd = open("/dev/mem", O_RDWR | O_SYNC );
	if (fd < 0) {
		syslog(LOG_ERR, "%s - cannot open /dev/mem", __FILE__);
		return -1;
	}
	base_addr = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, base);
	read_addr = (char*)base_addr + offset;
	*value = *(volatile uint32_t*) read_addr;
	close(fd);

	return ret;
}

int
set_register(uint32_t base, uint32_t offset, uint32_t value) {
	void *base_addr, *write_addr;
	int ret = 0;
	int fd = 0;

	fd = open("/dev/mem", O_RDWR | O_SYNC );
	if (fd < 0) {
		syslog(LOG_ERR, "%s - cannot open /dev/mem", __FILE__);
		return -1;
	}
	base_addr = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, base);
	write_addr = (char*)base_addr + offset;
	*(volatile uint32_t*) write_addr = value;

	close(fd);
	return value;
}

int
reg_set_bit(uint32_t base, uint32_t offset, uint8_t bit) {
	int ret = 0;
	uint32_t value = 0;

	ret = get_register(base, offset, &value);
	if (ret < 0) {
		return -1;
	}
	value |= (0x1 << bit);
	ret = set_register(base, offset, value);

	return ret;
}

int
reg_clear_bit(uint32_t base, uint32_t offset, uint8_t bit) {
	int ret = 0;
	uint32_t value = 0;

	ret = get_register(base, offset, &value);
	if (ret < 0) {
		return -1;
	}
	value &= ~(0x1 << bit);
	ret = set_register(base, offset, value);

	return ret;
}

int
gpio_set(char* name, char* shadow, uint8_t value) {
	gpio_desc_t *gdesc = NULL;
	int ret = 0;
	char path[64] = {0};

	sprintf(path, GPIO_TMP_PATH, shadow);
	if (access(path, F_OK) != 0) {
		gpio_export_by_name(ASPPED_CHIP, name, shadow);
	}
	gdesc = gpio_open_by_shadow(shadow);
	if (gdesc == NULL) {
		syslog(LOG_ERR, "Fail to open GPIO %s", shadow);
		return -1;
	}

	if (value == 1) {
		ret = gpio_set_init_value(gdesc, GPIO_VALUE_HIGH);
	} else {
		ret = gpio_set_init_value(gdesc, GPIO_VALUE_LOW);
	}	
	if (ret < 0) {
		syslog(LOG_ERR, "Fail to set GPIO %s value", shadow);
	}

error_exit:
	gpio_close(gdesc);

	return ret;
}

int
main(int argc, char **argv) {
	int ret = 0;
	uint8_t slot_12v_on, slot_prsnt;
	uint32_t reg[LASTEST_REG];

	printf("Set up GPIO pins.....\n");
	// Read register initial value
	get_register(SCU_BASE, REG_SCU80, &reg[SCU80]);
	get_register(SCU_BASE, REG_SCU84, &reg[SCU84]);
	get_register(SCU_BASE, REG_SCU88, &reg[SCU88]);
	get_register(SCU_BASE, REG_SCU8C, &reg[SCU8C]);
	get_register(SCU_BASE, REG_SCU90, &reg[SCU90]);
	get_register(SCU_BASE, REG_SCU94, &reg[SCU94]);
	get_register(SCU_BASE, REG_SCU2C, &reg[SCU2C]);
	get_register(SCU_BASE, REG_SCUA0, &reg[SCUA0]);
	get_register(SCU_BASE, REG_SCUA4, &reg[SCUA4]);
	get_register(SCU_BASE, REG_SCUA8, &reg[SCUA8]);

	// To use GPIOE0~E5, SCU80[21:16] must be 0
	// To use GPIOF0~F3, SCU80[27:24] must be 0
	// To use GPIOA0   , SCU80[0] must be 0
	// To use GPIOB5~B6, SCU80[14:13] must be 0
	reg[SCU80] &= ~(0x0F3F6001);
	
	// To use GPIOJ0~J3, SCU84[11:8] must be 0
	// To use GPIOM0~M3, SCU84[27:24] must be 0
	// To use GPIOG0~G3, SCU84[3:0] must be 0
	// To use GPIOG6   , SCU84[7] must be 0
	reg[SCU84] &= ~(0x0F000F8F);
	
	// To use GPIOR3   , SCU88[27] must be 0
	// To use GPIOO4~O7, SCU88[15:12] must be 0
	// To use GPIOP0~P3, SCU88[19:16] must be 0
	// Enable GPIO pins: I2C_SLOTx_ALERT_N pins for BIC firmware update, SCU88[2:5] must be 0
	reg[SCU88] &= ~(0x080FF03C);
	
	// To use GPIOAA0~AA3, GPIOZ0~Z3, SCU90[31] must be 0
	// To use GPIOD1   , SCU90[1] must be 0
	// To use GPIOM0~M3, SCU90[5:4] must be 0
	// To use GPIOF0   , SCU90[30] must be 0
	// To use GPIOH5   , SCU90[7] must be 0
	reg[SCU90] &= ~(0xC00000B2);
	
	// To use GPIOAB1  , SCU94[1:0] must be 0
	// To use GPIOG6   , SCU94[12] must be 0
	// To use GPIOH5   , SCU94[7] must be 0
	reg[SCU94] &= ~(0x00001083);

	// To use GPIOQ6   , SCU2C[1] must be 0
	reg[SCU2C] &= ~(0x00000002);
	
	// To use GPIOAA0~AA7, SCUA4[31:24] must be 0
	// To use GPIOZ0~Z3, SCUA4[19:16] must be 0
	// enable I2C ctrl reg (SCUA4),  SCUA4[15:12] must be 1
	reg[SCUA4] &= ~(0xFF0F0000);
	reg[SCUA4] |= 0x0000F000;
	
	// To use GPIOAB1~AB3, SCUA8[3:1] must be 0
	reg[SCUA8] &= ~(0x0000000E);

	set_register(SCU_BASE, REG_SCU80, reg[SCU80]);
	set_register(SCU_BASE, REG_SCU84, reg[SCU84]);
	set_register(SCU_BASE, REG_SCU88, reg[SCU88]);
	set_register(SCU_BASE, REG_SCU8C, reg[SCU8C]);
	set_register(SCU_BASE, REG_SCU90, reg[SCU90]);
	set_register(SCU_BASE, REG_SCU94, reg[SCU94]);
	set_register(SCU_BASE, REG_SCU2C, reg[SCU2C]);
	set_register(SCU_BASE, REG_SCUA0, reg[SCUA0]);
	set_register(SCU_BASE, REG_SCUA4, reg[SCUA4]);
	set_register(SCU_BASE, REG_SCUA8, reg[SCUA8]);

	// To use GPIOI0~GPIOI7, SCU70[13:12] must be 0
	// To use GPIOD1, SCU70[21] shall be 0
	// To use GPIOE0, GPIOE4, SCU70[22] must be 0
	// To use GPIOB4, SCU70[23] must be 0
	set_register(SCU_BASE, REG_SCU7C, 0x00E03000);

	// SLOT1_PRSNT_N, GPIOAA0 (208)
	gpio_export_by_name(ASPPED_CHIP, "GPIOAA0", "SLOT1_PRSNT_N");
	
	// SLOT2_PRSNT_N, GPIOAA1 (209)
	gpio_export_by_name(ASPPED_CHIP, "GPIOAA1", "SLOT2_PRSNT_N");
	
	// SLOT3_PRSNT_N, GPIOAA2 (210)
	gpio_export_by_name(ASPPED_CHIP, "GPIOAA2", "SLOT3_PRSNT_N");
	
	// SLOT4_PRSNT_N, GPIOAA3 (211)
	gpio_export_by_name(ASPPED_CHIP, "GPIOAA3", "SLOT4_PRSNT_N");
	
	// SLOT1_PRSNT_B_N, GPIOZ0 (200)
	gpio_export_by_name(ASPPED_CHIP, "GPIOZ0", "SLOT1_PRSNT_B_N");
	
	// SLOT2_PRSNT_B_N, GPIOZ1 (201)
	gpio_export_by_name(ASPPED_CHIP, "GPIOZ1", "SLOT2_PRSNT_B_N");
	
	// SLOT3_PRSNT_B_N, GPIOZ2 (202)
	gpio_export_by_name(ASPPED_CHIP, "GPIOZ2", "SLOT3_PRSNT_B_N");
	
	// SLOT4_PRSNT_B_N, GPIOZ3 (203)
	gpio_export_by_name(ASPPED_CHIP, "GPIOZ3", "SLOT4_PRSNT_B_N");
	
	// BMC_PWR_BTN_IN_N, uServer power button in, on GPIO D0(24)
	gpio_export_by_name(ASPPED_CHIP, "GPIOD0", "BMC_PWR_BTN_IN_N");
	
	// PWR_SLOT1_BTN_N, 1S Server power out, on GPIO D1
	gpio_set("GPIOD1", "PWR_SLOT1_BTN_N", 1);
	
	// PWR_SLOT2_BTN_N, 1S Server power out, on GPIO D3
	// Make sure the Power Control Pin is Set properly
	gpio_set("GPIOD3", "PWR_SLOT2_BTN_N", 1);
	
	// PWR_SLOT3_BTN_N, 1S Server power out, on GPIO D5
	gpio_set("GPIOD5", "PWR_SLOT3_BTN_N", 1);
	
	// PWR_SLOT4_BTN_N, 1S Server power out, on GPIO D7
	gpio_set("GPIOD7", "PWR_SLOT4_BTN_N", 1);
	
	// SMB_SLOT1_NIC_ALERT_N, alert for 1S Server NIC I2C, GPIO B0
	gpio_export_by_name(ASPPED_CHIP, "GPIOB0", "SMB_SLOT1_NIC_ALERT_N");

	// SMB_SLOT2_NIC_ALERT_N, alert for 1S Server NIC I2C, GPIO B1
	gpio_export_by_name(ASPPED_CHIP, "GPIOB1", "SMB_SLOT2_NIC_ALERT_N");

	// SMB_SLOT3_NIC_ALERT_N, alert for 1S Server NIC I2C, GPIO B2
	gpio_export_by_name(ASPPED_CHIP, "GPIOB2", "SMB_SLOT3_NIC_ALERT_N");

	// SMB_SLOT4_NIC_ALERT_N, alert for 1S Server NIC I2C, GPIO B3
	gpio_export_by_name(ASPPED_CHIP, "GPIOB3", "SMB_SLOT4_NIC_ALERT_N");

	// Enable P3V3: GPIOAB1(217)
	gpio_set("GPIOAB1", "P3V3_EN_R", 1);
	
	// BMC_SELF_HW_RST: GPIOAB2(218)
	gpio_set("GPIOAB2", "BMC_SELF_HW_RST", 0);

	// VGA Mux
	gpio_export_by_name(ASPPED_CHIP, "GPIOJ2", "VGA_SELECT_ID0");
	gpio_export_by_name(ASPPED_CHIP, "GPIOJ3", "VGA_SELECT_ID1");
	
	//========================================================================================================//	
	// Setup GPIOs to Mux Enable: GPIOAB3(219), Channel Select: GPIOE4(36), GPIOE5(37)
	gpio_export_by_name(ASPPED_CHIP, "GPIOAB3", "USB_MUX_EN_R_N");
	gpio_export_by_name(ASPPED_CHIP, "GPIOE4", "FM_USB_SW0");
	gpio_export_by_name(ASPPED_CHIP, "GPIOE5", "FM_USB_SW1");
	//========================================================================================================//
	
	// USB_OC_N, resettable fuse tripped, GPIO Q6
	gpio_export_by_name(ASPPED_CHIP, "GPIOQ6", "USB_OC_N");
	
	// FM_POST_CARD_PRES_BMC_N: GPIOR3(139)
	gpio_export_by_name(ASPPED_CHIP, "GPIOR3", "FM_POST_CARD_PRES_BMC_N");
	
	// DEBUG UART Controls
	// 4 signals: DEBUG_UART_SEL_0/1/2 and DEBUG_UART_RX_SEL_N
	// GPIOE0 (32), GPIOE1 (33), GPIOE2 (34) and GPIOE3 (35)	
	gpio_export_by_name(ASPPED_CHIP, "GPIOE0", "DEBUG_UART_SEL_0");	
	gpio_export_by_name(ASPPED_CHIP, "GPIOE1", "DEBUG_UART_SEL_1");
	gpio_export_by_name(ASPPED_CHIP, "GPIOE2", "DEBUG_UART_SEL_2");
	gpio_export_by_name(ASPPED_CHIP, "GPIOE3", "DEBUG_UART_RX_SEL_N");
	
	// Power LED for Slot#2:
	gpio_set("GPIOM0", "PWR1_LED", 1);
	
	// Power LED for Slot#1:
	gpio_set("GPIOM1", "PWR2_LED", 1);

	// Power LED for Slot#4:
	gpio_set("GPIOM2", "PWR3_LED", 1);
	
	// Power LED for Slot#3:
	gpio_set("GPIOM3", "PWR4_LED", 1);
	
	// Identify LED for Slot#2:
	gpio_set("GPIOF0", "SYSTEM_ID1_LED_N", 1);
	
	// Identify LED for Slot#1:
	gpio_set("GPIOF1", "SYSTEM_ID2_LED_N", 1);
	
	// Identify LED for Slot#4:
	gpio_set("GPIOF2", "SYSTEM_ID3_LED_N", 1);
	
	// Identify LED for Slot#3:
	gpio_set("GPIOF3", "SYSTEM_ID4_LED_N", 1);
	
	// MEZZ_PRSNTID_A_SEL_N: GPIOF4
	gpio_export_by_name(ASPPED_CHIP, "GPIOF4", "MEZZ_PRSNTID_A_SEL_N");
	
	// MEZZ_PRSNTID_B_SEL_N: GPIOF5
	gpio_export_by_name(ASPPED_CHIP, "GPIOF5", "MEZZ_PRSNTID_B_SEL_N");
	
	// Front Panel Hand Switch GPIO setup
	// HAND_SW_ID1: GPIOAA4(212)
	gpio_export_by_name(ASPPED_CHIP, "GPIOAA4", "HAND_SW_ID1");

	// HAND_SW_ID2: GPIOAA5(213)
	gpio_export_by_name(ASPPED_CHIP, "GPIOAA5", "HAND_SW_ID2");

	// HAND_SW_ID4: GPIOAA6(214)
	gpio_export_by_name(ASPPED_CHIP, "GPIOAA6", "HAND_SW_ID4");
	
	// HAND_SW_ID8: GPIOAA7(215)
	gpio_export_by_name(ASPPED_CHIP, "GPIOAA7", "HAND_SW_ID8");
	
	// SLOT1_LED: GPIOAC0(224)
	gpio_export_by_name(ASPPED_CHIP, "GPIOAC0", "SLOT1_LED");
	
	// SLOT2_LED: GPIOAC1(225)
	gpio_export_by_name(ASPPED_CHIP, "GPIOAC1", "SLOT2_LED");
	
	// SLOT3_LED: GPIOAC2(226)
	gpio_export_by_name(ASPPED_CHIP, "GPIOAC2", "SLOT3_LED");

	// SLOT4_LED: GPIOAC3(227)
	gpio_export_by_name(ASPPED_CHIP, "GPIOAC3", "SLOT4_LED");
	
	// SLED_SEATED_N: GPIOAC7(231)
	gpio_export_by_name(ASPPED_CHIP, "GPIOAC7", "SLED_SEATED_N");

	// LED POST CODES: 8 GPIO signals
	// LED_POSTCODE_0: GPIOG0 (48)
	gpio_set("GPIOG0", "LED_POSTCODE_0", 0);
		
	// LED_POSTCODE_1: GPIOG1 (49)
	gpio_set("GPIOG1", "LED_POSTCODE_1", 0);
	
	// LED_POSTCODE_2: GPIOG2 (50)
	gpio_set("GPIOG2", "LED_POSTCODE_2", 0);
	
	// LED_POSTCODE_3: GPIOG3 (51)
	gpio_set("GPIOG3", "LED_POSTCODE_3", 0);
	
	// DUAL FAN DETECT: GPIOG6 (54)
	gpio_export_by_name(ASPPED_CHIP, "GPIOG6", "DUAL_FAN_DETECT");

	// LED_POSTCODE_4: GPIOP4 (124)
	gpio_set("GPIOP4", "LED_POSTCODE_4", 0);

	// LED_POSTCODE_5: GPIOP5 (125)
	gpio_set("GPIOP5", "LED_POSTCODE_5", 0);

	// LED_POSTCODE_6: GPIOP6 (126)
	gpio_set("GPIOP6", "LED_POSTCODE_6", 0);

	// LED_POSTCODE_7: GPIOP7 (127)
	gpio_set("GPIOP7", "LED_POSTCODE_7", 0);

	// BMC_READY_N: GPIOA0 (0)
	gpio_set("GPIOA0", "BMC_READY_N", 0);

	// BMC_RST_BTN_IN_N: GPIOAB0 (216)
	gpio_export_by_name(ASPPED_CHIP, "GPIOAB0", "BMC_RST_BTN_IN_N");

	// RESET for all Slots
	// RST_SLOT1_SYS_RESET_N: GPIOS0 (144)
	gpio_set("GPIOS0", "RST_SLOT1_SYS_RESET_N", 1);

	// RST_SLOT2_SYS_RESET_N: GPIOS1 (145)
	gpio_set("GPIOS1", "RST_SLOT2_SYS_RESET_N", 1);

	// RST_SLOT3_SYS_RESET_N: GPIOS2 (146)
	gpio_set("GPIOS2", "RST_SLOT3_SYS_RESET_N", 1);

	// RST_SLOT4_SYS_RESET_N: GPIOS3 (147)
	gpio_set("GPIOS3", "RST_SLOT4_SYS_RESET_N", 1);

	// UART_SEL: GPIOO3 (115)
	gpio_export_by_name(ASPPED_CHIP, "GPIOO3", "UART_SEL");

	// 12V_STBY Enable for Slots
	// P12V_STBY_SLOT1_EN: GPIOO4 (116)
	gpio_export_by_name(ASPPED_CHIP, "GPIOO4", "P12V_STBY_SLOT1_EN");
	pal_is_fru_prsnt(FRU_SLOT1, &slot_prsnt);
	pal_is_server_12v_on(FRU_SLOT1, &slot_12v_on);
	if ((slot_prsnt == 1) && (slot_12v_on != 1)) {
		gpio_set("GPIOO4", "P12V_STBY_SLOT1_EN", 1);
	}	
	
	// P12V_STBY_SLOT2_EN: GPIOO5 (117)
	gpio_export_by_name(ASPPED_CHIP, "GPIOO5", "P12V_STBY_SLOT2_EN");
	pal_is_fru_prsnt(FRU_SLOT2, &slot_prsnt);
	pal_is_server_12v_on(FRU_SLOT2, &slot_12v_on);
	if ((slot_prsnt == 1) && (slot_12v_on != 1)) {
		gpio_set("GPIOO5", "P12V_STBY_SLOT2_EN", 1);
	}
	
	// P12V_STBY_SLOT3_EN: GPIOO6 (118)
	gpio_export_by_name(ASPPED_CHIP, "GPIOO6", "P12V_STBY_SLOT3_EN");
	pal_is_fru_prsnt(FRU_SLOT3, &slot_prsnt);
	pal_is_server_12v_on(FRU_SLOT3, &slot_12v_on);
	if ((slot_prsnt == 1) && (slot_12v_on != 1)) {
		gpio_set("GPIOO6", "P12V_STBY_SLOT3_EN", 1);
	}
	
	// P12V_STBY_SLOT4_EN: GPIOO7 (119)
	gpio_export_by_name(ASPPED_CHIP, "GPIOO7", "P12V_STBY_SLOT4_EN");
	pal_is_fru_prsnt(FRU_SLOT4, &slot_prsnt);
	pal_is_server_12v_on(FRU_SLOT4, &slot_12v_on);
	if ((slot_prsnt == 1) && (slot_12v_on != 1)) {
		gpio_set("GPIOO7", "P12V_STBY_SLOT4_EN", 1);
	}

	// SLOT1_EJECTOR_LATCH_DETECT_N: GPIOP0 (120)
	gpio_export_by_name(ASPPED_CHIP, "GPIOP0", "SLOT1_EJECTOR_LATCH_DETECT_N");
	
	// SLOT2_EJECTOR_LATCH_DETECT_N: GPIOP1 (121)
	gpio_export_by_name(ASPPED_CHIP, "GPIOP1", "SLOT2_EJECTOR_LATCH_DETECT_N");
	
	// SLOT3_EJECTOR_LATCH_DETECT_N: GPIOP2 (122)
	gpio_export_by_name(ASPPED_CHIP, "GPIOP2", "SLOT3_EJECTOR_LATCH_DETECT_N");

	// SLOT4_EJECTOR_LATCH_DETECT_N: GPIOP3 (123)
	gpio_export_by_name(ASPPED_CHIP, "GPIOP3", "SLOT4_EJECTOR_LATCH_DETECT_N");
	
	// FAN_LATCH_DETECT : GPIOH5 (61)
	gpio_export_by_name(ASPPED_CHIP, "GPIOH5", "FAN_LATCH_DETECT");
	
	// Enable GPIO pins: I2C_SLOTx_ALERT_N pins for BIC firmware update
	gpio_export_by_name(ASPPED_CHIP, "GPION2", "I2C_SLOT1_ALERT_N");
	gpio_export_by_name(ASPPED_CHIP, "GPION3", "I2C_SLOT2_ALERT_N");
	gpio_export_by_name(ASPPED_CHIP, "GPION4", "I2C_SLOT3_ALERT_N");
	gpio_export_by_name(ASPPED_CHIP, "GPION5", "I2C_SLOT4_ALERT_N");
	gpio_export_by_name(ASPPED_CHIP, "GPION7", "SMB_HOTSWAP_ALERT_N");
	
	// SLOT1_POWER_EN: GPIOI0 (64)
	gpio_export_by_name(ASPPED_CHIP, "GPIOI0", "SLOT1_POWER_EN");
	// SLOT2_POWER_EN: GPIOI1 (65)
	gpio_export_by_name(ASPPED_CHIP, "GPIOI1", "SLOT2_POWER_EN");
	// SLOT3_POWER_EN: GPIOI2 (66)
	gpio_export_by_name(ASPPED_CHIP, "GPIOI2", "SLOT3_POWER_EN");
	// SLOT4_POWER_EN: GPIOI3 (67)
	gpio_export_by_name(ASPPED_CHIP, "GPIOI3", "SLOT4_POWER_EN");

	// Set SLOT throttle pin
	// BMC_THROTTLE_SLOT1_N: GPIOI4 (68)
	gpio_set("GPIOI4", "BMC_THROTTLE_SLOT1_N", 1);
	// BMC_THROTTLE_SLOT2_N: GPIOI5 (69)
	gpio_set("GPIOI5", "BMC_THROTTLE_SLOT2_N", 1);
	// BMC_THROTTLE_SLOT3_N: GPIOI6 (70)
	gpio_set("GPIOI6", "BMC_THROTTLE_SLOT3_N", 1);
	// BMC_THROTTLE_SLOT4_N: GPIOI7 (71)
	gpio_set("GPIOI7", "BMC_THROTTLE_SLOT4_N", 1);
	
	// Set FAN disable pin
	// DISABLE_FAN_N: GPIOM4 (100)
	gpio_set("GPIOM4", "DISABLE_FAN_N", 1);
	
	// Set FAST PROCHOT pin (Default Enable)
	// FAST_PROCHOT_EN: GPIOR4 (140)
	gpio_set("GPIOR4", "FAST_PROCHOT_EN", 1);

	// PE_BUFF_OE_0_N: GPIOB4 (12)
	gpio_export_by_name(ASPPED_CHIP, "GPIOB4", "PE_BUFF_OE_0_N");
	
	// PE_BUFF_OE_1_N: GPIOB5 (13)
	gpio_export_by_name(ASPPED_CHIP, "GPIOB5", "PE_BUFF_OE_1_N");
	
	// PE_BUFF_OE_2_N: GPIOB6 (14)
	gpio_export_by_name(ASPPED_CHIP, "GPIOB6", "PE_BUFF_OE_2_N");
	
	// PE_BUFF_OE_3_N: GPIOB7 (15)
	gpio_export_by_name(ASPPED_CHIP, "GPIOB7", "PE_BUFF_OE_3_N");
	
	// CLK_BUFF1_PWR_EN_N: GPIOJ0 (72)
	gpio_export_by_name(ASPPED_CHIP, "GPIOJ0", "CLK_BUFF1_PWR_EN_N");
	
	// CLK_BUFF2_PWR_EN_N: GPIOJ1 (73)
	gpio_export_by_name(ASPPED_CHIP, "GPIOJ1", "CLK_BUFF2_PWR_EN_N");
	
	// MEZZ_PRSNTA2_N: GPIOL0
	gpio_export_by_name(ASPPED_CHIP, "GPIOL0", "MEZZ_PRSNTA2_N");
	
	// MEZZ_PRSNTB2_N: GPIOL1
	gpio_export_by_name(ASPPED_CHIP, "GPIOL1", "MEZZ_PRSNTB2_N");
	
	// Disable PWM reset during external reset
	reg_clear_bit(SCU_BASE, REG_SCU9C, 17);

	// Disable PWM reset during WDT1 reset
	reg_clear_bit(WDT_BASE, REG_WDT1_RESET, 17);
	
	// Set debounce timer #1 value to 0x179A7B0 ~= 2s
	set_register(GPIO_BASE, 0x50, 0x179A7B0);
	
	// Select debounce timer #1 for GPIOZ0~GPIOZ3 and GPIOAA0~GPIOAA3
	set_register(GPIO_BASE, 0x194, 0xF0F00);
	
	// Set debounce timer #2 value to 0xBCD3D8 ~= 1s
	set_register(GPIO_BASE, 0x54, 0xBCD3D8);
	
	// Select debounce timer #2 for GPIOP0~GPIOP3
	set_register(GPIO_BASE, 0x100, 0xF000000);
	
	return 0;
}
