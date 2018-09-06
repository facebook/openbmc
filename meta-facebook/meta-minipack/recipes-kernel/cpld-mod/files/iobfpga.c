/*
 * iobfpga.c - The i2c driver for IOBFPGA
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

static ssize_t qsfp_temp_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
  int result = -1;
  int sign = 0;

  result = i2c_dev_read_word_bigendian(dev, attr);

  if (result < 0) {
    /* error case */
    PP_DEBUG("I2C read error, result: %d\n", result);
    return -1;
  }
  sign = (result >> 15) & 0x1;
  result = result & (~(1 << 15));
  result = ((result * 1000) / 256) | (sign << 31);

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static const i2c_dev_attr_st iobfpga_attr_table[] = {
  {
    "board_ver",
    "0x0: R0A\n"
    "0x1: R0B\n"
    "0x2: R0C\n"
    "0x4: R01\n"
    "0x5: R02\n"
    "0x6: R03",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0, 0, 4,
  },
  {
    "fpga_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x01, 0, 6,
  },
  {
    "fpga_released",
    "0x0: Not released\n"
    "0x1: Released version after PVT",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x01, 6, 1,
  },
  {
    "fpga_sub_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x02, 0, 8,
  },
  {
    "led_stream_mode",
    "0x0: N/A\n"
    "0x1: LED_STREAM FBOSS Mode\n"
    "0x2: LED_STREAM Accton Mode\n"
    "0x3: LED_STREAM FBOSS Test Mode\n"
    "0x4: LED_STREAM Accton Test Mode",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x10, 0, 8,
  },
  {
    "temp1_input",
    NULL,
    qsfp_temp_show,
    NULL,
    0x20, 0, 8,
  },
  {
    "temp2_input",
    NULL,
    qsfp_temp_show,
    NULL,
    0x22, 0, 8,
  },
  {
    "temp3_input",
    NULL,
    qsfp_temp_show,
    NULL,
    0x24, 0, 8,
  },
  {
    "temp4_input",
    NULL,
    qsfp_temp_show,
    NULL,
    0x26, 0, 8,
  },
  {
    "temp5_input",
    NULL,
    qsfp_temp_show,
    NULL,
    0x28, 0, 8,
  },
  {
    "temp6_input",
    NULL,
    qsfp_temp_show,
    NULL,
    0x2a, 0, 8,
  },
  {
    "temp7_input",
    NULL,
    qsfp_temp_show,
    NULL,
    0x2c, 0, 8,
  },
  {
    "temp8_input",
    NULL,
    qsfp_temp_show,
    NULL,
    0x2e, 0, 8,
  },
};

static i2c_dev_data_st iobfpga_data;

/*
 * IOBFPGA i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x35, I2C_CLIENT_END
};

/* IOBFPGA id */
static const struct i2c_device_id iobfpga_id[] = {
  { "iobfpga", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, iobfpga_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int iobfpga_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the IOBFPGA
   */
  strlcpy(info->type, "iobfpga", I2C_NAME_SIZE);
  return 0;
}

static int iobfpga_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(iobfpga_attr_table) / sizeof(iobfpga_attr_table[0]);
  return i2c_dev_sysfs_data_init(client, &iobfpga_data,
                                 iobfpga_attr_table, n_attrs);
}

static int iobfpga_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &iobfpga_data);
  return 0;
}

static struct i2c_driver iobfpga_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "iobfpga",
  },
  .probe    = iobfpga_probe,
  .remove   = iobfpga_remove,
  .id_table = iobfpga_id,
  .detect   = iobfpga_detect,
  .address_list = normal_i2c,
};

static int __init iobfpga_mod_init(void)
{
  return i2c_add_driver(&iobfpga_driver);
}

static void __exit iobfpga_mod_exit(void)
{
  i2c_del_driver(&iobfpga_driver);
}

MODULE_AUTHOR("Tian Fang <tfang@fb.com>");
MODULE_DESCRIPTION("IOBFPGA Driver");
MODULE_LICENSE("GPL");

module_init(iobfpga_mod_init);
module_exit(iobfpga_mod_exit);
