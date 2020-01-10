/*
 * max6615.c - Support for Maxim MAX6615
 *
 * 2-Channel Temperature Monitor with Dual PWM Fan-Speed Controller
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>

/* Addresses to scan */
static const unsigned short normal_i2c[] = { 0x18 };

/* The MAX6615 registers, valid channel numbers: 0, 1 */
#define MAX6615_REG_TEMP_CONFIG             0x02
#define MAX6615_REG_TEMP_STATUS             0x05
#define MAX6615_REG_TEMP_OUTPUT_MASK        0x06
#define MAX6615_REG_TEMP(ch)                (0x00 + (ch))
#define MAX6615_REG_TEMP_OT_LIMIT(ch)       (0x03 + (ch))
#define MAX6615_REG_TEMP_EXT(ch)            (0x1E + (ch))

#define MAX6615_REG_FAN_CONFIG              0x11
#define MAX6615_REG_PWM_RATE                0x12
#define MAX6615_REG_PWM_STEP                0x13
#define MAX6615_REG_PWM_FREQ                0x14
#define MAX6615_REG_FAN_STATUS              0x1C
#define MAX6615_REG_PWM_START(ch)           (0x07 + (ch))
#define MAX6615_REG_PWM_MAX(ch)             (0x09 + (ch))
#define MAX6615_REG_TARGT_PWM(ch)           (0x0B + (ch))
#define MAX6615_REG_INSTANTANEOUS_PWM(ch)   (0x0D + (ch))
#define MAX6615_REG_FAN_START_TEMP(ch)      (0x0F + (ch))
#define MAX6615_REG_FAN_CNT(ch)             (0x18 + (ch))
#define MAX6615_REG_FAN_LIMIT(ch)           (0x1A + (ch))

#define MAX6615_REG_DEVID                    0xFE
#define MAX6615_REG_MANUID                   0xFF
#define MAX6615_REG_DEVREV                   0xFD

/* Register bits */
#define MAX6615_GCONFIG_STANDBY              0x80
#define MAX6615_GCONFIG_POR                  0x18

#define MAX6615_PWM_FREQ_35K                 0xE0
#define MAX6615_PWM_MAX                      0xF0

#define MAX6615_FAN_FAIL_OPT                 0x01

static const int rpm_ranges = 57600;
#define FAN_FULL_SPEED                      24000
#define FAN_FROM_REG(val)     ((val) == 0 || (val) == 255 ? 0 : (rpm_ranges) / (val))
#define TEMP_TO_REG(val)      clamp_val((val) / 1000, 0, 255)

#define GET_CHANNEL(attr)      (attr & 0x1) 

enum max6615_ch_t {
    CH1,
    CH2,
    MAX_CH
};

enum max6615_attr_t {
    TEMP1_INPUT = 0x0,
    TEMP2_INPUT,
    TEMP1_EMERGENCY,
    TEMP2_EMERGENCY,
    TEMP1_ALARM,
    TEMP2_ALARM,

    PWM1 = 0x10,
    PWM2,
    PWM1_INSTANTANEOUS,
    PWM2_INSTANTANEOUS,
    PWM1_START,
    PWM2_START,
    PWM1_MAX,
    PWM2_MAX,
    PWM1_STEP,
    PWM2_STEP,
    PWM1_RATE,
    PWM2_RATE,
    PWM_FREQ,

    FAN1_INPUT = 0x20,
    FAN2_INPUT,
    FAN1_START_TEMP,
    FAN2_START_TEMP,
    FAN1_LIMIT,
    FAN2_LIMIT,
    FAN1_FAULT,
    FAN2_FAULT,
};

/*
 * Client data (each client gets its own)
 */
struct max6615_data {
    struct i2c_client *client;
    struct mutex update_lock;
    char valid;        /* !=0 if following fields are valid */
    unsigned long last_updated;    /* In jiffies */

    /* Register values sampled regularly */
    u16 temp[2];        /* Temperature, in 1/8 C, 0..255 C */
    u8 temp_ot[2];        /* OT Temperature, 0..255 C (->_emergency) */
    u8 temp_status;        /* Detected channel alarms and fan failures */

    /* Register values only written to */
    u8 pwm[2];        /* Register value: Duty cycle 0..120 */
    u8 pwm_start[2];
    u8 pwm_max[2];
    u8 pwm_instantaneous[2];
    u8 pwm_step;
    u8 pwm_rate;
    u8 pwm_freq;

    u8 fan[2];        /* Register value: TACH count for fans >=30 */
    u8 fan_start_temp[2];
    u8 fan_limit[2];
    u8 fan_status;        /* Detected channel alarms and fan failures */
};

#define TEMP_XLAT_TABLE_SIZE 256
/*
 * Ait inlet temp and register mapping table, len TEMP_XLAT_TABLE_SIZE, unit: mC
 * We sample many set of register and real temp, then we do linear calibration to
 * generate a formula which is 0.000000208*reg_msb^5 - 0.00005133*reg_msb^4
 * + 0.005049*reg_msb^3 - 0.25423*reg_msb^2 + 7.949*reg_msb - 78.186,
 * then we use formula to generate this table.
 */
static int temp1_table[TEMP_XLAT_TABLE_SIZE] = {-11700, -11700, -11700,
      -11700, -11700, -11700, -11700, -11700, -11700, -11700, -11700,
      -11700, -11700, -8109, -4730, -1550, 1450, 4290, 6980, 9520, 11950,
      14250, 16450, 18560, 20580, 22520, 24380, 26190, 27930, 29630, 31280,
      32880, 34450, 35990, 37490, 38970, 40420, 41860, 43270, 44660, 46040,
      47400, 48740, 50070, 51390, 52690, 53980, 55250, 56520, 57770, 59000,
      60220, 61430, 62630, 63810, 64970, 66130, 67270, 68390, 69510, 70610,
      71710, 72800, 73880, 74950, 76030, 77100, 78180, 79270, 80360, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480, 81480,
      81480, 81480, 81480, 81480, 81480};
/*
 * Ait outlet temp and register mapping table, len TEMP_XLAT_TABLE_SIZE, unit: mC
 * We sample many set of register and real temp, then we do linear calibration to
 * generate a formula which is 0.0000000007*reg_msb^5 - 0.00000665*reg_msb^4
 * + 0.001422*reg_msb^3 - 0.1174*reg_msb^2 + 5.5609*reg_msb - 62.601,
 * then we use formula to generate this table.
 */
static int temp2_table[TEMP_XLAT_TABLE_SIZE] = {-10460, -10460, -10460,
      -10460, -10460, -10460, -10460, -10460, -10460, -10460, -10460,
      -10460, -10460, -7220, -4110, -1140, 1710, 4440, 7050, 9560, 11970,
      14280, 16500, 18640, 20700, 22670, 24580, 26420, 28200, 29920, 31590,
      33210, 34780, 36300, 37790, 39240, 40660, 42050, 43400, 44740, 46050,
      47340, 48620, 49870, 51110, 52340, 53560, 54770, 55970, 57160, 58350,
      59530, 60700, 61870, 63040, 64200, 65360, 66510, 67650, 68790, 69930,
      71050, 72170, 73280, 74380, 75460, 76540, 77600, 78640, 79660, 80660,
      81630, 82580, 83510, 84390, 85250, 86070, 86840, 87570, 88250, 88880,
      89460, 89970, 90430, 90810, 91120, 91350, 91500, 91500, 91500, 91500,
      91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500,
      91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500,
      91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500,
      91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500,
      91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500,
      91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500,
      91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500,
      91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500,
      91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500,
      91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500,
      91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500,
      91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500,
      91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500,
      91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500,
      91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500,
      91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500, 91500,
      91500, 91500, 91500, 91500, 91500};

static struct max6615_data *max6615_update_device(struct device *dev)
{
    struct max6615_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    struct max6615_data *ret = data;
    int i;
    int status_reg;

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + 2 * HZ) || !data->valid) {
        int res;

        dev_dbg(&client->dev, "Starting max6615 update\n");

        status_reg = i2c_smbus_read_byte_data(client, MAX6615_REG_TEMP_STATUS);
        if (status_reg < 0) {
            ret = ERR_PTR(status_reg);
            goto abort;
        }
        data->temp_status = status_reg;

        status_reg = i2c_smbus_read_byte_data(client, MAX6615_REG_FAN_STATUS);
        if (status_reg < 0) {
            ret = ERR_PTR(status_reg);
            goto abort;
        }
        data->fan_status = status_reg;

        status_reg = i2c_smbus_read_byte_data(client, MAX6615_REG_PWM_STEP);
        if (status_reg < 0) {
            ret = ERR_PTR(status_reg);
            goto abort;
        }
        data->pwm_step = status_reg;

        status_reg = i2c_smbus_read_byte_data(client, MAX6615_REG_PWM_RATE);
        if (status_reg < 0) {
            ret = ERR_PTR(status_reg);
            goto abort;
        }
        data->pwm_rate = status_reg;

        status_reg = i2c_smbus_read_byte_data(client, MAX6615_REG_PWM_FREQ);
        if (status_reg < 0) {
            ret = ERR_PTR(status_reg);
            goto abort;
        }
        data->pwm_freq = status_reg;

        for (i = 0; i < MAX_CH; i++) {
            res = i2c_smbus_read_byte_data(client, MAX6615_REG_TEMP_EXT(i));
            if (res < 0) {
                ret = ERR_PTR(res);
                goto abort;
            }
            data->temp[i] = res >> 5;

            res = i2c_smbus_read_byte_data(client, MAX6615_REG_TEMP(i));
            if (res < 0) {
                ret = ERR_PTR(res);
                goto abort;
            }
            data->temp[i] |= res << 3;

            res = i2c_smbus_read_byte_data(client, MAX6615_REG_TARGT_PWM(i));
            if (res < 0) {
                ret = ERR_PTR(res);
                goto abort;
            }
            data->pwm[i] = res;

            res = i2c_smbus_read_byte_data(client, MAX6615_REG_INSTANTANEOUS_PWM(i));
            if (res < 0) {
                ret = ERR_PTR(res);
                goto abort;
            }
            data->pwm_instantaneous[i] = res;

            res = i2c_smbus_read_byte_data(client, MAX6615_REG_PWM_START(i));
            if (res < 0) {
                ret = ERR_PTR(res);
                goto abort;
            }
            data->pwm_start[i] = res;

            res = i2c_smbus_read_byte_data(client, MAX6615_REG_PWM_MAX(i));
            if (res < 0) {
                ret = ERR_PTR(res);
                goto abort;
            }
            data->pwm_max[i] = res;

            res = i2c_smbus_read_byte_data(client, MAX6615_REG_FAN_CNT(i));
            if (res < 0) {
                ret = ERR_PTR(res);
                goto abort;
            }
            data->fan[i] = res;

            res = i2c_smbus_read_byte_data(client, MAX6615_REG_FAN_START_TEMP(i));
            if (res < 0) {
                ret = ERR_PTR(res);
                goto abort;
            }
            data->fan_start_temp[i] = res;

            res = i2c_smbus_read_byte_data(client, MAX6615_REG_FAN_LIMIT(i));
            if (res < 0) {
                ret = ERR_PTR(res);
                goto abort;
            }
            data->fan_limit[i] = res;
        }

        data->last_updated = jiffies;
        data->valid = 1;
    }
abort:
    mutex_unlock(&data->update_lock);

    return ret;
}

static ssize_t show_temp(struct device *dev,
                         struct device_attribute *dev_attr, char *buf)
{
    u8 reg;
    long temp;
    struct max6615_data *data = max6615_update_device(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);

    if (IS_ERR(data))
        return PTR_ERR(data);

    switch(attr->index) {
        case TEMP1_INPUT:
            /*
             * From datesheet reg addr 0x1E & 0x1F, 
             * the temps LSBs(decimal part) are three bits.
             * But lookup table don't use them, need to right shift 3 bits.
             */
            reg = data->temp[GET_CHANNEL(attr->index)] >> 3;
            temp = temp1_table[reg];
            break;
        case TEMP2_INPUT:
            /*
             * From datesheet reg addr 0x1E & 0x1F, 
             * the temps LSBs(decimal part) are three bits.
             * But lookup table don't use them, need to right shift 3 bits.
             */
            reg = data->temp[GET_CHANNEL(attr->index)] >> 3;
            temp = temp2_table[reg];
            break;
        case TEMP1_EMERGENCY:
        case TEMP2_EMERGENCY:
            temp = data->temp_ot[GET_CHANNEL(attr->index)] * 1000;
            break;
        case FAN1_START_TEMP:
        case FAN2_START_TEMP:
            temp = data->fan_start_temp[GET_CHANNEL(attr->index)] * 1000;
        default:
            return PTR_ERR(data);
    }

    return sprintf(buf, "%ld\n", temp);
}

static ssize_t set_temp(struct device *dev,
                        struct device_attribute *dev_attr,
                        const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct max6615_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    long val;
    unsigned int reg_addr;
    int res;

    res = kstrtoul(buf, 10, &val);
    if (res)
        return res;

    switch(attr->index) {
        case TEMP1_EMERGENCY:
        case TEMP2_EMERGENCY:
            reg_addr = MAX6615_REG_TEMP_OT_LIMIT(GET_CHANNEL(attr->index));
            break;
        case FAN1_START_TEMP:
        case FAN2_START_TEMP:
            reg_addr = MAX6615_REG_FAN_START_TEMP(GET_CHANNEL(attr->index));
            break;
        default:
            return PTR_ERR(data);
    }

    mutex_lock(&data->update_lock);
    i2c_smbus_write_byte_data(client, reg_addr, TEMP_TO_REG(val));
    mutex_unlock(&data->update_lock);
    return count;
}

static ssize_t show_temp_alarm(struct device *dev,
                               struct device_attribute *dev_attr, char *buf)
{
    struct max6615_data *data = max6615_update_device(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);

    if (IS_ERR(data))
        return PTR_ERR(data);

    return sprintf(buf, "%d\n", !!(data->temp_status & (1 << attr->index)));
}

static ssize_t show_pwm(struct device *dev,
                        struct device_attribute *dev_attr, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct max6615_data *data = max6615_update_device(dev);
    int pwm;

    if (IS_ERR(data))
        return PTR_ERR(data);

    switch(attr->index) {
        case PWM1:
        case PWM2:
            pwm = data->pwm[GET_CHANNEL(attr->index)];
            break;
        case PWM1_INSTANTANEOUS:
        case PWM2_INSTANTANEOUS:
            pwm = data->pwm[GET_CHANNEL(attr->index)];
            break;
        case PWM1_START:
        case PWM2_START:
            pwm = data->pwm_start[GET_CHANNEL(attr->index)];
            break;
        case PWM1_MAX:
        case PWM2_MAX:
            pwm = data->pwm_max[GET_CHANNEL(attr->index)];
            break;
        case PWM1_STEP:
            pwm = (data->pwm_step & 0xF0) >> 4;
        case PWM2_STEP:
            pwm = data->pwm_step & 0x0F;
            break;
        case PWM_FREQ:
            if(data->pwm_freq & 0x20) {
                return sprintf(buf, "%d\n", 35000);
            } else {
                int freq[] = { 20, 33, 50, 100 };
                return sprintf(buf, "%d\n", freq[(data->pwm_freq & 0xE0) >> 5]);
            }
        default:
            return PTR_ERR(data);
    }

    return sprintf(buf, "%d\n", pwm);
}

static ssize_t set_pwm(struct device *dev,
                       struct device_attribute *dev_attr,
                       const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct max6615_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    unsigned long val;
    unsigned int reg_addr;
    int res;

    res = kstrtoul(buf, 10, &val);
    if (res)
        return res;

    val = clamp_val(val, 0, MAX6615_PWM_MAX);

    if (IS_ERR(data))
        return PTR_ERR(data);

    switch(attr->index) {
        case PWM1:
        case PWM2:
            reg_addr = MAX6615_REG_TARGT_PWM(GET_CHANNEL(attr->index));
            break;
        case PWM1_INSTANTANEOUS:
        case PWM2_INSTANTANEOUS:
            reg_addr = MAX6615_REG_INSTANTANEOUS_PWM(GET_CHANNEL(attr->index));
            break;
        case PWM1_START:
        case PWM2_START:
            reg_addr = MAX6615_REG_PWM_START(GET_CHANNEL(attr->index));
            break;
        case PWM1_MAX:
        case PWM2_MAX:
            reg_addr = MAX6615_REG_PWM_MAX(GET_CHANNEL(attr->index));
            break;
        default:
            return PTR_ERR(data);
    }

    mutex_lock(&data->update_lock);
    i2c_smbus_write_byte_data(client, reg_addr, val);
    mutex_unlock(&data->update_lock);
    return count;
}

static ssize_t show_fan(struct device *dev,
                        struct device_attribute *dev_attr, char *buf)
{
    struct max6615_data *data = max6615_update_device(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    int fan;

    if (IS_ERR(data))
        return PTR_ERR(data);

    switch(attr->index) {
        case FAN1_INPUT:
        case FAN2_INPUT:
            fan = data->fan[GET_CHANNEL(attr->index)];
            break;
        case FAN1_LIMIT:
        case FAN2_LIMIT:
            fan = data->fan_limit[GET_CHANNEL(attr->index)];
            break;
        default:
            return PTR_ERR(data);
    }

    return sprintf(buf, "%d\n", FAN_FROM_REG(fan) > FAN_FULL_SPEED ? 
                   FAN_FULL_SPEED : FAN_FROM_REG(fan));
}

static ssize_t set_fan(struct device *dev,
                       struct device_attribute *dev_attr,
                       const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct max6615_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    unsigned long val;
    unsigned int reg_addr;
    int res;

    res = kstrtoul(buf, 10, &val);
    if (res)
        return res;

    val = clamp_val(val, 0, 0xFF);

    switch(attr->index) {
        case FAN1_LIMIT:
        case FAN2_LIMIT:
            reg_addr = MAX6615_REG_FAN_LIMIT(GET_CHANNEL(attr->index));
            break;
        default:
            return PTR_ERR(data);
    }

    mutex_lock(&data->update_lock);
    i2c_smbus_write_byte_data(client, reg_addr, val);
    mutex_unlock(&data->update_lock);
    return count;
}

static ssize_t show_fan_alarm(struct device *dev,
                              struct device_attribute *dev_attr, char *buf)
{
    struct max6615_data *data = max6615_update_device(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);

    if (IS_ERR(data))
        return PTR_ERR(data);

    return sprintf(buf, "%d\n", !!(data->fan_status & (1 << attr->index)));
}

static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_temp, NULL, TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(temp2_input, S_IRUGO, show_temp, NULL, TEMP2_INPUT);
static SENSOR_DEVICE_ATTR(temp1_emergency, S_IWUSR | S_IRUGO,
                          show_temp, set_temp, TEMP1_EMERGENCY);
static SENSOR_DEVICE_ATTR(temp2_emergency, S_IWUSR | S_IRUGO,
                          show_temp, set_temp, TEMP2_EMERGENCY);
static SENSOR_DEVICE_ATTR(temp1_emergency_alarm, S_IRUGO, show_temp_alarm, NULL, TEMP1_ALARM);
static SENSOR_DEVICE_ATTR(temp2_emergency_alarm, S_IRUGO, show_temp_alarm, NULL, TEMP2_ALARM);

static SENSOR_DEVICE_ATTR(pwm1, S_IWUSR | S_IRUGO, show_pwm, set_pwm, PWM1);
static SENSOR_DEVICE_ATTR(pwm2, S_IWUSR | S_IRUGO, show_pwm, set_pwm, PWM2);
static SENSOR_DEVICE_ATTR(pwm1_input, S_IRUGO, show_pwm, NULL, PWM1_INSTANTANEOUS);
static SENSOR_DEVICE_ATTR(pwm2_input, S_IRUGO, show_pwm, NULL, PWM2_INSTANTANEOUS);
static SENSOR_DEVICE_ATTR(pwm1_start, S_IWUSR | S_IRUGO, show_pwm, set_pwm, PWM1_START);
static SENSOR_DEVICE_ATTR(pwm2_start, S_IWUSR | S_IRUGO, show_pwm, set_pwm, PWM2_START);
static SENSOR_DEVICE_ATTR(pwm1_max, S_IWUSR | S_IRUGO, show_pwm, set_pwm, PWM1_MAX);
static SENSOR_DEVICE_ATTR(pwm2_max, S_IWUSR | S_IRUGO, show_pwm, set_pwm, PWM2_MAX);
static SENSOR_DEVICE_ATTR(pwm1_step, S_IRUGO, show_pwm, NULL, PWM1_STEP);
static SENSOR_DEVICE_ATTR(pwm2_step, S_IRUGO, show_pwm, NULL, PWM2_STEP);
static SENSOR_DEVICE_ATTR(pwm_freq, S_IRUGO, show_pwm, NULL, PWM_FREQ);

static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, show_fan, NULL, FAN1_INPUT);
static SENSOR_DEVICE_ATTR(fan2_input, S_IRUGO, show_fan, NULL, FAN2_INPUT);
static SENSOR_DEVICE_ATTR(fan1_start_temp, S_IWUSR | S_IRUGO, show_temp, set_temp, FAN1_START_TEMP);
static SENSOR_DEVICE_ATTR(fan2_start_temp, S_IWUSR | S_IRUGO, show_temp, set_temp, FAN2_START_TEMP);
static SENSOR_DEVICE_ATTR(fan1_limit, S_IRUGO, show_fan, set_fan, FAN1_LIMIT);
static SENSOR_DEVICE_ATTR(fan2_limit, S_IRUGO, show_fan, set_fan, FAN2_LIMIT);
static SENSOR_DEVICE_ATTR(fan1_fault, S_IRUGO, show_fan_alarm, NULL, FAN1_FAULT);
static SENSOR_DEVICE_ATTR(fan2_fault, S_IRUGO, show_fan_alarm, NULL, FAN2_FAULT);

static struct attribute *max6615_attrs[] = {
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    &sensor_dev_attr_temp2_input.dev_attr.attr,
    &sensor_dev_attr_temp1_emergency.dev_attr.attr,
    &sensor_dev_attr_temp2_emergency.dev_attr.attr,
    &sensor_dev_attr_temp1_emergency_alarm.dev_attr.attr,
    &sensor_dev_attr_temp2_emergency_alarm.dev_attr.attr,

    &sensor_dev_attr_pwm1.dev_attr.attr,
    &sensor_dev_attr_pwm2.dev_attr.attr,
    &sensor_dev_attr_pwm1_input.dev_attr.attr,
    &sensor_dev_attr_pwm2_input.dev_attr.attr,
    &sensor_dev_attr_pwm1_start.dev_attr.attr,
    &sensor_dev_attr_pwm2_start.dev_attr.attr,
    &sensor_dev_attr_pwm1_max.dev_attr.attr,
    &sensor_dev_attr_pwm2_max.dev_attr.attr,
    &sensor_dev_attr_pwm1_step.dev_attr.attr,
    &sensor_dev_attr_pwm2_step.dev_attr.attr,
    &sensor_dev_attr_pwm_freq.dev_attr.attr,

    &sensor_dev_attr_fan1_input.dev_attr.attr,
    &sensor_dev_attr_fan2_input.dev_attr.attr,
    &sensor_dev_attr_fan1_start_temp.dev_attr.attr,
    &sensor_dev_attr_fan2_start_temp.dev_attr.attr,
    &sensor_dev_attr_fan1_limit.dev_attr.attr,
    &sensor_dev_attr_fan2_limit.dev_attr.attr,
    &sensor_dev_attr_fan1_fault.dev_attr.attr,
    &sensor_dev_attr_fan2_fault.dev_attr.attr,
    NULL
};
ATTRIBUTE_GROUPS(max6615);

static int max6615_init_client(struct i2c_client *client,
                               struct max6615_data *data)
{
    int i;
    int err;

    /* Fans config PWM */
    err = i2c_smbus_write_byte_data(client, MAX6615_REG_FAN_STATUS, MAX6615_FAN_FAIL_OPT);
    if (err)
        goto exit;

    err = i2c_smbus_write_byte_data(client, MAX6615_REG_PWM_RATE, 0);
    if (err)
        goto exit;
    err = i2c_smbus_write_byte_data(client, MAX6615_REG_PWM_FREQ, MAX6615_PWM_FREQ_35K);
    if (err)
        goto exit;

    for (i = 0; i < 2; i++) {
        /* Max. temp. 100C */
        data->temp_ot[i] = 100;
        err = i2c_smbus_write_byte_data(client,
                MAX6615_REG_TEMP_OT_LIMIT(i), data->temp_ot[i]);
        if (err)
            goto exit;

        /* PWM 72/240 (i.e. 30%) */
        data->pwm[i] = 72;
        err = i2c_smbus_write_byte_data(client,
                MAX6615_REG_TARGT_PWM(i), data->pwm[i]);
        if (err)
            goto exit;
    }
    /* Start monitoring */
    err = i2c_smbus_write_byte_data(client, MAX6615_REG_TEMP_CONFIG,
                  MAX6615_GCONFIG_POR);
    if (err)
        goto exit;
exit:
    return err;
}

/* Return 0 if detection is successful, -ENODEV otherwise */
static int max6615_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = client->adapter;
    int dev_id, manu_id;

    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
        return -ENODEV;

    /* Actual detection via device and manufacturer ID */
    dev_id = i2c_smbus_read_byte_data(client, MAX6615_REG_DEVID);
    manu_id = i2c_smbus_read_byte_data(client, MAX6615_REG_MANUID);
    if (dev_id != 0x68 || manu_id != 0x4D) {
        printk(KERN_INFO "max6615 not detect");
        return -ENODEV;
    }

    strlcpy(info->type, "max6615", I2C_NAME_SIZE);

    return 0;
}

static int max6615_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
    struct device *dev = &client->dev;
    struct max6615_data *data;
    struct device *hwmon_dev;
    int err;

    data = devm_kzalloc(dev, sizeof(struct max6615_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->client = client;
    mutex_init(&data->update_lock);

    /* Initialize the max6615 chip */
    err = max6615_init_client(client, data);
    if (err < 0)
        return err;

    hwmon_dev = devm_hwmon_device_register_with_groups(dev, client->name,
                               data,
                               max6615_groups);
    return PTR_ERR_OR_ZERO(hwmon_dev);
}

#ifdef CONFIG_PM_SLEEP
static int max6615_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    int data = i2c_smbus_read_byte_data(client, MAX6615_REG_TEMP_CONFIG);
    if (data < 0)
        return data;

    return i2c_smbus_write_byte_data(client,
           MAX6615_REG_TEMP_CONFIG, data | MAX6615_GCONFIG_STANDBY);
}

static int max6615_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    int data = i2c_smbus_read_byte_data(client, MAX6615_REG_TEMP_CONFIG);
    if (data < 0)
        return data;

    return i2c_smbus_write_byte_data(client,
           MAX6615_REG_TEMP_CONFIG, data & ~MAX6615_GCONFIG_STANDBY);
}
#endif /* CONFIG_PM_SLEEP */

static const struct i2c_device_id max6615_id[] = {
    {"max6615", 0},
    { }
};

MODULE_DEVICE_TABLE(i2c, max6615_id);

static SIMPLE_DEV_PM_OPS(max6615_pm_ops, max6615_suspend, max6615_resume);

static struct i2c_driver max6615_driver = {
    .class = I2C_CLASS_HWMON,
    .driver = {
        .name = "max6615",
        .pm = &max6615_pm_ops,
    },
    .probe = max6615_probe,
    .id_table = max6615_id,
    .detect = max6615_detect,
    .address_list = normal_i2c,
};

module_i2c_driver(max6615_driver);

MODULE_AUTHOR("Tianhui Xu <tianhui@celestica.com>");
MODULE_DESCRIPTION("max6615 driver");
MODULE_LICENSE("GPL");
