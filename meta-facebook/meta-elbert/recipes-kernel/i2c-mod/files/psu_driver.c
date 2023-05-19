/*
 * psu_driver.c - The i2c driver for Elbert 2400W PSU.
 * 
 * Copyright 2021-present Facebook. All Rights Reserved.
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
#include <linux/delay.h>

#include <i2c_dev_sysfs.h>

#ifdef DEBUG
#define PSU_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define PSU_DEBUG(fmt, ...)
#endif

#define POW2(x) (1 << (x))

#define PSU_DELAY 15
#define PMBUS_MFR_MODEL  0x9a
#define VOUT_MODE -9

typedef enum {
  DELTA_2400,
  LITEON_2400,
  UNKNOWN
} model_name;

model_name model;

enum {
  LINEAR_11,
  LINEAR_16
};

/*
 * PMBus Linear-11 Data Format
 * X = Y*2^N
 * X is the "real world" value;
 * Y is an 11 bit, two's complement integer;
 * N is a 5 bit, two's complement integer.
 *
 * PMBus Linear-16 Data Format
 * X = Y*2^N
 * X is the "real world" value;
 * Y is a 16 bit unsigned binary integer;
 * N is a 5 bit, two's complement integer.
 */
static int linear_convert(int type, int value, int n)
{
  int msb_y, msb_n, data_y, data_n;
  int value_x = 0;

  if (type == LINEAR_11) {
    msb_y = (value >> 10) & 0x1;
    data_y = msb_y ? -1 * ((~value & 0x3ff) + 1)
                   : value & 0x3ff;

    if (n != 0) {
      value_x = (n < 0) ? (data_y * 1000) / POW2(abs(n))
                        : (data_y * 1000) * POW2(n);
    } else {
      msb_n = (value >> 15) & 0x1;

      if (msb_n) {
        data_n = (~(value >> 11) & 0xf) + 1;
        value_x = (data_y * 1000) / POW2(data_n);
      } else {
        data_n = ((value >> 11) & 0xf);
        value_x = (data_y * 1000) * POW2(data_n);
      }
    }
  } else {
    if (n != 0) {
      value_x = (n < 0) ? (value * 1000) / POW2(abs(n))
                        : (value * 1000) * POW2(n);
    }
  }

  return value_x;
}

static int psu_convert(struct device *dev, struct device_attribute *attr)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int value = -1;
  int count = 10;
  int ret = -1;
  u8 block[I2C_SMBUS_BLOCK_MAX + 1];

  mutex_lock(&data->idd_lock);

  ret = i2c_smbus_read_block_data(client, PMBUS_MFR_MODEL, block);
  msleep(PSU_DELAY);

  if (ret < 0 || block[0] == 0 || block[0] == '\n') {
    PSU_DEBUG("Failed to read Manufacturer Model\n");
  } else {
    while((value < 0 || value == 0xffff) && count--) {
      value = i2c_smbus_read_word_data(client, (dev_attr->ida_reg));
    }
  }

  mutex_unlock(&data->idd_lock);

  if (value < 0) {
    /* error case */
    PSU_DEBUG("I2C read error, value: %d\n", value);
    return -1;
  }

  if (!strncmp(block, "ECD15020056", 11)) {
    model = DELTA_2400;
  } else if (!strncmp(block, "PS-2242-3A", 10) || !strncmp(block, "PS-2242-9A", 10)) {
    model = LITEON_2400;
  } else {
    model = UNKNOWN;
  }

  return value;
}

static ssize_t psu_vin_show(struct device *dev,
                            struct device_attribute *attr,
                            char *buf)
{
  int result = psu_convert(dev, attr);

  if (result < 0) {
    /* error case */
    return -1;
  }

  switch (model) {
    case DELTA_2400:
    case LITEON_2400:
      result = linear_convert(LINEAR_11, result, 0);
      break;
    default:
      break;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t psu_iin_show(struct device *dev,
                            struct device_attribute *attr,
                            char *buf)
{
  int result = psu_convert(dev, attr);

  if (result < 0) {
    /* error case */
    return -1;
  }

  switch (model) {
    case DELTA_2400:
    case LITEON_2400:
      result = linear_convert(LINEAR_11, result, 0);
      break;
    default:
      break;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t psu_vout_show(struct device *dev,
                             struct device_attribute *attr,
                             char *buf)
{
  int result = psu_convert(dev, attr);

  if (result < 0) {
    /* error case */
    return -1;
  }

  switch (model) {
    case DELTA_2400:
    case LITEON_2400:
      result = linear_convert(LINEAR_16, result, VOUT_MODE);
      break;
    default:
      break;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t psu_iout_show(struct device *dev,
                             struct device_attribute *attr,
                             char *buf)
{
  int result = psu_convert(dev, attr);

  if (result < 0) {
    /* error case */
    return -1;
  }

  switch (model) {
    case DELTA_2400:
    case LITEON_2400:
      result = linear_convert(LINEAR_11, result, 0);
      break;
    default:
      break;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t psu_temp_show(struct device *dev,
                             struct device_attribute *attr,
                             char *buf)
{
  int result = psu_convert(dev, attr);

  if (result < 0) {
    /* error case */
    return -1;
  }

  switch (model) {
    case DELTA_2400:
    case LITEON_2400:
      result = linear_convert(LINEAR_11, result, 0);
      break;
    default:
      break;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t psu_fan_show(struct device *dev,
                            struct device_attribute *attr,
                            char *buf)
{
  int result = psu_convert(dev, attr);

  if (result < 0) {
    /* error case */
    return -1;
  }

  switch (model) {
    case DELTA_2400:
    case LITEON_2400:
      result = linear_convert(LINEAR_11, result, 0) / 1000;
      break;
    default:
      break;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t psu_power_show(struct device *dev,
                              struct device_attribute *attr,
                              char *buf)
{
  int result = psu_convert(dev, attr);

  if (result < 0) {
    /* error case */
    return -1;
  }

  switch (model) {
    case DELTA_2400:
    case LITEON_2400:
      result = linear_convert(LINEAR_11, result, 0);
      break;
    default:
      break;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static const i2c_dev_attr_st psu_attr_table[] = {
  {
    "in0_input",
    NULL,
    psu_vin_show,
    NULL,
    0x88, 0, 8,
  },
  {
    "curr1_input",
    NULL,
    psu_iin_show,
    NULL,
    0x89, 0, 8,
  },
  {
    "in1_input",
    NULL,
    psu_vout_show,
    NULL,
    0x8b, 0, 8,
  },
  {
    "curr2_input",
    NULL,
    psu_iout_show,
    NULL,
    0x8c, 0, 8,
  },
  {
    "temp1_input",
    NULL,
    psu_temp_show,
    NULL,
    0x8d, 0, 8,
  },
  {
    "temp2_input",
    NULL,
    psu_temp_show,
    NULL,
    0x8e, 0, 8,
  },
  {
    "temp3_input",
    NULL,
    psu_temp_show,
    NULL,
    0x8f, 0, 8,
  },
  {
    "fan1_input",
    NULL,
    psu_fan_show,
    NULL,
    0x90, 0, 8,
  },
  {
    "power2_input",
    NULL,
    psu_power_show,
    NULL,
    0x96, 0, 8,
  },
  {
    "power1_input",
    NULL,
    psu_power_show,
    NULL,
    0x97, 0, 8,
  },
};

static i2c_dev_data_st psu_data;

/*
 * psu i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x58, 0x59, I2C_CLIENT_END
};

/* psu_driver id */
static const struct i2c_device_id psu_id[] = {
  {"psu_driver", 0},
  { },
};
MODULE_DEVICE_TABLE(i2c, psu_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int psu_detect(struct i2c_client *client,
                      struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the driver
   */
  strlcpy(info->type, "psu_driver", I2C_NAME_SIZE);
  return 0;
}

static int psu_probe(struct i2c_client *client,
                     const struct i2c_device_id *id)
{
  int n_attrs = sizeof(psu_attr_table) / sizeof(psu_attr_table[0]);

  return i2c_dev_sysfs_data_init(client, &psu_data,
                                 psu_attr_table, n_attrs);
}

static int psu_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &psu_data);
  return 0;
}

static struct i2c_driver psu_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "psu_driver",
  },
  .probe    = psu_probe,
  .remove   = psu_remove,
  .id_table = psu_id,
  .detect   = psu_detect,
  .address_list = normal_i2c,
};

static int __init psu_mod_init(void)
{
  return i2c_add_driver(&psu_driver);
}

static void __exit psu_mod_exit(void)
{
  i2c_del_driver(&psu_driver);
}

MODULE_AUTHOR("Adam Calabrigo");
MODULE_DESCRIPTION("PSU Driver");
MODULE_LICENSE("GPL");

module_init(psu_mod_init);
module_exit(psu_mod_exit);
