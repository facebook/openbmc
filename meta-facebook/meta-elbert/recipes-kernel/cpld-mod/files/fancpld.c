/*
 * fancpld.c - The i2c driver for the CPLD on fan board
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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
  "0x0: present\n"                              \
  "0x1: not present"

#define led_bit_help_str                        \
  "0x0: LED On\n"                               \
  "0x1: LED Off"

#define fan_ok_bit_help_str                     \
  "0x0: Fan fault occured\n"                    \
  "0x1: Fan functions normally."

#define fan_id_change_bit_help_str              \
  "Fan ID change: Clear on writing '1'\n"       \
  "0x0: Fan ID changed\n"                       \
  "0x1: Fan ID did not change."

#define fan_change_latched                      \
  "0x1: Change latched (W0C)\n"                 \
  "0x0: No change"

#define ELBERT_FAN_RPM_CONSTANT 3000000 // 2 TACH pulses per revolution
#define ELBERT_FAN1_RPM 0x11
#define ELBERT_FAN2_RPM 0x21
#define ELBERT_FAN3_RPM 0x31
#define ELBERT_FAN4_RPM 0x41
#define ELBERT_FAN5_RPM 0x51
#define ELBERT_FAN_INSTANCES 5
#define ELBERT_FAN_PRESENCE 0x70

static bool detect_fan_presence(struct device *dev,
                                struct device_attribute *attr,
                                char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int instance = -1, i = 0;
  int ret_val = 0;
  int elbert_fan_map[ELBERT_FAN_INSTANCES] = { ELBERT_FAN1_RPM, ELBERT_FAN2_RPM,
                                               ELBERT_FAN3_RPM, ELBERT_FAN4_RPM,
                                               ELBERT_FAN5_RPM };
  int reg_addr = dev_attr->ida_reg;
  // Based on register addr, try detecting instance number
  for (i = 0; i < ELBERT_FAN_INSTANCES; i++) {
    if (reg_addr == elbert_fan_map[i])
      instance = i;
  }

  // If the address is none of the fan rpm register, it's an invalid input.
  // Therefore, just return false.
  if (instance == -1) {
    return false;
  }

  // Now, read fan presence register
  ret_val = i2c_smbus_read_byte_data(client, ELBERT_FAN_PRESENCE);
  if (ret_val < 0)
    return false;

  // Check if our instance of fan exists, if so return true
  if (ret_val & (1 << instance))
    return true;

  // Fan not present. Return false
  return false;
}

// This is RPM SHOW function tailored for ELBERT. The function will
// first detect fan presence, and if the fan is present, it will
// read RPM register, calculate the RPM and return the value.
// Otherwise, it returns error.
static ssize_t fancpld_fan_rpm_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  int val;

  // In ELBERT, when a fan is missing or dead, FANCPLD will
  // still show the RPM value from the last successful read,
  // which is not correct. So we check if the fan is present
  // , and will return error if the fan is not present.
  if (!detect_fan_presence(dev, attr, buf))
    return -EINVAL;

  // Fan exists. Move ahead to read RPM register
  val = i2c_dev_read_word_littleendian(dev, attr);
  if (val < 0) {
    return val;
  }

  // Follow the formula from ELBERT fan controller manual
  val = ELBERT_FAN_RPM_CONSTANT / val;

  return scnprintf(buf, PAGE_SIZE, "%u\n", val);
}

// FANCPLD PWM STORE function. If the RPM value is not within [0,255] range,
// change the value to fit into this range. For example, if a user try to
// write 280, it will still write 255, instead of writing 280 to cause
// overflow, which ends up writing 24 (280 - 256) as PWM.
// This behavior is required by FSCD boost algorithm.
static ssize_t fancpld_fan_pwm_store(struct device *dev,
                                     struct device_attribute *attr,
                                     const char *buf, size_t count)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int val;
  int req_val;

  /* parse the buffer */
  if (sscanf(buf, "%i", &req_val) <= 0) {
    return -EINVAL;
  }
  if (req_val > 255)
    req_val=255;
  if (req_val < 0)
    req_val = 0;

  mutex_lock(&data->idd_lock);
  val = i2c_smbus_write_byte_data(client, dev_attr->ida_reg, req_val);
  mutex_unlock(&data->idd_lock);

  if (val < 0) {
    /* error case */
    return val;
  }
  return count;
}

static const i2c_dev_attr_st fancpld_attr_table[] = {
  {
    "cpld_ver_minor",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x0, 0, 8,
  },
  {
    "cpld_ver_major",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x1, 0, 8,
  },
  {
    "scratch",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x2, 0, 8,
  },
  {
    "fan1_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    ELBERT_FAN1_RPM, 0, 8,
  },
  {
    "fan2_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    ELBERT_FAN2_RPM, 0, 8,
  },
  {
    "fan3_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    ELBERT_FAN3_RPM, 0, 8,
  },
  {
    "fan4_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    ELBERT_FAN4_RPM, 0, 8,
  },
  {
    "fan5_input",
    NULL,
    fancpld_fan_rpm_show,
    NULL,
    ELBERT_FAN5_RPM, 0, 8,
  },
  {
    "fan1_tach",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    ELBERT_FAN1_RPM, 0, 8,
  },
  {
    "fan2_tach",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    ELBERT_FAN2_RPM, 0, 8,
  },
  {
    "fan3_tach",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    ELBERT_FAN3_RPM, 0, 8,
  },
  {
    "fan4_tach",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    ELBERT_FAN4_RPM, 0, 8,
  },
  {
    "fan5_tach",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    ELBERT_FAN5_RPM, 0, 8,
  },
  {
    "fan1_pwm",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    fancpld_fan_pwm_store,
    0x10, 0, 8,
  },
  {
    "fan2_pwm",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    fancpld_fan_pwm_store,
    0x20, 0, 8,
  },
  {
    "fan3_pwm",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    fancpld_fan_pwm_store,
    0x30, 0, 8,
  },
  {
    "fan4_pwm",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    fancpld_fan_pwm_store,
    0x40, 0, 8,
  },
  {
    "fan5_pwm",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    fancpld_fan_pwm_store,
    0x50, 0, 8,
  },
  {
    "fan1_id",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x61, 0, 8,
  },
  {
    "fan2_id",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x62, 0, 8,
  },
  {
    "fan3_id",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x63, 0, 8,
  },
  {
    "fan4_id",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x64, 0, 8,
  },
  {
    "fan5_id",
    NULL,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x65, 0, 8,
  },
  {
    "fan1_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    ELBERT_FAN_PRESENCE, 0, 1,
  },
  {
    "fan2_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    ELBERT_FAN_PRESENCE, 1, 1,
  },
  {
    "fan3_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    ELBERT_FAN_PRESENCE, 2, 1,
  },
  {
    "fan4_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    ELBERT_FAN_PRESENCE, 3, 1,
  },
  {
    "fan5_present",
    present_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    ELBERT_FAN_PRESENCE, 4, 1,
  },
  {
    "fan1_ok",
    fan_ok_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x71, 0, 1,
  },
  {
    "fan2_ok",
    fan_ok_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x71, 1, 1,
  },
  {
    "fan3_ok",
    fan_ok_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x71, 2, 1,
  },
  {
    "fan4_ok",
    fan_ok_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x71, 3, 1,
  },
  {
    "fan5_ok",
    fan_ok_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x71, 4, 1,
  },
  {
    "fan1_led_blue",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x73, 0, 1,
  },
  {
    "fan2_led_blue",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x73, 1, 1,
  },
  {
    "fan3_led_blue",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x73, 2, 1,
  },
  {
    "fan4_led_blue",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x73, 3, 1,
  },
  {
    "fan5_led_blue",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x73, 4, 1,
  },
  {
    "fan1_led_amber",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x74, 0, 1,
  },
  {
    "fan2_led_amber",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x74, 1, 1,
  },
  {
    "fan3_led_amber",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x74, 2, 1,
  },
  {
    "fan4_led_amber",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x74, 3, 1,
  },
  {
    "fan5_led_amber",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x74, 4, 1,
  },
  {
    "fan1_led_green",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x75, 0, 1,
  },
  {
    "fan2_led_green",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x75, 1, 1,
  },
  {
    "fan3_led_green",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x75, 2, 1,
  },
  {
    "fan4_led_green",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x75, 3, 1,
  },
  {
    "fan5_led_green",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x75, 4, 1,
  },
  {
    "fan1_led_red",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x76, 0, 1,
  },
  {
    "fan2_led_red",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x76, 1, 1,
  },
  {
    "fan3_led_red",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x76, 2, 1,
  },
  {
    "fan4_led_red",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x76, 3, 1,
  },
  {
    "fan5_led_red",
    led_bit_help_str,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x76, 4, 1,
  },
  {
    "fan_ok_change_interrupt",
    "One of the fans experienced change in FAN ok\n"
    "0x1: One fan experienced change in FAN ok\n"
    "0x0: No change in any FAN ok\n" ,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x77, 0, 1,
  },
  {
    "fan_presence_change_interrupt",
    "One of the fans experienced change in FAN presence\n"
    "0x1: One fan experienced change in FAN presence\n"
    "0x0: No change in any FAN presence\n" ,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x77, 1, 1,
  },
  {
    "fan_id_change_interrupt",
    "One of the fans experienced change in FAN ID\n"
    "0x1: One fan experienced change in FAN ID\n"
    "0x0: No change in any FAN ID\n" ,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x77, 2, 1,
  },
  {
    "fan1_id_change",
    "(Write 1 to clear)"
    fan_change_latched,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x78, 0, 1,
  },
  {
    "fan2_id_change",
    "(Write 1 to clear)"
    fan_change_latched,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x78, 1, 1,
  },
  {
    "fan3_id_change",
    "(Write 1 to clear)"
    fan_change_latched,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x78, 2, 1,
  },
  {
    "fan4_id_change",
    "(Write 1 to clear)"
    fan_change_latched,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x78, 3, 1,
  },
  {
    "fan5_id_change",
    "(Write 1 to clear)"
    fan_change_latched,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x78, 4, 1,
  },
  {
    "fan1_present_change",
    "(Write 1 to clear)"
    fan_change_latched,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x80, 0, 1,
  },
  {
    "fan2_present_change",
    "(Write 1 to clear)"
    fan_change_latched,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x80, 1, 1,
  },
  {
    "fan3_present_change",
    "(Write 1 to clear)"
    fan_change_latched,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x80, 2, 1,
  },
  {
    "fan4_present_change",
    "(Write 1 to clear)"
    fan_change_latched,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x80, 3, 1,
  },
  {
    "fan5_present_change",
    "(Write 1 to clear)"
    fan_change_latched,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    0x80, 4, 1,
  },
  {
    "fan1_ok_change",
    fan_change_latched,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x82, 0, 1,
  },
  {
    "fan2_ok_change",
    fan_change_latched,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x82, 1, 1,
  },
  {
    "fan3_ok_change",
    fan_change_latched,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x82, 2, 1,
  },
  {
    "fan4_ok_change",
    fan_change_latched,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x82, 3, 1,
  },
  {
    "fan5_ok_change",
    fan_change_latched,
    I2C_DEV_ATTR_SHOW_DEFAULT,
    NULL,
    0x82, 4, 1,
  }
};

static i2c_dev_data_st fancpld_data;

/*
 * FANCPLD i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x60, I2C_CLIENT_END
};

/* FANCPLD id */
static const struct i2c_device_id fancpld_id[] = {
  { "fancpld", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, fancpld_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int fancpld_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the FANCPLD
   */
  strlcpy(info->type, "fancpld", I2C_NAME_SIZE);
  return 0;
}

static int fancpld_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(fancpld_attr_table) / sizeof(fancpld_attr_table[0]);
  return i2c_dev_sysfs_data_init(client, &fancpld_data,
                                 fancpld_attr_table, n_attrs);
}

static int fancpld_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &fancpld_data);
  return 0;
}

static struct i2c_driver fancpld_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "fancpld",
  },
  .probe    = fancpld_probe,
  .remove   = fancpld_remove,
  .id_table = fancpld_id,
  .detect   = fancpld_detect,
  .address_list = normal_i2c,
};

static int __init fancpld_mod_init(void)
{
  return i2c_add_driver(&fancpld_driver);
}

static void __exit fancpld_mod_exit(void)
{
  i2c_del_driver(&fancpld_driver);
}

MODULE_AUTHOR("Dean Kalla");
MODULE_DESCRIPTION("ELBERT Fan CPLD Driver");
MODULE_LICENSE("GPL");

module_init(fancpld_mod_init);
module_exit(fancpld_mod_exit);
