/*
 * delta_swpld2.c - The i2c driver for AGC032A SWPLD2
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

static const i2c_dev_attr_st swpld_2_attr_table[] = {
    {
        "swpld2_ver_type",
        "SWPLD2 Version Type",
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x1,
        7,
        1,
    },
    {
        "swpld2_ver",
        "SWPLD2 Version",
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x1,
        0,
        6,
    },
    {
        "qsfp_p01_rst",
        "QSFP P01:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        7,
        1,
    },
    {
        "qsfp_p02_rst",
        "QSFP P02:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        6,
        1,
    },
    {
        "qsfp_p03_rst",
        "QSFP P03:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        5,
        1,
    },
    {
        "qsfp_p04_rst",
        "QSFP P04:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        4,
        1,
    },
    {
        "qsfp_p05_rst",
        "QSFP P05:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        3,
        1,
    },
    {
        "qsfp_p06_rst",
        "QSFP P06:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        2,
        1,
    },
    {
        "qsfp_p07_rst",
        "QSFP P07 :\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        1,
        1,
    },
    {
        "qsfp_p08_rst",
        "QSFP P08 :\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        0,
        1,
    },
    {
        "qsfp_p09_rst",
        "QSFP P09:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        7,
        1,
    },
    {
        "qsfp_p10_rst",
        "QSFP P10:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        6,
        1,
    },
    {
        "qsfp_p11_rst",
        "QSFP P11:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        5,
        1,
    },
    {
        "qsfp_p12_rst",
        "QSFP P12:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        4,
        1,
    },
    {
        "qsfp_p13_rst",
        "QSFP P13:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        3,
        1,
    },
    {
        "qsfp_p14_rst",
        "QSFP P14:\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        2,
        1,
    },
    {
        "qsfp_p15_rst",
        "QSFP P15 :\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        1,
        1,
    },
    {
        "qsfp_p16_rst",
        "QSFP P16 :\n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        0,
        1,
    },
    {
        "qsfp_p01_lpmode",
        "QSFP P01:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x21,
        7,
        1,
    },
    {
        "qsfp_p02_lpmode",
        "QSFP P02:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x21,
        6,
        1,
    },
    {
        "qsfp_p03_lpmode",
        "QSFP P03:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x21,
        5,
        1,
    },
    {
        "qsfp_p04_lpmode",
        "QSFP P04:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x21,
        4,
        1,
    },
    {
        "qsfp_p05_lpmode",
        "QSFP P05:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x21,
        3,
        1,
    },
    {
        "qsfp_p06_lpmode",
        "QSFP P06:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x21,
        2,
        1,
    },
    {
        "qsfp_p07_lpmode",
        "QSFP P07 :\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x21,
        1,
        1,
    },
    {
        "qsfp_p08_lpmode",
        "QSFP P08 :\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x21,
        0,
        1,
    },
    {
        "qsfp_p09_lpmode",
        "QSFP P09:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x22,
        7,
        1,
    },
    {
        "qsfp_p10_lpmode",
        "QSFP P10:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x22,
        6,
        1,
    },
    {
        "qsfp_p11_lpmode",
        "QSFP P11:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x22,
        5,
        1,
    },
    {
        "qsfp_p12_lpmode",
        "QSFP P12:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x22,
        4,
        1,
    },
    {
        "qsfp_p13_lpmode",
        "QSFP P13:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x22,
        3,
        1,
    },
    {
        "qsfp_p14_lpmode",
        "QSFP P14:\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x22,
        2,
        1,
    },
    {
        "qsfp_p15_lpmode",
        "QSFP P15 :\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x22,
        1,
        1,
    },
    {
        "qsfp_p16_lpmode",
        "QSFP P16 :\n" lp_mode_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x22,
        0,
        1,
    },
    {
        "qsfp_p01_modprs_set",
        "QSFP P01:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x31,
        7,
        1,
    },
    {
        "qsfp_p02_modprs_set",
        "QSFP P02:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x31,
        6,
        1,
    },
    {
        "qsfp_p03_modprs_set",
        "QSFP P03:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x31,
        5,
        1,
    },
    {
        "qsfp_p04_modprs_set",
        "QSFP P04:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x31,
        4,
        1,
    },
    {
        "qsfp_p05_modprs_set",
        "QSFP P05:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x31,
        3,
        1,
    },
    {
        "qsfp_p06_modprs_set",
        "QSFP P06:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x31,
        2,
        1,
    },
    {
        "qsfp_p07_modprs_set",
        "QSFP P07 :\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x31,
        1,
        1,
    },
    {
        "qsfp_p08_modprs_set",
        "QSFP P08 :\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x31,
        0,
        1,
    },
    {
        "qsfp_p09_modprs_set",
        "QSFP P09:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x32,
        7,
        1,
    },
    {
        "qsfp_p10_modprs_set",
        "QSFP P10:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x32,
        6,
        1,
    },
    {
        "qsfp_p11_modprs_set",
        "QSFP P11:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x32,
        5,
        1,
    },
    {
        "qsfp_p12_modprs_set",
        "QSFP P12:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x32,
        4,
        1,
    },
    {
        "qsfp_p13_modprs_set",
        "QSFP P13:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x32,
        3,
        1,
    },
    {
        "qsfp_p14_modprs_set",
        "QSFP P14:\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x32,
        2,
        1,
    },
    {
        "qsfp_p15_modprs_set",
        "QSFP P15 :\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x32,
        1,
        1,
    },
    {
        "qsfp_p16_modprs_set",
        "QSFP P16 :\n" respond_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x32,
        0,
        1,
    },
    {
        "qsfp_p01_modprs",
        "QSFP P01:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x51,
        7,
        1,
    },
    {
        "qsfp_p02_modprs",
        "QSFP P02:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x51,
        6,
        1,
    },
    {
        "qsfp_p03_modprs",
        "QSFP P03:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x51,
        5,
        1,
    },
    {
        "qsfp_p04_modprs",
        "QSFP P04:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x51,
        4,
        1,
    },
    {
        "qsfp_p05_modprs",
        "QSFP P05:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x51,
        3,
        1,
    },
    {
        "qsfp_p06_modprs",
        "QSFP P06:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x51,
        2,
        1,
    },
    {
        "qsfp_p07_modprs",
        "QSFP P07 :\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x51,
        1,
        1,
    },
    {
        "qsfp_p08_modprs",
        "QSFP P08 :\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x51,
        0,
        1,
    },
    {
        "qsfp_p09_modprs",
        "QSFP P09:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x52,
        7,
        1,
    },
    {
        "qsfp_p10_modprs",
        "QSFP P10:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x52,
        6,
        1,
    },
    {
        "qsfp_p11_modprs",
        "QSFP P11:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x52,
        5,
        1,
    },
    {
        "qsfp_p12_modprs",
        "QSFP P12:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x52,
        4,
        1,
    },
    {
        "qsfp_p13_modprs",
        "QSFP P13:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x52,
        3,
        1,
    },
    {
        "qsfp_p14_modprs",
        "QSFP P14:\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x52,
        2,
        1,
    },
    {
        "qsfp_p15_modprs",
        "QSFP P15 :\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x52,
        1,
        1,
    },
    {
        "qsfp_p16_modprs",
        "QSFP P16 :\n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x52,
        0,
        1,
    },
    {
        "qsfp_p01_intr",
        "QSFP P01:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x61,
        7,
        1,
    },
    {
        "qsfp_p02_intr",
        "QSFP P02:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x61,
        6,
        1,
    },
    {
        "qsfp_p03_intr",
        "QSFP P03:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x61,
        5,
        1,
    },
    {
        "qsfp_p04_intr",
        "QSFP P04:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x61,
        4,
        1,
    },
    {
        "qsfp_p05_intr",
        "QSFP P05:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x61,
        3,
        1,
    },
    {
        "qsfp_p06_intr",
        "QSFP P06:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x61,
        2,
        1,
    },
    {
        "qsfp_p07_intr",
        "QSFP P07 :\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x61,
        1,
        1,
    },
    {
        "qsfp_p08_intr",
        "QSFP P08 :\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x61,
        0,
        1,
    },
    {
        "qsfp_p09_intr",
        "QSFP P09:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x62,
        7,
        1,
    },
    {
        "qsfp_p10_intr",
        "QSFP P10:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x62,
        6,
        1,
    },
    {
        "qsfp_p11_intr",
        "QSFP P11:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x62,
        5,
        1,
    },
    {
        "qsfp_p12_intr",
        "QSFP P12:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x62,
        4,
        1,
    },
    {
        "qsfp_p13_intr",
        "QSFP P13:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x62,
        3,
        1,
    },
    {
        "qsfp_p14_intr",
        "QSFP P14:\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x62,
        2,
        1,
    },
    {
        "qsfp_p15_intr",
        "QSFP P15 :\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x62,
        1,
        1,
    },
    {
        "qsfp_p16_intr",
        "QSFP P16 :\n" assert_intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x62,
        0,
        1,
    },

};

static i2c_dev_data_st swpld_2_data;

/*
 * SWPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {0x34, I2C_CLIENT_END};

/* SWPLD id */
static const struct i2c_device_id swpld_2_id[] = {
    {"swpld_2", 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, swpld_2_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int swpld_2_detect(
    struct i2c_client* client,
    struct i2c_board_info* info) {
  /*
   * We don't currently do any detection of SWPLD
   */
  strlcpy(info->type, "swpld_2", I2C_NAME_SIZE);
  return 0;
}

static int swpld_2_probe(
    struct i2c_client* client,
    const struct i2c_device_id* id) {
  int n_attrs = sizeof(swpld_2_attr_table) / sizeof(swpld_2_attr_table[0]);
  return i2c_dev_sysfs_data_init(
      client, &swpld_2_data, swpld_2_attr_table, n_attrs);
}

static int swpld_2_remove(struct i2c_client* client) {
  i2c_dev_sysfs_data_clean(client, &swpld_2_data);
  return 0;
}

static struct i2c_driver swpld_2_driver = {
    .class = I2C_CLASS_HWMON,
    .driver =
        {
            .name = "swpld_2",
        },
    .probe = swpld_2_probe,
    .remove = swpld_2_remove,
    .id_table = swpld_2_id,
    .detect = swpld_2_detect,
    .address_list = normal_i2c,
};

static int __init swpld_2_mod_init(void) {
  return i2c_add_driver(&swpld_2_driver);
}

static void __exit swpld_2_mod_exit(void) {
  i2c_del_driver(&swpld_2_driver);
}

MODULE_AUTHOR("Leila Lin");
MODULE_DESCRIPTION("SWPLD2 Driver");
MODULE_LICENSE("GPL");

module_init(swpld_2_mod_init);
module_exit(swpld_2_mod_exit);
