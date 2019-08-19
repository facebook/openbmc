/*
 * ltc4281.c - The i2c driver for LTC4281 for Wedge100
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
 *
 * This file contains code to support LTC4281.pdf datasheet available @
 * https://www.analog.com/media/en/technical-documentation/data-sheets/LTC4281.pdf
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
#include <linux/delay.h>

#include <i2c_dev_sysfs.h>

#ifdef DEBUG
#define LTC_DEBUG(fmt, ...) do {                   \
  printk(KERN_DEBUG "%s:%d " fmt "\n",            \
         __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
} while (0)

#else /* !DEBUG */

#define LTC_DEBUG(fmt, ...)
#endif

#define LTC4281_DELAY 11      /* ms */

/* chip registers */
#define LTC4281_CONTROL           0x00
#define LTC4281_FAULT_LOG         0x04
#define LTC4281_ADC_ALERT_LOG     0x05
#define LTC4281_VGPIO_ALARM_MIN   0x08
#define LTC4281_VGPIO_ALARM_MAX   0x09
#define LTC4281_VSOURCE_ALARM_MIN 0x0A
#define LTC4281_VSOURCE_ALARM_MAX 0x0B
#define LTC4281_VSENSE_ALARM_MIN  0x0C
#define LTC4281_VSENSE_ALARM_MAX  0X0D
#define LTC4281_POWER_ALARM_MIN   0x0E
#define LTC4281_POWER_ALARM_MAX   0x0F
#define LTC4281_ILIM_ADJUST       0x11
#define LTC4281_ADC_CONTROL       0x1D
#define LTC4281_STATUS_LSB        0x1E
#define LTC4281_STATUS_MSB        0x1F
#define LTC4281_FAULT_LOG_EE      0x24
#define LTC4281_ADC_ALERT_LOG_EE  0x25
#define LTC4281_VGPIO_MSB         0x34
#define LTC4281_VSOURCE_VDD_MSB   0x3A
#define LTC4281_VSENSE_MSB        0x40
#define LTC4281_POWER_MSB         0x46


#define NTC_VDD_VOLTAGE           3300
#define NTC_PULLDOWN_OHM          430

#define RSENSE        175 /* 1 / 0.00571429 */

#define VOLT_MODE     BIT(2)

struct ntc_rt_table {
  int		temp_c;
  uint32_t	ohm;
};

/* NTC thermistor TSM1B103F3381RZ */
static const struct ntc_rt_table ntc_data[] = {
  { .temp_c	= -40, .ohm	= 199970 },
  { .temp_c	= -35, .ohm	= 151840 },
  { .temp_c	= -30, .ohm	= 116250 },
  { .temp_c	= -25, .ohm	= 89611 },
  { .temp_c	= -20, .ohm	= 69593 },
  { .temp_c	= -15, .ohm	= 54493 },
  { .temp_c	= -10, .ohm	= 43029 },
  { .temp_c	= -5, .ohm	= 34254 },
  { .temp_c	= 0, .ohm	= 27473 },
  { .temp_c	= 5, .ohm	= 22184 },
  { .temp_c	= 10, .ohm	= 18025 },
  { .temp_c	= 15, .ohm	= 14731 },
  { .temp_c	= 20, .ohm	= 12105 },
  { .temp_c	= 25, .ohm	= 10000 },
  { .temp_c	= 30, .ohm	= 8305 },
  { .temp_c	= 35, .ohm	= 6934 },
  { .temp_c	= 40, .ohm	= 5819 },
  { .temp_c	= 45, .ohm	= 4908 },
  { .temp_c	= 50, .ohm	= 4160 },
  { .temp_c	= 55, .ohm	= 3543 },
  { .temp_c	= 60, .ohm	= 3031 },
  { .temp_c	= 65, .ohm	= 2604 },
  { .temp_c	= 70, .ohm	= 2246 },
  { .temp_c	= 75, .ohm	= 1944 },
  { .temp_c	= 80, .ohm	= 1689 },
  { .temp_c	= 85, .ohm	= 1472 },
  { .temp_c	= 90, .ohm	= 1287 },
  { .temp_c	= 95, .ohm	= 1129 },
  { .temp_c	= 100, .ohm	= 993 },
  { .temp_c	= 105, .ohm	= 876 },
  { .temp_c	= 110, .ohm	= 776 },
  { .temp_c	= 115, .ohm	= 689 },
  { .temp_c	= 120, .ohm	= 614 },
  { .temp_c	= 125, .ohm	= 550 },
};

static const int n_ntc_data = ARRAY_SIZE(ntc_data);

static void lookup_table(uint32_t ohm, int *i_low, int *i_high)
{
  int start, end, mid;

  /*
   * Handle special cases: Resistance is higher than or equal to
   * resistance in first table entry, or resistance is lower or equal
   * to resistance in last table entry.
   * In these cases, return i_low == i_high, either pointing to the
   * beginning or to the end of the table depending on the condition.
   */

  if (ohm >= ntc_data[0].ohm) {
    *i_low = 0;
    *i_high = 0;
    return;
  }
  if (ohm <= ntc_data[n_ntc_data - 1].ohm) {
    *i_low = n_ntc_data - 1;
    *i_high = n_ntc_data - 1;
    return;
  }

  /* Do a binary search on R-T table */
  start = 0;
  end = n_ntc_data;
  while (start < end) {
    mid = start + (end - start) / 2;
    /*
     * start <= mid < end
     * ntc_data[start].ohm > ohm >= ntc_data[end].ohm
     *
     * We could check for "ohm == ntc_data[mid].ohm" here, but
     * that is a quite unlikely condition, and we would have to
     * check again after updating start. Check it at the end instead
     * for simplicity.
     */
    if (ohm >= ntc_data[mid].ohm) {
      end = mid;
    } else {
      start = mid + 1;
      /*
       * ohm >= ntc_data[start].ohm might be true here,
       * since we set start to mid + 1. In that case, we are
       * done. We could keep going, but the condition is quite
       * likely to occur, so it is worth checking for it.
       */
      if (ohm >= ntc_data[start].ohm)
        end = start;
    }
    /*
     * start <= end
     * ntc_data[start].ohm >= ohm >= ntc_data[end].ohm
     */
  }
  /*
   * start == end
   * ohm >= ntc_data[end].ohm
   */
  *i_low = end;
  if (ohm == ntc_data[end].ohm)
    *i_high = end;
  else
    *i_high = end - 1;
}

static int ntc_thermistor_get_ohm(int voltage)
{
  int value;

  if (voltage <= 0)
    return -1;

  /*
   * This formula is based on voltage divider rule,
   * it is derived from kirchhof voltage law.
   * voltage = NTC_VDD_VOLTAGE * (NTC_PULLDOWN_OHM/(NTC_PULLDOWN_OHM + NTC_OHM))
   */
  value = NTC_PULLDOWN_OHM * (((NTC_VDD_VOLTAGE * 1000) / voltage) - 1000);

  return (value / 1000);
}

static int ntc_thermistor_get_temp(uint32_t resistor)
{
  int low, high;
  int temp;

  lookup_table(resistor, &low, &high);

  if (low == high) {
    /* Unable to use linear approximation */
    temp = ntc_data[low].temp_c * 1000;
  } else {
    temp = ntc_data[low].temp_c * 1000 +
      ((ntc_data[high].temp_c - ntc_data[low].temp_c) *
      1000 * ((int)resistor - (int)ntc_data[low].ohm)) /
      ((int)ntc_data[high].ohm - (int)ntc_data[low].ohm);
  }

  return temp;
}

static int ltc_curr_read(struct device *dev)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  int i;
  int ret_val = -1, curr_value = -1;
  uint8_t values[2];

  mutex_lock(&data->idd_lock);
  for (i = 0; i < 2; ++i) {
    ret_val = i2c_smbus_read_byte_data(client, LTC4281_VSENSE_MSB + i);
    if (ret_val < 0) {
      /* error case */
      LTC_DEBUG("I2C read error");
      mutex_unlock(&data->idd_lock);
      return -1;
    }
    if (ret_val > 255) {
      mutex_unlock(&data->idd_lock);
      return -1;
    }
    values[i] = ret_val;
  }
  mutex_unlock(&data->idd_lock);

  curr_value = (values[0] << 8) + values[1];
  /*
   * This formula is taken from the LTC4281 datasheet Rev A page 21
   * Current = ((code * 0.04) * 10000) / ((2^16 - 1) / Rsense)
   */
  curr_value = (curr_value * 400) / (65535 / RSENSE);

  return curr_value;
}

static int ltc_vgpio_read(struct device *dev, struct device_attribute *attr)
{
  int value = -1;

  value = i2c_dev_read_word_bigendian(dev, attr);
  if (value < 0) {
    /* error case */
    LTC_DEBUG("I2C read error");
    return -1;
  }
  /*
   * This formula is taken from the LTC4281 datasheet Rev A page 21
   * Vgpio = ((code * 1.28) * 1000) / (2^16 - 1)
   */
  value = (value * 1280) / 65535;

  return value;
}

static ssize_t ltc_vol_show(struct device *dev,
                            struct device_attribute *attr,
                            char *buf)
{
  struct i2c_client *client = to_i2c_client(dev);
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
  const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;
  int value = -1;
  int ilm_value = -1;
  uint8_t vsouce = dev_attr->ida_bit_offset; /* 1:Vsouce, 0:Vdd */

  mutex_lock(&data->idd_lock);
  ilm_value = i2c_smbus_read_byte_data(client, LTC4281_ILIM_ADJUST);
  if (ilm_value < 0) {
    /* error case */
    LTC_DEBUG("I2C read error");
    mutex_unlock(&data->idd_lock);
    return -1;
  }

  if (vsouce)
    ilm_value |= VOLT_MODE;
  else
    ilm_value &= ~VOLT_MODE;

  if (i2c_smbus_write_byte_data(client, LTC4281_ILIM_ADJUST, ilm_value) < 0) {
    /* error case */
    LTC_DEBUG("I2C write error");
    mutex_unlock(&data->idd_lock);
    return -1;
  }
  /* Sleep a while to make sure data write done */
  msleep(LTC4281_DELAY);
  mutex_unlock(&data->idd_lock);

  value = i2c_dev_read_word_bigendian(dev, attr);
  if (value < 0) {
    /* error case */
    LTC_DEBUG("I2C read error");
    return -1;
  }
  /*
   * This formula is taken from the LTC4281 datasheet Rev A page 21
   * Voltage = (code * 16.64) * 1000) / (2^16 - 1)
   */
  value = (value * 16640) / 65535;

  return scnprintf(buf, PAGE_SIZE, "%d\n", value);
}

static ssize_t ltc_curr_show(struct device *dev,
                             struct device_attribute *attr,
                             char *buf)
{
  int value = -1;

  value = ltc_curr_read(dev);
  if (value < 0) {
    /* error case */
    LTC_DEBUG("Current value read error");
    return -1;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", value);
}

static ssize_t ltc_power_show(struct device *dev,
                              struct device_attribute *attr,
                              char *buf)
{
  int value = -1;

  value = i2c_dev_read_word_bigendian(dev, attr);
  if (value < 0) {
    /* error case */
    LTC_DEBUG("I2C read error");
    return -1;
  }
  /*
   * This formula is taken from the LTC4281 datasheet Rev A page 21
   * Power = ((code * 0.04 * 16.64 * 2^16) * 10000) / ((2^16 - 1)^2 / Rsense)
   */
  value = (value * 4 * 1664) / (65535 / RSENSE);

  return scnprintf(buf, PAGE_SIZE, "%d\n", value);
}

static ssize_t ltc_temp_show(struct device *dev,
                             struct device_attribute *attr,
                             char *buf)
{
  int value = -1, ntc_resistor = -1;

  value = ltc_vgpio_read(dev, attr);
  if (value < 0) {
    /* error case */
    LTC_DEBUG("I2C read error");
    return -1;
  }

  ntc_resistor = ntc_thermistor_get_ohm(value);
  if (ntc_resistor < 0) {
    /* error case */
    LTC_DEBUG("Get NTC resister value error");
    return ntc_resistor;
  }

  value = ntc_thermistor_get_temp(ntc_resistor);

  return scnprintf(buf, PAGE_SIZE, "%d\n", value);
}

static ssize_t ltc_vgpio_show(struct device *dev,
                             struct device_attribute *attr,
                             char *buf)
{
  int value = -1;

  value = ltc_vgpio_read(dev, attr);
  if (value < 0) {
    /* error case */
    LTC_DEBUG("I2C read error");
    return -1;
  }

  return scnprintf(buf, PAGE_SIZE, "%d\n", value);
}

static const i2c_dev_attr_st ltc4281_attr_table[] = {
  {
    "in0_input",
    NULL,
    ltc_vol_show,
    NULL,
    LTC4281_VSOURCE_VDD_MSB, 1, 16,
  },
  {
    "in0_label",
    "Source Voltage",
    i2c_dev_show_label,
    NULL,
    0x0, 0, 0,
  },
  {
    "in1_input",
    NULL,
    ltc_vol_show,
    NULL,
    LTC4281_VSOURCE_VDD_MSB, 0, 16,
  },
  {
    "in1_label",
    "Drain Voltage",
    i2c_dev_show_label,
    NULL,
    0x0, 0, 0,
  },
  {
    "curr1_input",
    NULL,
    ltc_curr_show,
    NULL,
    LTC4281_VSENSE_MSB, 0, 16,
  },
  {
    "in2_label",
    "GPIO3 Voltage",
    i2c_dev_show_label,
    NULL,
    0x0, 0, 0,
  },
  {
    "in2_input",
    NULL,
    ltc_vgpio_show,
    NULL,
    LTC4281_VGPIO_MSB, 0, 16,
  },
  {
    "curr1_label",
    "Current",
    i2c_dev_show_label,
    NULL,
    0x0, 0, 0,
  },
  {
    "power1_input",
    NULL,
    ltc_power_show,
    NULL,
    LTC4281_POWER_MSB, 0, 16,
  },
  {
    "power1_label",
    "Power",
    i2c_dev_show_label,
    NULL,
    0x0, 0, 0,
  },
  {
    "temp1_input",
    NULL,
    ltc_temp_show,
    NULL,
    LTC4281_VGPIO_MSB, 0, 16,
  },
  {
    "temp1_label",
    "MOSFET Temp",
    i2c_dev_show_label,
    NULL,
    0x0, 0, 0,
  },
  {
    "on_fault_mask",
    "If 1, blocks the ON pin from clearing the FAULT register\n"
    "to prevent repeated logged faults and alerts.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_CONTROL, 7, 1,
  },
  {
    "ilim_adjust_adc_mode",
    "1: make the ADC operate in 16-bit mode\n"
    "0: make the ADC operate in 12-bit mode",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ILIM_ADJUST, 0, 1,
  },
  {
    "ilim_adjust_gpio_mode",
    "1: makes the ADC monitor GPIO2\n"
    "0: makes the ADC monitor GPIO3",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ILIM_ADJUST, 1, 1,
  },
  {
    "fault_log_enable",
    "Setting this bit to 1 enables registers 0x04 and 0x05\n"
    "to be written to the EEPROM when a fault bit transitions high",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ADC_CONTROL, 2, 1,
  },
  {
    "vgpio_alarm_max",
    "Threshold for Max Alarm on VGPIO",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_VGPIO_ALARM_MAX, 0, 8,
  },
  {
    "vgpio_alarm_min",
    "Threshold for Min Alarm on VGPIO",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_VGPIO_ALARM_MIN, 0, 8,
  },
  {
    "vsource_alarm_max",
    "Threshold for Max Alarm on VSOURCE",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_VSOURCE_ALARM_MAX, 0, 8,
  },
  {
    "vsource_alarm_min",
    "Threshold for Min Alarm on VSOURCE",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_VSOURCE_ALARM_MIN, 0, 8,
  },
  {
    "vsense_alarm_max",
    "Threshold for Max Alarm on VSENSE",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_VSENSE_ALARM_MAX, 0, 8,
  },
  {
    "vsense_alarm_min",
    "Threshold for Min Alarm on VSENSE",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_VSENSE_ALARM_MIN, 0, 8,
  },
  {
    "power_alarm_max",
    "Threshold for Max Alarm on POWER",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_POWER_ALARM_MAX, 0, 8,
  },
  {
    "power_alarm_min",
    "Threshold for Min Alarm on POWER",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_POWER_ALARM_MIN, 0, 8,
  },
  {
    "ov_status",
    "1 indicates that the input voltage is above the overvoltage threshold",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_STATUS_LSB, 0, 1,
  },
  {
    "uv_status",
    "1 indicates that the input voltage is below the undervoltage threshold",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_STATUS_LSB, 1, 1,
  },
  {
    "oc_cooldown_status",
    "1 indicates that an overcurrent fault has occurred\n"
    "and the part is going through a cool-down cycle.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_STATUS_LSB, 2, 1,
  },

  {
    "power_good_status",
    "1 indicates if the output voltage is greater than the power good threshold",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_STATUS_LSB, 3, 1,
  },
  {
    "on_pin_status",
    "1 indicates the status of the ON pin, 1 = high",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_STATUS_LSB, 4, 1,
  },

  {
    "fet_short_present",
    "1 indicates that the ADCs have detected a shorted MOSFET",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_STATUS_LSB, 5, 1,
  },
  {
    "fet_bad_cooldown_status",
    "1 indicates that an FET-BAD fault has occurred\n"
    "and the part is going through a cool-down cycle",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_STATUS_LSB, 6, 1,
  },
  {
    "on_status",
    "1 indicates if the MOSFETs are commanded to turn on",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_STATUS_LSB, 7, 1,
  },
  {
    "meter_overflow_present",
    "1 indicates that the energy meter accumulator has overflowed",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_STATUS_MSB, 0, 1,
  },
  {
    "ticker_overflow_present",
    "1 indicates that the tick counter has overflowed",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_STATUS_MSB, 1, 1,
  },
  {
    "adc_idle",
    "This bit indicates that the ADC is idle. It is always read as 0\n"
    "when the ADCs are free running,\n"
    "and will read a 1 when the ADC is idle in single shot mode",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_STATUS_MSB, 2, 1,
  },
  {
    "eeprom_busy",
    "This bit is high whenever the EEPROM is writing,\n"
    "and indicates that the EEPROM is not available\n"
    "until the write is complete",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_STATUS_MSB, 3, 1,
  },
  {
    "alert_status",
    "A 1 indicates that the ALERT pin is above its input threshold",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_STATUS_MSB, 4, 1,
  },

  {
    "gpio1_status",
    "A 1 indicates that the GPIO1 pin is above its input threshold",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_STATUS_MSB, 5, 1,
  },
  {
    "gpio2_status",
    "A 1 indicates that the GPIO2 pin is above its input threshold",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_STATUS_MSB, 6, 1,
  },
  {
    "gpio3_status",
    "A 1 indicates that the GPIO3 pin is above its input threshold",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_STATUS_MSB, 7, 1,
  },
  {
    "ov_fault",
    "Set to 1 by an overvoltage fault occurring.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_FAULT_LOG, 0, 1,
  },
  {
    "uv_fault",
    "Set to 1 by an undervoltage fault occurring.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_FAULT_LOG, 1, 1,
  },
  {
    "oc_fault",
    "Set to 1 by an overcurrent fault occurring.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_FAULT_LOG, 2, 1,
  },
  {
    "power_bad_fault",
    "Set to 1 by a power-bad fault occurring.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_FAULT_LOG, 3, 1,
  },
  {
    "on_fault",
    "Set to 1 by the ON pin changing state.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_FAULT_LOG, 4, 1,
  },

  {
    "fet_short_fault",
    "Set to 1 when the ADC detects a FET-short fault.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_FAULT_LOG, 5, 1,
  },
  {
    "fet_bad_fault",
    "Set to 1 when a FET-BAD fault occurs.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_FAULT_LOG, 6, 1,
  },
  {
    "eeprom_done",
    "Set to 1 when the EEPROM finishes a write.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_FAULT_LOG, 7, 1,
  },
  {
    "gpio_alarm_low",
    "Set to 1 when the ADC makes a measurement\n"
    "below the VGPIO_ALARM_MIN threshold.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ADC_ALERT_LOG, 0, 1,
  },
  {
    "gpio_alarm_high",
    "Set to 1 when the ADC makes a measurement\n"
    "above the VGPIO_ALARM_MAX threshold.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ADC_ALERT_LOG, 1, 1,
  },
  {
    "vsource_alarm_low",
    "Set to 1 when the ADC makes a measurement\n"
    "above the VSOURCE_ALARM_MIN threshold.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ADC_ALERT_LOG, 2, 1,
  },
  {
    "vsource_alarm_high",
    "Set to 1 when the ADC makes a measurement\n"
    "above the VSOURCE_ALARM_MAX threshold.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ADC_ALERT_LOG, 3, 1,
  },
  {
    "vsense_alarm_low",
    "Set to 1 when the ADC makes a measurement\n"
    "above the VSENSE_ALARM_MIN threshold.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ADC_ALERT_LOG, 4, 1,
  },

  {
    "vsense_alarm_high",
    "Set to 1 when the ADC makes a measurement\n"
    "above the VSENSE_ALARM_MAX threshold.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ADC_ALERT_LOG, 5, 1,
  },
  {
    "power_alarm_low",
    "Set to 1 when the ADC makes a measurement\n"
    "above the POWER_ALARM_MIN threshold.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ADC_ALERT_LOG, 6, 1,
  },
  {
    "power_alarm_high",
    "Set to 1 when the ADC makes a measurement\n"
    "above the POWER_ALARM_MAX threshold.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ADC_ALERT_LOG, 7, 1,
  },
  {
    "ov_fault_ee",
    "Set to 1 by an overvoltage fault occurring.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_FAULT_LOG_EE, 0, 1,
  },
  {
    "uv_fault_ee",
    "Set to 1 by an undervoltage fault occurring.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_FAULT_LOG_EE, 1, 1,
  },
  {
    "oc_fault_ee",
    "Set to 1 by an overcurrent fault occurring.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_FAULT_LOG_EE, 2, 1,
  },
  {
    "power_bad_fault_ee",
    "Set to 1 by a power-bad fault occurring.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_FAULT_LOG_EE, 3, 1,
  },
  {
    "on_fault_ee",
    "Set to 1 by the ON pin changing state.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_FAULT_LOG_EE, 4, 1,
  },

  {
    "fet_short_fault_ee",
    "Set to 1 when the ADC detects a FET-short fault.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_FAULT_LOG_EE, 5, 1,
  },
  {
    "fet_bad_fault_ee",
    "Set to 1 when a FET-BAD fault occurs.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_FAULT_LOG_EE, 6, 1,
  },
  {
    "eeprom_done_ee",
    "Set to 1 when the EEPROM finishes a write.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_FAULT_LOG_EE, 7, 1,
  },
  {
    "gpio_alarm_low_ee",
    "Set to 1 when the ADC makes a measurement\n"
    "below the VGPIO_ALARM_MIN threshold.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ADC_ALERT_LOG_EE, 0, 1,
  },
  {
    "gpio_alarm_high_ee",
    "Set to 1 when the ADC makes a measurement\n"
    "above the VGPIO_ALARM_MAX threshold.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ADC_ALERT_LOG_EE, 1, 1,
  },
  {
    "vsource_alarm_low_ee",
    "Set to 1 when the ADC makes a measurement\n"
    "above the VSOURCE_ALARM_MIN threshold.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ADC_ALERT_LOG_EE, 2, 1,
  },
  {
    "vsource_alarm_high_ee",
    "Set to 1 when the ADC makes a measurement\n"
    "above the VSOURCE_ALARM_MAX threshold.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ADC_ALERT_LOG_EE, 3, 1,
  },
  {
    "vsense_alarm_low_ee",
    "Set to 1 when the ADC makes a measurement\n"
    "above the VSENSE_ALARM_MIN threshold.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ADC_ALERT_LOG_EE, 4, 1,
  },

  {
    "vsense_alarm_high_ee",
    "Set to 1 when the ADC makes a measurement\n"
    "above the VSENSE_ALARM_MAX threshold.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ADC_ALERT_LOG_EE, 5, 1,
  },
  {
    "power_alarm_low_ee",
    "Set to 1 when the ADC makes a measurement\n"
    "above the POWER_ALARM_MIN threshold.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ADC_ALERT_LOG_EE, 6, 1,
  },
  {
    "power_alarm_high_ee",
    "Set to 1 when the ADC makes a measurement\n"
    "above the POWER_ALARM_MAX threshold.",
    I2C_DEV_ATTR_SHOW_DEFAULT,
    I2C_DEV_ATTR_STORE_DEFAULT,
    LTC4281_ADC_ALERT_LOG_EE, 7, 1,
  },
};

/*
 * LTC4281 i2c addresses.
 */
static const unsigned short normal_i2c[] = {
  0x4a, I2C_CLIENT_END
};

/*LTC4281  id */
static const struct i2c_device_id ltc4281_id[] = {
  {"ltc4281", 0},
  { },
};
MODULE_DEVICE_TABLE(i2c, ltc4281_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int ltc4281_detect(struct i2c_client *client,
                           struct i2c_board_info *info)
{
  /*
   * We don't currently do any detection of the driver
   */
  strlcpy(info->type, "ltc4281", I2C_NAME_SIZE);
  return 0;
}

static int ltc4281_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  int n_attrs = ARRAY_SIZE(ltc4281_attr_table);
  struct device *dev = &client->dev;
  i2c_dev_data_st *data;

  data = devm_kzalloc(dev, sizeof(i2c_dev_data_st), GFP_KERNEL);
  if (!data) {
    return -ENOMEM;
  }

  return i2c_dev_sysfs_data_init(client, data, ltc4281_attr_table, n_attrs);
}

static int ltc4281_remove(struct i2c_client *client)
{
  i2c_dev_data_st *data = i2c_get_clientdata(client);
  i2c_dev_sysfs_data_clean(client, data);

  return 0;
}

static struct i2c_driver ltc4281_driver = {
  .class    = I2C_CLASS_HWMON,
  .driver = {
    .name = "ltc4281",
  },
  .probe    = ltc4281_probe,
  .remove   = ltc4281_remove,
  .id_table = ltc4281_id,
  .detect   = ltc4281_detect,
  .address_list = normal_i2c,
};

static int __init ltc4281_mod_init(void)
{
  return i2c_add_driver(&ltc4281_driver);
}

static void __exit ltc4281_mod_exit(void)
{
  i2c_del_driver(&ltc4281_driver);
}

MODULE_AUTHOR("Mickey Zhan");
MODULE_DESCRIPTION("LTC4281 Driver");
MODULE_LICENSE("GPL");

module_init(ltc4281_mod_init);
module_exit(ltc4281_mod_exit);
