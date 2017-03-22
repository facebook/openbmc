/*
 * galaxy100_ec.c - The i2c driver for EC
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
#include <linux/delay.h>

#include <i2c_dev_sysfs.h>

#ifdef DEBUG
#define EC_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define EC_DEBUG(fmt, ...)
#endif

#define EC_MAC_LEN 6
#define EC_SERIAL_NUM 32
#define EC_PRODUCT_NAME_LEN 4
#define EC_CUST_NAME_LEN 3

#define EC_DELAY 11 //ms

static int i2c_smbus_read_byte_data_retry(struct i2c_client *client, unsigned char reg)
{
	int count = 10;
	int ret = -1;

	while((ret < 0 || ret == 0xff) && count--) {
		ret = i2c_smbus_read_byte_data(client, reg);
	}

	return ret;
}
static ssize_t ec_cpu_temp_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int val;

  mutex_lock(&data->idd_lock);

  val = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg);
  mutex_unlock(&data->idd_lock);

  if (val < 0) {
    /* error case */
    EC_DEBUG("I2C read error!\n");
    return -1;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", val * 10);
}

static ssize_t ec_mem_temp_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int lsb_val, msb_val;
  int result;

  mutex_lock(&data->idd_lock);

  msb_val = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg);
  msleep(EC_DELAY);
  lsb_val = i2c_smbus_read_byte_data_retry(client, (dev_attr->ida_reg + 0x1));
  mutex_unlock(&data->idd_lock);

  if (lsb_val < 0 || msb_val < 0) {
    /* error case */
	EC_DEBUG("I2C read error!\n");
    return -1;
  }

  result = ((((msb_val << 8) + lsb_val) >> 4) & 0xff) * 10 + (((lsb_val >> 1) & 0x07) * 10)/ 8;

//return scnprintf(buf, PAGE_SIZE, "%d.%d C\n", result / 8, (((result % 8) * 10) / 8));
  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t ec_wdt_cfg_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int val;

  mutex_lock(&data->idd_lock);
  val = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg);
  mutex_unlock(&data->idd_lock);

  if (val < 0) {
    /* error case */
	EC_DEBUG("I2C read error!\n");
    return -1;
  }

  return scnprintf(buf, PAGE_SIZE, "0x%x\n", val);
}

static ssize_t ec_wdt_crm_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int val;

  mutex_lock(&data->idd_lock);
  val = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg);
  mutex_unlock(&data->idd_lock);

  if (val < 0) {
    /* error case */
	EC_DEBUG("I2C read error!\n");
    return -1;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t ec_wdt_crs_show(struct device *dev,
									struct device_attribute *attr,
									char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int val;

  mutex_lock(&data->idd_lock);
  val = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg);
  mutex_unlock(&data->idd_lock);

  if (val < 0) {
	/* error case */
	EC_DEBUG("I2C read error!\n");
	return -1;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t ec_hw_monitor_cfg_show(struct device *dev,
									struct device_attribute *attr,
									char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int val;

  mutex_lock(&data->idd_lock);
  val = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg);
  mutex_unlock(&data->idd_lock);

  if (val < 0) {
	/* error case */
	EC_DEBUG("I2C read error!\n");
	return -1;
  }

  return scnprintf(buf, PAGE_SIZE, "0x%x\n", val);
}

static ssize_t ec_version_show(struct device *dev,
									struct device_attribute *attr,
									char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int val_r, val_e, val_t;
  int year, month, day;

  mutex_lock(&data->idd_lock);
  val_r = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg);
  msleep(EC_DELAY);
  val_e = i2c_smbus_read_byte_data_retry(client, (dev_attr->ida_reg + 0x1));
  msleep(EC_DELAY);
  val_t = i2c_smbus_read_byte_data_retry(client, (dev_attr->ida_reg + 0x2));
  mutex_unlock(&data->idd_lock);

  if (val_r < 0 || val_e < 0 || val_t < 0) {
	/* error case */
	EC_DEBUG("I2C read error!\n");
	return -1;
  }
  if(val_t >> 7) {
	mutex_lock(&data->idd_lock);
	year = i2c_smbus_read_byte_data_retry(client, 0x2d);
	year &= 0x0f;
	msleep(EC_DELAY);
	month = i2c_smbus_read_byte_data_retry(client, 0x2e);
	msleep(EC_DELAY);
	day = i2c_smbus_read_byte_data_retry(client, 0x2f);
	mutex_unlock(&data->idd_lock);
	return scnprintf(buf, PAGE_SIZE, "%d%02x%02xT%02x\n", year, month, day, val_t & 0x0f);
  } else {
	return scnprintf(buf, PAGE_SIZE, "V%02dE%02d\n", val_r, val_e);
  }
}

static ssize_t ec_gpio_dir_show(struct device *dev,
									struct device_attribute *attr,
									char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int val;

  mutex_lock(&data->idd_lock);
  val = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg);
  mutex_unlock(&data->idd_lock);

  if (val < 0) {
	/* error case */
	EC_DEBUG("I2C read error!\n");
	return -1;
  }

  return scnprintf(buf, PAGE_SIZE, "0x%x\n", val);
}

static ssize_t ec_gpio_data_show(struct device *dev,
									struct device_attribute *attr,
									char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int val;

  mutex_lock(&data->idd_lock);
  val = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg);
  mutex_unlock(&data->idd_lock);

  if (val < 0) {
	/* error case */
	EC_DEBUG("I2C read error!\n");
	return -1;
  }

  return scnprintf(buf, PAGE_SIZE, "0x%x\n", val);
}

static ssize_t ec_build_date_show(struct device *dev,
									struct device_attribute *attr,
									char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int year, month, day;

  mutex_lock(&data->idd_lock);
  year = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg);
  if (year < 0) {
	/* error case */
	EC_DEBUG("I2C read 0x%x error!\n", dev_attr->ida_reg);
	return -1;
  }
  msleep(EC_DELAY);
  month = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg + 1);
  if (month < 0) {
	/* error case */
	EC_DEBUG("I2C read 0x%x error!\n", dev_attr->ida_reg + 1);
	return -1;
  }
  msleep(EC_DELAY);
  day = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg + 2);
  if (day < 0) {
	/* error case */
	EC_DEBUG("I2C read 0x%x error!\n", dev_attr->ida_reg + 2);
	return -1;
  }

  mutex_unlock(&data->idd_lock);

  return scnprintf(buf, PAGE_SIZE, "20%x-%x-%x\n", year, month, day);
}

static ssize_t ec_cpu_vol_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int lsb_val, msb_val;
  int result;

  mutex_lock(&data->idd_lock);

  lsb_val = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg);
  msleep(EC_DELAY);
  msb_val = i2c_smbus_read_byte_data_retry(client, (dev_attr->ida_reg + 0x1));
  mutex_unlock(&data->idd_lock);

  if (lsb_val < 0 || msb_val < 0) {
    /* error case */
	EC_DEBUG("I2C read error!\n");
    return -1;
  }

  result = (msb_val << 8) + lsb_val;

  //scnprintf(buf, PAGE_SIZE, "%d.%d V\n", result / 341, (((result % 341) * 10) / 341));
  scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t ec_3v_vol_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int lsb_val, msb_val;
  int result;

  mutex_lock(&data->idd_lock);

  lsb_val = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg);
  msleep(EC_DELAY);
  msb_val = i2c_smbus_read_byte_data_retry(client, (dev_attr->ida_reg + 0x1));
  mutex_unlock(&data->idd_lock);

  if (lsb_val < 0 || msb_val < 0) {
    /* error case */
	EC_DEBUG("I2C read error!\n");
    return -1;
  }

  result = ((msb_val << 8) + lsb_val) * 2;

  //scnprintf(buf, PAGE_SIZE, "%d.%d V\n", result / 341, (((result % 341) * 10) / 341));
  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t ec_5v_vol_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int lsb_val, msb_val;
  int result;

  mutex_lock(&data->idd_lock);

  lsb_val = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg);
  msleep(EC_DELAY);
  msb_val = i2c_smbus_read_byte_data_retry(client, (dev_attr->ida_reg + 0x1));
  mutex_unlock(&data->idd_lock);

  if (lsb_val < 0 || msb_val < 0) {
    /* error case */
	EC_DEBUG("I2C read error!\n");
    return -1;
  }

  result = ((msb_val << 8) + lsb_val) * 16;

  //scnprintf(buf, PAGE_SIZE, "%d.%d V\n", result / 1705, (((result % 1705) * 10) / 1705));
  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t ec_12v_vol_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int lsb_val, msb_val;
  int result;

  mutex_lock(&data->idd_lock);

  lsb_val = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg);
  msleep(EC_DELAY);
  msb_val = i2c_smbus_read_byte_data_retry(client, (dev_attr->ida_reg + 0x1));
  mutex_unlock(&data->idd_lock);

  if (lsb_val < 0 || msb_val < 0) {
    /* error case */
	EC_DEBUG("I2C read error!\n");
    return -1;
  }

  result = ((msb_val << 8) + lsb_val) * 33;

  //scnprintf(buf, PAGE_SIZE, "%d.%d V\n", result / 1705, (((result % 1705) * 10) / 1705));
  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t ec_dimm_vol_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int lsb_val, msb_val;
  int result;

  mutex_lock(&data->idd_lock);

  lsb_val = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg);
  msleep(EC_DELAY);
  msb_val = i2c_smbus_read_byte_data_retry(client, (dev_attr->ida_reg + 0x1));
  mutex_unlock(&data->idd_lock);

  if (lsb_val < 0 || msb_val < 0) {
    /* error case */
	EC_DEBUG("I2C read error!\n");
    return -1;
  }

  result = (msb_val << 8) + lsb_val;

  //scnprintf(buf, PAGE_SIZE, "%d.%d V\n", result / 341, (((result % 341) * 10) / 341));
  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t ec_product_name_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int result, i, len = EC_PRODUCT_NAME_LEN;
  unsigned char product_name[EC_PRODUCT_NAME_LEN];

  mutex_lock(&data->idd_lock);
  for(i = 0; i < len; i++) {
	result = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg + i);
	if (result < 0) {
	  /* error case */
	  EC_DEBUG("I2C read error!\n");
	  return -1;
	}
	product_name[i] = (unsigned char)result;
	msleep(EC_DELAY);
  }
  mutex_unlock(&data->idd_lock);

  return scnprintf(buf, PAGE_SIZE, "%c%c%c%c\n",
	product_name[0], product_name[1], product_name[2], product_name[3]);
}

static ssize_t ec_customize_name_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int result, i, len = EC_CUST_NAME_LEN;
  unsigned char cust_name[EC_CUST_NAME_LEN];

  mutex_lock(&data->idd_lock);
  for(i = 0; i < len; i++) {
	result = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg + i);
	if (result < 0) {
	  /* error case */
	  EC_DEBUG("I2C read error!\n");
	  return -1;
	}
	cust_name[i] = (unsigned char)result;
	msleep(EC_DELAY);
  }
  mutex_unlock(&data->idd_lock);

  return scnprintf(buf, PAGE_SIZE, "%s\n", cust_name);
}

static ssize_t ec_mac_addr_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int result, i, len = EC_MAC_LEN;
  unsigned char mac[EC_MAC_LEN];

  mutex_lock(&data->idd_lock);
  for(i = 0; i < len; i++) {
    result = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg + i);
    if (result < 0) {
      /* error case */
	  EC_DEBUG("I2C read error!\n");
      return -1;
    }
    mac[i] = (unsigned char)result;
    msleep(EC_DELAY);
  }
  mutex_unlock(&data->idd_lock);

  return scnprintf(buf, PAGE_SIZE, "%02x:%02x:%02x:%02x:%02x:%02x\n",
	mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static ssize_t ec_serial_number_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int result, i, len = EC_SERIAL_NUM;
  unsigned char serial[EC_SERIAL_NUM];

  mutex_lock(&data->idd_lock);
  for(i = 0; i < len; i++) {
	  result = i2c_smbus_read_byte_data_retry(client, dev_attr->ida_reg + i);
	  if (result < 0) {
	    /* error case */
		EC_DEBUG("I2C read error!\n");
	    return -1;
	  }
	  serial[i] = (unsigned char)result;
	  //msleep(EC_DELAY / 4);
  }

  mutex_unlock(&data->idd_lock);

  return scnprintf(buf, PAGE_SIZE, "%s\n", serial);
}

static const i2c_dev_attr_st ec_attr_table[] = {
  {
    "temp1_input",
    NULL,
    ec_cpu_temp_show,
    NULL,
    0, 0, 8,
  },
  {
    "temp2_input",
    NULL,
    ec_mem_temp_show,
    NULL,
    0x4, 0, 8,
  },
  {
    "wdt_cfg",
    NULL,
    ec_wdt_cfg_show,
    NULL,
    0x6, 0, 8,
  },
  {
    "wdt_crm",
    NULL,
    ec_wdt_crm_show,
    NULL,
    0x7, 0, 8,
  },
  {
    "wdt_crs",
    NULL,
    ec_wdt_crs_show,
    NULL,
    0x8, 0, 8,
  },
  {
    "hw_monitor_cfg",
    NULL,
    ec_hw_monitor_cfg_show,
    NULL,
    0x15, 0, 8,
  },
  {
    "version",
    NULL,
    ec_version_show,
    NULL,
    0x1d, 0, 8,
  },
  {
    "in0_input",
    NULL,
    ec_cpu_vol_show,
    NULL,
    0x20, 0, 8,
  },
  {
    "in1_input",
    NULL,
    ec_3v_vol_show,
    NULL,
    0x22, 0, 8,
  },
  {
    "in2_input",
    NULL,
    ec_5v_vol_show,
    NULL,
    0x24, 0, 8,
  },
  {
    "gpio_dir",
    NULL,
    ec_gpio_dir_show,
    NULL,
    0x2b, 0, 8,
  },
  {
    "gpio_data",
    NULL,
    ec_gpio_data_show,
    NULL,
    0x2c, 0, 8,
  },
  {
    "build_date_input",
    NULL,
    ec_build_date_show,
    NULL,
    0x2d, 0, 8,
  },
  {
    "in3_input",
    NULL,
    ec_12v_vol_show,
    NULL,
    0x30, 0, 8,
  },
  {
    "in4_input",
    NULL,
    ec_dimm_vol_show,
    NULL,
    0x32, 0, 8,
  },
  {
    "product_name",
    NULL,
    ec_product_name_show,
    NULL,
    0x3c, 0, 8,
  },
  {
    "customize_name_input",
    NULL,
    ec_customize_name_show,
    NULL,
    0x4d, 0, 8,
  },
  {
    "mac_addr",
    NULL,
    ec_mac_addr_show,
    NULL,
    0x50, 0, 8,
  },
  {
    "serial_number",
    NULL,
    ec_serial_number_show,
    NULL,
    0x60, 0, 8,
  },
};

static i2c_dev_data_st ec_data;

/*
 * EC i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x33, I2C_CLIENT_END
};

/* EC id */
static const struct i2c_device_id ec_id[] = {
  {"galaxy100_ec", 0},
  { },
};
MODULE_DEVICE_TABLE(i2c, ec_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int ec_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the EC
   */
  strlcpy(info->type, "galaxy100_ec", I2C_NAME_SIZE);
  return 0;
}

static int ec_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(ec_attr_table) / sizeof(ec_attr_table[0]);
  return i2c_dev_sysfs_data_init(client, &ec_data,
                                 ec_attr_table, n_attrs);
}

static int ec_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &ec_data);
  return 0;
}

static struct i2c_driver ec_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "galaxy100_ec",
  },
  .probe    = ec_probe,
  .remove   = ec_remove,
  .id_table = ec_id,
  .detect   = ec_detect,
  .address_list = normal_i2c,
};

static int __init ec_mod_init(void)
{
  return i2c_add_driver(&ec_driver);
}

static void __exit ec_mod_exit(void)
{
  i2c_del_driver(&ec_driver);
}

MODULE_AUTHOR("Mickey Zhan");
MODULE_DESCRIPTION("EC Driver");
MODULE_LICENSE("GPL");

module_init(ec_mod_init);
module_exit(ec_mod_exit);
