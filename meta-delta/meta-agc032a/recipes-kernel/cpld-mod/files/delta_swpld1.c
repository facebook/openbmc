/*
 * delta_swpld1.c - The i2c driver for AGC032A SWPLD1
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

#define avs_help_str "MAC ROV value for VCC_MAC_AVS_0V91 voltage\n"

#define reset_help_str                            \
  "0: Reset\n"                                    \
  "1: Normal Operation\n"

#define present_help_str                          \
  "0: Present\n"                                  \
  "1: Not Present\n"

#define pwr_state_help_str                        \
  "0: Power Good\n"                               \
  "1: Power Not Good\n"

#define alert_help_str                            \
  "0: Alert Active\n"                             \
  "1: Alert Not Active\n"

#define smb_alert_help_str                        \
  "0: Alert\n"                                    \
  "1: Normal Operation\n"

#define vr_alert_help_str                         \
  "0: OC/OV/OP\n"                                 \
  "1: Normal Operation\n"

#define enable_help_str                           \
  "0: Enable\n"                                   \
  "1: Disable\n"

#define eeprom_wp_help_str                        \
  "0: Enable EEPROM Write Protect\n"              \
  "1: Disable EEPROM Write Protect\n"

#define pwr_rail_help_str                          \
  "0: Power Rail Not Good\n"                       \
  "1: Power Rail Good\n"

#define lp_mode_help_str                           \
  "0: Not LP Mode\n"                               \
  "1: LP Mode\n"

#define intr_help_str                              \
  "0: Interrupt Occured\n"                         \
  "1: Interrupt Not Occured\n"

#define assert_intr_help_str                       \
  "0: Assert Interrupt \n"                         \
  "1: Not Assert Intrrupt\n"

/* LED */
#define led_psu_help_str                           \
  "0: Auto\n"                                      \
  "1: Green\n"                                     \
  "2: Red\n"                                       \
  "3: Off\n"

#define led_sys_help_str                           \
  "0: Off\n"                                       \
  "1: Green\n"                                     \
  "2: Blink Green\n"                               \
  "3: Red\n"

#define led_fan_1_help_str                         \
  "0: Off\n"                                       \
  "1: Green\n"                                     \
  "2: Amber\n"                                     \
  "3: Off\n"

#define led_fan_2_help_str                         \
  "0: Off\n"                                       \
  "1: Green\n"                                     \
  "2: Red\n"                                       \
  "3: Off\n"

static const i2c_dev_attr_st swpld_1_attr_table[] = {
    {
        "board_id",
        "Board ID",
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x1,
        0,
        8,
    },
    {
        "board_ver",
        "Board Version",
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x2,
        0,
        8,
    },
    {
        "swpld1_ver_type",
        "SWPLD1 Version Type",
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x3,
        7,
        1,
    },
    {
        "swpld1_ver",
        "SWPLD1 Version",
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x3,
        0,
        7,
    },
    {
        "usb_hub_rst",
        "USB: \n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x6,
        6,
        1,
    },
    {
        "synce_rst",
        "SyncE: \n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x6,
        5,
        1,
    },
    {
        "bmc_rst",
        "BMC: \n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x6,
        4,
        1,
    },
    {
        "lpc_rst",
        "BMC LPC: \n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x6,
        3,
        1,
    },
    {
        "oob_pcie_rst",
        "OOB PCIE: \n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x6,
        2,
        1,
    },
    {
        "mac_pcie_rst",
        "MAC PCIE: \n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x6,
        1,
        1,
    },
    {
        "mac_rst",
        "MAC: \n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x6,
        0,
        1,
    },
    {
        "vr_3v3_rst",
        "VR 3V3: \n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x7,
        3,
        1,
    },
    {
        "vr_0v8_rst",
        "VR 0V8: \n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x7,
        2,
        1,
    },
    {
        "fan_mux_rst",
        "FAN I2C MUX: \n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x8,
        6,
        1,
    },
    {
        "qsfp5_mux_rst",
        "QSFP5: \n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x8,
        5,
        1,
    },
    {
        "qsfp4_mux_rst",
        "QSFP4: \n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x8,
        4,
        1,
    },
    {
        "qsfp3_mux_rst",
        "QSFP3: \n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x8,
        3,
        1,
    },
    {
        "qsfp2_mux_rst",
        "QSFP2: \n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x8,
        2,
        1,
    },
    {
        "qsfp1_mux_rst",
        "QSFP1: \n" reset_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x8,
        1,
        1,
    },
    {
        "psu1_present",
        "PSU1: \n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x11,
        6,
        1,
    },
    {
        "psu1_state",
        "PSU1: \n" pwr_state_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x11,
        5,
        1,
    },
    {
        "psu1_alert",
        "PSU1: \n" alert_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x11,
        4,
        1,
    },
    {
        "psu2_present",
        "PSU2: \n" present_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x11,
        2,
        1,
    },
    {
        "psu2_state",
        "PSU2: \n" pwr_state_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x11,
        1,
        1,
    },
    {
        "psu2_alert",
        "PSU2: \n" alert_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x11,
        0,
        1,
    },
    {
        "psu1_en",
        "PSU1: \n" enable_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        7,
        1,
    },
    {
        "psu1_eeprom_wp",
        "PSU1: \n" eeprom_wp_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        6,
        1,
    },
    {
        "psu2_en",
        "PSU2: \n" enable_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x12,
        3,
        1,
    },
    {
        "psu2_eeprom_wp",
        "PSU2: \n" eeprom_wp_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x11,
        2,
        1,
    },
    {
        "vcc_5v_pwr",
        "VCC 5V: \n" pwr_rail_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x21,
        7,
        1,
    },
    {
        "vcc_3v3_pwr",
        "VCC 3V3: \n" pwr_rail_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x21,
        6,
        1,
    },
    {
        "vcc_3v_pwr",
        "VCC 3V: \n" pwr_rail_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x21,
        5,
        1,
    },
    {
        "vcc_2V5_pwr",
        "VCC 2V5: \n" pwr_rail_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x21,
        4,
        1,
    },
    {
        "vcc_mac_1v8_pwr",
        "VCC MAC 1V8: \n" pwr_rail_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x21,
        3,
        1,
    },
    {
        "vcc_bmc_1v2_pwr",
        "VCC BMC 1V2: \n" pwr_rail_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x21,
        2,
        1,
    },
    {
        "vcc_bmc_1v15_pwr",
        "VCC BMC 1V15: \n" pwr_rail_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x21,
        1,
        1,
    },
    {
        "vcc_mac_1v2_pwr",
        "VCC MAC 1V2: \n" pwr_rail_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x21,
        0,
        1,
    },
    {
        "vcc_mac_avs_0v91_pwr",
        "MAC AVS 0V91: \n" pwr_rail_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x22,
        6,
        1,
    },
    {
        "vcc_mac_0v8_pwr",
        "VCC MAC 0V8: \n" pwr_rail_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x22,
        5,
        1,
    },
    {
        "vcc_mac_pll_0v8_pwr",
        "VCC MAC PLL 0V8: \n" pwr_rail_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x22,
        4,
        1,
    },
    {
        "vcc_5v_en",
        "VCC 5V: \n" enable_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x23,
        7,
        1,
    },
    {
        "vcc_3v3_en",
        "VCC 3V3: \n" enable_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x23,
        6,
        1,
    },
    {
        "vcc_vcore_en",
        "VCC MAC AVS 0V91: \n" enable_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x23,
        5,
        1,
    },
    {
        "vcc_mac_1v8_en",
        "VCC MAC 1V8: \n" enable_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x23,
        4,
        1,
    },
    {
        "vcc_mac_1v2_en",
        "VCC MAC 1V2: \n" enable_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x23,
        3,
        1,
    },
    {
        "vcc_mac_0v8_en",
        "VCC MAC 0V8: \n" enable_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x23,
        2,
        1,
    },
    {
        "vcc_pll_0v8_en",
        "VCC PLL 0V8: \n" enable_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x23,
        1,
        1,
    },
    {
        "vcc_sys_en",
        "VCC SYS: \n" enable_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x23,
        0,
        1,
    },
    {
        "oob_pwr_state",
        "I210 Power: \n" pwr_rail_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x24,
        7,
        1,
    },
    {
        "oob_op_en",
        "I210 OOB Operation: \n" enable_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x24,
        6,
        1,
    },
    {
        "usb1_op_en",
        "USB1 Operation: \n" enable_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x24,
        1,
        1,
    },
    {
        "rov_avs7",
        "BCM_AVS7" avs_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x25,
        7,
        1,
    },
    {
        "rov_avs6",
        "BCM_AVS6" avs_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x25,
        6,
        1,
    },
    {
        "rov_avs5",
        "BCM_AVS5" avs_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x25,
        5,
        1,
    },
    {
        "rov_avs4",
        "BCM_AVS4" avs_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x25,
        4,
        1,
    },
    {
        "rov_avs3",
        "BCM_AVS3" avs_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x25,
        3,
        1,
    },
    {
        "rov_avs2",
        "BCM_AVS2" avs_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x25,
        2,
        1,
    },
    {
        "rov_avs1",
        "BCM_AVS1" avs_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x25,
        1,
        1,
    },
    {
        "rov_avs0",
        "BCM_AVS0" avs_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x25,
        0,
        1,
    },
    {
        "psu_intr",
        "PSU: \n" intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x26,
        1,
        1,
    },
    {
        "fan_alert",
        "FAN: \n" intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x26,
        0,
        1,
    },
    {
        "swpld2_intr",
        "SWPLD2: \n" intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x27,
        1,
        1,
    },
    {
        "swpld3_intr",
        "SWPLD3: \n" intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x27,
        0,
        1,
    },
    {
        "cpld_psu_fan_event",
        "MB to CPU PSU/FAN: \n" intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x28,
        2,
        1,
    },
    {
        "cpld_op_mod_event",
        "MB to CPU OP Module: \n" intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x28,
        1,
        1,
    },
    {
        "cpld_misc_intr",
        "MB to CPU MISC: \n" intr_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x28,
        0,
        1,
    },
    {
        "smb_mac_vr_0v8_alert",
        "VR MAC 0V8 SMB: \n" smb_alert_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x29,
        7,
        1,
    },
    {
        "smb_mac_vr_avs_0v91_alert",
        "VR MAC AVS 0V91 SMB: \n" smb_alert_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x29,
        6,
        1,
    },
    {
        "smb_mac_vr_3v3_alert",
        "VR MAC 3V3 SMB: \n" smb_alert_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x29,
        5,
        1,
    },
    {
        "smb_cpld_i210_alert",
        "CPLD I210 SMB: \n" smb_alert_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x29,
        4,
        1,
    },
    {
        "smb_synce_intr",
        "SYNCE Interrupt SMB: \n" smb_alert_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        NULL,
        0x29,
        3,
        1,
    },
    {
        "psu1_led",
        "PSU1 LED: \n" led_psu_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x41,
        2,
        2,
    },
    {
        "psu2_led",
        "PSU2 LED: \n" led_psu_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x41,
        0,
        2,
    },
    {
        "sys_led",
        "SYS LED: \n" led_sys_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x42,
        2,
        2,
    },
    {
        "fan_led",
        "FAN LED: \n" led_fan_1_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x42,
        0,
        2,
    },
    {
        "fan1_led",
        "FAN1 LED: \n" led_fan_2_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x46,
        6,
        2,
    },
    {
        "fan2_led",
        "FAN2 LED: \n" led_fan_2_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x46,
        4,
        2,
    },
    {
        "fan3_led",
        "FAN3 LED: \n" led_fan_2_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x46,
        2,
        2,
    },
    {
        "fan4_led",
        "FAN4 LED: \n" led_fan_2_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x46,
        0,
        2,
    },
    {
        "fan5_led",
        "FAN5 LED: \n" led_fan_2_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x47,
        6,
        2,
    },
    {
        "fan6_led",
        "FAN6 LED: \n" led_fan_2_help_str,
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x47,
        4,
        2,
    },
    {
        "synce_eeprom_wp",
        "SYNCE EEPROM Write Protection: \n0: Normal Write Operation\n1: Inhibited Write Operation\n",
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x51,
        2,
        1,
    },
    {
        "console_sel",
        "CPU/BMC UART MUX selection: \n0: Switch to BMC console\n1: Switch to CPU console\n",
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x51,
        1,
        1,
    },
    {
        "sys_eeprom_wp",
        "System EEPROM Write Protection: \n0: Normal Write Operation\n1: Inhibited Write Operation\n",
        I2C_DEV_ATTR_SHOW_DEFAULT,
        I2C_DEV_ATTR_STORE_DEFAULT,
        0x51,
        0,
        1,
    },

};

static i2c_dev_data_st swpld_1_data;

/*
 * SWPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {0x32, I2C_CLIENT_END};

/* SWPLD id */
static const struct i2c_device_id swpld_1_id[] = {
    {"swpld_1", 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, swpld_1_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int swpld_1_detect(
    struct i2c_client* client,
    struct i2c_board_info* info) {
  /*
   * We don't currently do any detection of SWPLD
   */
  strlcpy(info->type, "swpld_1", I2C_NAME_SIZE);
  return 0;
}

static int swpld_1_probe(
    struct i2c_client* client,
    const struct i2c_device_id* id) {
  int n_attrs = sizeof(swpld_1_attr_table) / sizeof(swpld_1_attr_table[0]);
  return i2c_dev_sysfs_data_init(
      client, &swpld_1_data, swpld_1_attr_table, n_attrs);
}

static int swpld_1_remove(struct i2c_client* client) {
  i2c_dev_sysfs_data_clean(client, &swpld_1_data);
  return 0;
}

static struct i2c_driver swpld_1_driver = {
    .class = I2C_CLASS_HWMON,
    .driver =
        {
            .name = "swpld_1",
        },
    .probe = swpld_1_probe,
    .remove = swpld_1_remove,
    .id_table = swpld_1_id,
    .detect = swpld_1_detect,
    .address_list = normal_i2c,
};

static int __init swpld_1_mod_init(void) {
  return i2c_add_driver(&swpld_1_driver);
}

static void __exit swpld_1_mod_exit(void) {
  i2c_del_driver(&swpld_1_driver);
}

MODULE_AUTHOR("Leila Lin");
MODULE_DESCRIPTION("SWPLD1 Driver");
MODULE_LICENSE("GPL");

module_init(swpld_1_mod_init);
module_exit(swpld_1_mod_exit);
