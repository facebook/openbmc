/*
 * com_e_driver.c - The virtual i2c com_e driver
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
#define COM_E_DRIVER_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define COM_E_DRIVER_DEBUG(fmt, ...)
#endif

// Sensors under Bridge IC
enum {
  BIC_SENSOR_MB_OUTLET_TEMP = 0x01,
  BIC_SENSOR_MB_INLET_TEMP = 0x07,
  BIC_SENSOR_PCH_TEMP = 0x08,
  BIC_SENSOR_VCCIN_VR_TEMP = 0x80,
  BIC_SENSOR_1V05MIX_VR_TEMP = 0x81,
  BIC_SENSOR_SOC_TEMP = 0x05,
  BIC_SENSOR_SOC_THERM_MARGIN = 0x09,
  BIC_SENSOR_VDDR_VR_TEMP = 0x82,
  BIC_SENSOR_SOC_DIMMA0_TEMP = 0xB4,
  BIC_SENSOR_SOC_DIMMB0_TEMP = 0xB6,
  BIC_SENSOR_SOC_PACKAGE_PWR = 0x2C,
  BIC_SENSOR_VCCIN_VR_POUT = 0x8B,
  BIC_SENSOR_VDDR_VR_POUT = 0x8D,
  BIC_SENSOR_SOC_TJMAX = 0x30,
  BIC_SENSOR_P3V3_MB = 0xD0,
  BIC_SENSOR_P12V_MB = 0xD2,
  BIC_SENSOR_P1V05_PCH = 0xD3,
  BIC_SENSOR_P3V3_STBY_MB = 0xD5,
  BIC_SENSOR_P5V_STBY_MB = 0xD6,
  BIC_SENSOR_PV_BAT = 0xD7,
  BIC_SENSOR_PVDDR = 0xD8,
  BIC_SENSOR_P1V05_MIX = 0x8E,
  BIC_SENSOR_1V05MIX_VR_CURR = 0x84,
  BIC_SENSOR_VDDR_VR_CURR = 0x85,
  BIC_SENSOR_VCCIN_VR_CURR = 0x83,
  BIC_SENSOR_VCCIN_VR_VOL = 0x88,
  BIC_SENSOR_VDDR_VR_VOL = 0x8A,
  BIC_SENSOR_P1V05MIX_VR_VOL = 0x89,
  BIC_SENSOR_P1V05MIX_VR_Pout = 0x8C,
  BIC_SENSOR_INA230_POWER = 0x29,
  BIC_SENSOR_INA230_VOL = 0x2A,
};

typedef struct _sensor_info_t {
  uint8_t sensor_num;
  int sensor_value;
} sensor_info_t;

sensor_info_t com_e_sensor[] = {
  {BIC_SENSOR_MB_OUTLET_TEMP, 0},
  {BIC_SENSOR_MB_INLET_TEMP, 0},
  {BIC_SENSOR_PCH_TEMP, 0},
  {BIC_SENSOR_VCCIN_VR_TEMP, 0},
  {BIC_SENSOR_1V05MIX_VR_TEMP, 0},
  {BIC_SENSOR_SOC_TEMP, 0},
  {BIC_SENSOR_SOC_THERM_MARGIN, 0},
  {BIC_SENSOR_VDDR_VR_TEMP, 0},
  {BIC_SENSOR_SOC_DIMMA0_TEMP, 0},
  {BIC_SENSOR_SOC_DIMMB0_TEMP, 0},
  {BIC_SENSOR_SOC_PACKAGE_PWR, 0},
  {BIC_SENSOR_VCCIN_VR_POUT, 0},
  {BIC_SENSOR_VDDR_VR_POUT, 0},
  {BIC_SENSOR_SOC_TJMAX, 0},
  {BIC_SENSOR_P3V3_MB, 0},
  {BIC_SENSOR_P12V_MB, 0},
  {BIC_SENSOR_P1V05_PCH, 0},
  {BIC_SENSOR_P3V3_STBY_MB, 0},
  {BIC_SENSOR_P5V_STBY_MB, 0},
  {BIC_SENSOR_PV_BAT, 0},
  {BIC_SENSOR_PVDDR, 0},
  {BIC_SENSOR_P1V05_MIX, 0},
  {BIC_SENSOR_1V05MIX_VR_CURR, 0},
  {BIC_SENSOR_VDDR_VR_CURR, 0},
  {BIC_SENSOR_VCCIN_VR_CURR, 0},
  {BIC_SENSOR_VCCIN_VR_VOL, 0},
  {BIC_SENSOR_VDDR_VR_VOL, 0},
  {BIC_SENSOR_P1V05MIX_VR_VOL, 0},
  {BIC_SENSOR_P1V05MIX_VR_Pout, 0},
  {BIC_SENSOR_INA230_POWER, 0},
  {BIC_SENSOR_INA230_VOL, 0},
};

static const int size_com_e_sensor =
    sizeof(com_e_sensor) / sizeof(com_e_sensor[0]);

static ssize_t sensor_read(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int i = 0;

  while (i < size_com_e_sensor) {
    if (dev_attr->ida_reg == com_e_sensor[i].sensor_num) {
      return scnprintf(buf, PAGE_SIZE, "%d\n",
                       com_e_sensor[i].sensor_value);
    }
    i++;
  }
}

static ssize_t sensor_write(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf, size_t count)
{
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int i = 0;

  while (i < size_com_e_sensor) {
    if (dev_attr->ida_reg == com_e_sensor[i].sensor_num) {
      if (sscanf(buf, "%i", &com_e_sensor[i].sensor_value) <= 0) {
        return -EINVAL;
      }
      return count;
    }
    i++;
  }
}


static const i2c_dev_attr_st com_e_driver_attr_table[] = {
  {
    "in0_input",
    NULL,
    sensor_read,
    sensor_write,
    0xd0, 0, 0,
  },
  {
    "in1_input",
    NULL,
    sensor_read,
    sensor_write,
    0xd2, 0, 0,
  },
  {
    "in2_input",
    NULL,
    sensor_read,
    sensor_write,
    0xd3, 0, 0,
  },
  {
    "in3_input",
    NULL,
    sensor_read,
    sensor_write,
    0xd5, 0, 0,
  },
  {
    "in4_input",
    NULL,
    sensor_read,
    sensor_write,
    0xd6, 0, 0,
  },
  {
    "in5_input",
    NULL,
    sensor_read,
    sensor_write,
    0xd7, 0, 0,
  },
  {
    "in6_input",
    NULL,
    sensor_read,
    sensor_write,
    0xd8, 0, 0,
  },
  {
    "in7_input",
    NULL,
    sensor_read,
    sensor_write,
    0x8e, 0, 0,
  },
  {
    "in8_input",
    NULL,
    sensor_read,
    sensor_write,
    0x88, 0, 0,
  },
  {
    "in9_input",
    NULL,
    sensor_read,
    sensor_write,
    0x8a, 0, 0,
  },
  {
    "in10_input",
    NULL,
    sensor_read,
    sensor_write,
    0x89, 0, 0,
  },
  {
    "in11_input",
    NULL,
    sensor_read,
    sensor_write,
    0x2a, 0, 0,
  },
  {
    "temp1_input",
    NULL,
    sensor_read,
    sensor_write,
    0x01, 0, 0,
  },
  {
    "temp2_input",
    NULL,
    sensor_read,
    sensor_write,
    0x07, 0, 0,
  },
  {
    "temp3_input",
    NULL,
    sensor_read,
    sensor_write,
    0x08, 0, 0,
  },
  {
    "temp4_input",
    NULL,
    sensor_read,
    sensor_write,
    0x80, 0, 0,
  },
  {
    "temp5_input",
    NULL,
    sensor_read,
    sensor_write,
    0x81, 0, 0,
  },
  {
    "temp6_input",
    NULL,
    sensor_read,
    sensor_write,
    0x05, 0, 0,
  },
  {
    "temp7_input",
    NULL,
    sensor_read,
    sensor_write,
    0x09, 0, 0,
  },
  {
    "temp8_input",
    NULL,
    sensor_read,
    sensor_write,
    0x82, 0, 0,
  },
  {
    "temp9_input",
    NULL,
    sensor_read,
    sensor_write,
    0xb4, 0, 0,
  },
  {
    "temp10_input",
    NULL,
    sensor_read,
    sensor_write,
    0xb6, 0, 0,
  },
  {
    "temp11_input",
    NULL,
    sensor_read,
    sensor_write,
    0x30, 0, 0,
  },
  {
    "power1_input",
    NULL,
    sensor_read,
    sensor_write,
    0x2c, 0, 0,
  },
  {
    "power2_input",
    NULL,
    sensor_read,
    sensor_write,
    0x8b, 0, 0,
  },
  {
    "power3_input",
    NULL,
    sensor_read,
    sensor_write,
    0x8d, 0, 0,
  },
  {
    "power4_input",
    NULL,
    sensor_read,
    sensor_write,
    0x8c, 0, 0,
  },
  {
    "power5_input",
    NULL,
    sensor_read,
    sensor_write,
    0x29, 0, 0,
  },
  {
    "curr1_input",
    NULL,
    sensor_read,
    sensor_write,
    0x84, 0, 0,
  },
  {
    "curr2_input",
    NULL,
    sensor_read,
    sensor_write,
    0x85, 0, 0,
  },
  {
    "curr3_input",
    NULL,
    sensor_read,
    sensor_write,
    0x83, 0, 0,
  },
};

static i2c_dev_data_st com_e_driver_data;

/*
 * COM_E_DRIVER i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x33, I2C_CLIENT_END
};


/* COM_E_DRIVER id */
static const struct i2c_device_id com_e_driver_id[] = {
  {"com_e_driver", 0},
  { },
};
MODULE_DEVICE_TABLE(i2c, com_e_driver_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int com_e_driver_detect(struct i2c_client *client,
                         struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the driver
   */
  strlcpy(info->type, "com_e_driver", I2C_NAME_SIZE);
  return 0;
}

static int com_e_driver_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = sizeof(com_e_driver_attr_table) / sizeof(com_e_driver_attr_table[0]);

  return i2c_dev_sysfs_data_init(client, &com_e_driver_data,
                                 com_e_driver_attr_table, n_attrs);
}

static int com_e_driver_remove(struct i2c_client *client)
{
  i2c_dev_sysfs_data_clean(client, &com_e_driver_data);
  return 0;
}

static struct i2c_driver com_e_driver_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "com_e_driver",
  },
  .probe    = com_e_driver_probe,
  .remove   = com_e_driver_remove,
  .id_table = com_e_driver_id,
  .detect   = com_e_driver_detect,
  .address_list = normal_i2c,
};

static int __init com_e_driver_mod_init(void)
{
  return i2c_add_driver(&com_e_driver_driver);
}

static void __exit com_e_driver_mod_exit(void)
{
  i2c_del_driver(&com_e_driver_driver);
}

MODULE_AUTHOR("Mickey Zhan");
MODULE_DESCRIPTION("COM-e Driver");
MODULE_LICENSE("GPL");

module_init(com_e_driver_mod_init);
module_exit(com_e_driver_mod_exit);
