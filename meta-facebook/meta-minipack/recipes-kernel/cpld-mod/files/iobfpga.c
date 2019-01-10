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

// #define DEBUG

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <i2c_dev_sysfs.h>

#ifdef DEBUG

#define IOB_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define IOB_DEBUG(fmt, ...)

#endif

#define IOB_DELAY 10

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
    IOB_DEBUG("val\n");
    return -EINVAL;
  }

  if (dev_attr->ida_reg != INDIRECT_ADDR_REG &&
      dev_attr->ida_reg != INDIRECT_WRITE_REG ) {
    mask = (1 << dev_attr->ida_n_bits) - 1;;
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
      IOB_DEBUG("I2C write error, ret: %d, byte cnt: %d\n", ret, i);
      return ret;
    }
  }

  if (dev_attr->ida_reg == INDIRECT_WRITE_REG) {
    ret = i2c_smbus_write_byte_data(client, INDIRECT_ACCESS_REG, 1);
    if (ret < 0) {
      /* error case */
      mutex_unlock(&data->idd_lock);
      IOB_DEBUG("I2C write error, ret: %d\n", ret);
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
      IOB_DEBUG("I2C read error, ret: %d\n", ret);
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

static const i2c_dev_attr_st iobfpga_attr_table[] = {
  {
    "device_id",
    "Minipack IOB FPGA ID should be 0xa2",
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
    "0x4: EVTA\n"
    "0x0: EVTB\n"
    "0x1: DVTA\n"
    "0x2: DVTB\n"
    "0x3: PVT",
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
    "pim_present_change_intr",
    "PIM present status change interrupt status",
    fpga_read,
    NULL,
    0x2c, 0, 1,
  },
  {
    "pim_fpga_intr",
    "PIM fpga interrupt status",
    fpga_read,
    NULL,
    0x2c, 1, 1,
  },
  {
    "pim_slpc_intr",
    "PIM SLPC interrupt status",
    fpga_read,
    NULL,
    0x2c, 2, 1,
  },
  {
    "pim_present_change",
    "PIM Present Status Change\n"
    "1: corresponding PIM presence status change has happened\n"
    "0: corresponding PIM presence status change has not happened",
    fpga_read,
    fpga_write,
    0x40, 0, 8,
  },
  {
    "pim_intr",
    "PIM interrupt status\n"
    "1: interrupt\n"
    "0: no interrupt",
    fpga_read,
    NULL,
    0x40, 8, 8,
  },
  {
    "pim_present",
    "PIM present status is forced to enabled (0ohm to ground) on PIM card\n"
    "1: corresponding PIM is present\n"
    "0: corresponding PIM is not present",
    fpga_read,
    NULL,
    0x40, 16, 8,
  },
  {
    "pim_present_change_intr_mask",
    "PIM Present Status Change interrupt mask\n"
    "1: PIM Present Status Change will not generate interrupt\n"
    "0: PIM Present Status Change will generate interrupt",
    fpga_read,
    fpga_write,
    0x44, 0, 8,
  },
  {
    "pim_intr_mask",
    "PIM interrupt mask\n"
    "1: PIM interrupt will not generate interrupt\n"
    "0: PIM interrupt will generate interrupt",
    fpga_read,
    fpga_write,
    0x44, 8, 8,
  },
  {
    "pim_slpc_parity_en",
    "PIM SLPC parity enable\n"
    "SLPC uses odd parity once enabled",
    fpga_read,
    fpga_write,
    0x48, 0, 8,
  },
  {
    "pim_slpc_err_intr_mask",
    "SLPC interface error interrupt mask",
    fpga_read,
    fpga_write,
    0x48, 16, 8,
  },
  {
    "pim_slpc_parity_intr_err",
    "Once a bit is asserted, "
    "it is either a parity error "
    "or a timeout error has occurred on the particular SLPC interface",
    fpga_read,
    fpga_write,
    0x4c, 0, 8,
  },
  {
    "pim1_slpc_parity_err_cnt",
    "PIM1 SLPC master received parity error",
    fpga_read,
    fpga_write,
    0x50, 0, 16,
  },
  {
    "pim2_slpc_parity_err_cnt",
    "PIM2 SLPC master received parity error",
    fpga_read,
    fpga_write,
    0x50, 16, 16,
  },
  {
    "pim4_slpc_parity_err_cnt",
    "PIM4 SLPC master received parity error",
    fpga_read,
    fpga_write,
    0x54, 0, 16,
  },
  {
    "pim3_slpc_parity_err_cnt",
    "PIM3 SLPC master received parity error",
    fpga_read,
    fpga_write,
    0x54, 16, 16,
  },
  {
    "pim6_slpc_parity_err_cnt",
    "PIM6 SLPC master received parity error",
    fpga_read,
    fpga_write,
    0x58, 0, 16,
  },
  {
    "pim5_slpc_parity_err_cnt",
    "PIM5 SLPC master received parity error",
    fpga_read,
    fpga_write,
    0x58, 16, 16,
  },
  {
    "pim8_slpc_parity_err_cnt",
    "PIM8 SLPC master received parity error",
    fpga_read,
    fpga_write,
    0x5c, 0, 16,
  },
  {
    "pim7_slpc_parity_err_cnt",
    "PIM7 SLPC master received parity error",
    fpga_read,
    fpga_write,
    0x5c, 16, 16,
  },
  {
    "pim1_slpc_parity_err_timeout",
    "PIM1 SLPC master received parity error",
    fpga_read,
    fpga_write,
    0x60, 0, 16,
  },
  {
    "pim2_slpc_parity_err_timeout",
    "PIM2 SLPC master received parity error",
    fpga_read,
    fpga_write,
    0x60, 16, 16,
  },
  {
    "pim4_slpc_parity_err_timeout",
    "PIM4 SLPC master received parity error",
    fpga_read,
    fpga_write,
    0x64, 0, 16,
  },
  {
    "pim3_slpc_parity_err_timeout",
    "PIM3 SLPC master received parity error",
    fpga_read,
    fpga_write,
    0x64, 16, 16,
  },
  {
    "pim6_slpc_parity_err_timeout",
    "PIM6 SLPC master received parity error",
    fpga_read,
    fpga_write,
    0x68, 0, 16,
  },
  {
    "pim5_slpc_parity_err_timeout",
    "PIM5 SLPC master received parity error",
    fpga_read,
    fpga_write,
    0x68, 16, 16,
  },
  {
    "pim8_slpc_parity_err_timeout",
    "PIM8 SLPC master received parity error",
    fpga_read,
    fpga_write,
    0x6c, 0, 16,
  },
  {
    "pim7_slpc_parity_err_timeout",
    "PIM7 SLPC master received parity error",
    fpga_read,
    fpga_write,
    0x6c, 16, 16,
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
    "ltssm_status",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0xff, 0, 4,
  },
  {
    "pcie_l0_state",
    "PCIe entered L0 state",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0xff, 4, 1,
  },
  {
    "pcie_poll_state",
    "PCIe entered POLL state",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0xff, 5, 1,
  },
  {
    "data_link_layer_status",
    "Data link layer link up status",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0xff, 6, 1,
  },
  {
    "msi_en_status",
    "MSI enable status",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0xff, 7, 1,
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
