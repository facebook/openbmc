/*
 * fancpld.c - The i2c driver for the CPLD on fan board
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

#define present_help_str                        \
  "0: present\n"                                \
  "1: not present"

#define intr_help_str                           \
  "0: no interrupt\n"                           \
  "1: interrupt active"

#define cmm_bmc_hb_help_str                     \
  "1: clears watchdog count in fan cpld\n"      \
  "   (the watchdog times out in 500s)"

#define intr_wc_help_str                        \
  "0: no interrupt\n"                           \
  "1: interrupt happened (write to clear)"

#define wp_help_str                             \
  "0: normal\n"                                 \
  "1: write protect"

#define fantray_pwm_help_str                    \
  "each value represents 1/32 duty cycle"

#define fantray_led_ctrl_help_str               \
  "0x0: under HW control\n"                     \
  "0x1: red off, blue on\n"                     \
  "0x2: red on, blue off\n"                     \
  "0x3: red off, blue off"

#define alive_help_str                          \
  "0x0: alive\n"                                \
  "0x1: bad"

#define mask_help_str                           \
  "0x0: not masked\n"                           \
  "0x1: masked"

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

#define FAN_ENREIS(fan1, fan2, fantray, base)   \
  {                                             \
    fan1 "_input",                              \
    NULL,                                       \
    fancpld_fan_rpm_show,                       \
    NULL,                                       \
    base, 0, 8,                                 \
  },                                            \
  {                                             \
    fan2 "_input",                              \
    NULL,                                       \
    fancpld_fan_rpm_show,                       \
    NULL,                                       \
    base + 1, 0, 8,                             \
  },                                            \
  {                                             \
    fantray "_pwm",                             \
    fantray_pwm_help_str,                       \
    I2C_DEV_ATTR_SHOW_DEFAULT,                  \
    I2C_DEV_ATTR_STORE_DEFAULT,                 \
    base + 2, 0, 5,                             \
  },                                            \
  {                                             \
    fantray "_led_ctrl",                        \
    fantray_led_ctrl_help_str,                  \
    I2C_DEV_ATTR_SHOW_DEFAULT,                  \
    I2C_DEV_ATTR_STORE_DEFAULT,                 \
    base + 4, 0, 2,                             \
  },                                            \
  {                                             \
    fantray "_eeprom_wp",                       \
    wp_help_str,                                \
    I2C_DEV_ATTR_SHOW_DEFAULT,                  \
    I2C_DEV_ATTR_STORE_DEFAULT,                 \
    base + 5, 0, 1,                             \
  },                                            \
  {                                             \
    fantray "_present",                         \
    present_help_str,                           \
    I2C_DEV_ATTR_SHOW_DEFAULT,                  \
    NULL,                                       \
    base + 8, 0, 1,                             \
  },                                            \
  {                                             \
    fantray "_alive",                           \
    alive_help_str,                             \
    I2C_DEV_ATTR_SHOW_DEFAULT,                  \
    NULL,                                       \
    base + 8, 1, 1,                             \
  },                                            \
  {                                             \
    fan2 "_alive",                              \
    alive_help_str,                             \
    I2C_DEV_ATTR_SHOW_DEFAULT,                  \
    NULL,                                       \
    base + 8, 2, 1,                             \
  },                                            \
  {                                             \
    fan1 "_alive",                              \
    alive_help_str,                             \
    I2C_DEV_ATTR_SHOW_DEFAULT,                  \
    NULL,                                       \
    base + 8, 3, 1,                             \
  },                                            \
  {                                             \
    fantray "_present_mask",                    \
    mask_help_str,                              \
    I2C_DEV_ATTR_SHOW_DEFAULT,                  \
    NULL,                                       \
    base + 9, 0, 1,                             \
  },                                            \
  {                                             \
    fantray "_alive_mask",                      \
    mask_help_str,                              \
    I2C_DEV_ATTR_SHOW_DEFAULT,                  \
    NULL,                                       \
    base + 9, 1, 1,                             \
  },                                            \
  {                                             \
    fan2 "_alive_mask",                         \
    mask_help_str,                              \
    I2C_DEV_ATTR_SHOW_DEFAULT,                  \
    NULL,                                       \
    base + 9, 2, 1,                             \
  },                                            \
  {                                             \
    fan1 "_alive_mask",                         \
    mask_help_str,                              \
    I2C_DEV_ATTR_SHOW_DEFAULT,                  \
    NULL,                                       \
    base + 9, 3, 1,                             \
  },                                            \
  {                                             \
    fantray "_present_int",                     \
    intr_wc_help_str,                           \
    I2C_DEV_ATTR_SHOW_DEFAULT,                  \
    I2C_DEV_ATTR_STORE_DEFAULT,                 \
    base + 0xa, 0, 1,                           \
  },                                            \
  {                                             \
    fantray "_alive_int",                       \
    intr_wc_help_str,                           \
    I2C_DEV_ATTR_SHOW_DEFAULT,                  \
    I2C_DEV_ATTR_STORE_DEFAULT,                 \
    NULL,                                       \
    base + 0xa, 1, 1,                           \
  },                                            \
  {                                             \
    fan2 "_alive_int",                          \
    intr_wc_help_str,                           \
    I2C_DEV_ATTR_SHOW_DEFAULT,                  \
    I2C_DEV_ATTR_STORE_DEFAULT,                 \
    base + 0xa, 2, 1,                           \
  },                                            \
  {                                             \
    fan1 "_alive_int",                          \
    intr_wc_help_str,                           \
    I2C_DEV_ATTR_SHOW_DEFAULT,                  \
    I2C_DEV_ATTR_STORE_DEFAULT,                 \
    base + 0xa, 3, 1,                           \
  }

static const i2c_dev_attr_st fancpld_attr_table[] = {
  {
    "pcb_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0, 0, 4,
  },
  {
    "cpld_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    1, 0, 6,
  },
  {
    "release_status",
    "0: not released\n"
    "1: released",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    1, 6, 1,
  },
  {
    "cpld_sub_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    2, 0, 8,
  },
  {
    "fan_block_version",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    6, 0, 8,
  },
  {
    "tmp1_status",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    7, 0, 1,
  },
  {
    "tmp2_status",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    7, 1, 1,
  },
  {
    "fan_int_trig_mod",
    "0x0: fan interrupt triggered by fallng edge\n"
    "0x1: fan interrupt triggered by rising edge\n"
    "0x2: fan interrupt triggered by both rising edge and fallng edge\n"
    "0x3: fan interrupt triggered by low level",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    8, 0, 2,
  },
  {
    "fan1_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    9, 0, 1,
  },
  {
    "fan2_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    9, 1, 1,
  },
  {
    "fan3_int",
    intr_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    9, 2, 1,
  },
  {
    "cmm_bmc_heart_beat",
    cmm_bmc_hb_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xa, 0, 1,
  },
  {
    "fcb_eeprom_wp",
    wp_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0xf, 0, 1,
  },
  FAN_ENREIS("fan1", "fan2", "fantray1", 0x20),
  FAN_ENREIS("fan3", "fan4", "fantray2", 0x30),
  FAN_ENREIS("fan5", "fan6", "fantray3", 0x40),
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

static void fancpld_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &fancpld_data);
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
