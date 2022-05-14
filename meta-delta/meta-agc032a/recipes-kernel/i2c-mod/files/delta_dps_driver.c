/*
 * delta_dps_driver.c - The i2c driver to get following PSU information:
 *
 * DPS-1600AB-29-A/B
 *
 * Copyright 2020-present Delta Eletronics, Inc. All Rights Reserved.
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

typedef enum {
  DELTA_1600AB,
  DELTA_1600_AB,
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
  u8 length, model_chr;

  mutex_lock(&data->idd_lock);

  /*
   * If read block length byte > 32, it will cause kernel panic.
   * Using read word to replace read block to identifer PSU model.
   */
  ret = i2c_smbus_read_word_data(client, PMBUS_MFR_MODEL);
  length = ret & 0xff;

  if (ret < 0 || length > 32) {
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

  model_chr = (ret >> 8) & 0xff;
  if (length == 14 && model_chr == 'D') {
    /* PSU model name: DPS-1600AB-29A */
    /* PSU model name: DPS-1600AB-29B */
    model = DELTA_1600AB;
  } else if (length == 15 && model_chr == 'D') {
    /* PSU model name: DPS-1600AB-29-A */
    /* PSU model name: DPS-1600AB-29-B */
    model = DELTA_1600_AB;
  } else {
    model = UNKNOWN;
  }

  return value;
}

static ssize_t psu_block_info(struct device *dev,
                                struct device_attribute *attr,
                                char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  uint8_t block[I2C_SMBUS_BLOCK_MAX + 1];
  int ret = -1;

  memset(block, 0, sizeof(block));
  mutex_lock(&data->idd_lock);
  ret = i2c_smbus_read_block_data(client, (dev_attr->ida_reg), block);
  mutex_unlock(&data->idd_lock);
  if (ret < 0 || ret > 32) {
    /* error case */
    PSU_DEBUG("I2C read block error, ret: %d\n", ret);
    return -1;
  }

  return scnprintf(buf, PAGE_SIZE, "%s\n", block);;
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
    case DELTA_1600AB:
    case DELTA_1600_AB:
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
    case DELTA_1600AB:
    case DELTA_1600_AB:
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
    case DELTA_1600AB:
    case DELTA_1600_AB:
      result = linear_convert(LINEAR_16, result, -9);
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
    case DELTA_1600AB:
    case DELTA_1600_AB:
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
    case DELTA_1600AB:
    case DELTA_1600_AB:
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
    case DELTA_1600AB:
    case DELTA_1600_AB:
      result = linear_convert(LINEAR_16, result, -9);
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
    case DELTA_1600AB:
    case DELTA_1600_AB:
      result = linear_convert(LINEAR_11, result, 0);
      break;
    default:
      break;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}
#if 0
static ssize_t psu_vstby_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
  int result = psu_convert(dev, attr);

  if (result < 0) {
    /* error case */
    return -1;
  }

  switch (model) {
    case DELTA_1600AB:
    case DELTA_1600_AB:
      result = linear_convert(LINEAR_11, result, 0);
      break;
    default:
      break;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t psu_istby_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
  int result = psu_convert(dev, attr);

  if (result < 0) {
    /* error case */
    return -1;
  }

  switch (model) {
    case DELTA_1600AB:
    case DELTA_1600_AB:
      result = linear_convert(LINEAR_11, result, 0);
      break;
    default:
      break;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t psu_pstby_show(struct device *dev,
                                  struct device_attribute *attr,
                                  char *buf)
{
  int result = psu_convert(dev, attr);

  if (result < 0) {
    /* error case */
    return -1;
  }

  switch (model) {
    case DELTA_1600AB:
    case DELTA_1600_AB:
      result = linear_convert(LINEAR_11, result, 0);
      break;
    default:
      break;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}
#endif
// #1 = in
// #2 = out
// #in = v
// #curr = i
// #power = p

// VOUT_MODE = 0x20,
// READ_VIN = 0x88,
// READ_IIN = 0x89,
// READ_VOUT = 0x8B,
// READ_IOUT = 0x8C,
// READ_TEMP1 = 0x8D,
// READ_TEMP2 = 0x8E,
// READ_FAN_SPD = 0x90,
// READ_POUT = 0x96,
// READ_PIN = 0x97,
// MFR_ID = 0x99,
// MFR_MODEL = 0x9A,
// MFR_REVISION = 0x9B,
// MFR_DATE = 0x9D,
// MFR_SERIAL = 0x9E,
// PRI_FW_VER = 0xDD,
// SEC_FW_VER = 0xD7,
// STATUS_WORD = 0x79,
// STATUS_VOUT = 0x7A,
// STATUS_IOUT = 0x7B,
// STATUS_INPUT = 0x7C,
// STATUS_TEMP = 0x7D,
// STATUS_CML = 0x7E,
// STATUS_FAN = 0x81,
static const i2c_dev_attr_st psu_attr_table[] = {
  {
    "read_vin",
    NULL,
    psu_vin_show,
    NULL,
    0x88, 0, 8,
  },
  {
    "read_iin",
    NULL,
    psu_iin_show,
    NULL,
    0x89, 0, 8,
  },
  {
    "read_vout",
    NULL,
    psu_vout_show,
    NULL,
    0x8b, 0, 8,
  },
  {
    "read_iout",
    NULL,
    psu_iout_show,
    NULL,
    0x8c, 0, 8,
  },
  {
    "read_temp1",
    NULL,
    psu_temp_show,
    NULL,
    0x8d, 0, 8,
  },
  {
    "read_temp2",
    NULL,
    psu_temp_show,
    NULL,
    0x8e, 0, 8,
  },
  {
    "read_fan_spd",
    NULL,
    psu_fan_show,
    NULL,
    0x90, 0, 8,
  },
  {
    "read_pout",
    NULL,
    psu_power_show,
    NULL,
    0x96, 0, 8,
  },
  {
    "read_pin",
    NULL,
    psu_power_show,
    NULL,
    0x97, 0, 8,
  },
  {
    "mfr_id",
    NULL,
    psu_block_info,
    NULL,
    0x99, 0, 0,
  },
  {
    "mfr_model",
    NULL,
    psu_block_info,
    NULL,
    0x9a, 0, 0,
  },
  {
    "mfr_revision",
    NULL,
    psu_block_info,
    NULL,
    0x9b, 0, 0,
  },
  {
    "mfr_date",
    NULL,
    psu_block_info,
    NULL,
    0x9d, 0, 0,
  },
  {
    "mfr_serial",
    NULL,
    psu_block_info,
    NULL,
    0x9e, 0, 0,
  },
};


/*
 * psu i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x58, I2C_CLIENT_END
};

/* dps_driver id */
static const struct i2c_device_id psu_id[] = {
    {"dps_driver", 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, psu_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int psu_detect(struct i2c_client *client,
                         struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the driver
   */
  strlcpy(info->type, "dps_driver", I2C_NAME_SIZE);
  return 0;
}

static int psu_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(psu_attr_table) / sizeof(psu_attr_table[0]);
  struct device *dev = &client->dev;
  i2c_dev_data_st *data;

  data = devm_kzalloc(dev, sizeof(i2c_dev_data_st), GFP_KERNEL);
  if (!data) {
    return -ENOMEM;
  }

  return i2c_dev_sysfs_data_init(client, data,
                                 psu_attr_table, n_attrs);
}

static int psu_remove(struct i2c_client *client)
{
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_dev_sysfs_data_clean(client, data);

  return 0;
}

static struct i2c_driver dps_driver = {
    .class = I2C_CLASS_HWMON,
    .driver =
        {
            .name = "dps_driver",
        },
    .probe = psu_probe,
    .remove = psu_remove,
    .id_table = psu_id,
    .detect = psu_detect,
    .address_list = normal_i2c,
};

static int __init psu_mod_init(void)
{
  return i2c_add_driver(&dps_driver);
}

static void __exit psu_mod_exit(void)
{
  i2c_del_driver(&dps_driver);
}

MODULE_AUTHOR("Leila Lin");
MODULE_DESCRIPTION("Delta DPS PSU Driver");
MODULE_LICENSE("GPL");

module_init(psu_mod_init);
module_exit(psu_mod_exit);
