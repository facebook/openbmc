/*
 * domfpga.c - The i2c driver for DOMFPGA
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

#define DOM_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define DOM_DEBUG(fmt, ...)

#endif

#define DOM_DELAY 10

#define INDIRECT_ADDR_REG    0xf0
#define INDIRECT_WRITE_REG   0xf4
#define INDIRECT_READ_REG    0xf8
#define INDIRECT_ACCESS_REG  0xfc

static ssize_t fpga_write(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf, size_t count)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int i, mask, ret;
  uint32_t val, result = 0;
  uint8_t values[4];

  if (sscanf(buf, "%i", &val) <= 0) {
    DOM_DEBUG("val\n");
    return -EINVAL;
  }

  if (dev_attr->ida_reg != INDIRECT_ADDR_REG &&
      dev_attr->ida_reg != INDIRECT_WRITE_REG ) {
    mask = (1 << dev_attr->ida_n_bits) - 1;
    val &= mask;
    ret = i2c_dev_read_nbytes(dev, attr, values, 4);
    if (ret < 0) {
      /* error case */
      return ret;
    }

    result = (values[0] << 24) | (values[1] << 16) |
             (values[2] << 8) | values[3];
    result &= ~(mask << dev_attr->ida_bit_offset);
  }

  result |= val << dev_attr->ida_bit_offset;

  values[0] = (result >> 24) & 0xff;
  values[1] = (result >> 16) & 0xff;
  values[2] = (result >> 8) & 0xff;
  values[3] = result & 0xff;

  mutex_lock(&data->idd_lock);
  for (i = 0; i < 4; i++) {
    ret = i2c_smbus_write_byte_data(client, dev_attr->ida_reg + i, values[i]);
    if (ret < 0) {
      /* error case */
      mutex_unlock(&data->idd_lock);
      DOM_DEBUG("I2C write error, ret: %d, byte cnt: %d\n", ret, i);
      return ret;
    }
  }

  if (dev_attr->ida_reg == INDIRECT_WRITE_REG) {
    ret = i2c_smbus_write_byte_data(client, INDIRECT_ACCESS_REG, 1);
    if (ret < 0) {
      /* error case */
      mutex_unlock(&data->idd_lock);
      DOM_DEBUG("I2C write error, ret: %d\n", ret);
      return ret;
    }
  }
  mutex_unlock(&data->idd_lock);

  return count;
}

static ssize_t fpga_read(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int result;
  int mask, ret;
  uint8_t values[4];
  uint32_t reg_val = 0;

  if (dev_attr->ida_reg == INDIRECT_READ_REG) {
    mutex_lock(&data->idd_lock);
    ret = i2c_smbus_write_byte_data(client, INDIRECT_ACCESS_REG, 0);
    mutex_unlock(&data->idd_lock);
    if (ret < 0) {
      /* error case */
      DOM_DEBUG("I2C read error, ret: %d\n", ret);
      return ret;
    }
  }

  ret = i2c_dev_read_nbytes(dev, attr, values, 4);
  if (ret < 0) {
    /* error case */
    return ret;
  }

  mask = (1 << (dev_attr->ida_n_bits)) - 1;
  reg_val = (values[0] << 24) | (values[1] << 16) |
           (values[2] << 8) | values[3];
  result = (reg_val >> dev_attr->ida_bit_offset) & mask;

  return scnprintf(buf, PAGE_SIZE,
                   "%#x\n\nNote:\n%s\n"
                   "Bit[%d:%d] @ register %#x, register value %#x\n",
                   result, (dev_attr->ida_help) ? dev_attr->ida_help : "",
                   dev_attr->ida_bit_offset + dev_attr->ida_n_bits - 1,
                   dev_attr->ida_bit_offset, dev_attr->ida_reg, reg_val);
}

static int dom_ctrl_read(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int i, ret;
  uint8_t values[4];
  uint32_t reg_val = 0;

  values[0] = (dev_attr->ida_reg >> 24) & 0xff;
  values[1] = (dev_attr->ida_reg >> 16) & 0xff;
  values[2] = (dev_attr->ida_reg >> 8) & 0xff;
  values[3] = dev_attr->ida_reg & 0xff;

  mutex_lock(&data->idd_lock);

  for (i = 0; i < 4; i++) {
    ret = i2c_smbus_write_byte_data(client, INDIRECT_ADDR_REG + i, values[i]);
    if (ret < 0) {
      /* error case */
      mutex_unlock(&data->idd_lock);
      DOM_DEBUG("I2C write error, ret: %d, byte cnt: %d\n", ret, i);
      return ret;
    }
  }

  ret = i2c_smbus_write_byte_data(client, INDIRECT_ACCESS_REG, 0);
  if (ret < 0) {
    /* error case */
    DOM_DEBUG("I2C read error, ret: %d\n", ret);
    mutex_unlock(&data->idd_lock);
    return ret;
  }

  for (i = 0; i < 4; i++) {
    ret = i2c_smbus_read_byte_data(client, INDIRECT_READ_REG + i);
    if (ret < 0) {
      /* error case */
      DOM_DEBUG("I2C read error, ret: %d\n", ret);
      mutex_unlock(&data->idd_lock);
      return ret;
    }
    if (ret > 255) {
      /* error case */
      DOM_DEBUG("I2C read error, ret: %d\n", ret);
      mutex_unlock(&data->idd_lock);
      return EFAULT;
    }
    values[i] = ret;
  }

  mutex_unlock(&data->idd_lock);

  reg_val = (values[0] << 24) | (values[1] << 16) |
            (values[2] << 8) | values[3];

  return scnprintf(buf, PAGE_SIZE,
                   "%#x\n\nNote:\n%s\n"
                   "Bit[%d:%d] @ register %#x, register value %#x\n",
                   reg_val, (dev_attr->ida_help) ? dev_attr->ida_help : "",
                   dev_attr->ida_bit_offset + dev_attr->ida_n_bits - 1,
                   dev_attr->ida_bit_offset, dev_attr->ida_reg, reg_val);
}

static int dom_ctrl_write(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf, size_t count)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int i, ret;
  uint32_t val;
  uint8_t values[4];

  if (sscanf(buf, "%i", &val) <= 0) {
    DOM_DEBUG("val\n");
    return -EINVAL;
  }

  values[0] = (dev_attr->ida_reg >> 24) & 0xff;
  values[1] = (dev_attr->ida_reg >> 16) & 0xff;
  values[2] = (dev_attr->ida_reg >> 8) & 0xff;
  values[3] = dev_attr->ida_reg & 0xff;

  mutex_lock(&data->idd_lock);

  for (i = 0; i < 4; i++) {
    ret = i2c_smbus_write_byte_data(client, INDIRECT_ADDR_REG + i, values[i]);
    if (ret < 0) {
      /* error case */
      mutex_unlock(&data->idd_lock);
      DOM_DEBUG("I2C write error, ret: %d, byte cnt: %d\n", ret, i);
      return ret;
    }
  }

  values[0] = (val >> 24) & 0xff;
  values[1] = (val >> 16) & 0xff;
  values[2] = (val >> 8) & 0xff;
  values[3] = val & 0xff;

  for (i = 0; i < 4; i++) {
    ret = i2c_smbus_write_byte_data(client, INDIRECT_WRITE_REG + i, values[i]);
    if (ret < 0) {
      /* error case */
      mutex_unlock(&data->idd_lock);
      DOM_DEBUG("I2C write error, ret: %d, byte cnt: %d\n", ret, i);
      return ret;
    }
  }

  ret = i2c_smbus_write_byte_data(client, INDIRECT_ACCESS_REG, 1);
  if (ret < 0) {
    /* error case */
    mutex_unlock(&data->idd_lock);
    DOM_DEBUG("I2C write error, ret: %d\n", ret);
    return ret;
  }

  mutex_unlock(&data->idd_lock);

  return count;
}

static ssize_t qsfp_temp_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
  int result = -1;
  int sign = 0;

  result = i2c_dev_read_word_bigendian(dev, attr);

  if (result < 0) {
    /* error case */
    DOM_DEBUG("I2C read error, result: %d\n", result);
    return -1;
  }
  sign = (result >> 15) & 0x1;
  result = sign ? -1 * ((~result & 0xffff) + 1)
                : result & 0xffff;

  result = ((result * 1000) / 256);

  return scnprintf(buf, PAGE_SIZE, "%d\n", result);
}

static const i2c_dev_attr_st domfpga_attr_table[] = {
  {
    "device_id",
    "Minipack DOM FPGA ID should be 0xa3",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x00, 0, 8,
  },
  {
    "fpga_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x01, 0, 8,
  },
  {
    "fpga_sub_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x02, 0, 8,
  },
  {
    "board_ver",
    "Board ID Register. Data is from board strapping resistor",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x03, 0, 8,
  },
  {
    "scratch_pad",
    "Scratch pad. Software can write anything",
    fpga_read,
    fpga_write,
    0x04, 0, 32,
  },
  {
    "system_led_blue_ctrl",
    "Color B brightness control\n"
    "0: 0% duty cycle\n"
    "1: 1/15 duty cycle\n"
    "...\n"
    "F: 15/15 duty cycle",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0e, 0, 4,
  },
  {
    "system_led_en",
    "0: Off\n"
    "1: On",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0e, 4, 1,
  },
  {
    "system_led_flash",
    "0: Normal operation\n"
    "1: Flashing enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0e, 5, 1,
  },
  {
    "system_led_color_mode",
    "0: bit [15] determines the color of the LED\n"
    "1: bit [11:0] determines the color of the LED",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0e, 6, 1,
  },
  {
    "system_led",
    "0: Amber\n"
    "1: Blue",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0e, 7, 1,
  },
  {
    "system_led_red_ctrl",
    "Color R brightness control\n"
    "0: 0% duty cycle\n"
    "1: 1/15 duty cycle\n"
    "...\n"
    "F: 15/15 duty cycle",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0f, 0, 4,
  },
  {
    "system_led_green_ctrl",
    "Color G brightness control\n"
    "0: 0% duty cycle\n"
    "1: 1/15 duty cycle\n"
    "...\n"
    "F: 15/15 duty cycle",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x0f, 4, 4,
  },
  {
    "uptime",
    "FPGA up time counter. "
    "It increases by 1 every second after out of reset.\n"
    "On EVT-B and earlier units, "
    "this counter increases by 1 every 2 seconds.",
    fpga_read,
    NULL,
    0x10, 0, 32,
  },
  {
    "msi_debug",
    "MSI generation for debug purpose.\n"
    "When writing to 1, "
    "corresponding MSI interrupt is generated "
    "if a correct password is input at 0x18 bit[24:0].",
    fpga_read,
    fpga_write,
    0x18, 0, 8,
  },
  {
    "msi_debug_pwd",
    "MSI debug password",
    fpga_read,
    fpga_write,
    0x18, 8, 24,
  },

  {
    "latency_debug_ack",
    "For HW testing purpose\n"
    "0: Reading to latency_debug register will be ACKed ASAP\n"
    "Other value: additional latency will be added to return "
    "latency_debug read ACK",
    fpga_read,
    fpga_write,
    0x1c, 0, 16,
  },
  {
    "logic_reset",
    "For HW testing purpose\n"
    "0: Reading to latency_debug register will be ACKed ASAP\n"
    "Other value: additional latency will be added to return "
    "latency_debug read ACK",
    fpga_read,
    fpga_write,
    0x20, 0, 4,
  },
  {
    "thread_id",
    "When Thread Lock bit is 0, software can write anything to Thread ID.\n"
    "Once Thread ID is successfully written, Thread Lock will be set to 1.\n"
    "When Thread Lock bit is 1, only writing the same value of Thread ID "
    "can clear both Thread ID and Thread Lock.\n"
    "No other write operation can take any effect",
    fpga_read,
    fpga_write,
    0x24, 0, 24,
  },
  {
    "thread_lock",
    "Please see Thread ID register for description.",
    fpga_read,
    NULL,
    0x24, 31, 1,
  },
  {
    "qsfp_present_intr",
    "QSFP present interrupt status",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x2f, 0, 1,
  },
  {
    "qsfp_transceiver_intr",
    "QSFP transceiver interrupt status",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x2f, 1, 1,
  },
  {
    "dom_engine_intr",
    "DOM engine interrupt status",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x2f, 2, 1,
  },
  {
    "qsfp_rtc_i2c_intr",
    "QSFP RTC I2C interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x2f, 3, 1,
  },
  {
    "device_intr",
    "Please refer to device interrupt register for details",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x2f, 4, 1,
  },
  {
    "power_intr",
    "Power Interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x2f, 5, 1,
  },
  {
    "mdio_intr",
    "MDIO interrupt",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x2f, 6, 1,
  },
  {
    "gpio_control",
    "GPIO signal control for MODSEL signals",
    fpga_read,
    fpga_write,
    0x40, 0, 15,
  },
  {
    "gpio_control_mode",
    "0: This GPIO register controls GPIO signals\n"
    "1: FPGA engine controls GPIO signals",
    fpga_read,
    fpga_write,
    0x40, 16, 1,
  },
  {
    "qsfp_max_temp_num",
    "Max temperature sequence number",
    fpga_read,
    NULL,
    0x44, 24, 8,
  },
  {
    "qsfp_max_temp_location",
    "Max temperature QSFP location",
    fpga_read,
    NULL,
    0x45, 16, 8,
  },
  {
    "temp1_input",
    NULL,
    qsfp_temp_show,
    NULL,
    0x46, 0, 16,
  },
  {
    "qsfp_reset",
    "Max temperature sequence number",
    fpga_read,
    fpga_write,
    0x70, 0, 16,
  },
  {
    "indirect_addr",
    "Indirect Access Address register",
    fpga_read,
    fpga_write,
    INDIRECT_ADDR_REG, 0, 32,
  },
  {
    "indirect_write",
    "Indirect Access Write data register",
    fpga_read,
    fpga_write,
    INDIRECT_WRITE_REG, 0, 32,
  },
  {
    "indirect_read",
    "Indirect Access Read data",
    fpga_read,
    NULL,
    INDIRECT_READ_REG, 0, 32,
  },
  {
    "dom_ctrl_config",
    "DOM Control Configuration",
    dom_ctrl_read,
    dom_ctrl_write,
    0x410, 0, 32,
  },
};

static i2c_dev_data_st domfpga_data;

/*
 * DOMFPGA i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x60, I2C_CLIENT_END
};

/* DOMFPGA id */
static const struct i2c_device_id domfpga_id[] = {
  { "domfpga", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, domfpga_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int domfpga_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the DOMFPGA
   */
  strlcpy(info->type, "domfpga", I2C_NAME_SIZE);
  return 0;
}

static int domfpga_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(domfpga_attr_table) / sizeof(domfpga_attr_table[0]);
  return i2c_dev_sysfs_data_init(client, &domfpga_data,
                                 domfpga_attr_table, n_attrs);
}

static int domfpga_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &domfpga_data);
  return 0;
}

static struct i2c_driver domfpga_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "domfpga",
  },
  .probe    = domfpga_probe,
  .remove   = domfpga_remove,
  .id_table = domfpga_id,
  .detect   = domfpga_detect,
  .address_list = normal_i2c,
};

static int __init domfpga_mod_init(void)
{
  return i2c_add_driver(&domfpga_driver);
}

static void __exit domfpga_mod_exit(void)
{
  i2c_del_driver(&domfpga_driver);
}

MODULE_AUTHOR("Tian Fang <tfang@fb.com>");
MODULE_DESCRIPTION("DOMFPGA Driver");
MODULE_LICENSE("GPL");

module_init(domfpga_mod_init);
module_exit(domfpga_mod_exit);
