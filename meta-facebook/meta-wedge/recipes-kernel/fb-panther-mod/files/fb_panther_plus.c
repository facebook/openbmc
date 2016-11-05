/*
 * fb_panther_plus.c - Driver for Panther+ microserver
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
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

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>

#ifdef DEBUG

#define PP_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define PP_DEBUG(fmt, ...)

#endif

static const unsigned short normal_i2c[] = {
  0x40, I2C_CLIENT_END
};

/*
 * Driver data (common to all clients)
 */

static const struct i2c_device_id panther_plus_id[] = {
  { "fb_panther_plus", 0 },
  { },
};
MODULE_DEVICE_TABLE(i2c, panther_plus_id);

struct panther_plus_data {
  struct device *hwmon_dev;
  struct mutex update_lock;
};

// Identifies the sysfs attribute panther_plus_show is requesting.
enum {
  PANTHER_PLUS_SYSFS_CPU_TEMP,
  PANTHER_PLUS_SYSFS_DIMM_TEMP,
  PANTHER_PLUS_SYSFS_GPIO_INPUTS,
  PANTHER_PLUS_SYSFS_SMS_KCS,
  PANTHER_PLUS_SYSFS_ALERT_CONTROL,
  PANTHER_PLUS_SYSFS_ALERT_STATUS,
  PANTHER_PLUS_SYSFS_DISCOVERY_SPEC_VER,
  PANTHER_PLUS_SYSFS_DISCOVERY_HW_VER,
  PANTHER_PLUS_SYSFS_DISCOVERY_MANUFACTURER_ID,
  PANTHER_PLUS_SYSFS_DISCOVERY_DEVICE_ID,
  PANTHER_PLUS_SYSFS_DISCOVERY_PRODUCT_ID,
};

// Function Block ID identifiers.
enum panther_plus_fbid_en {
  PANTHER_PLUS_FBID_IPMI_SMS_KCS = 0x0,
  PANTHER_PLUS_FBID_GPIO_INPUTS = 0xd,
  PANTHER_PLUS_FBID_READ_REGISTER = 0x10,
  PANTHER_PLUS_FBID_ALERT_CONTROL = 0xFD,
  PANTHER_PLUS_FBID_ALERT_STATUS = 0xFE,
  PANTHER_PLUS_FBID_DISCOVERY = 0xFF,
};

static inline void panther_plus_make_read(struct i2c_msg *msg,
                                          u8 addr,
                                          u8 *buf,
                                          int len)
{
  msg->addr = addr;
  msg->flags = I2C_M_RD;
  msg->buf = buf;
  msg->len = len;
}

static inline void panther_plus_make_write(struct i2c_msg *msg,
                                           u8 addr,
                                           u8 *buf,
                                           int len)
{
  msg->addr = addr;
  msg->flags = 0;
  msg->buf = buf;
  msg->len = len;
}

static int panther_plus_fbid_io(struct i2c_client *client,
                                  enum panther_plus_fbid_en fbid,
                                  const u8 *write_buf, u8 write_len,
                                  u8 *read_buf, u8 read_len)
{
  // The Intel uServer Module Management Interface Spec defines SMBus blocks,
  // but block sizes exceed the SMBus maximum block sizes
  // (32, see I2C_SMBUS_BLOCK_MAX).  So we basically have to re-implement the
  // smbus functions with a larger max.
  struct i2c_msg msg[2];
  u8 buf[255];
  u8 buf_len;
  int rc;
  u8 num_msgs = 1;

  if (write_len + 1 > sizeof(buf)) {
    return -EINVAL;
  }

  /* first, write the FBID, followed by the write_buf if there is one */
  buf[0] = fbid;
  buf_len = 1;
  if (write_buf) {
    memcpy(&buf[1], write_buf, write_len);
    buf_len += write_len;
  }
  panther_plus_make_write(&msg[0], client->addr, buf, buf_len);

  /* then, read */
  if (read_buf) {
    panther_plus_make_read(&msg[1], client->addr, read_buf, read_len);
    num_msgs = 2;
  }

  rc = i2c_transfer(client->adapter, msg, num_msgs);
  if (rc < 0) {
    PP_DEBUG("Failed to read FBID: %d, error=%d", fbid, rc);
    return rc;
  }

  if (rc != num_msgs) { /* expect 2 */
    PP_DEBUG("Unexpected rc (%d != %d) when reading FBID: %d", rc, num_msgs, fbid);
    return -EIO;
  }

  /* the first byte read should match fbid */

  if (read_buf && read_buf[0] != fbid) {
    PP_DEBUG("Unexpected FBID returned (%d != %d)", read_buf[0], fbid);
    return -EIO;
  }

  return 0;
}

#define PP_GPIO_POWER_ON 0x1
#define PP_GPIO_PWRGD_P1V35 (0x1 << 1)
#define PP_GPIO_RST_EAGE_N (0x1 << 2)
#define PP_GPIO_FM_BIOS_POST_CMPLT_N (0x1 << 3)
#define PP_GPIO_IERR_FPGA (0x1 << 4)
#define PP_GPIO_AVN_PLD_PROCHOT_N (0x1 << 5)
#define PP_GPIO_BUS1_ERROR (0x1 << 6)
#define PP_GPIO_AVN_PLD_THERMTRIP_N (0x1 << 7)
#define PP_GPIO_MCERR_FPGA (0x1 << 8)
#define PP_GPIO_ERROR_AVN_2 (0x1 << 9)
#define PP_GPIO_ERROR_AVN_1 (0x1 << 10)
#define PP_GPIO_ERROR_AVN_0 (0x1 << 11)
#define PP_GPIO_H_MEMHOT_CO_N (0x1 << 12)
#define PP_GPIO_SLP_S45_N (0x1 << 13)
#define PP_GPIO_PLTRST_FPGA_N (0x1 << 14)
#define PP_GPIO_FPGA_GPI_PWD_FAIL (0x1 << 15)
#define PP_GPIO_FPGA_GPI_NMI (0x1 << 16)
#define PP_GPIO_GPI_VCCP_VRHOT_N (0x1 << 17)
#define PP_GPIO_FPGA_GPI_TMP75_ALERT (0x1 << 18)
#define PP_GPIO_LPC_CLKRUN_N (0x1 << 19)

static int panther_plus_read_gpio_inputs_value(
    struct i2c_client *client, u32 *val)
{
  int rc;
  u8 read_buf[5];

  rc = panther_plus_fbid_io(client, PANTHER_PLUS_FBID_GPIO_INPUTS,
                              NULL, 0, read_buf, sizeof(read_buf));
  if (rc < 0) {
    return rc;
  }

  /*
   * expect receiving 5 bytes as:
   * 0xd 0x3 <gpio0-7> <gpio8-15> <gpio9-23>
   */
  if (read_buf[1] != 0x3) {
    PP_DEBUG("Unexpected length %d != 3", read_buf[1]);
    return -EIO;
  }

  *val = read_buf[2] | (read_buf[3] << 8) | (read_buf[4] << 16);

  return 0;
}

static int panther_plus_is_in_post(struct i2c_client *client)
{
  u32 val;
  int rc;

  rc = panther_plus_read_gpio_inputs_value(client, &val);
  if (rc < 0) {
    /* failed to read gpio, treat it as in post */
    return 1;
  }

  /* if PP_GPIO_FM_BIOS_POST_CMPLT_N is set, post is _not_ done yet */
  return (val & PP_GPIO_FM_BIOS_POST_CMPLT_N);
}

static int panther_plus_read_register(struct i2c_client *client,
                                      u8 reg_idx, u32 *reg_val)
{
  u8 write_buf[2];
  u8 read_buf[8];
  int rc;

  /* need to send the register index for the reading */
  write_buf[0] = 0x1;           /* one byte */
  write_buf[1] = reg_idx;

  rc = panther_plus_fbid_io(client, PANTHER_PLUS_FBID_READ_REGISTER,
                              write_buf, sizeof(write_buf),
                              read_buf, sizeof(read_buf));
  if (rc < 0) {
    return -EIO;
  }

  /*
   * expect receiving 8 bytes as:
   * 0x10 0x6 <reg_idx> LSB LSB+1 LSB+2 LSB+3 valid
   */
  if (read_buf[1] != 0x6
      || read_buf[2] != reg_idx
      || read_buf[7] != 0) {
    return -EIO;
  }

  *reg_val = read_buf[3] | (read_buf[4] << 8)
    | (read_buf[5] << 16) | (read_buf[6] << 24);

  PP_DEBUG("Read register %d: 0x%x", reg_idx, *reg_val);

  return 0;
}

#define PANTHER_PLUS_REG_SOC_TJMAX 0
#define PANTHER_PLUS_REG_SOC_RUNTIME 1
#define PANTHER_PLUS_REG_SOC_THERMAL_MARGIN 2
#define PANTHER_PLUS_REG_SOC_DIMM0_A_TEMP 3
#define PANTHER_PLUS_REG_SOC_DIMM0_B_TEMP 4
#define PANTHER_PLUS_REG_SOC_POWER_UNIT 5
#define PANTHER_PLUS_REG_SOC_POWER_CONSUMPTION 6
#define PANTHER_PLUS_REG_SOC_POWER_CALC 7
#define PANTHER_PLUS_REG_SOC_DIMM1_A_TEMP 8
#define PANTHER_PLUS_REG_SOC_DIMM1_B_TEMP 9

static int panther_plus_read_cpu_temp(struct i2c_client *client, char *ret)
{
  int rc;
  u32 tjmax;
  u32 tmargin;
  int val;
  int temp;

  /*
   * make sure POST is done, accessing CPU temperature during POST phase could
   * confusing POST and make it hang
   */
  if (panther_plus_is_in_post(client)) {
    return -EBUSY;
  }

  mdelay(10);

  /* first read Tjmax: register 0, bit[16-23] */
  rc = panther_plus_read_register(client, PANTHER_PLUS_REG_SOC_TJMAX, &tjmax);
  if (rc < 0) {
    return rc;
  }

  mdelay(10);

  /* then, read the thermal margin */
  rc = panther_plus_read_register(client, PANTHER_PLUS_REG_SOC_THERMAL_MARGIN,
                                  &tmargin);
  if (rc < 0) {
    return rc;
  }
  /*
   * thermal margin is 16b 2's complement value representing a number of 1/64
   * degress centigrade.
   */
  tmargin &= 0xFFFF;
  if ((tmargin & 0x8000)) {
    /* signed */
    val = -((tmargin - 1) ^ 0xFFFF);
  } else {
    val = tmargin;
  }

  /*
   * now val holds the margin (a number of 1/64), add it to the Tjmax.
   * Times 1000 for lm-sensors.
   */
  temp = ((tjmax >> 16) & 0xFF) * 1000 + val * 1000 / 64;

  return sprintf(ret, "%d\n", temp);
}

static int panther_plus_read_dimm_temp(struct i2c_client *client,
                                       int dimm, char *ret)
{
  int rc;
  u32 val;
  int temp;

  /*
   * make sure POST is done, accessing DIMM temperature will fail anyway if
   * POST is not done.
   */
  if (panther_plus_is_in_post(client)) {
    return -EBUSY;
  }

  mdelay(10);

  rc = panther_plus_read_register(client, dimm, &val);
  if (rc < 0) {
    return rc;
  }

  /*
   * DIMM temperature is encoded in 16b as the following:
   * b15-b12: TCRIT HIGH LOW SIGN
   * b11-b08: 128 64 32 16
   * b07-b04: 8 4 2 1
   * b03-b00: 0.5 0.25 0.125 0.0625
   */
  /* For now, only care about those 12 data bits and SIGN */
  val &= 0x1FFF;
  if ((val & 0x1000)) {
    /* signed */
    val = -((val - 1) ^ 0x1FFF);
  }

  /*
   * now val holds the value as a number of 1/16, times 1000 for lm-sensors */
  temp =  val * 1000 / 16;

  return sprintf(ret, "%d\n", temp);
}

static int panther_plus_read_gpio_inputs(struct i2c_client *client, char *ret)
{
  u32 val;
  int rc;

  rc = panther_plus_read_gpio_inputs_value(client, &val);
  if (rc < 0) {
    return rc;
  }
  return sprintf(ret, "0x%x\n", val);
}

static int panther_plus_read_sms_kcs(struct i2c_client *client, char *ret)
{
  int rc;
  u8 read_buf[255] = {0x0};

  rc = panther_plus_fbid_io(client, PANTHER_PLUS_FBID_IPMI_SMS_KCS,
                              NULL, 0, read_buf, sizeof(read_buf));
  if (rc < 0) {
    return rc;
  }

  memcpy(ret, read_buf, read_buf[1]+2);

  return (read_buf[1]+2);
}

static int panther_plus_write_sms_kcs(struct i2c_client *client, const char *buf, u8 count)
{
  int rc;

  rc = panther_plus_fbid_io(client, PANTHER_PLUS_FBID_IPMI_SMS_KCS,
                              buf, count, NULL, 0);
  if (rc < 0) {
    return rc;
  }

  return count;
}

static int panther_plus_read_alert_status(struct i2c_client *client, char *ret)
{
  int rc;
  u8 rbuf[5] = {0};

  rc = panther_plus_fbid_io(client, PANTHER_PLUS_FBID_ALERT_STATUS,
                              NULL, 0, rbuf, sizeof(rbuf));
  if (rc < 0) {
    return rc;
  }

  memcpy(ret, rbuf, rbuf[1]+2);

  return (rbuf[1]+2);
}

static int panther_plus_read_alert_control(struct i2c_client *client, char *ret)
{
  int rc;
  u8 rbuf[5] = {0};

  rc = panther_plus_fbid_io(client, PANTHER_PLUS_FBID_ALERT_CONTROL,
                              NULL, 0, rbuf, sizeof(rbuf));
  if (rc < 0) {
    return rc;
  }

  memcpy(ret, rbuf, rbuf[1]+2);

  return (rbuf[1]+2);
}

static int panther_plus_read_discovery(struct i2c_client *client, char *ret,
                                       int which_attribute)
{
  int rc;
  u8 datalen;
#define DISCOVERY_DATA_SIZE 10
  u8 rbuf[DISCOVERY_DATA_SIZE+2] = {0};
  rc = panther_plus_fbid_io(client, PANTHER_PLUS_FBID_DISCOVERY,
                            NULL, 0, rbuf, sizeof(rbuf));
  if (rc < 0) {
    return rc;
  }
  datalen = rbuf[1];
  if (datalen < DISCOVERY_DATA_SIZE) {
    return -EINVAL;
  }
  switch (which_attribute) {
    case PANTHER_PLUS_SYSFS_DISCOVERY_SPEC_VER:
      return scnprintf(ret, PAGE_SIZE, "%u.%u\n", rbuf[2], rbuf[3]);
    case PANTHER_PLUS_SYSFS_DISCOVERY_HW_VER:
      return scnprintf(ret, PAGE_SIZE, "%u.%u\n", rbuf[4], rbuf[5]);
    case PANTHER_PLUS_SYSFS_DISCOVERY_MANUFACTURER_ID:
      return scnprintf(ret, PAGE_SIZE, "0x%02X%02X%02X\n", rbuf[8], rbuf[7],
                       rbuf[6]);
    case PANTHER_PLUS_SYSFS_DISCOVERY_DEVICE_ID:
      return scnprintf(ret, PAGE_SIZE, "0x%02X\n", rbuf[9]);
    case PANTHER_PLUS_SYSFS_DISCOVERY_PRODUCT_ID:
      return scnprintf(ret, PAGE_SIZE, "0x%02X%02X\n", rbuf[11], rbuf[10]);
    default:
      return -EINVAL;
  }
  return -EINVAL;
}

static int panther_plus_write_alert_control(struct i2c_client *client, const char *buf, u8 count)
{
  int rc;

  rc = panther_plus_fbid_io(client, PANTHER_PLUS_FBID_ALERT_CONTROL,
                              buf, count, NULL, 0);
  if (rc < 0) {
    return rc;
  }

  return count;
}

static ssize_t panther_plus_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  struct panther_plus_data *data = i2c_get_clientdata(client);
  struct sensor_device_attribute_2 *sensor_attr = to_sensor_dev_attr_2(attr);
  int which = sensor_attr->index;
  int rc = -EIO;

  mutex_lock(&data->update_lock);
  switch (which) {
  case PANTHER_PLUS_SYSFS_CPU_TEMP:
    rc = panther_plus_read_cpu_temp(client, buf);
    break;
  case PANTHER_PLUS_SYSFS_DIMM_TEMP:
    rc = panther_plus_read_dimm_temp(client, sensor_attr->nr, buf);
    break;
  case PANTHER_PLUS_SYSFS_GPIO_INPUTS:
    rc = panther_plus_read_gpio_inputs(client, buf);
    break;
  case PANTHER_PLUS_SYSFS_SMS_KCS:
    rc = panther_plus_read_sms_kcs(client, buf);
    break;
  case PANTHER_PLUS_SYSFS_ALERT_STATUS:
    rc = panther_plus_read_alert_status(client, buf);
    break;
  case PANTHER_PLUS_SYSFS_ALERT_CONTROL:
    rc = panther_plus_read_alert_control(client, buf);
    break;
  case PANTHER_PLUS_SYSFS_DISCOVERY_SPEC_VER:
  case PANTHER_PLUS_SYSFS_DISCOVERY_HW_VER:
  case PANTHER_PLUS_SYSFS_DISCOVERY_MANUFACTURER_ID:
  case PANTHER_PLUS_SYSFS_DISCOVERY_DEVICE_ID:
  case PANTHER_PLUS_SYSFS_DISCOVERY_PRODUCT_ID:
    rc = panther_plus_read_discovery(client, buf, which);
  default:
    break;
  }

  /*
   * With the current i2c driver, the bus/kernel could hang if accessing the
   * FPGA too fast. Adding some delay here until we fix the i2c driver bug
   */
  mdelay(10);

  mutex_unlock(&data->update_lock);

  return rc;
}

static ssize_t panther_plus_set(struct device *dev,
                                 struct device_attribute *attr,
                                 const char *buf, size_t count)
{
  struct i2c_client *client = to_i2c_client(dev);
  struct panther_plus_data *data = i2c_get_clientdata(client);
  struct sensor_device_attribute_2 *sensor_attr = to_sensor_dev_attr_2(attr);
  int which = sensor_attr->index;

  int rc = -EIO;
  mutex_lock(&data->update_lock);
  switch (which) {
    case PANTHER_PLUS_SYSFS_SMS_KCS:
      rc = panther_plus_write_sms_kcs(client, buf, count);
      break;
    case PANTHER_PLUS_SYSFS_ALERT_CONTROL:
      rc = panther_plus_write_alert_control(client, buf, count);
      break;
    default:
      break;
  }

  mdelay(10);

  mutex_unlock(&data->update_lock);

  return rc;
}

static SENSOR_DEVICE_ATTR_2(temp1_input, S_IRUGO, panther_plus_show, NULL,
                            0, PANTHER_PLUS_SYSFS_CPU_TEMP);
static SENSOR_DEVICE_ATTR_2(temp2_input, S_IRUGO, panther_plus_show, NULL,
                            PANTHER_PLUS_REG_SOC_DIMM0_A_TEMP,
                            PANTHER_PLUS_SYSFS_DIMM_TEMP);
static SENSOR_DEVICE_ATTR_2(temp3_input, S_IRUGO, panther_plus_show, NULL,
                            PANTHER_PLUS_REG_SOC_DIMM0_B_TEMP,
                            PANTHER_PLUS_SYSFS_DIMM_TEMP);
static SENSOR_DEVICE_ATTR_2(temp4_input, S_IRUGO, panther_plus_show, NULL,
                            PANTHER_PLUS_REG_SOC_DIMM1_A_TEMP,
                            PANTHER_PLUS_SYSFS_DIMM_TEMP);
static SENSOR_DEVICE_ATTR_2(temp5_input, S_IRUGO, panther_plus_show, NULL,
                            PANTHER_PLUS_REG_SOC_DIMM1_B_TEMP,
                            PANTHER_PLUS_SYSFS_DIMM_TEMP);
static SENSOR_DEVICE_ATTR_2(gpio_inputs, S_IRUGO, panther_plus_show, NULL,
                            0, PANTHER_PLUS_SYSFS_GPIO_INPUTS);
static SENSOR_DEVICE_ATTR_2(sms_kcs, S_IWUSR | S_IRUGO, panther_plus_show, panther_plus_set,
                            0, PANTHER_PLUS_SYSFS_SMS_KCS);
static SENSOR_DEVICE_ATTR_2(alert_status, S_IRUGO, panther_plus_show, NULL,
                            0, PANTHER_PLUS_SYSFS_ALERT_STATUS);
static SENSOR_DEVICE_ATTR_2(alert_control, S_IWUSR | S_IRUGO, panther_plus_show, panther_plus_set,
                            0, PANTHER_PLUS_SYSFS_ALERT_CONTROL);
static SENSOR_DEVICE_ATTR_2(spec_ver, S_IRUGO, panther_plus_show, NULL,
                            0, PANTHER_PLUS_SYSFS_DISCOVERY_SPEC_VER);
static SENSOR_DEVICE_ATTR_2(hw_ver, S_IRUGO, panther_plus_show, NULL,
                            0, PANTHER_PLUS_SYSFS_DISCOVERY_HW_VER);
static SENSOR_DEVICE_ATTR_2(manufacturer_id, S_IRUGO, panther_plus_show, NULL,
                            0, PANTHER_PLUS_SYSFS_DISCOVERY_MANUFACTURER_ID);
static SENSOR_DEVICE_ATTR_2(device_id, S_IRUGO, panther_plus_show, NULL,
                            0, PANTHER_PLUS_SYSFS_DISCOVERY_DEVICE_ID);
static SENSOR_DEVICE_ATTR_2(product_id, S_IRUGO, panther_plus_show, NULL,
                            0, PANTHER_PLUS_SYSFS_DISCOVERY_PRODUCT_ID);

static struct attribute *panther_plus_attributes[] = {
  &sensor_dev_attr_temp1_input.dev_attr.attr,
  &sensor_dev_attr_temp2_input.dev_attr.attr,
  &sensor_dev_attr_temp3_input.dev_attr.attr,
  &sensor_dev_attr_temp4_input.dev_attr.attr,
  &sensor_dev_attr_temp5_input.dev_attr.attr,
  &sensor_dev_attr_gpio_inputs.dev_attr.attr,
  &sensor_dev_attr_sms_kcs.dev_attr.attr,
  &sensor_dev_attr_alert_status.dev_attr.attr,
  &sensor_dev_attr_alert_control.dev_attr.attr,
  &sensor_dev_attr_spec_ver.dev_attr.attr,
  &sensor_dev_attr_hw_ver.dev_attr.attr,
  &sensor_dev_attr_manufacturer_id.dev_attr.attr,
  &sensor_dev_attr_device_id.dev_attr.attr,
  &sensor_dev_attr_product_id.dev_attr.attr,
  NULL
};

static const struct attribute_group panther_plus_group = {
  .attrs = panther_plus_attributes,
};

/* Return 0 if detection is successful, -ENODEV otherwise */
static int panther_plus_detect(struct i2c_client *client,
                               struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the Panther+, although
   * presumably we could try to query FBID 0xFF for HW ID.
   */
  strlcpy(info->type, "fb_panther_plus", I2C_NAME_SIZE);
  return 0;
}

static int panther_plus_probe(struct i2c_client *client,
                              const struct i2c_device_id *id)
{
  struct panther_plus_data *data;
  int err;

  data = kzalloc(sizeof(struct panther_plus_data), GFP_KERNEL);
  if (!data) {
    err = -ENOMEM;
    goto exit;
  }

  i2c_set_clientdata(client, data);
  mutex_init(&data->update_lock);

  /* Register sysfs hooks */
  if ((err = sysfs_create_group(&client->dev.kobj, &panther_plus_group)))
    goto exit_free;

  data->hwmon_dev = hwmon_device_register(&client->dev);
  if (IS_ERR(data->hwmon_dev)) {
    err = PTR_ERR(data->hwmon_dev);
    goto exit_remove_files;
  }

  printk(KERN_INFO "Panther+ driver successfully loaded.\n");

  return 0;

 exit_remove_files:
  sysfs_remove_group(&client->dev.kobj, &panther_plus_group);
 exit_free:
  kfree(data);
 exit:
  return err;
}

static int panther_plus_remove(struct i2c_client *client)
{
  struct panther_plus_data *data = i2c_get_clientdata(client);

  hwmon_device_unregister(data->hwmon_dev);
  sysfs_remove_group(&client->dev.kobj, &panther_plus_group);

  kfree(data);
  return 0;
}

static struct i2c_driver panther_plus_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "fb_panther_plus",
  },
  .probe    = panther_plus_probe,
  .remove   = panther_plus_remove,
  .id_table = panther_plus_id,
  .detect   = panther_plus_detect,
  .address_list = normal_i2c,
};

static int __init sensors_panther_plus_init(void)
{
  return i2c_add_driver(&panther_plus_driver);
}

static void __exit sensors_panther_plus_exit(void)
{
  i2c_del_driver(&panther_plus_driver);
}

MODULE_AUTHOR("Tian Fang <tfang@fb.com>");
MODULE_DESCRIPTION("Facebook Panther+ Driver");
MODULE_LICENSE("GPL");

module_init(sensors_panther_plus_init);
module_exit(sensors_panther_plus_exit);
