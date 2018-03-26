/*
* pfe1100.c - The i2c driver for PFE1100-12-054NA and PFE1500-12-054NAC
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

// #define DEBUG

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/i2c.h>

#include <i2c_dev_sysfs.h>

#ifdef DEBUG
#define PFE1100_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define PFE1100_DEBUG(fmt, ...)
#endif

#define PFE1100_DELAY 10

static int pfe1100_convert(struct device *dev, struct device_attribute *attr)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int value = -1;
  int count = 10;

  mutex_lock(&data->idd_lock);

  while((value < 0 || value == 0xffff) && count--) {
    value = i2c_smbus_read_word_data(client, (dev_attr->ida_reg));
  }

  mutex_unlock(&data->idd_lock);

  if (value < 0) {
    /* error case */
    PFE1100_DEBUG("I2C read error, value: %d\n", value);
    return -1;
  }

  return value;
}

static ssize_t pfe1100_vin_show(struct device *dev,
                                struct device_attribute *attr,
                                char *buf)
{
  int result = -1;
  result = pfe1100_convert(dev, attr);

  if (result < 0) {
    /* error case */
    return -1;
  }

  result = ((result & 0x7ff) * 1000) / 2;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t pfe1100_iin_show(struct device *dev,
                                struct device_attribute *attr,
                                char *buf)
{
  int result = -1;
  result = pfe1100_convert(dev, attr);

  if (result < 0) {
    /* error case */
    return -1;
  }

  result = ((result & 0x7ff) * 1000) / 64;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t pfe1100_vout_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
  int result = -1;
  result = pfe1100_convert(dev, attr);

  if (result < 0) {
    /* error case */
    return -1;
  }

  result = ((result & 0x7ff) * 1000) / 64;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t pfe1100_iout_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
  int result = -1;
  result = pfe1100_convert(dev, attr);

  if (result < 0) {
    /* error case */
    return -1;
  }

  result = ((result & 0x7ff) * 1000) / 8;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t pfe1100_temp_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
  int result = -1;
  result = pfe1100_convert(dev, attr);

  if (result < 0) {
    /* error case */
    return -1;
  }

  result = ((result & 0x7ff) * 1000) / 8;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t pfe1100_fan_show(struct device *dev,
                                struct device_attribute *attr,
                                char *buf)
{
  int result = -1;
  result = pfe1100_convert(dev, attr);

  if (result < 0) {
    /* error case */
    return -1;
  }

  result = (result & 0x7ff) * 32;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t pfe1100_power_show(struct device *dev,
                                  struct device_attribute *attr,
                                  char *buf)
{
  int result = -1;
  result = pfe1100_convert(dev, attr);

  if (result < 0) {
    /* error case */
    return -1;
  }

  result = (result & 0x7ff) * 1000 *1000 * 2;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static const i2c_dev_attr_st pfe1100_attr_table[] = {
  {
    "in0_input",
    NULL,
    pfe1100_vin_show,
    NULL,
    0x88, 0, 8,
  },
  {
    "curr1_input",
    NULL,
    pfe1100_iin_show,
    NULL,
    0x89, 0, 8,
  },
  {
    "in1_input",
    NULL,
    pfe1100_vout_show,
    NULL,
    0x8b, 0, 8,
  },
  {
    "curr2_input",
    NULL,
    pfe1100_iout_show,
    NULL,
    0x8c, 0, 8,
  },
  {
    "temp1_input",
    NULL,
    pfe1100_temp_show,
    NULL,
    0x8d, 0, 8,
  },
  {
    "temp2_input",
    NULL,
    pfe1100_temp_show,
    NULL,
    0x8e, 0, 8,
  },
  {
    "fan1_input",
    NULL,
    pfe1100_fan_show,
    NULL,
    0x90, 0, 8,
  },
  {
    "power2_input",
    NULL,
    pfe1100_power_show,
    NULL,
    0x96, 0, 8,
  },
  {
    "power1_input",
    NULL,
    pfe1100_power_show,
    NULL,
    0x97, 0, 8,
  },
  {
    "in2_input",
    NULL,
    pfe1100_vout_show,
    NULL,
    0xd0, 0, 8,
  },
  {
    "curr3_input",
    NULL,
    pfe1100_iout_show,
    NULL,
    0xd1, 0, 8,
  },
  {
    "power3_input",
    NULL,
    pfe1100_power_show,
    NULL,
    0xd2, 0, 8,
  },
};

static i2c_dev_data_st pfe1100_data;

/*
 * pfe1100 i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x58, 0x59, I2C_CLIENT_END
};


/* pfe1100 id */
static const struct i2c_device_id pfe1100_id[] = {
  {"pfe1100", 0},
  { },
};
MODULE_DEVICE_TABLE(i2c, pfe1100_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int pfe1100_detect(struct i2c_client *client,
                         struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the driver
   */
  strlcpy(info->type, "pfe1100", I2C_NAME_SIZE);
  return 0;
}

static int pfe1100_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(pfe1100_attr_table) / sizeof(pfe1100_attr_table[0]);
  client->flags |= I2C_CLIENT_PEC;

  return i2c_dev_sysfs_data_init(client, &pfe1100_data,
                                 pfe1100_attr_table, n_attrs);
}

static int pfe1100_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &pfe1100_data);
  return 0;
}

static struct i2c_driver pfe1100_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "pfe1100",
  },
  .probe    = pfe1100_probe,
  .remove   = pfe1100_remove,
  .id_table = pfe1100_id,
  .detect   = pfe1100_detect,
  .address_list = normal_i2c,
};

static int __init pfe1100_mod_init(void)
{
  return i2c_add_driver(&pfe1100_driver);
}

static void __exit pfe1100_mod_exit(void)
{
  i2c_del_driver(&pfe1100_driver);
}

MODULE_AUTHOR("Mickey Zhan");
MODULE_DESCRIPTION("pfe1100 Driver");
MODULE_LICENSE("GPL");

module_init(pfe1100_mod_init);
module_exit(pfe1100_mod_exit);
