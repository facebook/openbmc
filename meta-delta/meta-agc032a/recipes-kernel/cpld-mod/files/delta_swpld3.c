/*
 * delta_swpld3.c - The i2c driver for AGC032A SWPLD3
 *
 * Copyright 2020-present Delta Eletronics, Inc. All Rights Reserved.
 *
 */
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

#define reset_help_str \
  "0: Reset\n"         \
  "1: Normal Operation\n"

#define lp_mode_help_str \
  "0: Not LP Mode\n"     \
  "1: LP Mode\n"

#define present_help_str \
  "0: Present\n"         \
  "1: Not Present\n"

#define respond_help_str \
  "0: Respond\n"         \
  "1: Not Respond\n"

#define assert_intr_help_str \
  "0: Assert Interrupt \n"   \
  "1: Not Assert Intrrupt\n"

#define rx_loss_help_str   \
  "0: Normal Operation \n" \
  "1: Receive Loss\n"

#define tx_fault_help_str  \
  "0: Normal Operation \n" \
  "1: Transmit Fault\n"

#define opt_dis_help_str  \
  "0: Normal Operation \n" \
  "1: Disabled\n"

static const i2c_dev_attr_st swpld_3_attr_table[] = {
    {
        "swpld3_ver_type",
        "SWPLD3 Version Type",
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x1,
        7,
        1,
    },
    {
        "swpld3_ver",
        "SWPLD3 Version",
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x1,
        0,
        6,
    },
    {
        "qsfp_p17_rst",
        "QSFP P17:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        7,
        1,
    },
    {
        "qsfp_p18_rst",
        "QSFP P18:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        6,
        1,
    },
    {
        "qsfp_p19_rst",
        "QSFP P19:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        5,
        1,
    },
    {
        "qsfp_p20_rst",
        "QSFP P20:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        4,
        1,
    },
    {
        "qsfp_p21_rst",
        "QSFP P21:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        3,
        1,
    },
    {
        "qsfp_p22_rst",
        "QSFP P22:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        2,
        1,
    },
    {
        "qsfp_p23_rst",
        "QSFP P23 :\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        1,
        1,
    },
    {
        "qsfp_p24_rst",
        "QSFP P24 :\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        0,
        1,
    },
    {
        "qsfp_p25_rst",
        "QSFP P25:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        7,
        1,
    },
    {
        "qsfp_p26_rst",
        "QSFP P26:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        6,
        1,
    },
    {
        "qsfp_p27_rst",
        "QSFP P27:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        5,
        1,
    },
    {
        "qsfp_p28_rst",
        "QSFP P28:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        4,
        1,
    },
    {
        "qsfp_p29_rst",
        "QSFP P29:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        3,
        1,
    },
    {
        "qsfp_p30_rst",
        "QSFP P30:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        2,
        1,
    },
    {
        "qsfp_p31_rst",
        "QSFP P31 :\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        1,
        1,
    },
    {
        "qsfp_p32_rst",
        "QSFP P32 :\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        0,
        1,
    },
    {
        "qsfp_p17_lpmode",
        "QSFP P17:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x21,
        7,
        1,
    },
    {
        "qsfp_p18_lpmode",
        "QSFP P18:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x21,
        6,
        1,
    },
    {
        "qsfp_p19_lpmode",
        "QSFP P19:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x21,
        5,
        1,
    },
    {
        "qsfp_p20_lpmode",
        "QSFP P20:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x21,
        4,
        1,
    },
    {
        "qsfp_p21_lpmode",
        "QSFP P21:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x21,
        3,
        1,
    },
    {
        "qsfp_p22_lpmode",
        "QSFP P22:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x21,
        2,
        1,
    },
    {
        "qsfp_p23_lpmode",
        "QSFP P23 :\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x21,
        1,
        1,
    },
    {
        "qsfp_p24_lpmode",
        "QSFP P24 :\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x21,
        0,
        1,
    },
    {
        "qsfp_p25_lpmode",
        "QSFP P25:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x22,
        7,
        1,
    },
    {
        "qsfp_p26_lpmode",
        "QSFP P26:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x22,
        6,
        1,
    },
    {
        "qsfp_p27_lpmode",
        "QSFP P27:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x22,
        5,
        1,
    },
    {
        "qsfp_p28_lpmode",
        "QSFP P28:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x22,
        4,
        1,
    },
    {
        "qsfp_p29_lpmode",
        "QSFP P29:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x22,
        3,
        1,
    },
    {
        "qsfp_p30_lpmode",
        "QSFP P30:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x22,
        2,
        1,
    },
    {
        "qsfp_p31_lpmode",
        "QSFP P31 :\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x22,
        1,
        1,
    },
    {
        "qsfp_p32_lpmode",
        "QSFP P32 :\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x22,
        0,
        1,
    },
    {
        "qsfp_p17_modprs_set",
        "QSFP P17:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x31,
        7,
        1,
    },
    {
        "qsfp_p18_modprs_set",
        "QSFP P18:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x31,
        6,
        1,
    },
    {
        "qsfp_p19_modprs_set",
        "QSFP P19:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x31,
        5,
        1,
    },
    {
        "qsfp_p20_modprs_set",
        "QSFP P20:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x31,
        4,
        1,
    },
    {
        "qsfp_p21_modprs_set",
        "QSFP P21:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x31,
        3,
        1,
    },
    {
        "qsfp_p22_modprs_set",
        "QSFP P22:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x31,
        2,
        1,
    },
    {
        "qsfp_p23_modprs_set",
        "QSFP P23 :\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x31,
        1,
        1,
    },
    {
        "qsfp_p24_modprs_set",
        "QSFP P24 :\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x31,
        0,
        1,
    },
    {
        "qsfp_p25_modprs_set",
        "QSFP P25:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x32,
        7,
        1,
    },
    {
        "qsfp_p26_modprs_set",
        "QSFP P26:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x32,
        6,
        1,
    },
    {
        "qsfp_p27_modprs_set",
        "QSFP P27:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x32,
        5,
        1,
    },
    {
        "qsfp_p28_modprs_set",
        "QSFP P28:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x32,
        4,
        1,
    },
    {
        "qsfp_p29_modprs_set",
        "QSFP P29:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x32,
        3,
        1,
    },
    {
        "qsfp_p30_modprs_set",
        "QSFP P30:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x32,
        2,
        1,
    },
    {
        "qsfp_p31_modprs_set",
        "QSFP P31 :\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x32,
        1,
        1,
    },
    {
        "qsfp_p32_modprs_set",
        "QSFP P32 :\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x32,
        0,
        1,
    },
    {
        "qsfp_p17_modprs",
        "QSFP P17:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x51,
        7,
        1,
    },
    {
        "qsfp_p18_modprs",
        "QSFP P18:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x51,
        6,
        1,
    },
    {
        "qsfp_p19_modprs",
        "QSFP P19:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x51,
        5,
        1,
    },
    {
        "qsfp_p20_modprs",
        "QSFP P20:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x51,
        4,
        1,
    },
    {
        "qsfp_p21_modprs",
        "QSFP P21:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x51,
        3,
        1,
    },
    {
        "qsfp_p22_modprs",
        "QSFP P22:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x51,
        2,
        1,
    },
    {
        "qsfp_p23_modprs",
        "QSFP P23 :\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x51,
        1,
        1,
    },
    {
        "qsfp_p24_modprs",
        "QSFP P24 :\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x51,
        0,
        1,
    },
    {
        "qsfp_p25_modprs",
        "QSFP P25:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x52,
        7,
        1,
    },
    {
        "qsfp_p26_modprs",
        "QSFP P26:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x52,
        6,
        1,
    },
    {
        "qsfp_p27_modprs",
        "QSFP P27:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x52,
        5,
        1,
    },
    {
        "qsfp_p28_modprs",
        "QSFP P28:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x52,
        4,
        1,
    },
    {
        "qsfp_p29_modprs",
        "QSFP P29:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x52,
        3,
        1,
    },
    {
        "qsfp_p30_modprs",
        "QSFP P30:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x52,
        2,
        1,
    },
    {
        "qsfp_p31_modprs",
        "QSFP P31 :\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x52,
        1,
        1,
    },
    {
        "qsfp_p32_modprs",
        "QSFP P32 :\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x52,
        0,
        1,
    },
    {
        "qsfp_p17_intr",
        "QSFP P17:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x61,
        7,
        1,
    },
    {
        "qsfp_p18_intr",
        "QSFP P18:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x61,
        6,
        1,
    },
    {
        "qsfp_p19_intr",
        "QSFP P19:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x61,
        5,
        1,
    },
    {
        "qsfp_p20_intr",
        "QSFP P20:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x61,
        4,
        1,
    },
    {
        "qsfp_p21_intr",
        "QSFP P21:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x61,
        3,
        1,
    },
    {
        "qsfp_p22_intr",
        "QSFP P22:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x61,
        2,
        1,
    },
    {
        "qsfp_p23_intr",
        "QSFP P23 :\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x61,
        1,
        1,
    },
    {
        "qsfp_p24_intr",
        "QSFP P24 :\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x61,
        0,
        1,
    },
    {
        "qsfp_p25_intr",
        "QSFP P25:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x62,
        7,
        1,
    },
    {
        "qsfp_p26_intr",
        "QSFP P26:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x62,
        6,
        1,
    },
    {
        "qsfp_p27_intr",
        "QSFP P27:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x62,
        5,
        1,
    },
    {
        "qsfp_p28_intr",
        "QSFP P28:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x62,
        4,
        1,
    },
    {
        "qsfp_p29_intr",
        "QSFP P29:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x62,
        3,
        1,
    },
    {
        "qsfp_p30_intr",
        "QSFP P30:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x62,
        2,
        1,
    },
    {
        "qsfp_p31_intr",
        "QSFP P31 :\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x62,
        1,
        1,
    },
    {
        "qsfp_p32_intr",
        "QSFP P32 :\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x62,
        0,
        1,
    },
    {
        "sfp_p0_modprs",
        "SFP P0 :\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x71,
        6,
        1,
    },
    {
        "sfp_p0_rxlos",
        "SFP P0 :\n" rx_loss_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x71,
        5,
        1,
    },
    {
        "sfp_p0_txfault",
        "SFP P0 :\n" tx_fault_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x71,
        4,
        1,
    },
    {
        "sfp_p1_modprs",
        "SFP P1 :\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x71,
        2,
        1,
    },
    {
        "sfp_p1_rxlos",
        "SFP P1 :\n" rx_loss_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x71,
        1,
        1,
    },
    {
        "sfp_p1_txfault",
        "SFP P1 :\n" tx_fault_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x71,
        0,
        1,
    },
    {
        "sfp_p0_txdis",
        "SFP P0 :\n" opt_dis_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x72,
        7,
        1,
    },
    {
        "sfp_p1_txdis",
        "SFP P1 :\n" opt_dis_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x72,
        3,
        1,
    },

};

static i2c_dev_data_st swpld_3_data;

/*
 * SWPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {0x35, I2C_CLIENT_END};

/* SWPLD id */
static const struct i2c_device_id swpld_3_id[] = {
    {"swpld_3", 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, swpld_3_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int swpld_3_detect(
    struct i2c_client* client,
    struct i2c_board_info* info) {
  /*
   * We don't currently do any detection of SWPLD
   */
  strlcpy(info->type, "swpld_3", I2C_NAME_SIZE);
  return 0;
}

static int swpld_3_probe(
    struct i2c_client* client,
    const struct i2c_device_id* id) {
  int n_attrs = sizeof(swpld_3_attr_table) / sizeof(swpld_3_attr_table[0]);
  return i2c_dev_sysfs_data_init(
      client, &swpld_3_data, swpld_3_attr_table, n_attrs);
}

static int swpld_3_remove(struct i2c_client* client) {
  i2c_dev_sysfs_data_clean(client, &swpld_3_data);
  return 0;
}

static struct i2c_driver swpld_3_driver = {
    .class = I2C_CLASS_HWMON,
    .driver =
        {
            .name = "swpld_3",
        },
    .probe = swpld_3_probe,
    .remove = swpld_3_remove,
    .id_table = swpld_3_id,
    .detect = swpld_3_detect,
    .address_list = normal_i2c,
};

static int __init swpld_3_mod_init(void) {
  return i2c_add_driver(&swpld_3_driver);
}

static void __exit swpld_3_mod_exit(void) {
  i2c_del_driver(&swpld_3_driver);
}

MODULE_AUTHOR("Leila Lin");
MODULE_DESCRIPTION("SWPLD3 Driver");
MODULE_LICENSE("GPL");

module_init(swpld_3_mod_init);
module_exit(swpld_3_mod_exit);
