/*
 * Driver for Linear Technology LTC4282 I2C Negative Voltage Hot Swap Controller
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option)any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/jiffies.h>
#include <linux/delay.h>

/* chip registers */
#define LTC4282_CONTROL                 0x00    /* word */
#define LTC4282_ALERT                   0x02    /* word */
#define LTC4282_FAULT_LOG               0x04    /* byte */
#define LTC4282_ADC_ALERT_LOG           0x05    /* byte */
#define LTC4282_ADJUST                  0x11    /* byte */
#define LTC4282_ADC_CONTROL             0x1D    /* byte */
#define LTC4282_STATUS                  0x1E    /* readonly word */

#define LTC4282_VGPIO_ALARM_MIN         0x08    /* byte */
#define LTC4282_VGPIO_ALARM_MAX         0x09    /* byte */
#define LTC4282_VSOURCE_ALARM_MIN       0x0A    /* byte */
#define LTC4282_VSOURCE_ALARM_MAX       0x0B    /* byte */
#define LTC4282_VSENSE_ALARM_MIN        0x0C    /* byte */
#define LTC4282_VSENSE_ALARM_MAX        0x0D    /* byte */
#define LTC4282_VGPIO                   0x34    /* word */
#define LTC4282_VGPIO_MIN               0x36    /* word */
#define LTC4282_VGPIO_MAX               0x38    /* word */
#define LTC4282_VSOURCE                 0x3A    /* word */
#define LTC4282_VSOURCE_MIN             0x3C    /* word */
#define LTC4282_VSOURCE_MAX             0x3E    /* word */
#define LTC4282_VSENSE                  0x40    /* word */
#define LTC4282_VSENSE_MIN              0x42    /* word */
#define LTC4282_VSENSE_MAX              0x44    /* word */
#define LTC4282_POWER                   0x46    /* word */
#define LTC4282_POWER_MIN               0x48    /* word */
#define LTC4282_POWER_MAX               0x4A    /* word */
#define LTC4282_TEMP                    LTC4282_VGPIO
#define LTC4282_TEMP_MIN                LTC4282_VGPIO_MIN
#define LTC4282_TEMP_MAX                LTC4282_VGPIO_MAX

/*
 * Calculate sensor value
 * value = (reg_val * slope + intercept) / unit
 * 
 * Following formula come from linear correction by sampling 
 * a set of reg & real values.
 * Values must be recalibration if schematic change
 */
#define VSOURCE_SLOPE                   2523
#define VSOURCE_INTERCEPT               578530
#define VSENSE_SLOPE                    34902
#define VSENSE_INTERCEPT                971700
#define VGPIO3_SLOPE                    14412
#define VGPIO3_INTERCEPT                (-15000000)
#define TEMP_SLOPE                      VGPIO3_SLOPE
#define TEMP_INTERCEPT                  VGPIO3_INTERCEPT
#define UNIT_MV                         10000
#define UNIT_MA                         10000
#define UNIT_C                          10000

#define LTC4282_REBOOT                  (1 << 7)
#define ON_FAULT_MASK                   (1 << 7)
#define RESOLUTION_16_BIT               (1 << 0)
#define VOLTAGE_SELECT                  (1 << 2)
#define VGPIO_SELECT                    (1 << 1)

#define ADC_16_BIT_MODE_DELAY           1100
#define ADC_12_BIT_MODE_DELAY           100

#define OC_AUTORETRY                    (1 << 2)

#define FAULT_LOG_EN                    (1 << 2)
/* Fault log register bits */
#define FAULT_OV                    0
#define FAULT_UV                    1
#define FAULT_OC                    2
#define FAULT_POWER                 3
#define ON_FAULT                    4
#define FAULT_FET_SHORT             5
#define FAULT_FET_BAD               6
#define EEPROM_DONE                 7

/* ADC Alert log */
#define POWER_ALARM_HIGH            7
#define POWER_ALARM_LOW             6
#define VSENSE_ALARM_HIGH           5
#define VSENSE_ALARM_LOW            4
#define VSOURCE_ALARM_HIGH          3
#define VSOURCE_ALARM_LOW           2
#define GPIO_ALARM_HIGH             1
#define GPIO_ALARM_LOW              0
#define TEMP_ALARM_HIGH             GPIO_ALARM_HIGH

/* Fault status register bits */
#define ON_STATUS                    15
#define FET_BAD                      14
#define FET_SHORT                    13
#define ON_PIN_STATUS                12
#define POWER_GOOD                   11
#define OC_STATUS                    10
#define UV_STATUS                    9
#define OV_STATUS                    8
#define GPIO3_STATUS                 7
#define GPIO2_STATUS                 6
#define GPIO1_STATUS                 5
#define ALERT_STATUS                 4
#define EEPROM_BUSY                  3
#define ADC_IDLE                     2
#define TICKER_OVERFLOW              1
#define METER_OVERFLOW               0
#define FAULT_TEMP                   GPIO3_STATUS

enum registers_t {
    ltc4282_reg_status,
    ltc4282_reg_fault,
    ltc4282_reg_alert,
    ltc4282_reg_adc_alert,
    ltc4282_reg_control,
    ltc4282_reg_adjust,

    ltc4282_reg_vin,
    ltc4282_reg_vout,
    ltc4282_reg_curr,
    ltc4282_reg_power,
    ltc4282_reg_temp,

    ltc4282_reg_max
};

enum sysfs_t {
    ltc4282_vin,
    ltc4282_vout,
    ltc4282_curr, 
    ltc4282_power,
    ltc4282_temp,
}; 

struct ltc4282_data {
    struct i2c_client *client;

    struct mutex update_lock;
    bool valid;
    unsigned long last_updated; /* in jiffies */

    /* Registers */
    u32 regs[ltc4282_reg_max];
}; 

#define TEMP_XLAT_TABLE_SIZE 256
/*
 * Hotswap mosfet temp and register mapping table, len TEMP_XLAT_TABLE_SIZE, unit: mC
 * We sample many set of register and real temp, then we do linear calibration to generate
 * a formula which is -0.00000000002656*reg_msb^6 + 0.000000018325*reg_msb^5
 * - 0.000005001*reg_msb^4 + 0.000694575*reg_msb^3 - 0.0537*reg_msb^2 + 2.764*reg_msb
 * - 21.059, then we use formula to generate this table.
 */
static int temp_table[TEMP_XLAT_TABLE_SIZE] = {-8500, -8500, -8500, -8500,
      -8500, -8500, -6260, -4120, -2050, -60, 1860, 3700, 5480, 7190, 8840,
      10420, 11950, 13430, 14850, 16230, 17550, 18840, 20070, 21270, 22430,
      23550, 24640, 25690, 26710, 27700, 28660, 29590, 30500, 31390, 32250,
      33090, 33910, 34710, 35490, 36250, 37000, 37730, 38450, 39150, 39850,
      40530, 41190, 41850, 42500, 43140, 43770, 44390, 45000, 45610, 46210,
      46800, 47390, 47970, 48550, 49120, 49690, 50250, 50810, 51360, 51910,
      52450, 53000, 53530, 54070, 54600, 55130, 55660, 56180, 56700, 57210,
      57730, 58240, 58740, 59250, 59750, 60250, 60740, 61230, 61720, 62210,
      62690, 63170, 63650, 64120, 64590, 65050, 65510, 65970, 66430, 66880,
      67330, 67770, 68210, 68650, 69080, 69510, 69930, 70350, 70770, 71180,
      71590, 71990, 72390, 72790, 73180, 73570, 73950, 74330, 74710, 75080,
      75450, 75810, 76170, 76530, 76890, 77240, 77580, 77930, 78270, 78600,
      78940, 79270, 79600, 79930, 80250, 80570, 80890, 81210, 81520, 81840,
      82150, 82460, 82770, 83080, 83380, 83690, 83990, 84300, 84600, 84910,
      85210, 85520, 85820, 86130, 86440, 86750, 87050, 87360, 87670, 87990,
      88300, 88620, 88930, 89250, 89570, 89900, 90220, 90550, 90880, 91210,
      91550, 91880, 92220, 92560, 92900, 93250, 93590, 93940, 94290, 94640,
      94990, 95350, 95700, 96050, 96410, 96760, 97110, 97470, 97820, 98170,
      98510, 98850, 99190, 99530, 99860, 100180, 100500, 100810, 101110,
      101400, 101680, 101950, 102210, 102460, 102690, 102900, 103100,
      103280, 103430, 103570, 103680, 103770, 103830, 103870, 103870,
      103870, 103870, 103870, 103870, 103870, 103870, 103870, 103870,
      103870, 103870, 103870, 103870, 103870, 103870, 103870, 103870,
      103870, 103870, 103870, 103870, 103870, 103870, 103870, 103870,
      103870, 103870, 103870, 103870, 103870, 103870, 103870, 103870,
      103870, 103870, 103870, 103870, 103870, 103870, 103870, 103870,
      103870, 103870, 103870, 103870, 103870, 103870};

/* 
 * ltc4282 Vin/Vout adc convert to mVolts 
 * refer to ltc4282 datasheet page 23
 */
static inline int ltc4282_vsourve_calculate(int reg_val) {
    return reg_val > 0 ? (reg_val * 16640 / 65535) : 0;
}

/* 
 * ltc4282 Current adc convert to mAmps
 * refer to ltc4282 datasheet page 23:
 * Rsense = 0.0001875 ohm
 * (reg_val * 0.04 / 65535 / 0.0001875) = xxx Amp
 * To make it compatible with sensors command, convert it to mAmp
 * (reg_val * 4 / 65535 / 1875 * 10^5) * 1000 = xxx mAmp
 * But if calculately directly, it will overflow and get a wrong result.
 * Assume reg_val is 20000, with (reg_val * 4 / 65535 / 1875 * 10^5) * 1000,
 * we will get a zero result. And to make it be closer with the real value,
 * we need to change the order of the formula:
 * (reg_val * 4 * 1000 / 255 * 1000 / 255 * 1000 / 1875 / 10) = xxx mAmp
 * (20000 * 4 * 1000 / 255 * 1000 / 255 * 1000 / 1875 / 10) = 65615 mAmp
 * new formula is better than the original one.
 */
static inline int ltc4282_vsense_calculate(int reg_val) {
    if (reg_val > 0) {
        int curr = reg_val * 4 * 1000;
        curr /= 255;
        curr *= 1000;
        curr /= 255;
        curr *= 1000;
        curr /= 1875;
        return curr / 10;
    } else {
        return 0;
    }
}

/* 
 * ltc4282 Power adc convert to uWatts
 * refer to ltc4282 datasheet page 23
 * Rsense = 0.0001875 ohm
 * (reg_val * 16.64 * 0.04 / 65535 / 0.0001875) = xxx Watts
 * To make it compatible with sensors command, convert it to uWatts
 * (reg_val * 1664 * 4 / 65535 / 1875 * 1000) * 1000 * 1000 = xxx uWatts
 * But if calculately directly, it will overflow and get a wrong result.
 * Assume reg_val is 20000,
 * with (reg_val * 1664 * 4 / 65535 / 1875 * 1000) * 1000 * 1000,
 * we will get a zero result. And to make it be closer with the real value,
 * we need to change the order of the formula:
 * (reg_val * 1664 * 4 / 255 * 1000 / 255 / 1875) * 1000 * 1000 = 1 * 10^6 uWatts
 * (20000 * 1664 * 4 / 255 * 1000 / 255 / 1875) * 1000 * 1000 = 1.091 * 10^9 uWatts
 * new formula is better than the original one.
 */
static inline int ltc4282_power_calculate(int reg_val) {
    if (reg_val > 0) {
        int power = reg_val * 1664 * 4;
        power /= 255;
        power *= 1000;
        power /= 255;
        power /= 1875;
        return power * 1000 * 1000;
    } else {
        return 0;
    }
}

/* 
 * ltc4282 Hot Swap Temp adc convert to degree mC
 * As lookup table discard LSBs, need shift to right 8 bits
 */
static inline int ltc4282_temp_calculate(int reg_val) {
    u8 reg;
    reg = reg_val >> 8;
    return temp_table[reg];
}

static struct ltc4282_data *ltc4282_update_adc(struct device *dev, int reg) {
    struct ltc4282_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    struct ltc4282_data *ret = data;
    int val;

    mutex_lock(&data->update_lock);

    switch (reg) {
    case ltc4282_reg_vin:
        val = i2c_smbus_read_byte_data(client, LTC4282_ADJUST);
        val |= VOLTAGE_SELECT;
        i2c_smbus_write_byte_data(client, LTC4282_ADJUST, val);
        if(val | RESOLUTION_16_BIT) {
            msleep(ADC_16_BIT_MODE_DELAY);
        } else {
            msleep(ADC_12_BIT_MODE_DELAY);
        }
        val = i2c_smbus_read_word_data(client, LTC4282_VSOURCE);
        val = (u16)(val << 8) | (val >> 8);
        break; 
    case ltc4282_reg_vout:
        val = i2c_smbus_read_byte_data(client, LTC4282_ADJUST);
        val &= (~VOLTAGE_SELECT);
        i2c_smbus_write_byte_data(client, LTC4282_ADJUST, val);
        if(val | RESOLUTION_16_BIT) {
            msleep(ADC_16_BIT_MODE_DELAY);
        } else {
            msleep(ADC_12_BIT_MODE_DELAY);
        }
        val = i2c_smbus_read_word_data(client, LTC4282_VSOURCE);
        val = (u16)(val << 8) | (val >> 8);
        break; 
    case ltc4282_reg_curr:
        val = i2c_smbus_read_word_data(client, LTC4282_VSENSE);
        val = (u16)(val << 8) | (val >> 8);
        break; 
    case ltc4282_reg_power:
        val = i2c_smbus_read_word_data(client, LTC4282_POWER);
        val = (u16)(val << 8) | (val >> 8);
        break;
    case ltc4282_reg_temp:
        val = i2c_smbus_read_byte_data(client, LTC4282_ADJUST);
        if(val | VGPIO_SELECT) {
            val &= (~VGPIO_SELECT);
            i2c_smbus_write_byte_data(client, LTC4282_ADJUST, val);
        }
        if(val | RESOLUTION_16_BIT) {
            msleep(ADC_16_BIT_MODE_DELAY);
        } else {
            msleep(ADC_12_BIT_MODE_DELAY);
        }
        val = i2c_smbus_read_word_data(client, LTC4282_TEMP);
        val = (u16)(val << 8) | (val >> 8);
        break;
    default:
        break;
    }
    
    if (unlikely(val < 0)) {
        dev_dbg(dev, "Failed to read ADC value: error %d\n", val);
        ret = ERR_PTR(val); 
        goto abort; 
    }
    data->regs[reg] = val;

abort:
    mutex_unlock( &data->update_lock);
    return ret;
}

static struct ltc4282_data *ltc4282_update_device(struct device *dev) {
    struct ltc4282_data *data = dev_get_drvdata(dev); 
    struct i2c_client *client = data->client;
    struct ltc4282_data *ret = data;

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + HZ) && !data->valid) {
        int i;

        /* Read registers */
        for (i = 0; i < ARRAY_SIZE(data->regs); i++) {
            int val; 
            switch (i) {
            case ltc4282_reg_status:
                val = i2c_smbus_read_word_data(client, LTC4282_STATUS);
                val = (u16)(val << 8) | (val >> 8);
                break;
            case ltc4282_reg_fault:
                val = i2c_smbus_read_byte_data(client, LTC4282_FAULT_LOG);
                break; 
            case ltc4282_reg_alert:
                val = i2c_smbus_read_word_data(client, LTC4282_ALERT);
                val = (u16)(val << 8) | (val >> 8);
                break;
            case ltc4282_reg_adc_alert:
                val = i2c_smbus_read_byte_data(client, LTC4282_ADC_ALERT_LOG);
                break; 
            case ltc4282_reg_control:
                val = i2c_smbus_read_byte_data(client, LTC4282_CONTROL); 
                break;
            case ltc4282_reg_adjust:
                val = i2c_smbus_read_byte_data(client, LTC4282_ADJUST); 
                break;
            default:
                val = 0;
                break;
            }
            
            if (unlikely(val < 0)) {
                dev_dbg(dev, "Failed to read ADC value: error %d\n", val);
                ret = ERR_PTR(val);
                data->valid = 0;
                goto abort;
            }
            data->regs[i] = val;
        }
        data->last_updated = jiffies;
        data->valid = 1; 
    }
abort:
    mutex_unlock( &data->update_lock); 
    return ret; 
}

/* Return the voltage from the given register in mV or mA */
static int ltc4282_reg_to_value(struct ltc4282_data * data, u8 reg) {
    int val;

    if (IS_ERR(data))
        return PTR_ERR(data);

    val = data->regs[reg];

    switch (reg) {
    case ltc4282_reg_vin:
    case ltc4282_reg_vout:
        /* Linear. Convert to mV. */
        val = ltc4282_vsourve_calculate(val);
        break; 
    case ltc4282_reg_curr:
        /* Linear. Convert to mA. */
        val = ltc4282_vsense_calculate(data->regs[reg]);
        break;
    case ltc4282_reg_power:
        /* Linear Convert to mW */
        val = ltc4282_power_calculate(val);
        break;
    case ltc4282_reg_temp:
        /* Linear. Convert to mC. */
        val = ltc4282_temp_calculate(val);
        break;
    default:
        /* If we get here, the developer messed up */
        WARN_ON_ONCE(1); 
        val = 0; 
        break; 
    }

    return val; 
}

static ssize_t ltc4282_show_value(struct device * dev, 
                                  struct device_attribute *dev_attr, char *buf) {
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr); 
    struct ltc4282_data *data = ltc4282_update_device(dev);
    int value;

    if (IS_ERR(data))
        return PTR_ERR(data);

    switch(attr->index) {
    case ltc4282_vin:
        data = ltc4282_update_adc(dev, ltc4282_reg_vin);
        value = ltc4282_reg_to_value(data, ltc4282_reg_vin); 
        break;
    case ltc4282_vout:
        data = ltc4282_update_adc(dev, ltc4282_reg_vout);
        value = ltc4282_reg_to_value(data, ltc4282_reg_vout); 
        break;
    case ltc4282_curr:
        data = ltc4282_update_adc(dev, ltc4282_reg_curr);
        value = ltc4282_reg_to_value(data, ltc4282_reg_curr); 
        break;
    case ltc4282_power:
        data = ltc4282_update_adc(dev, ltc4282_reg_power);
        value = ltc4282_reg_to_value(data, ltc4282_reg_power);
        break;
    case ltc4282_temp:
        data = ltc4282_update_adc(dev, ltc4282_reg_temp);
        value = ltc4282_reg_to_value(data, ltc4282_reg_temp);
        break;
    }
    
    return snprintf(buf, PAGE_SIZE, "%d\n", value); 
}

static ssize_t ltc4282_show_alarm(struct device *dev, 
                                  struct device_attribute *dev_attr, char *buf) {
    struct sensor_device_attribute * attr = to_sensor_dev_attr(dev_attr); 
    struct ltc4282_data *data = dev_get_drvdata(dev);
    u8 val = 0;
    u8 fault = 0; 

    if (IS_ERR(data))
        return PTR_ERR(data);

    val = i2c_smbus_read_byte_data(data->client, LTC4282_ADC_ALERT_LOG);
    fault = val & (1 << attr->index);
    if (fault)        /* Clear reported faults in chip register */
        i2c_smbus_write_byte_data(data->client, LTC4282_ADC_ALERT_LOG, ~fault & val);

    return snprintf(buf, PAGE_SIZE, "%d\n", fault?1:0);
}

static ssize_t ltc4282_show_fault(struct device *dev,
                                  struct device_attribute *dev_attr, char *buf) {
    struct sensor_device_attribute * attr = to_sensor_dev_attr(dev_attr);
    struct ltc4282_data *data = dev_get_drvdata(dev);
    u8 val = 0;
    u8 fault = 0;

    if (IS_ERR(data))
        return PTR_ERR(data);

    val = i2c_smbus_read_byte_data(data->client, LTC4282_FAULT_LOG);
    fault = val & (1 << attr->index);
    if (fault)        /* Clear reported faults in chip register */
        i2c_smbus_write_byte_data(data->client, LTC4282_FAULT_LOG, ~fault & val);

    return snprintf(buf, PAGE_SIZE, "%d\n", fault?1:0);
}

static ssize_t ltc4282_show_status(struct device *dev,
                                   struct device_attribute *dev_attr, char *buf) {
    struct sensor_device_attribute * attr = to_sensor_dev_attr(dev_attr);
    struct ltc4282_data *data = dev_get_drvdata(dev);
    u16 val = 0;
    u16 fault = 0;

    if (IS_ERR(data))
        return PTR_ERR(data);

    val = i2c_smbus_read_word_data(data->client, LTC4282_STATUS);
    val = (u16)(val << 8) | (val >> 8);
    fault = val & (1 << attr->index);
    return snprintf(buf, PAGE_SIZE, "%d\n", fault?1:0);
}

static ssize_t ltc4282_show_fet_as_voltage(struct device *dev,
                                           struct device_attribute *dev_attr, char *buf) {
    struct sensor_device_attribute * attr = to_sensor_dev_attr(dev_attr);
    struct ltc4282_data *data = dev_get_drvdata(dev);
    u16 val = 0;
    u16 fault = 0;

    if (IS_ERR(data))
        return PTR_ERR(data);

    val = i2c_smbus_read_word_data(data->client, LTC4282_STATUS);
    val = (u16)(val << 8) | (val >> 8);
    fault = val & (1 << attr->index);
    return snprintf(buf, PAGE_SIZE, "%d\n", fault ? 1000 : 0);
}

static ssize_t ltc4282_do_reboot(struct device *dev, struct device_attribute *dev_attr,
                                 const char *buf, size_t count){
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct ltc4282_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    long val;
    int res;

    res = kstrtoul(buf, 10, &val);
    if (res)
    return res;
    
    if(val == 1) {
        int reg_var = i2c_smbus_read_byte_data(client, LTC4282_ADC_CONTROL);
        reg_var |= LTC4282_REBOOT;
        i2c_smbus_write_byte_data(client, LTC4282_ADC_CONTROL, reg_var);
    }

    return count;
}

/* Input/Output voltages */
static SENSOR_DEVICE_ATTR(in1_input, S_IRUGO, ltc4282_show_value, NULL,
              ltc4282_vin);
static SENSOR_DEVICE_ATTR(in2_input, S_IRUGO, ltc4282_show_value, NULL,
              ltc4282_vout);

/* Current (via sense resistor) */
static SENSOR_DEVICE_ATTR(curr1_input, S_IRUGO, ltc4282_show_value, NULL,
              ltc4282_curr);

/* Power */
static SENSOR_DEVICE_ATTR(power1_input, S_IRUGO, ltc4282_show_value, NULL,
              ltc4282_power);

/* Mosfet Temp(GPIO3 ADC mode) */
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, ltc4282_show_value, NULL,
              ltc4282_temp);

/* Fault log system file nodes */
static SENSOR_DEVICE_ATTR(fault_ov, S_IRUGO, ltc4282_show_fault, NULL,
              FAULT_OV);
static SENSOR_DEVICE_ATTR(fault_uv, S_IRUGO, ltc4282_show_fault, NULL,
              FAULT_UV);
static SENSOR_DEVICE_ATTR(fault_oc, S_IRUGO, ltc4282_show_fault, NULL,
              FAULT_OC);
static SENSOR_DEVICE_ATTR(fault_power, S_IRUGO, ltc4282_show_fault, NULL,
              FAULT_POWER);
static SENSOR_DEVICE_ATTR(on_fault, S_IRUGO, ltc4282_show_fault, NULL,
              ON_FAULT);
static SENSOR_DEVICE_ATTR(fault_fet_short, S_IRUGO, ltc4282_show_fault, NULL,
              FAULT_FET_SHORT);
static SENSOR_DEVICE_ATTR(fault_fet_bad, S_IRUGO, ltc4282_show_fault, NULL,
              FAULT_FET_BAD);
static SENSOR_DEVICE_ATTR(eeprom_done, S_IRUGO, ltc4282_show_fault, NULL,
              EEPROM_DONE);

/* ADC alert system file nodes */
static SENSOR_DEVICE_ATTR(power_alarm_high, S_IRUGO, ltc4282_show_alarm, NULL,
              POWER_ALARM_HIGH);
static SENSOR_DEVICE_ATTR(power_alarm_low, S_IRUGO, ltc4282_show_alarm, NULL,
              POWER_ALARM_LOW);
static SENSOR_DEVICE_ATTR(vsense_alarm_high, S_IRUGO, ltc4282_show_alarm, NULL,
              VSENSE_ALARM_HIGH);
static SENSOR_DEVICE_ATTR(vsense_alarm_low, S_IRUGO, ltc4282_show_alarm, NULL,
              VSENSE_ALARM_LOW);
static SENSOR_DEVICE_ATTR(vsource_alarm_high, S_IRUGO, ltc4282_show_alarm, NULL,
              VSOURCE_ALARM_HIGH);
static SENSOR_DEVICE_ATTR(vsource_alarm_low, S_IRUGO, ltc4282_show_alarm, NULL,
              VSOURCE_ALARM_LOW);
static SENSOR_DEVICE_ATTR(gpio_alarm_high, S_IRUGO, ltc4282_show_alarm, NULL,
              GPIO_ALARM_HIGH);
static SENSOR_DEVICE_ATTR(gpio_alarm_low, S_IRUGO, ltc4282_show_alarm, NULL,
              GPIO_ALARM_LOW);

/* Fault status system file nodes */
static SENSOR_DEVICE_ATTR(on_status, S_IRUGO, ltc4282_show_status, NULL,
              ON_STATUS);
static SENSOR_DEVICE_ATTR(fet_bad, S_IRUGO, ltc4282_show_status, NULL,
              FET_BAD);
static SENSOR_DEVICE_ATTR(fet_short, S_IRUGO, ltc4282_show_status, NULL,
              FET_SHORT);
static SENSOR_DEVICE_ATTR(on_pin_status, S_IRUGO, ltc4282_show_status, NULL,
              ON_PIN_STATUS);
static SENSOR_DEVICE_ATTR(power_good, S_IRUGO, ltc4282_show_status, NULL,
              POWER_GOOD);
static SENSOR_DEVICE_ATTR(oc_status, S_IRUGO, ltc4282_show_status, NULL,
              OC_STATUS);
static SENSOR_DEVICE_ATTR(uv_status, S_IRUGO, ltc4282_show_status, NULL,
              UV_STATUS);
static SENSOR_DEVICE_ATTR(ov_status, S_IRUGO, ltc4282_show_status, NULL,
              OV_STATUS);
static SENSOR_DEVICE_ATTR(gpio3_status, S_IRUGO, ltc4282_show_status, NULL,
              GPIO3_STATUS);
static SENSOR_DEVICE_ATTR(gpio2_status, S_IRUGO, ltc4282_show_status, NULL,
              GPIO2_STATUS);
static SENSOR_DEVICE_ATTR(gpio1_status, S_IRUGO, ltc4282_show_status, NULL,
              GPIO1_STATUS);
static SENSOR_DEVICE_ATTR(alert_status, S_IRUGO, ltc4282_show_status, NULL,
              ALERT_STATUS);
static SENSOR_DEVICE_ATTR(eeprom_busy, S_IRUGO, ltc4282_show_status, NULL,
              EEPROM_BUSY);
static SENSOR_DEVICE_ATTR(adc_idle, S_IRUGO, ltc4282_show_status, NULL,
              ADC_IDLE);
static SENSOR_DEVICE_ATTR(ticker_overflow, S_IRUGO, ltc4282_show_status, NULL,
              TICKER_OVERFLOW);
static SENSOR_DEVICE_ATTR(meter_overflow, S_IRUGO, ltc4282_show_status, NULL,
              METER_OVERFLOW);

/* expose fet_bad_status fet_short_status as voltage */
static SENSOR_DEVICE_ATTR(in3_input, S_IRUGO, ltc4282_show_fet_as_voltage, NULL,
              FET_BAD);
static SENSOR_DEVICE_ATTR(in4_input, S_IRUGO, ltc4282_show_fet_as_voltage, NULL,
              FET_SHORT);

static SENSOR_DEVICE_ATTR(reboot, S_IWUSR, NULL, ltc4282_do_reboot,
              0);

static struct attribute *ltc4282_attrs[] =  {
    &sensor_dev_attr_in1_input.dev_attr.attr,
    &sensor_dev_attr_in2_input.dev_attr.attr,
    &sensor_dev_attr_curr1_input.dev_attr.attr,
    &sensor_dev_attr_power1_input.dev_attr.attr,
    &sensor_dev_attr_temp1_input.dev_attr.attr,

    &sensor_dev_attr_fault_ov.dev_attr.attr,
    &sensor_dev_attr_fault_uv.dev_attr.attr,
    &sensor_dev_attr_fault_oc.dev_attr.attr,
    &sensor_dev_attr_fault_power.dev_attr.attr,
    &sensor_dev_attr_on_fault.dev_attr.attr,
    &sensor_dev_attr_fault_fet_short.dev_attr.attr,
    &sensor_dev_attr_fault_fet_bad.dev_attr.attr,
    &sensor_dev_attr_eeprom_done.dev_attr.attr,

    &sensor_dev_attr_power_alarm_high.dev_attr.attr,
    &sensor_dev_attr_power_alarm_low.dev_attr.attr,
    &sensor_dev_attr_vsense_alarm_high.dev_attr.attr,
    &sensor_dev_attr_vsense_alarm_low.dev_attr.attr,
    &sensor_dev_attr_vsource_alarm_high.dev_attr.attr,
    &sensor_dev_attr_vsource_alarm_low.dev_attr.attr,
    &sensor_dev_attr_gpio_alarm_high.dev_attr.attr,
    &sensor_dev_attr_gpio_alarm_low.dev_attr.attr,

    &sensor_dev_attr_on_status.dev_attr.attr,
    &sensor_dev_attr_fet_bad.dev_attr.attr,
    &sensor_dev_attr_fet_short.dev_attr.attr,
    &sensor_dev_attr_on_pin_status.dev_attr.attr,
    &sensor_dev_attr_power_good.dev_attr.attr,
    &sensor_dev_attr_oc_status.dev_attr.attr,
    &sensor_dev_attr_uv_status.dev_attr.attr,
    &sensor_dev_attr_ov_status.dev_attr.attr,
    &sensor_dev_attr_gpio3_status.dev_attr.attr,
    &sensor_dev_attr_gpio2_status.dev_attr.attr,
    &sensor_dev_attr_gpio1_status.dev_attr.attr,
    &sensor_dev_attr_alert_status.dev_attr.attr,
    &sensor_dev_attr_eeprom_busy.dev_attr.attr,
    &sensor_dev_attr_adc_idle.dev_attr.attr,
    &sensor_dev_attr_ticker_overflow.dev_attr.attr,
    &sensor_dev_attr_meter_overflow.dev_attr.attr,

    &sensor_dev_attr_in3_input.dev_attr.attr,
    &sensor_dev_attr_in4_input.dev_attr.attr,

    &sensor_dev_attr_reboot.dev_attr.attr,
    NULL, 
}; 
ATTRIBUTE_GROUPS(ltc4282); 

static int ltc4282_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    struct i2c_adapter *adapter = client->adapter;
    struct device *dev =  &client->dev;
    struct ltc4282_data *data;
    struct device *hwmon_dev;
    int val;

    if ( !i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
        return -ENODEV; 

    if (i2c_smbus_read_word_data(client, LTC4282_STATUS) < 0) {
        dev_err(dev, "Failed to read status register\n");
        return -ENODEV; 
    }

    val = i2c_smbus_read_word_data(client, LTC4282_CONTROL);
    val |= OC_AUTORETRY;
    val &= ~ON_FAULT_MASK;
    i2c_smbus_write_word_data(client, LTC4282_CONTROL, val);

    /* enable 12-bit mode and set ADC converter to monitor gpio3 */
    val = i2c_smbus_read_byte_data(client, LTC4282_ADJUST);
    val &= ~RESOLUTION_16_BIT;
    val &= ~VGPIO_SELECT;
    i2c_smbus_write_byte_data(client, LTC4282_ADJUST, val);

    val = i2c_smbus_read_byte_data(client, LTC4282_ADC_CONTROL);
    val |= FAULT_LOG_EN;
    i2c_smbus_write_byte_data(client, LTC4282_ADC_CONTROL, val);

    data = devm_kzalloc(dev, sizeof( *data), GFP_KERNEL);
    if ( !data)
        return -ENOMEM;

    data->client = client;
    mutex_init( &data ->update_lock);

    /* Clear faults */
    i2c_smbus_write_byte_data(client, LTC4282_FAULT_LOG, 0x00);

    hwmon_dev = devm_hwmon_device_register_with_groups(dev, client->name,
                                                       data, ltc4282_groups);
    return PTR_ERR_OR_ZERO(hwmon_dev);
}

static const struct i2c_device_id ltc4282_id[] = {
    {"ltc4282", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, ltc4282_id); 

/* This is the driver that will be inserted */
static struct i2c_driver ltc4282_driver =  {
    .driver =  {
        .name = "ltc4282", 
    }, 
    .probe = ltc4282_probe, 
    .id_table = ltc4282_id, 
}; 

module_i2c_driver(ltc4282_driver); 

MODULE_AUTHOR("Tianhui Xu <tianhui@celestica.com>"); 
MODULE_DESCRIPTION("LTC4282 driver"); 
MODULE_LICENSE("GPL"); 
