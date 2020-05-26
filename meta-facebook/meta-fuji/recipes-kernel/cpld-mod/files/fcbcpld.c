/*
 * fcbcpld.c - The i2c driver for FCBCPLD
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

//#define DEBUG

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <i2c_dev_sysfs.h>

#ifdef DEBUG

#define PP_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define PP_DEBUG(fmt, ...)

#endif

static ssize_t fcbcpld_fan_rpm_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int val;

  val = i2c_dev_read_byte(dev, attr);
  if (val < 0) {
    return val;
  }
  /* Multiply by 150 to get the RPM */
  val *= 150;

  return scnprintf(buf, PAGE_SIZE, "%u\n", val);
}

#define fan_pwm_help_str                        \
  "each value represents 1/64 duty cycle"
  
#define fan_led_ctrl_help_str                   \
  "0x0: Under HW control\n"                     \
  "0x1: Red off, Blue on\n"                     \
  "0x2: Red on, Blue off\n"                     \
  "0x3: Red off, Blue off"

#define fan_led_blink_help_str                  \
  "0: no blink\n"                               \
  "1: blink"

#define intr_help_str                           \
  "0: Interrupt active\n"                       \
  "1: No interrupt"
  
#define cpld_cpu_blk_help_str                   \
  "0: CPLD passes the interrupt to CPU\n"       \
  "1: CPLD blocks incoming the interrupt"
	  
#define enable_help_str                         \
  "0: Disable\n"                                \
  "1: Enable"

static const i2c_dev_attr_st fcbcpld_attr_table[] = {
  {
    "board_id",
    "0x0: Minipack2 FCB board\n"
    "other: Reserved",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0, 0, 2,
  },
  {
    "board_ver",
    "0x0: R0A\n"
    "other: Reserved",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0, 4, 3,
  },
  {
    "cpld_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x01, 0, 8,
  },
  {
    "cpld_minor_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x02, 0, 8,
  },
  {
    "cpld_sub_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x03, 0, 8,
  },
  {
    "software_scratch",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x04, 0, 8,
  },
  {
    "fan1_int",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x05, 0, 1,
  },
  {
    "fan2_int",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x05, 1, 1,
  },
  {
    "fan3_int",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x05, 2, 1,
  },
  {
    "fan4_int",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x05, 3, 1,
  },
  {
    "lm75_int_1",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x05, 4, 1,
  },
  {
    "lm75_int_2",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x05, 5, 1,
  },
  {
    "efuse_int",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x05, 6, 1,
  },
  {
    "hs_int",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x05, 7, 1,
  },
  {
    "lm75_1",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x07, 0, 1,
  },
  {
    "lm75_2",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x07, 1, 1,
  },
  {
    "lm75_1_mask",
    "0: Not mask\n"
    "1: Mask",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x08, 0, 1,
  },
  {
    "lm75_2_mask",
    "0: Not mask\n"
    "1: Mask",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x08, 1, 1,
  },
  {
    "fcb_eeprom_wp",
    "0: Not protect\n"
    "1: Protect",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0f, 0, 1,
  },
  {
    "fan1_enable_reg",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 0, 1,
  },
  {
    "fan2_enable_reg",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 1, 1,
  },
  {
    "fan3_enable_reg",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 2, 1,
  },
  {
    "fan4_enable_reg",
    enable_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 3, 1,
  },
  {
    "hotswap_pg",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 0, 1,
  },
  {
    "hs_alert1",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 1, 1,
  },
  {
    "hs_alert2",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 2, 1,
  },
  {
    "hs_fault",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x11, 3, 1,
  },
  {
    "hotswap_pg_mask",
    "0: Not mask\n"
    "1: Mask",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 0, 1,
  },
  {
    "hs_alert1_mask",
    "0: Not mask\n"
    "1: Mask",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 1, 1,
  },
  {
    "hs_alert_2_mask",
    "0: Not mask\n"
    "1: Mask",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 2, 1,
  },
  {
    "hs_fault_mask",
    "0: Not mask\n"
    "1: Mask",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x12, 3, 1,
  },
  {
    "fltb_fan1",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x13, 0, 1,
  },
  {
    "fltb_fan2",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x13, 1, 1,
  },
  {
    "fltb_fan3",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x13, 2, 1,
  },
  {
    "ffltb_fan4",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x13, 3, 1,
  },
  {
    "pg_fan1",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x13, 4, 1,
  },
  {
    "pg_fan2",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x13, 5, 1,
  },
  {
    "pg_fan3",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x13, 6, 1,
  },
  {
    "pg_fan4",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x13, 7, 1,
  },
  {
    "fltb_fan1_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x14, 0, 1,
  },
  {
    "fltb_fan2_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x14, 1, 1,
  },
  {
    "fltb_fan3_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x14, 2, 1,
  },
  {
    "fltb_fan4_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x14, 3, 1,
  },
  {
    "pg_fan1_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x14, 4, 1,
  },
  {
    "pg_fan2_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x14, 5, 1,
  },
  {
    "pg_fan3_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x14, 6, 1,
  },
  {
    "pg_fan4_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x14, 7, 1,
  },
  {
    "fan1_input",
    NULL,
    fcbcpld_fan_rpm_show,
    NULL,
    0x20, 0, 8,
  },
  {
    "fan2_input",
    NULL,
    fcbcpld_fan_rpm_show,
    NULL,
    0x21, 0, 8,
  },
  {
    "fan1_pwm",
    fan_pwm_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 0, 8,
  },
  {
    "fan1_led_ctrl",
    fan_led_ctrl_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x24, 0, 2,
  },
  {
    "fan1_eeprom_wp",
    "Fan1 EERPOM Write Protect\n"
    "0: Not protect\n"
    "1: Protect",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x25, 0, 1,
  },
  {
    "fan1_present",
    "Fan-1 Present\n"
    "0: Present\n"
    "1: Not Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x28, 0, 1,
  },
  {
    "fan1_alive",
    "Fan1 Alive Status\n"
    "0: Alive\n"
    "1: Bad",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x28, 1, 1,
  },
  {
    "fan1_rear_alive",
    "Rear Fan1 Alive Status\n"
    "0: Alive\n"
    "1: Bad",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x28, 2, 1,
  },
  {
    "fan1_front_alive",
    "Front Fan1 Alive Status\n"
    "0: Alive\n"
    "1: Bad",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x28, 3, 1,
  },
  {
    "fan1_present_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x29, 0, 1,
  },
  {
    "fan1_alive_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x29, 1, 1,
  },
  {
    "fan1_rear_alive_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x29, 2, 1,
  },
  {
    "fan1_front_alive_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x29, 3, 1,
  },
  {
    "fan3_input",
    NULL,
    fcbcpld_fan_rpm_show,
    NULL,
    0x30, 0, 8,
  },
  {
    "fan4_input",
    NULL,
    fcbcpld_fan_rpm_show,
    NULL,
    0x31, 0, 8,
  },
  {
    "fan2_pwm",
    fan_pwm_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 0, 8,
  },
  {
    "fan2_led_ctrl",
    fan_led_ctrl_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x34, 0, 2,
  },
  {
    "fan2_eeprom_wp",
    "Fan2 EERPOM Write Protect\n"
    "0: Not protect\n"
    "1: Protect",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x35, 0, 1,
  },
  {
    "fan2_present",
    "Fan-2 Present\n"
    "0: Present\n"
    "1: Not Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x38, 0, 1,
  },
  {
    "fan2_alive",
    "Fan2 Alive Status\n"
    "0: Alive\n"
    "1: Bad",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x38, 1, 1,
  },
  {
    "fan2_rear_alive",
    "Rear Fan2 Alive Status\n"
    "0: Alive\n"
    "1: Bad",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x38, 2, 1,
  },
  {
    "fan2_front_alive",
    "Front Fan2 Alive Status\n"
    "0: Alive\n"
    "1: Bad",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x38, 3, 1,
  },
  {
    "fan2_present_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 0, 1,
  },
  {
    "fan2_alive_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 1, 1,
  },
  {
    "fan2_rear_alive_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 2, 1,
  },
  {
    "fan2_front_alive_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x39, 3, 1,
  },
  {
    "fan5_input",
    NULL,
    fcbcpld_fan_rpm_show,
    NULL,
    0x40, 0, 8,
  },
  {
    "fan6_input",
    NULL,
    fcbcpld_fan_rpm_show,
    NULL,
    0x41, 0, 8,
  },
  {
    "fan3_pwm",
    fan_pwm_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x42, 0, 8,
  },
  {
    "fan3_led_ctrl",
    fan_led_ctrl_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x44, 0, 2,
  },
  {
    "fan3_eeprom_wp",
    "Fan3 EERPOM Write Protect\n"
    "0: Not protect\n"
    "1: Protect",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x45, 0, 1,
  },
  {
    "fan3_present",
    "Fan-3 Present\n"
    "0: Present\n"
    "1: Not Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x48, 0, 1,
  },
  {
    "fan3_alive",
    "Fan3 Alive Status\n"
    "0: Alive\n"
    "1: Bad",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x48, 1, 1,
  },
  {
    "fan3_rear_alive",
    "Rear Fan3 Alive Status\n"
    "0: Alive\n"
    "1: Bad",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x48, 2, 1,
  },
  {
    "fan3_front_alive",
    "Front Fan1 Alive Status\n"
    "0: Alive\n"
    "1: Bad",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x48, 3, 1,
  },
  {
    "fan3_present_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x49, 0, 1,
  },
  {
    "fan3_alive_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x49, 1, 1,
  },
  {
    "fan3_rear_alive_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x49, 2, 1,
  },
  {
    "fan3_front_alive_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x49, 3, 1,
  },
  {
    "fan7_input",
    NULL,
    fcbcpld_fan_rpm_show,
    NULL,
    0x50, 0, 8,
  },
  {
    "fan8_input",
    NULL,
    fcbcpld_fan_rpm_show,
    NULL,
    0x51, 0, 8,
  },
  {
    "fan4_pwm",
    fan_pwm_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 0, 8,
  },
  {
    "fan4_led_ctrl",
    fan_led_ctrl_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x54, 0, 2,
  },
  {
    "fan4_eeprom_wp",
    "Fan4 EERPOM Write Protect\n"
    "0: Not protect\n"
    "1: Protect",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x55, 0, 1,
  },
  {
    "fan4_present",
    "Fan-4 Present\n"
    "0: Present\n"
    "1: Not Present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x58, 0, 1,
  },
  {
    "fan4_alive",
    "Fan4 Alive Status\n"
    "0: Alive\n"
    "1: Bad",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x58, 1, 1,
  },
  {
    "fan4_rear_alive",
    "Rear Fan4 Alive Status\n"
    "0: Alive\n"
    "1: Bad",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x58, 2, 1,
  },
  {
    "fan4_front_alive",
    "Front Fan5 Alive Status\n"
    "0: Alive\n"
    "1: Bad",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x58, 3, 1,
  },
  {
    "fan4_present_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x59, 0, 1,
  },
  {
    "fan4_alive_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x59, 1, 1,
  },
  {
    "fan4_rear_alive_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x59, 2, 1,
  },
  {
    "fan4_front_alive_int_mask",
    cpld_cpu_blk_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x59, 3, 1,
  },
  {
    "upgrade_run",
    "0: No upgrade\n"
    "1: Start the upgrade",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa0, 0, 1,
  },
  {
    "upgrade_data",
    "UPGRADE DATA",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa1, 0, 8,
  },
  {
    "almost_fifo_empty",
    "0: No upgrade  data fifo almost empty\n"
    "1: Upgrade data fifo almost empty",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa2, 0, 1,
  },
  {
    "fifo_full",
    "0: No upgrade  data fifo full\n"
    "1: Upgrade data fifo full",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa2, 1, 1,
  },
  {
    "fifo_empty",
    "0: No upgrade  data fifo empty\n"
    "1: Upgrade data fifo empty",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa2, 2, 1,
  },
  {
    "almost_fifo_full",
    "0: No upgrade  data fifo almost full\n"
    "1: Upgrade data fifo almost full",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa2, 3, 1,
  },
  {
    "done",
    "0: No upgrade done\n"
    "1: Upgrade done",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa2, 4, 1,
  },
  {
    "error",
    "0: No upgrade error\n"
    "1: Upgrade error",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa2, 5, 1,
  },
  {
    "st_pgm_done_flag",
    "0: FSM No upgrade done\n"
    "1: FSM Upgrade done",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa2, 6, 1,
  },
  {
    "upgrade_refresh_r",
    "0: No update\n"
    "1: Start the upgrade refresh",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa3, 0, 1,
  }
};

static i2c_dev_data_st fcbcpld_data;

/*
 * FCBCPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x33, I2C_CLIENT_END
};

/* FCBCPLD id */
static const struct i2c_device_id fcbcpld_id[] = {
  { "fcbcpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, fcbcpld_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int fcbcpld_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the FCBCPLD
   */
  strlcpy(info->type, "fcbcpld", I2C_NAME_SIZE);
  return 0;
}

static int fcbcpld_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(fcbcpld_attr_table) / sizeof(fcbcpld_attr_table[0]);
  return i2c_dev_sysfs_data_init(client, &fcbcpld_data,
                                 fcbcpld_attr_table, n_attrs);
}

static int fcbcpld_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &fcbcpld_data);
  return 0;
}

static struct i2c_driver fcbcpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "fcbcpld",
  },
  .probe    = fcbcpld_probe,
  .remove   = fcbcpld_remove,
  .id_table = fcbcpld_id,
  .detect   = fcbcpld_detect,
  .address_list = normal_i2c,
};

static int __init fcbcpld_mod_init(void)
{
  return i2c_add_driver(&fcbcpld_driver);
}

static void __exit fcbcpld_mod_exit(void)
{
  i2c_del_driver(&fcbcpld_driver);
}

MODULE_AUTHOR("Xiaohua Wang");
MODULE_DESCRIPTION("fcbcpld Driver");
MODULE_LICENSE("GPL");

module_init(fcbcpld_mod_init);
module_exit(fcbcpld_mod_exit);
