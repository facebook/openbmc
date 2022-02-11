/*
 * pca9548.c - The i2c driver for SYSCPLD
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

/*
static ssize_t pca9548_set_mux(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
  int  err;

  err = i2c_dev_send_byte(dev, attr, (uint8_t)*buf);
  if (err < 0) {
    return err;
  }
  return 0;
}
*/

static const i2c_dev_attr_st pca9548_attr_table[] = {
  {
    "mux",
    NULL,
    NULL,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0, 0, 8,
  },
};

static i2c_dev_data_st pca9548_data;

/*
 * SYSCPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x70, I2C_CLIENT_END
};

/* SYSCPLD id */
static const struct i2c_device_id pca9548_id[] = {
  { "pca9548", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, pca9548_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int pca9548_detect(struct i2c_client *client, int kind,
                          struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the SYSCPLD
   */
  strlcpy(info->type, "pca9548", I2C_NAME_SIZE);
  return 0;
}

static int pca9548_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(pca9548_attr_table) / sizeof(pca9548_attr_table[0]);
  return i2c_dev_sysfs_data_init(client, &pca9548_data,
                                 pca9548_attr_table, n_attrs);

}

static int pca9548_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &pca9548_data);
  return 0;
}

static struct i2c_driver pca9548_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "pca9548",
  },
  .probe    = pca9548_probe,
  .remove   = pca9548_remove,
  .id_table = pca9548_id,
  .detect   = pca9548_detect,
  .address_list = normal_i2c,
};

static int __init pca9548_mod_init(void)
{
  return i2c_add_driver(&pca9548_driver);
}

static void __exit pca9548_mod_exit(void)
{
  i2c_del_driver(&pca9548_driver);
}

MODULE_AUTHOR("Barefoot Networks");
MODULE_DESCRIPTION("PCA9548 Driver");
MODULE_LICENSE("GPL");

module_init(pca9548_mod_init);
module_exit(pca9548_mod_exit);
