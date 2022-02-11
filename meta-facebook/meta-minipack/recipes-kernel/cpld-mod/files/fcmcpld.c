/*
 * fcmcpld.c - The i2c driver for FCMCPLD
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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

static ssize_t fcmcpld_fan_rpm_show(struct device *dev,
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

#define FANTRAY_PWM_HELP                        \
  "each value represents 1/64 duty cycle"
#define FANTRAY_LED_CTRL_HELP                   \
  "0x0: Under HW control\n"                     \
  "0x1: Red off, Blue on\n"                     \
  "0x2: Red on, Blue off\n"                     \
  "0x3: Red off, Blue off"
#define FANTRAY_LED_BLINK_HELP                  \
  "0: no blink\n"                               \
  "1: blink"

static const i2c_dev_attr_st fcmcpld_attr_table[] = {
  {
    "board_id",
    "0x0: ZZ\n"
    "0x1: Reserved\n"
    "0x2: Reserved\n"
    "0x3: Reserved",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0, 0, 4,
  },
  {
    "board_ver",
    "0x0: R0A\n"
    "0x1: R0B\n"
    "0x4: R01\n"
    "other: Reserved",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0, 4, 4,
  },
  {
    "cpld_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x01, 0, 6,
  },
  {
    "cpld_released",
    "0: Not released\n"
    "1: Released version after PVT",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x01, 6, 1,
  },
  {
    "cpld_sub_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x02, 0, 8,
  },
  {
    "fan_block_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x06, 0, 8,
  },
  {
    "temp_sensor1",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x07, 0, 1,
  },
  {
    "temp_sensor2",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x07, 1, 1,
  },
  {
    "bmc_wdt_trigger",
    "BMC writes this bit to 1 to clear the count in CPLD\n"
    "If BMC didn’t set this bit over 500S\n"
    "The FANx_PWM register will set to default value (50% duty cycle).\n"
    "This bit will always read back a 0 value.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0a, 0, 1,
  },
  {
    "fan1_input",
    NULL,
    fcmcpld_fan_rpm_show,
    NULL,
    0x20, 0, 8,
  },
  {
    "fan2_input",
    NULL,
    fcmcpld_fan_rpm_show,
    NULL,
    0x21, 0, 8,
  },
  {
    "fantray1_pwm",
    FANTRAY_PWM_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 0, 6,
  },
  {
    "fantray1_power_enable",
    "FAN1 Power Supply Enable\n"
    "0: Disable the fan power\n"
    "1: Enable the fan power",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 7, 1,
  },
  {
    "fantray1_led_ctrl",
    FANTRAY_LED_CTRL_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x24, 0, 2,
  },
  {
    "fantray1_eeprom_wp",
    "Fantray1 EERPOM Write Protect\n"
    "0: Not protect\n"
    "1: Protect",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x25, 0, 1,
  },
  {
    "fantray1_present",
    "FanTray-1 Present\n"
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
    "fan3_input",
    NULL,
    fcmcpld_fan_rpm_show,
    NULL,
    0x30, 0, 8,
  },
  {
    "fan4_input",
    NULL,
    fcmcpld_fan_rpm_show,
    NULL,
    0x31, 0, 8,
  },
  {
    "fantray2_pwm",
    FANTRAY_PWM_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 0, 6,
  },
  {
    "fantray2_power_enable",
    "FAN2 Power Supply Enable\n"
    "0: Disable the fan power\n"
    "1: Enable the fan power",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 7, 1,
  },
  {
    "fantray2_led_ctrl",
    FANTRAY_LED_CTRL_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x34, 0, 2,
  },
  {
    "fantray2_eeprom_wp",
    "Fantray2 EERPOM Write Protect\n"
    "0: Not protect\n"
    "1: Protect",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x35, 0, 1,
  },
  {
    "fantray2_present",
    "FanTray-2 Present\n"
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
    "Rear Fan1 Alive Status\n"
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
    "fan5_input",
    NULL,
    fcmcpld_fan_rpm_show,
    NULL,
    0x40, 0, 8,
  },
  {
    "fan6_input",
    NULL,
    fcmcpld_fan_rpm_show,
    NULL,
    0x41, 0, 8,
  },
  {
    "fantray3_pwm",
    FANTRAY_PWM_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x42, 0, 6,
  },
  {
    "fantray3_power_enable",
    "FAN3 Power Supply Enable\n"
    "0: Disable the fan power\n"
    "1: Enable the fan power",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x42, 7, 1,
  },
  {
    "fantray3_led_ctrl",
    FANTRAY_LED_CTRL_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x44, 0, 2,
  },
  {
    "fantray3_eeprom_wp",
    "Fantray3 EERPOM Write Protect\n"
    "0: Not protect\n"
    "1: Protect",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x45, 0, 1,
  },
  {
    "fantray3_present",
    "FanTray-3 Present\n"
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
    "Front Fan3 Alive Status\n"
    "0: Alive\n"
    "1: Bad",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x48, 3, 1,
  },
  {
    "fan7_input",
    NULL,
    fcmcpld_fan_rpm_show,
    NULL,
    0x50, 0, 8,
  },
  {
    "fan8_input",
    NULL,
    fcmcpld_fan_rpm_show,
    NULL,
    0x51, 0, 8,
  },
  {
    "fantray4_pwm",
    FANTRAY_PWM_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 0, 6,
  },
  {
    "fantray4_power_enable",
    "FAN4 Power Supply Enable\n"
    "0: Disable the fan power\n"
    "1: Enable the fan power",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 7, 1,
  },
  {
    "fantray4_led_ctrl",
    FANTRAY_LED_CTRL_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x54, 0, 2,
  },
  {
    "fantray4_eeprom_wp",
    "Fantray4 EERPOM Write Protect\n"
    "0: Not protect\n"
    "1: Protect",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x55, 0, 1,
  },
  {
    "fantray4_present",
    "FanTray-4 Present\n"
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
    "Front Fan4 Alive Status\n"
    "0: Alive\n"
    "1: Bad",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x58, 3, 1,
  },
};

/*
 * FCMCPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x33, I2C_CLIENT_END
};

/* FCMCPLD id */
static const struct i2c_device_id fcmcpld_id[] = {
  { "fcmcpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, fcmcpld_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int fcmcpld_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the FCMCPLD
   */
  strlcpy(info->type, "fcmcpld", I2C_NAME_SIZE);
  return 0;
}

static int fcmcpld_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(fcmcpld_attr_table) / sizeof(fcmcpld_attr_table[0]);
  struct device *dev = &client->dev;
  i2c_dev_data_st *data;

  data = devm_kzalloc(dev, sizeof(i2c_dev_data_st), GFP_KERNEL);
  if (!data) {
    return -ENOMEM;
  }

  return i2c_dev_sysfs_data_init(client, data,
                                 fcmcpld_attr_table, n_attrs);
}

static int fcmcpld_remove(struct i2c_client *client)
{
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_dev_sysfs_data_clean(client, data);

  return 0;
}

static struct i2c_driver fcmcpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "fcmcpld",
  },
  .probe    = fcmcpld_probe,
  .remove   = fcmcpld_remove,
  .id_table = fcmcpld_id,
  .detect   = fcmcpld_detect,
  .address_list = normal_i2c,
};

static int __init fcmcpld_mod_init(void)
{
  return i2c_add_driver(&fcmcpld_driver);
}

static void __exit fcmcpld_mod_exit(void)
{
  i2c_del_driver(&fcmcpld_driver);
}

MODULE_AUTHOR("Tian Fang <tfang@fb.com>");
MODULE_DESCRIPTION("fcmcpld Driver");
MODULE_LICENSE("GPL");

module_init(fcmcpld_mod_init);
module_exit(fcmcpld_mod_exit);
