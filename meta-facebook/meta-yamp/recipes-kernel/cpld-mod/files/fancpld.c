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

#define yamp_bmc_hb_help_str                     \
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

#define YAMP_FAN_RPM_CONSTANT 6000000

static ssize_t fancpld_fan_rpm_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int val;

  val = i2c_dev_read_word_littleendian(dev, attr);
  if (val < 0) {
    return val;
  }

  // Follow the formula from YAMP fan controller manual
  val = YAMP_FAN_RPM_CONSTANT / val;

  return scnprintf(buf, PAGE_SIZE, "%u\n", val);
}

static const i2c_dev_attr_st fancpld_attr_table[] = {
  {
    "cpld_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0, 0, 8,
  },
  {
    "scratch",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x1, 0, 8,
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
    0x12, 0, 8,
  },
  {
    "fan3_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    0x14, 0, 8,
  },
  {
    "fan4_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    0x16, 0, 8,
  },
  {
    "fan5_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    0x18, 0, 8,
  },
  {
    "fan1_pwm",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x30, 0, 8,
  },
  {
    "fan2_pwm",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 0, 8,
  },
  {
    "fan3_pwm",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 0, 8,
  },
  {
    "fan4_pwm",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x33, 0, 8,
  },
  {
    "fan5_pwm",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x34, 0, 8,
  },
  {
    "fan1_id",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x41, 0, 8,
  },
  {
    "fan2_id",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x42, 0, 8,
  },
  {
    "fan3_id",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x43, 0, 8,
  },
  {
    "fan4_id",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x44, 0, 8,
  },
  {
    "fan5_id",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x45, 0, 8,
  },
  {
    "fan1_present",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x49, 0, 1,
  },
  {
    "fan2_present",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x49, 1, 1,
  },
  {
    "fan3_present",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x49, 2, 1,
  },
  {
    "fan4_present",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x49, 3, 1,
  },
  {
    "fan5_present",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x49, 4, 1,
  },
  {
    "fan1_led_green",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4b, 0, 1,
  },
  {
    "fan2_led_green",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4b, 1, 1,
  },
  {
    "fan3_led_green",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4b, 2, 1,
  },
  {
    "fan4_led_green",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4b, 3, 1,
  },
  {
    "fan5_led_green",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4b, 4, 1,
  },
  {
    "fan1_led_red",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4c, 0, 1,
  },
  {
    "fan2_led_red",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4c, 1, 1,
  },
  {
    "fan3_led_red",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4c, 2, 1,
  },
  {
    "fan4_led_red",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4c, 3, 1,
  },
  {
    "fan5_led_red",
    "0x1: On\n"
    "0x0: Off",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x4c, 4, 1,
  },
  {
    "fan1_present_change",
    "0x1: Change latched (W0C)\n"
    "0x0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4f, 0, 1,
  },
  {
    "fan2_present_change",
    "0x1: Change latched (W0C)\n"
    "0x0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4f, 1, 1,
  },
  {
    "fan3_present_change",
    "0x1: Change latched (W0C)\n"
    "0x0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4f, 2, 1,
  },
  {
    "fan4_present_change",
    "0x1: Change latched (W0C)\n"
    "0x0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4f, 3, 1,
  },
  {
    "fan5_present_change",
    "0x1: Change latched (W0C)\n"
    "0x0: No change",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4f, 4, 1,
  },
  {
    "fan_card_overtemp",
    "0x1: Overtemp latched (W0C)\n"
    "0x0: No Overtemp",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x51, 0, 8,
  },
  {
    "fan1_ecb_en",
    "0x1: ECB Enable\n"
    "0x0: ECB Disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 0, 1,
  },
  {
    "fan2_ecb_en",
    "0x1: ECB Enable\n"
    "0x0: ECB Disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 1, 1,
  },
  {
    "fan3_ecb_en",
    "0x1: ECB Enable\n"
    "0x0: ECB Disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 2, 1,
  },
  {
    "fan4_ecb_en",
    "0x1: ECB Enable\n"
    "0x0: ECB Disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 3, 1,
  },
  {
    "fan5_ecb_en",
    "0x1: ECB Enable\n"
    "0x0: ECB Disable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 4, 1,
  },
};

static i2c_dev_data_st fancpld_data;

/*
 * FANCPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x60, I2C_CLIENT_END
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

MODULE_AUTHOR("Mike Choi <mikechoi@fb.com>");
MODULE_DESCRIPTION("FANCPLD Driver");
MODULE_LICENSE("GPL");

module_init(fancpld_mod_init);
module_exit(fancpld_mod_exit);
