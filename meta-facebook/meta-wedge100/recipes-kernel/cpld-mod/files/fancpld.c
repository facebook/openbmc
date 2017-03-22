/*
 * fancpld.c - The i2c driver for FANBOARDCPLD
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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

static ssize_t fancpld_fan_rpm_show(struct device *dev,
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
  "each value represents 1/32 duty cycle"
#define FANTRAY_LED_CTRL_HELP                   \
  "0x0: Under HW control\n"                     \
  "0x1: Red off, Blue on\n"                     \
  "0x2: Red on, Blue off\n"                     \
  "0x3: Red off, Blue off"
#define FANTRAY_LED_BLINK_HELP                  \
  "0: no blink\n"                               \
  "1: blink"

static const i2c_dev_attr_st fancpld_attr_table[] = {
  {
    "board_rev",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0, 0, 4,
  },
  {
    "model_id",
    "0x0: wedge100\n"
    "0x1: 6-pack100 linecard\n"
    "0x2: 6-pack100 fabric card\n"
    "0x3: reserved",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0, 4, 2,
  },
  {
    "cpld_rev",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    1, 0, 6,
  },
  {
    "cpld_released",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    1, 6, 1,
  },
  {
    "cpld_sub_rev",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    2, 0, 8,
  },
  {
    "slotid",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    3, 0, 5,
  },
  {
    "jaybox_gpio",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    4, 0, 8,
  },
  {
    "jaybox_status",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    5, 0, 2,
  },
  {
    "fantray_failure",
    "bit value 0: fan tray has failure\n"
    "bit value 1: fan try is good and alive",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    9, 0, 5,
  },
  {
    "fan1_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    0x10, 0, 8,
  },
  {
    "fan2_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    0x11, 0, 8,
  },
  {
    "fan3_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    0x12, 0, 8,
  },
  {
    "fan4_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    0x13, 0, 8,
  },
  {
    "fan5_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    0x14, 0, 8,
  },
  {
    "fan6_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    0x15, 0, 8,
  },
  {
    "fan7_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    0x16, 0, 8,
  },
  {
    "fan8_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    0x17, 0, 8,
  },
  {
    "fan9_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    0x18, 0, 8,
  },
  {
    "fan10_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    0x19, 0, 8,
  },
  {
    "fantray_present",
    "0: present\n"
    "1: not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x1d, 0, 5,
  },
  {
    "fantray1_pwm",
    FANTRAY_PWM_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 0, 5,
  },
  {
    "fantray2_pwm",
    FANTRAY_PWM_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 0, 5,
  },
  {
    "fantray3_pwm",
    FANTRAY_PWM_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 0, 5,
  },
  {
    "fantray4_pwm",
    FANTRAY_PWM_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x23, 0, 5,
  },
  {
    "fantray5_pwm",
    FANTRAY_PWM_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x24, 0, 5,
  },
  {
    "fantray1_led_ctrl",
    FANTRAY_LED_CTRL_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x25, 0, 2,
  },
  {
    "fantray1_led_blink",
    FANTRAY_LED_BLINK_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x25, 2, 1,
  },
  {
    "fantray2_led_ctrl",
    FANTRAY_LED_CTRL_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x25, 4, 2,
  },
  {
    "fantray2_led_blink",
    FANTRAY_LED_BLINK_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x25, 6, 1,
  },
  {
    "fantray3_led_ctrl",
    FANTRAY_LED_CTRL_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x26, 0, 2,
  },
  {
    "fantray3_led_blink",
    FANTRAY_LED_BLINK_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x26, 2, 1,
  },
  {
    "fantray4_led_ctrl",
    FANTRAY_LED_CTRL_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x26, 4, 2,
  },
  {
    "fantray4_led_blink",
    FANTRAY_LED_BLINK_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x26, 6, 1,
  },
  {
    "fantray5_led_ctrl",
    FANTRAY_LED_CTRL_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x27, 0, 2,
  },
  {
    "fantray5_led_blink",
    FANTRAY_LED_BLINK_HELP,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x27, 2, 1,
  },
};

static i2c_dev_data_st fancpld_data;

/*
 * FANCPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x33, I2C_CLIENT_END
};

/* FANCPLD id */
static const struct i2c_device_id fancpld_id[] = {
  { "fancpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, fancpld_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int fancpld_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the FANCPLD
   */
  strlcpy(info->type, "fancpld", I2C_NAME_SIZE);
  return 0;
}

static int fancpld_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(fancpld_attr_table) / sizeof(fancpld_attr_table[0]);
  return i2c_dev_sysfs_data_init(client, &fancpld_data,
                                 fancpld_attr_table, n_attrs);
}

static int fancpld_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &fancpld_data);
  return 0;
}

static struct i2c_driver fancpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "fancpld",
  },
  .probe    = fancpld_probe,
  .remove   = fancpld_remove,
  .id_table = fancpld_id,
  .detect   = fancpld_detect,
  .address_list = normal_i2c,
};

static int __init fancpld_mod_init(void)
{
  return i2c_add_driver(&fancpld_driver);
}

static void __exit fancpld_mod_exit(void)
{
  i2c_del_driver(&fancpld_driver);
}

MODULE_AUTHOR("Tian Fang <tfang@fb.com>");
MODULE_DESCRIPTION("FANCPLD Driver");
MODULE_LICENSE("GPL");

module_init(fancpld_mod_init);
module_exit(fancpld_mod_exit);
