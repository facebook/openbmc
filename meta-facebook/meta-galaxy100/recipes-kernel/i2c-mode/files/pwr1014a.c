/*
* pwr1014a.c - The i2c driver for PWR1014A
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
#define PWR_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define PWR_DEBUG(fmt, ...)
#endif

#define PWR1014A_DELAY 11 //ms
#define PWR1014A_CONTROL_REG 0x09

static int pwr1014a_vmon_convert(struct device *dev, struct device_attribute *attr, int num)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int lsb_val, msb_val;
  int result, count=100;

  mutex_lock(&data->idd_lock);
  if(i2c_smbus_write_byte_data(client, PWR1014A_CONTROL_REG, 0xf0 + num) < 0) {
	PWR_DEBUG("I2C write error!\n");
    return -1;
  }
  msleep(PWR1014A_DELAY);
  lsb_val = i2c_smbus_read_byte_data(client, dev_attr->ida_reg);
  while(count-- > 0) {
    if((lsb_val & 0x1) == 0x1)
		break;
	msleep(PWR1014A_DELAY);
	lsb_val = i2c_smbus_read_byte_data(client, dev_attr->ida_reg);
  }
  if(count <= 0) {
	PWR_DEBUG("Wait convert timeout!\n");
    return -1;
  }
  msb_val = i2c_smbus_read_byte_data(client, (dev_attr->ida_reg + 0x1));
  mutex_unlock(&data->idd_lock);

  if (lsb_val < 0 || msb_val < 0) {
    /* error case */
	PWR_DEBUG("I2C read error!\n");
    return -1;
  }

  result = ((msb_val << 4) + (lsb_val >> 4)) * 2;

  return result;
}

static ssize_t pwr1014a_vmon0_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int result;

  result = pwr1014a_vmon_convert(dev, attr, 0);
  if(result < 0)
	  return -1;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t pwr1014a_vmon1_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int result;

  result = pwr1014a_vmon_convert(dev, attr, 1);
  if(result < 0)
	  return -1;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t pwr1014a_vmon2_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int result;

  result = pwr1014a_vmon_convert(dev, attr, 2);
  if(result < 0)
	  return -1;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t pwr1014a_vmon3_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int result;

  result = pwr1014a_vmon_convert(dev, attr, 3);
  if(result < 0)
	  return -1;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t pwr1014a_vmon4_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int result;

  result = pwr1014a_vmon_convert(dev, attr, 4);
  if(result < 0)
	  return -1;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t pwr1014a_vmon5_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int result;

  result = pwr1014a_vmon_convert(dev, attr, 5);
  if(result < 0)
	  return -1;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t pwr1014a_vmon6_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int result;

  result = pwr1014a_vmon_convert(dev, attr, 6);
  if(result < 0)
	  return -1;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t pwr1014a_vmon7_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int result;

  result = pwr1014a_vmon_convert(dev, attr, 7);
  if(result < 0)
	  return -1;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t pwr1014a_vmon8_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int result;

  result = pwr1014a_vmon_convert(dev, attr, 8);
  if(result < 0)
	  return -1;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static ssize_t pwr1014a_vmon9_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int result;

  result = pwr1014a_vmon_convert(dev, attr, 9);
  if(result < 0)
	  return -1;

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static const i2c_dev_attr_st pwr1014a_attr_table[] = {
  {
    "in0_input",
    NULL,
    pwr1014a_vmon0_show,
    NULL,
    0x7, 0, 8,
  },
  {
    "in1_input",
    NULL,
    pwr1014a_vmon1_show,
    NULL,
    0x7, 0, 8,
  },
  {
    "in2_input",
    NULL,
    pwr1014a_vmon2_show,
    NULL,
    0x7, 0, 8,
  },
  {
    "in3_input",
    NULL,
    pwr1014a_vmon3_show,
    NULL,
    0x7, 0, 8,
  },
  {
    "in4_input",
    NULL,
    pwr1014a_vmon4_show,
    NULL,
    0x7, 0, 8,
  },
  {
    "in5_input",
    NULL,
    pwr1014a_vmon5_show,
    NULL,
    0x7, 0, 8,
  },
  {
    "in6_input",
    NULL,
    pwr1014a_vmon6_show,
    NULL,
    0x7, 0, 8,
  },
  {
    "in7_input",
    NULL,
    pwr1014a_vmon7_show,
    NULL,
    0x7, 0, 8,
  },
  {
    "in8_input",
    NULL,
    pwr1014a_vmon8_show,
    NULL,
    0x7, 0, 8,
  },
  {
    "in9_input",
    NULL,
    pwr1014a_vmon9_show,
    NULL,
    0x7, 0, 8,
  },
};

static i2c_dev_data_st pwr1014a_data;

/*
 * PWR1014A i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x40, I2C_CLIENT_END
};

/*PWR1014A  id */
static const struct i2c_device_id pwr1014a_id[] = {
  {"pwr1014a", 0},
  { },
};
MODULE_DEVICE_TABLE(i2c, pwr1014a_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int pwr1014a_detect(struct i2c_client *client,
                           struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the driver
   */
  strlcpy(info->type, "pwr1014a", I2C_NAME_SIZE);
  return 0;
}

static int pwr1014a_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(pwr1014a_attr_table) / sizeof(pwr1014a_attr_table[0]);
  return i2c_dev_sysfs_data_init(client, &pwr1014a_data,
                                 pwr1014a_attr_table, n_attrs);
}

static int pwr1014a_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &pwr1014a_data);
  return 0;
}

static struct i2c_driver pwr1014a_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "pwr1014a",
  },
  .probe    = pwr1014a_probe,
  .remove   = pwr1014a_remove,
  .id_table = pwr1014a_id,
  .detect   = pwr1014a_detect,
  .address_list = normal_i2c,
};

static int __init pwr1014a_mod_init(void)
{
  return i2c_add_driver(&pwr1014a_driver);
}

static void __exit pwr1014a_mod_exit(void)
{
  i2c_del_driver(&pwr1014a_driver);
}

MODULE_AUTHOR("Mickey Zhan");
MODULE_DESCRIPTION("PWR1014A Driver");
MODULE_LICENSE("GPL");

module_init(pwr1014a_mod_init);
module_exit(pwr1014a_mod_exit);
