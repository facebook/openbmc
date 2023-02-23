/*
 * cmmcpld.c - The i2c driver for the CPLD on CMM
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

#define present_help_str                        \
  "0: present\n"                                \
  "1: not present"

#define enabled_help_str                        \
  "0: disabled\n"                               \
  "1: enabled"

#define uart_help_str                           \
  "0x0: disconnect all\n"                       \
  "0x1: connect with FC1 BMC UART\n"            \
  "0x2: connect with FC2 BMC UART\n"            \
  "0x3: connect with FC3 BMC UART\n"            \
  "0x4: connect with FC4 BMC UART\n"            \
  "0x5: connect with LC101 BMC UART\n"          \
  "0x6: connect with LC201 BMC UART\n"          \
  "0x7: connect with LC301 BMC UART\n"          \
  "0x8: connect with LC401 BMC UART\n"          \
  "0x9: connect with LC102 BMC UART\n"          \
  "0xa: connect with LC202 BMC UART\n"          \
  "0xb: connect with LC302 BMC UART\n"          \
  "0xc: connect with LC402 BMC UART\n"          \
  "0xd: connect with peer BCM UART\n"           \
  "others: disconnect all\n"

#define rst_n_help_str                          \
  "0: reset\n"                                  \
  "1: normal"

#define wp_help_str                             \
  "0: normal\n"                                 \
  "1: write protect"

#define alert_help_str                          \
  "0: alert\n"                                  \
  "1: no alert"

#define ms_help_str                             \
  "1: slave\n"                                  \
  "0: master"

#define force_off_help_str                      \
  "0: no force off request\n"                   \
  "1: force off request"

static const i2c_dev_attr_st cmmcpld_attr_table[] = {
  {
    "pcb_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0, 0, 4,
  },
  {
    "mod_id",
    "0x0: Galaxy LC Type-1\n"
    "0x4: Galaxy FAB Type-1\n"
    "0xc: Galalxy CMM Type-1",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0, 4, 4,
  },
  {
    "cpld_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    1, 0, 6,
  },
  {
    "release_status",
    "0: not released\n"
    "1: released",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    1, 6, 1,
  },
  {
    "cpld_sub_ver",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    2, 0, 8,
  },
  {
    "slotid",
    "0x1: CMM1\n"
    "0xa: CMM2",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    3, 0, 4,
  },
  {
    "led_red",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x6, 0, 1,
  },
  {
    "led_green",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x6, 1, 1,
  },
  {
    "led_blue",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x6, 2, 1,
  },
  {
    "led_blink",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x6, 3, 1,
  },
  {
    "lc1_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x8, 0, 1,
  },
  {
    "lc2_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x8, 1, 1,
  },
  {
    "lc3_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x8, 2, 1,
  },
  {
    "lc4_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x8, 3, 1,
  },
  {
    "fc1_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x8, 4, 1,
  },
  {
    "fc2_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x8, 5, 1,
  },
  {
    "fc3_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x8, 6, 1,
  },
  {
    "fc4_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x8, 7, 1,
  },
  {
    "scm_fc1_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x9, 0, 1,
  },
  {
    "scm_fc2_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x9, 1, 1,
  },
  {
    "scm_lc1_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x9, 2, 1,
  },
  {
    "scm_lc2_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x9, 3, 1,
  },
  {
    "scm_lc3_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x9, 4, 1,
  },
  {
    "scm_lc4_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x9, 5, 1,
  },
  {
    "scm_fc3_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x9, 6, 1,
  },
  {
    "scm_fc4_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x9, 7, 1,
  },
  {
    "hotswap_pg_sta",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0xa, 0, 1,
  },
  {
    "bmc_main_reset_en",
    enabled_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 0, 1,
  },
  {
    "bmc_reset1_en",
    enabled_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 1, 1,
  },
  {
    "bmc_reset2_en",
    enabled_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 2, 1,
  },
  {
    "bmc_reset3_en",
    enabled_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 3, 1,
  },
  {
    "bmc_reset4_en",
    enabled_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 4, 1,
  },
  {
    "bmc_wdt1_en",
    enabled_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 5, 1,
  },
  {
    "bmc_wdt2_en",
    enabled_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x20, 6, 1,
  },
  {
    "cpld_bmc_rst_n",
    "0: trigger BMC reset\n"
    "1: normal",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x21, 0, 1,
  },
  {
    "uart2_mux_ctrl",
    uart_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x30, 0, 4,
  },
  {
    "uart3_mux_ctrl",
    uart_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x31, 0, 4,
  },
  {
    "uart4_mux_ctrl",
    uart_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x32, 0, 4,
  },
  {
    "com_e_phy_rst_n",
    rst_n_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 0, 1,
  },
  {
    "fp_phy_rst_n",
    rst_n_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 1, 1,
  },
  {
    "bmc_phy_rst_n",
    rst_n_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 2, 1,
  },
  {
    "bcm5396_rst_n",
    rst_n_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x40, 3, 1,
  },
  {
    "com_e_phy_eeprom_wp",
    wp_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x42, 0, 1,
  },
  {
    "fp_phy_eeprom_wp",
    wp_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x42, 1, 1,
  },
  {
    "bmc_phy_eeprom_wp",
    wp_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x42, 2, 1,
  },
  {
    "one_wire_rst_trig",
    "0x0: normal (read always returns 0)\n"
    "0x40: send out LC101 device reset code\n"
    "0x50: send out LC102 device reset code\n"
    "0x41: send out LC201 device reset code\n"
    "0x51: send out LC202 device reset code\n"
    "0x42: send out LC301 device reset code\n"
    "0x52: send out LC302 device reset code\n"
    "0x43: send out LC401 device reset code\n"
    "0x53: send out LC402 device reset code\n"
    "0x88: send out FC1 device reset code\n"
    "0x89: send out FC2 device reset code\n"
    "0x8a: send out FC3 device reset code\n"
    "0x8b: send out FC4 device reset code\n"
    "0xc2: send out SCM-FC1 left device reset code\n"
    "0xd2: Send out SCM-FC1 right device reset code\n"
    "0xc3: send out SCM-FC2 left device reset code\n"
    "0xd3: send out SCM-FC2 right device reset code\n"
    "0xc4: send out SCM-LC101 device reset code\n"
    "0xd4: send out SCM-LC102 device reset code\n"
    "0xc4: send out SCM-LC101 device reset code\n"
    "0xd4: send out SCM-LC102 device reset code\n"
    "0xc5: send out SCM-LC201 device reset code\n"
    "0xd5: send out SCM-LC202 device reset code\n"
    "0xc6: send out SCM-LC301 device reset code\n"
    "0xd6: send out SCM-LC302 device reset code\n"
    "0xc7: send out SCM-LC401 device reset code\n"
    "0xd7: send out SCM-LC402 device reset code\n"
    "0xc8: send out SCM-FC3 left device reset code\n"
    "0xd8: Send out SCM-FC3 right device reset code\n"
    "0xc9: send out SCM-FC4 left device reset code\n"
    "0xd9: send out SCM-FC4 right device reset code",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x50, 0, 8,
  },
  {
    "one_wire_rst_device",
    "0x1: reset the I2C devices in LC/FAB/SCM\n"
    "0x2: reset the COMe in SCM and BMC in LC/FAB\n"
    "other: Reserved",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x51, 0, 8,
  },
  {
    "fan1_rst_n",
    rst_n_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 0, 1,
  },
  {
    "fan2_rst_n",
    rst_n_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 1, 1,
  },
  {
    "fan3_rst_n",
    rst_n_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 2, 1,
  },
  {
    "fan4_rst_n",
    rst_n_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 3, 1,
  },
  {
    "pdu_rst_n",
    rst_n_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x52, 4, 1,
  },
  {
    "fan1_alert",
    alert_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 0, 1,
  },
  {
    "fan2_alert",
    alert_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 1, 1,
  },
  {
    "fan3_alert",
    alert_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 2, 1,
  },
  {
    "fan4_alert",
    alert_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 3, 1,
  },
  {
    "pdu_alert",
    alert_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x53, 4, 1,
  },
  {
    "pca9548_lcfc_rst_n",
    rst_n_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x54, 0, 1,
  },
  {
    "pca9548_fan_rst_n",
    rst_n_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x54, 1, 1,
  },
  {
    "pca9548_eeprom_rst_n",
    rst_n_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x54, 2, 1,
  },
  {
    "peer_cmm_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x60, 0, 1,
  },
  {
    "cmm_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x60, 1, 1,
  },
  {
    "peer_cmm_role",
    ms_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x60, 2, 1,
  },
  {
    "cmm_role",
    ms_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x60, 3, 1,
  },
  {
    "force_off_from_peer",
    force_off_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x60, 4, 1,
  },
  {
    "force_off_to_peer",
    force_off_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x60, 5, 1,
  },
};

static i2c_dev_data_st cmmcpld_data;

/*
 * CMMCPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x3e, I2C_CLIENT_END
};

/* CMMCPLD id */
static const struct i2c_device_id cmmcpld_id[] = {
  { "cmmcpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, cmmcpld_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int cmmcpld_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the CMMCPLD
   */
  strlcpy(info->type, "cmmcpld", I2C_NAME_SIZE);
  return 0;
}

static int cmmcpld_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(cmmcpld_attr_table) / sizeof(cmmcpld_attr_table[0]);
  return i2c_dev_sysfs_data_init(client, &cmmcpld_data,
                                 cmmcpld_attr_table, n_attrs);
}

static void cmmcpld_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &cmmcpld_data);
}

static struct i2c_driver cmmcpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "cmmcpld",
  },
  .probe    = cmmcpld_probe,
  .remove   = cmmcpld_remove,
  .id_table = cmmcpld_id,
  .detect   = cmmcpld_detect,
  .address_list = normal_i2c,
};

static int __init cmmcpld_mod_init(void)
{
  return i2c_add_driver(&cmmcpld_driver);
}

static void __exit cmmcpld_mod_exit(void)
{
  i2c_del_driver(&cmmcpld_driver);
}

MODULE_AUTHOR("Tian Fang <tfang@fb.com>");
MODULE_DESCRIPTION("CMMCPLD Driver");
MODULE_LICENSE("GPL");

module_init(cmmcpld_mod_init);
module_exit(cmmcpld_mod_exit);
