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
#include <openbmc/phymem.h>
#include <openbmc/libgpio.h>
#include <facebook/fbgc_gpio.h>

#define SCU_BASE	0x1E6E2000
#define REG_SCU61C	0x61C
#define REG_SCU630	0x630
#define REG_SCU634	0x634

#define LPC_BASE	0x1E789000
#define REG_HICR9	0x098

#ifdef CONFIG_GRANDCANYON2
/*
 * I2C IO Expander chip device names (as shown in /sys/bus/gpio/devices/).
 * These are used with gpio_export_by_offset() which calls gpiochip_lookup(),
 * matching by dev_name to find the correct gpiochip base.
 *
 * Bus 9 chips (PCA9555, 16 pins each):
 *   U14: "9-0024"  (base=748, pins 0-15)  → enum idx  0-15
 *   U15: "9-0021"  (base=764, pins 0-15)  → enum idx 16-31
 *
 * Bus 11 chips (PCA9557, 8 pins each):
 *   U126: "11-001b" (base=732, pins 0-6)  → enum idx 32-38
 *   U58:  "11-0019" (base=740, pins 0-7)  → enum idx 39-46
 */
#define IOEXP_BUS9_CHIP1   "9-0024"   /* U14: enum idx  0-15 */
#define IOEXP_BUS9_CHIP2   "9-0021"   /* U15: enum idx 16-31 */
#define IOEXP_BUS11_CHIP1  "11-001b"  /* U126: enum idx 32-38 */
#define IOEXP_BUS11_CHIP2  "11-0019"  /* A-side: U58, B-side: U59 (same logical range idx 39-46) */


/*
 * Map global enum index to (chip_device_name, chip_relative_offset).
 *
 * The original code passed GPIO_CHIP_I2C_IO_EXP ("i2c-io-expander") and
 * the raw enum index as offset. gpiochip_lookup() returns the FIRST chip
 * matching "i2c-io-expander" (which is 9-0024, base=748). This works for
 * bus9 pins (idx 0-31 → gpio 748-779) but FAILS for bus11 pins
 * (idx 32+ → 748+32=780 lands on AST2600 SGPIO, not bus11 chips).
 *
 * Fix: use explicit device names so gpiochip_lookup() finds the correct
 * chip, and compute the chip-relative offset.
 */
static const char* get_expander_chip_name(int idx) {
    if (idx < 0 || idx >= (int)MAX_GPIO_EXPANDER_GPIO_PINS)
        return NULL;
    if (idx >= GPIO_COMP_PRSNT_N && idx <= GPIO_E1S_2_PRSNT_N)
        return IOEXP_BUS9_CHIP1;
    if (idx >= GPIO_I2C_E1S_1_RST_N && idx <= GPIO_BMC_COMP_YLED)
        return IOEXP_BUS9_CHIP2;
    if (idx >= GPIO_ALRT_P12V_STBY_SCC_N && idx <= GPIO_FM_HSC_FAULT_N)
        return IOEXP_BUS11_CHIP1;
    if (idx >= GPIO_E1S_1_12VEFUSE_PGOOD && idx <= GPIO_E1S_2_12VEFUSE_PGOOD)
        return IOEXP_BUS11_CHIP2;
    return NULL;
}

static int get_expander_chip_offset(int idx) {
    if (idx < 0 || idx >= (int)MAX_GPIO_EXPANDER_GPIO_PINS)
        return -1;
    if (idx >= GPIO_COMP_PRSNT_N && idx <= GPIO_E1S_2_PRSNT_N)
        return idx - GPIO_COMP_PRSNT_N;
    if (idx >= GPIO_I2C_E1S_1_RST_N && idx <= GPIO_BMC_COMP_YLED)
        return idx - GPIO_I2C_E1S_1_RST_N;
    if (idx >= GPIO_ALRT_P12V_STBY_SCC_N && idx <= GPIO_FM_HSC_FAULT_N)
        return idx - GPIO_ALRT_P12V_STBY_SCC_N;
    if (idx >= GPIO_E1S_1_12VEFUSE_PGOOD && idx <= GPIO_E1S_2_12VEFUSE_PGOOD)
        return idx - GPIO_E1S_1_12VEFUSE_PGOOD;
    return -1;
}
#endif /* CONFIG_GRANDCANYON2 */

int setup_gpio_with_value(const char *chip_name, const char *shadow_name, const char *pin_name, int offset, gpio_direction_t direction, gpio_value_t value)
{
	int ret = 0;
	if (gpio_is_exported(shadow_name) == false) {
		if (strncmp(chip_name, GPIO_CHIP_ASPEED, GPIO_CHIP_MAX) == 0) {
			ret = gpio_export_by_name(GPIO_CHIP_ASPEED, pin_name, shadow_name);
		}
#ifdef CONFIG_GRANDCANYON2
		else {
			/* I2C IO Expander: chip_name is the device name (e.g., "9-0024") */
			ret = gpio_export_by_offset(chip_name, offset, shadow_name);
		}
#else
		else if (strncmp(chip_name, GPIO_CHIP_I2C_IO_EXP, GPIO_CHIP_MAX) == 0) {
			ret = gpio_export_by_offset(GPIO_CHIP_I2C_IO_EXP, offset, shadow_name);
		}
		else {
			printf("failed to recognize chip name: %s\n", chip_name);
			syslog(LOG_ERR, "failed to recognize chip name: %s\n", chip_name);
			return -1;
		}
#endif /* CONFIG_GRANDCANYON2 */

		if (ret != 0) {
#ifdef CONFIG_GRANDCANYON2
			printf("failed to export %s (chip=%s, offset=%d)\n", shadow_name, chip_name, offset);
			syslog(LOG_ERR, "failed to export %s (chip=%s, offset=%d)\n", shadow_name, chip_name, offset);
#else
			printf("failed to export %s\n", shadow_name);
			syslog(LOG_ERR, "failed to export %s\n", shadow_name);
#endif /* CONFIG_GRANDCANYON2 */
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

#ifdef CONFIG_GRANDCANYON2
    if (strncmp(chip_name, GPIO_CHIP_ASPEED, GPIO_CHIP_MAX) == 0) {
        offset = MAX_GPIO_EXPANDER_GPIO_PINS;
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
    } else {
        /*
         * Expander: iterate all entries, skip NULL (e.g., B side IOC_T7_SYS_PGOOD).
         *
         * For each pin, resolve the correct chip device name and chip-relative
         * offset based on the enum index boundaries.
         */
        for (int idx = 0; idx < (int)gpio_expander_gpio_table_size; idx++) {
            if (gpio_config[idx].shadow_name == NULL)
                continue;

            const char *dev_name    = get_expander_chip_name(idx);
            int         chip_offset = get_expander_chip_offset(idx);

            if (dev_name == NULL || chip_offset < 0) {
                printf("failed to map expander idx %d (%s) to chip\n",
                       idx,
                       gpio_config[idx].shadow_name ? gpio_config[idx].shadow_name : "<null>");
                syslog(LOG_ERR, "failed to map expander idx %d (%s) to chip",
                       idx,
                       gpio_config[idx].shadow_name ? gpio_config[idx].shadow_name : "<null>");
                continue;
            }

            ret = setup_gpio_with_value(dev_name,
                gpio_config[idx].shadow_name,
                gpio_config[idx].pin_name,
                chip_offset,
                gpio_config[idx].direction,
                gpio_config[idx].value);

            if (ret != 0) {
                printf("failed to setup %s (chip=%s, offset=%d, idx=%d)\n",
                       gpio_config[idx].shadow_name, dev_name, chip_offset, idx);
                syslog(LOG_ERR, "failed to setup %s (chip=%s, offset=%d, idx=%d)\n",
                       gpio_config[idx].shadow_name, dev_name, chip_offset, idx);
            }
        }
    }

#else /* !CONFIG_GRANDCANYON2 */

	if (strncmp(chip_name, GPIO_CHIP_ASPEED, GPIO_CHIP_MAX) == 0) {
		offset = MAX_GPIO_EXPANDER_GPIO_PINS; // BMC GPIO table index start after Expander GPIO table
	}

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

#endif /* CONFIG_GRANDCANYON2 */
}

int
main(int argc, char **argv) {
	uint32_t reg_value = 0;

	printf("Set up BMC GPIO pins...\n");
	setup_gpios_by_table(GPIO_CHIP_ASPEED, bmc_gpio_table);

#ifdef CONFIG_GRANDCANYON2
    if (fbgc_gpio_init() != 0) {
        syslog(LOG_ERR, "fbgc_gpio_init failed");
        return -1;
    }
#endif /* CONFIG_GRANDCANYON2 */

	printf("Set up GPIO-expander GPIO pins...\n");
	setup_gpios_by_table(GPIO_CHIP_I2C_IO_EXP, gpio_expander_gpio_table);

	/*
		Disable the below BMC GPIO internal pull-down to increase voltage level to get more margin
		1. DEBUG_GPIO_BMC_1/2/3/4/5/6 (GPION0-N5: SCU61C[8:13])
		2. LED_POSTCODE_0/1/2/3/4/5/6/7 (GPIOO7, GPIOP0-P6: SCU61C[23:30])
		3. FM_BMC_TPM_PRSNT_N (GPIOS2: SCU630[18])
		4. BMC_LED_PWR_BTN_EN_R (GPIOV6: SCU634[14])
	*/
	// DEBUG_GPIO_BMC_1/2/3/4/5/6 and LED_POSTCODE_0/1/2/3/4/5/6/7
	phymem_get_dword(SCU_BASE, REG_SCU61C, &reg_value);
	reg_value = reg_value | 0x7F803F00;
	phymem_set_dword(SCU_BASE, REG_SCU61C, reg_value);

	// FM_BMC_TPM_PRSNT_N
	phymem_get_dword(SCU_BASE, REG_SCU630, &reg_value);
	reg_value = reg_value | 0x00040000;
	phymem_set_dword(SCU_BASE, REG_SCU630, reg_value);

	// BMC_LED_PWR_BTN_EN_R
	phymem_get_dword(SCU_BASE, REG_SCU634, &reg_value);
	reg_value = reg_value | 0x00004000;
	phymem_set_dword(SCU_BASE, REG_SCU634, reg_value);

	// Route UART6 to IO6, set bit 8~11 to b'1010
	phymem_get_dword(LPC_BASE, REG_HICR9, &reg_value);
	reg_value = reg_value | 0x00000A00;
	phymem_set_dword(LPC_BASE, REG_HICR9, reg_value);

	printf("done.\n");

	return 0;
}