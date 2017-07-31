/*
 * syscpld.c - The i2c driver for SYSCPLD
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
 */

//#define DEBUG

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/i2c.h>

#include <i2c_dev_sysfs.h>

#ifdef DEBUG

#define PP_DEBUG(fmt, ...) do {                     \
    printk(KERN_DEBUG "%s:%d " fmt "\n",            \
           __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
  } while (0)

#else /* !DEBUG */

#define PP_DEBUG(fmt, ...)

#endif
#define SYSCPLD_SWE_ID 	0x05
#define SYSCPLD_SWE_ID_MASK 	0x03

static ssize_t syscpld_slotid_show(struct device *dev,
                                   struct device_attribute *attr,
                                   char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int val_msb, val_lsb;
  int val_mask;
  int slot_id;

  val_mask = ~(((-1) >> (dev_attr->ida_n_bits)) << (dev_attr->ida_n_bits));

  mutex_lock(&data->idd_lock);

  val_msb = i2c_smbus_read_byte_data(client, dev_attr->ida_reg);
  val_lsb = i2c_smbus_read_byte_data(client, SYSCPLD_SWE_ID);
  mutex_unlock(&data->idd_lock);

  if (val_msb < 0 || val_lsb < 0) {
    /* error case */
    return -1;
  }

  val_msb = (val_msb >> dev_attr->ida_bit_offset) & val_mask;

  if (val_msb < 4) { /* LC */
    /* val_lsb: 00->Left, 01->Right */
    val_lsb &= SYSCPLD_SWE_ID_MASK;
    slot_id = ((val_msb + 1) << 8) + (val_lsb + 1);
  } else {
    slot_id = val_msb - 7;
  }

  return scnprintf(buf, PAGE_SIZE, "%x\n", slot_id);
}


static const i2c_dev_attr_st syscpld_attr_table[] = {
  {
    "board_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0, 0, 4,
  },
  {
    "model_id",
    "0x0: Galaxy100 LC\n"
    "0x4: Galaxy100 FAB\n"
    "0xc: Galaxy100 CMM",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0, 4, 4,
  },
  {
    "cpld_rev",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x1, 0, 6,
  },
  {
    "cpld_released",
    "0x0: not released\n"
	"0x1: released after PVT",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x1, 6, 1,
  },
  {
    "cpld_sub_rev",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x2, 0, 8,
  },
  {
    "slotid",
    NULL,
    syscpld_slotid_show,
    NULL,
    0x3, 0, 4,
  },
  {
    "vcp_id",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x4, 0, 2,
  },
  {
    "led_red",
    "0x0: off\n"
	"0x1: on",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x6, 0, 1,
  },
  {
    "led_green",
    "0x0: off\n"
	"0x1: on",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x6, 1, 1,
  },
  {
    "led_blue",
    "0x0: off\n"
	"0x1: on",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x6, 2, 1,
  },
  {
    "led_blink",
    "0x0: no blink\n"
	"0x1: blink",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x6, 3, 1,
  },
  {
    "micro_srv_present",
    "0x0: present\n"
	"0x1: not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x8, 0, 1,
  },
  {
    "scm_oir_record",
    "0x0: write to clear\n"
	"0x1: SCM OIR",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x8, 1, 1,
  },
  {
    "peer_con_present",
    "0x0: present\n"
	"0x1: not present",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x9, 0, 4,
  },
  {
    "hotswap_pg_inv_sta",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0xa, 0, 1,
  },
  {
    "lm57_tover_sta",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0xb, 0, 1,
  },
  {
    "pwr_cyc_all_n",
    "0x0: power cycle all power\n"
    "0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 0, 1,
  },
  {
    "pwr_main_n",
    "0x0: main power is off\n"
	"0x1: main power is on",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 1, 1,
  },
  {
    "pwr_main_force",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x10, 4, 1,
  },
  {
    "hot_rst_req",
    "0x0: trigger hot reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x11, 0, 1,
  },
  {
    "warm_rst_req",
    "0x0: trigger warm reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x11, 1, 1,
  },
  {
    "cold_rst_req",
    "0x0: trigger cold reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x11, 2, 1,
  },
  {
    "pwr_rst_req",
    "0x0: trigger power reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x11, 3, 1,
  },
  {
    "th_sys_rst_n",
    "0x0: trigger tomahawk system reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 0, 1,
  },
  {
    "th_pcie_rst_n",
    "0x0: trigger tomahawk PCIe reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 1, 1,
  },
  {
    "th_sys_rst_mask",
	"0x0: normal\n"
    "0x1: mask CB_RESET_N from SCM card to TH_SYS_RST_N",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 2, 1,
  },
  {
    "th_pcie_rst_mask",
	"0x0: normal\n"
    "0x1: mask CB_RESET_N from SCM card to TH_PCIE_RST_N",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x12, 3, 1,
  },
  {
    "cp2112_rst",
    "0x0: reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 0, 1,
  },
  {
    "usb_hub_rst",
    "0x0: reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 1, 1,
  },
  {
    "bcm5389_rst",
    "0x0: reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 2, 1,
  },
  {
    "switch_phy_rst",
    "0x0: reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 3, 1,
  },
  {
    "bcm54616s_rst",
    "0x0: reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 4, 1,
  },
  {
    "scm_cpld_i2c_slave_rst",
    "0x0: reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 5, 1,
  },
  {
    "lpc_rst",
    "0x0: reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x14, 6, 1,
  },
  {
    "hw_shutdown_rec",
    "0x0: normal status\n"
	"0x11: hardware shutdown recorded",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x15, 0, 8,
  },
  {
    "th_rst_rec",
    "0x0: write to clear\n"
	"0x11: power on reset TH\n"
	"0x22: COMe CB_REST_N reset TH\n"
	"0x33: register control reset TH",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x16, 0, 8,
  },
  {
    "bmc_main_rest_en",
    "0x0: BMC main reset request is disabled\n"
	"0x1: BMC main reset request is enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 0, 1,
  },
  {
    "bmc_rst1_en",
    "0x0: BMC reset1 request is disabled\n"
	"0x1: BMC reset1 request is enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 1, 1,
  },
  {
    "bmc_rst2_en",
    "0x0: BMC reset2 request is disabled\n"
	"0x1: BMC reset2 request is enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 2, 1,
  },
  {
    "bmc_rst3_en",
    "0x0: BMC reset3 request is disabled\n"
	"0x1: BMC reset3 request is enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 3, 1,
  },
  {
    "bmc_rst4_en",
    "0x0: BMC reset4 request is disabled\n"
	"0x1: BMC reset4 request is enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 4, 1,
  },
  {
    "bmc_wdt1_en",
    "0x0: WDT1 is disabled\n"
	"0x1: WDT1 is enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 5, 1,
  },
  {
    "bmc_wdt2_en",
    "0x0: WDT2 is disabled\n"
	"0x1: WDT2 is enabled",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 6, 1,
  },
  {
    "bmc_rst",
    "0x0: trigger BMC reset\n"
	"0x1: BMC reset normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 0, 1,
  },
  {
    "bmc_rst_type",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x22, 0, 8,
  },
  {
    "uart_mux",
    "0x0: tomahawk uart 0 connected to BMC uart 2\n"
    "0x1: tomahawk uart 2 connected to BMC uart 2\n"
    "0x2: tomahawk uart 3 connected to BMC uart 2",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 0, 2,
  },
  {
    "sol_uart_sel",
    "0x0: use uart 1\n"
    "0x1: use uart 4",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x30, 2, 1,
  },
  {
    "rov_sta",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x40, 0, 4,
  },
  {
    "ir3581_vrrdy1",
    "0x0: power not OK\n"
	"0x1: power OK",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x41, 0, 1,
  },
  {
    "ir3581_vrhot",
    "0x0: over temperature is negative\n"
	"0x1: over temperature is active",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x41, 1, 1,
  },
  {
    "ir3584_vrrdy1",
    "0x0: power not OK\n"
	"0x1: power OK",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x41, 2, 1,
  },
  {
    "ir3584_vrhot",
    "0x0: over temperature is negative\n"
	"0x1: over temperature is active",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x41, 3, 1,
  },
  {
    "cp2112_int",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x50, 0, 8,
  },
  {
    "cp2112_oe",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x51, 0, 8,
  },
  {
    "cp2112_out",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 0, 8,
  },
  {
    "cmm1_sta",
    "0x0: CMM1 is master\n"
	"0x1: CMM1 is slave",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x60, 2, 1,
  },
  {
    "cmm2_sta",
    "0x0: CMM2 is master\n"
	"0x1: CMM2 is slave",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x60, 3, 1,
  },
  {
    "qsfp_cpld_rst",
    "0x0: reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x78, 0, 1,
  },
  {
    "pca9541_rst",
    "0x0: reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x78, 1, 1,
  },
  {
    "pca9548_rst1",
    "0x0: reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x78, 2, 1,
  },
  {
    "pca9548_rst2",
    "0x0: reset\n"
	"0x1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x78, 3, 1,
  },
  {
    "qsfp_cpld_rst_mask",
    "0x0: normal\n"
	"0x1: mask",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x78, 4, 1,
  },
  {
    "qsfp_cpld_mod_abs",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x79, 0, 1,
  },
  {
    "qsfp_cpld_int_n",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x79, 1, 1,
  },
  {
    "dual_boot_en",
    "0x0: single boot\n"
	"0x1: dual boot",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x80, 0, 1,
  },
  {
    "2nd_flash_wp",
    "0x0: writable\n"
	"0x1: write protected",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x80, 1, 1,
  },
  {
    "2nd_flash_wp_force",
    "0x0: normal mode\n"
	"0x1: debug mode",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x80, 2, 1,
  },
  {
    "bcm56960_iso_buf_en",
    "0x0: disable\n"
	"0x1: enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x80, 4, 1,
  },
  {
    "bmc_iso_buf_en",
    "0x0: disable\n"
	"0x1: enable",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x80, 5, 1,
  },
  {
    "iso_buf_en_force",
    "0x0: BMC control\n"
	"0x1: CPLD control",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x80, 6, 1,
  },
  {
    "i2c_sta_rst_raw",
    "Backdoor register for directly controlling I2C master for QSFP access\n"
    "To be used for CP2112 I2C CLK flush purpose only",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x96, 0, 8,
  },
};

static i2c_dev_data_st syscpld_data;

/*
 * SYSCPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x31, I2C_CLIENT_END
};

/* SYSCPLD id */
static const struct i2c_device_id syscpld_id[] = {
  { "syscpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, syscpld_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int syscpld_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the SYSCPLD
   */
  strlcpy(info->type, "syscpld", I2C_NAME_SIZE);
  return 0;
}

static int syscpld_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(syscpld_attr_table) / sizeof(syscpld_attr_table[0]);
  return i2c_dev_sysfs_data_init(client, &syscpld_data,
                                 syscpld_attr_table, n_attrs);
}

static int syscpld_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &syscpld_data);
  return 0;
}

static struct i2c_driver syscpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "syscpld",
  },
  .probe    = syscpld_probe,
  .remove   = syscpld_remove,
  .id_table = syscpld_id,
  .detect   = syscpld_detect,
  .address_list = normal_i2c,
};

static int __init syscpld_mod_init(void)
{
  return i2c_add_driver(&syscpld_driver);
}

static void __exit syscpld_mod_exit(void)
{
  i2c_del_driver(&syscpld_driver);
}

MODULE_AUTHOR("Mickey Zhan");
MODULE_DESCRIPTION("SYSCPLD Driver");
MODULE_LICENSE("GPL");

module_init(syscpld_mod_init);
module_exit(syscpld_mod_exit);
