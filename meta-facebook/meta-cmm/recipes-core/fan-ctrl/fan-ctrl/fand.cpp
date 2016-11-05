/*
 * fand
 *
 * Copyright 2016-present Celestica. All Rights Reserved.
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
 *
 * Daemon to manage the fan speed to ensure that we stay within a reasonable
 * temperature range.  We're using a simplistic algorithm to get started:
 *
 * If the fan is already on high, we'll move it to medium if we fall below
 * a top temperature.  If we're on medium, we'll move it to high
 * if the temperature goes over the top value, and to low if the
 * temperature falls to a bottom level.  If the fan is on low,
 * we'll increase the speed if the temperature rises to the top level.
 *
 * To ensure that we're not just turning the fans up, then back down again,
 * we'll require an extra few degrees of temperature drop before we lower
 * the fan speed.
 *
 * We check the RPM of the fans against the requested RPMs to determine
 * whether the fans are failing, in which case we'll turn up all of
 * the other fans and report the problem..
 *
 * TODO:  Implement a PID algorithm to closely track the ideal temperature.
 * TODO:  Determine if the daemon is already started.
 */

/* Yeah, the file ends in .cpp, but it's a C program.  Deal. */

#define uchar unsigned char
#define swab16(x)                                               \
  ((unsigned short)(                                            \
      (((unsigned short)(x) & (unsigned short)0x00ffU) << 8) |  \
      (((unsigned short)(x) & (unsigned short)0xff00U) >> 8) ))

#if !defined(CONFIG_GALAXY100)
#error "No hardware platform defined!"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include "watchdog.h"
#ifdef CONFIG_GALAXY100
#include <fcntl.h>
#include "i2c-dev.h"
#endif

/* Sensor definitions */
#define INTERNAL_TEMPS(x) (x)
#define EXTERNAL_TEMPS(x) (x)
#define PWM_UNIT_MAX 32

/*
 * The sensor for the uServer CPU is not on the CPU itself, so it reads
 * a little low.  We are special casing this, but we should obviously
 * be thinking about a way to generalize these tweaks, and perhaps
 * the entire configuration.  JSON file?
 */
#define USERVER_TEMP_FUDGE INTERNAL_TEMPS(0)
#define BAD_TEMP INTERNAL_TEMPS(-60)

#define FAN_FAILURE_THRESHOLD 2 /* How many times can a fan fail */
#define FAN_SHUTDOWN_THRESHOLD 60
#define FAN_LED_BLUE "1"
#define FAN_LED_RED "2"

#define REPORT_TEMP 720  /* Report temp every so many cycles */

/* Sensor limits and tuning parameters */
#define TEMP_TOP INTERNAL_TEMPS(70)
#define TEMP_BOTTOM INTERNAL_TEMPS(40)

/*
 * Toggling the fan constantly will wear it out (and annoy anyone who
 * can hear it), so we'll only turn down the fan after the temperature
 * has dipped a bit below the point at which we'd otherwise switch
 * things up.
 */

#define GALAXY100_FAN_LOW 32
#define GALAXY100_FAN_MEDIUM 51
#define GALAXY100_FAN_HIGH 72
#define GALAXY100_FAN_MAX 99
//#define GALAXY100_FAN_ONEFAILED_RAISE_PEC 20
#define GALAXY100_RAISING_TEMP_LOW 31
#define GALAXY100_RAISING_TEMP_HIGH 36
#define GALAXY100_FALLING_TEMP_LOW 27
#define GALAXY100_FALLING_TEMP_HIGH 33
#define GALAXY100_SYSTEM_LIMIT 60

#define GALAXY100_TEMP_I2C_BUS 1
#define GALAXY100_FANTRAY_I2C_BUS 8
#define GALAXY100_FANTRAY_CPLD_ADDR 0x33

#define GALAXY100_CMM_SYS_I2C_BUS 13
#define GALAXY100_CMM_SYS_CPLD_ADDR 0x3e
#define GALAXY100_CMM_SLOTID_REG 0x03
#define GALAXY100_MODULE_PRESENT_REG 0x08
#define GALAXY100_SCM_PRESENT_REG 0x09
#define GALAXY100_CMM_STATUS_REG 0x60
#define GALAXY100_CMM_CHANNEL1_ADDR_BAKUP 0x77


#define GALAXY100_PSUS 4
#define GALAXY100_PSU_CHANNEL_I2C_BUS 2
#define GALAXY100_PSU_CHANNEL_I2C_ADDR 0x71
#define GALAXY100_PSU_I2C_ADDR 0x10

#define FANS 4
#define GALAXY100_LC_NUM 4
#define GALAXY100_SCM_NUM 8

#define FAN_FAILURE_OFFSET 30


struct channel_info_stu {
  uchar addr;
  uchar channel;
};
struct sensor_info {
  struct channel_info_stu channel1;
  struct channel_info_stu channel2;
  struct channel_info_stu channel3;
  uchar bus;
  uchar addr;
  uchar temp;
  int (*read_temp)(struct channel_info_stu, struct channel_info_stu, struct channel_info_stu, int, int);
};
struct fan_info_stu {
  uchar front_fan_reg;
  uchar rear_fan_reg;
  uchar pwm_reg;
  uchar fan_led_reg;
  uchar fan_status;
  uchar present;
  uchar failed;
};

struct galaxy100_board_info_stu {
  const char *name;
  uint slot_id;
  struct sensor_info *critical;
  struct sensor_info *alarm;
};
struct galaxy100_fantray_info_stu {
  const char *name;
  int present;
  struct sensor_info channel_info;
  struct fan_info_stu fan1;
  struct fan_info_stu fan2;
  struct fan_info_stu fan3;
};

struct galaxy100_fan_failed_control_stu {
  int failed_num;
  int low_level;
  int mid_level;
  int high_level;
  int alarm_level;
};

static int galaxy100_i2c_read(int bus, int addr, uchar reg, int byte);
static int galaxy100_i2c_write(int bus, int addr, uchar reg, int value, int byte);
static int open_i2c_dev(int i2cbus, char *filename, size_t size);
static int read_temp(struct channel_info_stu channel1, struct channel_info_stu channel2, struct channel_info_stu channel3, int bus, int addr);
static int read_critical_max_temp(void);
static int read_alarm_max_temp(void);
static int calculate_falling_fan_pwm(int temp);
static int calculate_raising_fan_pwm(int temp);
static int galaxy100_set_fan(int fan, int value);
static int galaxy100_fan_is_okey(int fan);
static int galaxy100_write_fan_led(int fan, const char *color);
static int system_shutdown(const char *why);
static int galaxy100_fab_is_present(int fan);
static int galaxy100_lc_present_detect(void);
static int galaxy100_scm_present_detect(void);
static int galaxy100_cmm_is_master(void);
static int galaxy100_chanel_reinit(void);


/*CMM info*/
struct sensor_info cmm_sensor_critical_info = {
  .channel1 = {
    .addr = 0x75,
    .channel = 0xff,
  },
  .channel2 = {
    .addr = 0x73,
    .channel = 0xff,
  },
  .channel3 = {
    .addr = 0x70,
    .channel = 0xff,
  },
  .bus = 3,
  .addr = 0x48,
  .temp = 0,
  .read_temp = &read_temp,
};
struct sensor_info cmm_sensor_alarm_info = {
  .channel1 = {
    .addr = 0x75,
    .channel = 0xff,
  },
  .channel2 = {
    .addr = 0x73,
    .channel = 0xff,
  },
  .channel3 = {
    .addr = 0x70,
    .channel = 0xff,
  },
  .bus = 3,
  .addr = 0x49,
  .temp = 0,
  .read_temp = &read_temp,
};

/*SCM info*/
struct sensor_info scm_fab_1_sensor_critical_info = {
  .channel1 = {
    .addr = 0x75,
    .channel = 0x01,
  },
  .channel2 = {
    .addr = 0x70,
    .channel = 0x0,
  },
  .channel3 = {
    .addr = 0x73,
    .channel = 0x02,
  },
  .bus = 1,
  .addr = 0x48,
  .temp = 0,
  .read_temp = &read_temp,
};

struct sensor_info scm_fab_2_sensor_critical_info = {
  .channel1 = {
    .addr = 0x75,
    .channel = 0x02,
  },
  .channel2 = {
    .addr = 0x70,
    .channel = 0x0,
  },
  .channel3 = {
    .addr = 0x73,
    .channel = 0x02,
  },
  .bus = 1,
  .addr = 0x48,
  .temp = 0,
  .read_temp = &read_temp,
};

struct sensor_info scm_fab_3_sensor_critical_info = {
  .channel1 = {
    .addr = 0x75,
    .channel = 0x40,
  },
  .channel2 = {
    .addr = 0x70,
    .channel = 0x0,
  },
  .channel3 = {
    .addr = 0x73,
    .channel = 0x02,
  },
  .bus = 1,
  .addr = 0x48,
  .temp = 0,
  .read_temp = &read_temp,
};

struct sensor_info scm_fab_4_sensor_critical_info = {
  .channel1 = {
    .addr = 0x75,
    .channel = 0x80,
  },
  .channel2 = {
    .addr = 0x70,
    .channel = 0x0,
  },
  .channel3 = {
    .addr = 0x73,
    .channel = 0x02,
  },
  .bus = 1,
  .addr = 0x48,
  .temp = 0,
  .read_temp = &read_temp,
};

struct sensor_info scm_lc_1_sensor_critical_info = {
  .channel1 = {
    .addr = 0x75,
    .channel = 0x04,
  },
  .channel2 = {
    .addr = 0x70,
    .channel = 0x0,
  },
  .channel3 = {
    .addr = 0x73,
    .channel = 0x02,
  },
  .bus = 1,
  .addr = 0x48,
  .temp = 0,
  .read_temp = &read_temp,
};

struct sensor_info scm_lc_2_sensor_critical_info = {
  .channel1 = {
    .addr = 0x75,
    .channel = 0x08,
  },
  .channel2 = {
    .addr = 0x70,
    .channel = 0x0,
  },
  .channel3 = {
    .addr = 0x73,
    .channel = 0x02,
  },
  .bus = 1,
  .addr = 0x48,
  .temp = 0,
  .read_temp = &read_temp,
};

struct sensor_info scm_lc_3_sensor_critical_info = {
  .channel1 = {
    .addr = 0x75,
    .channel = 0x10,
  },
  .channel2 = {
    .addr = 0x70,
    .channel = 0x0,
  },
  .channel3 = {
    .addr = 0x73,
    .channel = 0x02,
  },
  .bus = 1,
  .addr = 0x48,
  .temp = 0,
  .read_temp = &read_temp,
};

struct sensor_info scm_lc_4_sensor_critical_info = {
  .channel1 = {
    .addr = 0x75,
    .channel = 0x20,
  },
  .channel2 = {
    .addr = 0x70,
    .channel = 0x0,
  },
  .channel3 = {
    .addr = 0x73,
    .channel = 0x02,
  },
  .bus = 1,
  .addr = 0x48,
  .temp = 0,
  .read_temp = &read_temp,
};

/* fantran info*/
struct sensor_info fantray1_channel_info = {
  .channel1 = {
    .addr = 0x75,
    .channel = 0x01,
  },
  .channel2 = {
    .addr = 0x70,
    .channel = 0x08,
  },
  .channel3 = {
    .addr = 0xff,//not use
    .channel = 0xff,
  },
  .bus = 0xff,//not use
  .addr = 0xff,//not use
  .temp = 0,
  .read_temp = NULL,
};
struct sensor_info fantray2_channel_info = {
  .channel1 = {
    .addr = 0x75,
    .channel = 0x02,
  },
  .channel2 = {
    .addr = 0x70,
    .channel = 0x08,
  },
  .channel3 = {
    .addr = 0xff,//not use
    .channel = 0xff,
  },
  .bus = 0xff,//not use
  .addr = 0xff,//not use
  .temp = 0,
  .read_temp = NULL,
};
struct sensor_info fantray3_channel_info = {
  .channel1 = {
    .addr = 0x75,
    .channel = 0x04,
  },
  .channel2 = {
    .addr = 0x70,
    .channel = 0x08,
  },
  .channel3 = {
    .addr = 0xff,//not use
    .channel = 0xff,
  },
  .bus = 0xff,//not use
  .addr = 0xff,//not use
  .temp = 0,
  .read_temp = NULL,
};
struct sensor_info fantray4_channel_info = {
  .channel1 = {
    .addr = 0x75,
    .channel = 0x08,
  },
  .channel2 = {
    .addr = 0x70,
    .channel = 0x08,
  },
  .channel3 = {
    .addr = 0xff,//not use
    .channel = 0xff,
  },
  .bus = 0xff,//not use
  .addr = 0xff,//not use
  .temp = 0,
  .read_temp = NULL,
};
struct fan_info_stu fan1_info = {
  .front_fan_reg = 0x20,
  .rear_fan_reg = 0x21,
  .pwm_reg = 0x22,
  .fan_led_reg = 0x24,
  .fan_status = 0x28,
  .present = 1,
  .failed = 0,
};
struct fan_info_stu fan2_info = {
  .front_fan_reg = 0x30,
  .rear_fan_reg = 0x31,
  .pwm_reg = 0x32,
  .fan_led_reg = 0x34,
  .fan_status = 0x38,
  .present = 1,
  .failed = 0,
};
struct fan_info_stu fan3_info = {
  .front_fan_reg = 0x40,
  .rear_fan_reg = 0x41,
  .pwm_reg = 0x42,
  .fan_led_reg = 0x44,
  .fan_status = 0x48,
  .present = 1,
  .failed = 0,
};


/************board and fantray info*****************/
struct galaxy100_board_info_stu galaxy100_board_info[] = {
/*CMM info*/
  {
    .name = "CMM-1",
    .slot_id = 0xff,
    .critical = &cmm_sensor_critical_info,
    .alarm = &cmm_sensor_alarm_info,
  },
#if 0
  {
    .name = "CMM-2",
    .slot_id = 0xff,
    .critical = &cmm_sensor_critical_info,
    .alarm = &cmm_sensor_alarm_info,
  },
#endif
/*SCM info*/
  {
    .name = "SCM-FAB-1",
    .slot_id = 0xff,
    .critical = &scm_fab_1_sensor_critical_info,
    .alarm = NULL,
  },
  {
    .name = "SCM-FAB-2",
    .slot_id = 0xff,
    .critical = &scm_fab_2_sensor_critical_info,
    .alarm = NULL,
  },
  {
    .name = "SCM-FAB-3",
    .slot_id = 0xff,
    .critical = &scm_fab_3_sensor_critical_info,
    .alarm = NULL,
  },
  {
    .name = "SCM-FAB-4",
    .slot_id = 0xff,
    .critical = &scm_fab_4_sensor_critical_info,
    .alarm = NULL,
  },
  {
    .name = "SCM-LC-1",
    .slot_id = 0xff,
    .critical = &scm_lc_1_sensor_critical_info,
    .alarm = NULL,
  },
  {
    .name = "SCM-LC-2",
    .slot_id = 0xff,
    .critical = &scm_lc_2_sensor_critical_info,
    .alarm = NULL,
  },
  {
    .name = "SCM-LC-3",
    .slot_id = 0xff,
    .critical = &scm_lc_3_sensor_critical_info,
    .alarm = NULL,
  },
  {
    .name = "SCM-LC-4",
    .slot_id = 0xff,
    .critical = &scm_lc_4_sensor_critical_info,
    .alarm = NULL,
  },
  NULL,
};

struct galaxy100_fantray_info_stu galaxy100_fantray_info[] = {
  {
    .name = "FCB-1",
    .present = 1,
    .channel_info = fantray1_channel_info,
    .fan1 = fan1_info,
    .fan2 = fan2_info,
    .fan3 = fan3_info,
  },
  {
    .name = "FCB-2",
    .present = 1,
    .channel_info = fantray2_channel_info,
    .fan1 = fan1_info,
    .fan2 = fan2_info,
    .fan3 = fan3_info,
  },
  {
    .name = "FCB-3",
    .present = 1,
    .channel_info = fantray3_channel_info,
    .fan1 = fan1_info,
    .fan2 = fan2_info,
    .fan3 = fan3_info,
  },
  {
    .name = "FCB-4",
    .present = 1,
    .channel_info = fantray4_channel_info,
    .fan1 = fan1_info,
    .fan2 = fan2_info,
    .fan3 = fan3_info,
  },

  NULL,
};

#define BOARD_INFO_SIZE (sizeof(galaxy100_board_info) / sizeof(struct galaxy100_board_info_stu))
#define FANTRAY_INFO_SIZE (sizeof(galaxy100_fantray_info) / sizeof(struct galaxy100_fantray_info_stu))


struct galaxy100_fan_failed_control_stu galaxy100_fan_failed_control[] = {
  {
    .failed_num = 1,
    .low_level = 51,
    .mid_level = 72,
    .high_level = 82,
    .alarm_level = 99,
  },
  {
    .failed_num = 2,
    .low_level = 63,
    .mid_level = 82,
    .high_level = 99,
    .alarm_level = 99,
  },
  {
    .failed_num = 3,
    .low_level = 82,
    .mid_level = 99,
    .high_level = 99,
    .alarm_level = 99,
  },

  NULL,
};



int fan_low = GALAXY100_FAN_LOW;
int fan_medium = GALAXY100_FAN_MEDIUM;
int fan_high = GALAXY100_FAN_HIGH;
int fan_max = GALAXY100_FAN_MAX;
int total_fans = FANS;
int fan_offset = 0;

int temp_bottom = TEMP_BOTTOM;
int temp_top = TEMP_TOP;

int report_temp = REPORT_TEMP;
bool verbose = false;
int lc_status[GALAXY100_LC_NUM] = {0};
int scm_status[GALAXY100_SCM_NUM] = {0};

static int galaxy100_i2c_read(int bus, int addr, uchar reg, int byte)
{
  int res, file;
  char filename[20];

  file = open_i2c_dev(bus, filename, sizeof(filename));
  if(file < 0) {
    return -1;
  }
  if(ioctl(file, I2C_SLAVE_FORCE, addr) < 0) {
    return -1;
  }
  if(byte == 1)
    res = i2c_smbus_read_byte_data(file, reg);
  else if(byte == 2)
    res = i2c_smbus_read_word_data(file, reg);
  else
    return -1;

  close(file);
  return res;
}
static int galaxy100_i2c_write(int bus, int addr, uchar reg, int value, int byte)
{
  int res, file;
  char filename[20];

  file = open_i2c_dev(bus, filename, sizeof(filename));
  if(file < 0) {
    return -1;
  }
  if(ioctl(file, I2C_SLAVE_FORCE, addr) < 0) {
    return -1;
  }
  if(byte == 1)
    res = i2c_smbus_write_byte_data(file, reg, value);
  else if(byte == 2)
    res = i2c_smbus_write_word_data(file, reg, value);
  else
    return -1;

  close(file);
  return res;
}
static int open_i2c_dev(int i2cbus, char *filename, size_t size)
{
  int file;

  snprintf(filename, size, "/dev/i2c/%d", i2cbus);
  filename[size - 1] = '\0';
  file = open(filename, O_RDWR);
  if (file < 0 && (errno == ENOENT || errno == ENOTDIR)) {
    sprintf(filename, "/dev/i2c-%d", i2cbus);
    file = open(filename, O_RDWR);
  }

  return file;
}

static int galaxy100_channel_set(int bus, struct channel_info_stu  channel1, struct channel_info_stu channel2, struct channel_info_stu channel3)
{
  int ret;

  if(channel1.channel!= 0xff) {
    ret = galaxy100_i2c_write(bus, channel1.addr, 0x0, channel1.channel, 1);
    if(ret < 0) {
      syslog(LOG_ERR, "%s: failed to set channel1 0x%x on bus %d, value 0x%x",
             __func__, channel1.addr, bus, channel1.channel);
      return -1;
    }
    usleep(11000);
  }

  if(channel2.channel != 0xff) {
    ret = galaxy100_i2c_write(bus, channel2.addr, 0x0, channel2.channel, 1);
    if(ret < 0) {
      syslog(LOG_ERR, "%s: failed to set channel2 0x%x on bus %d, value 0x%x",
             __func__, channel2.addr, bus, channel2.channel);
      return -1;
    }
    usleep(11000);
  }
  if(channel3.channel != 0xff) {
    ret = galaxy100_i2c_write(bus, channel3.addr, 0x0, channel3.channel, 1);
    if(ret < 0) {
      syslog(LOG_ERR, "%s: failed to set channel3 0x%x on bus %d, value 0x%x",
             __func__, channel3.addr, bus, channel3.channel);
      return -1;
    }
    usleep(11000);
  }

  return 0;
}

static int read_temp(struct channel_info_stu  channel1, struct channel_info_stu channel2, struct channel_info_stu channel3, int bus, int addr)
{
  int ret;
  int value;

  ret = galaxy100_channel_set(GALAXY100_TEMP_I2C_BUS, channel1, channel2, channel3);
  if(ret < 0)
    return -1;
  ret = galaxy100_i2c_read(bus, addr, 0x0, 2);
  value = (ret < 0) ? ret : swab16(ret);
  if(value < 0) {
    syslog(LOG_ERR, "%s: failed to read temperature bus %d, addr 0x%x", __func__, bus, addr);
    return -1;
  }
  usleep(11000);

  return ((short)value / 128) / 2;
}
static int read_critical_max_temp(void)
{
  int i;
  int temp, max_temp = 0;
  struct galaxy100_board_info_stu *info;

  for(i = 0; i < BOARD_INFO_SIZE; i++) {
    info = &galaxy100_board_info[i];
    if(info->critical) {
      temp = read_temp(info->critical->channel1, info->critical->channel2, info->critical->channel3,
                       info->critical->bus, info->critical->addr);
      if(temp != -1) {
        info->critical->temp = temp;
        if(temp > max_temp)
          max_temp = temp;
      }
    }
  }
  //printf("%s: critical: max_temp=%d\n", __func__, max_temp);

  return max_temp;
}
static int read_alarm_max_temp(void)
{
  int i;
  int temp, max_temp = 0;
  struct galaxy100_board_info_stu *info;

  for(i = 0; i < BOARD_INFO_SIZE; i++) {
    info = &galaxy100_board_info[i];
    if(info->alarm) {
      temp = read_temp(info->alarm->channel1, info->alarm->channel2, info->alarm->channel3,
                       info->alarm->bus, info->alarm->addr);
      if(temp != -1) {
        info->alarm->temp = temp;
        if(temp > max_temp)
          max_temp = temp;
      }
    }
  }
  //printf("%s: alarm: max_temp=%d\n", __func__, max_temp);

  return max_temp;
}
static int calculate_raising_fan_pwm(int temp)
{
  if(temp < GALAXY100_RAISING_TEMP_LOW) {
    return GALAXY100_FAN_LOW;
  } else if(temp >= GALAXY100_RAISING_TEMP_LOW && temp < GALAXY100_RAISING_TEMP_HIGH) {
    return GALAXY100_FAN_MEDIUM;
  } else  {
    return GALAXY100_FAN_HIGH;
  }
  return GALAXY100_FAN_HIGH;
}
static int calculate_falling_fan_pwm(int temp)
{
  if(temp < GALAXY100_FALLING_TEMP_LOW) {
    return GALAXY100_FAN_LOW;
  } else if(temp >= GALAXY100_FALLING_TEMP_LOW && temp < GALAXY100_FALLING_TEMP_HIGH) {
    return GALAXY100_FAN_MEDIUM;
  } else  {
    return GALAXY100_FAN_HIGH;
  }

  return GALAXY100_FAN_HIGH;
}
static int galaxy100_set_fan(int fan, int value)
{
  int ret;
  struct galaxy100_fantray_info_stu *info;
  struct sensor_info  *channel;

  info = &galaxy100_fantray_info[fan];
  channel = &info->channel_info;

  ret = galaxy100_fab_is_present(fan);
  if(ret == 0) {
    info->present = 0; //not preset
    return -1;
  } else if(ret == 1) {
    info->present = 1;
  } else {
    return -1;
  }

  ret = galaxy100_channel_set(GALAXY100_FANTRAY_I2C_BUS, channel->channel1, channel->channel2, channel->channel3);
  if(ret < 0)
    return -1;

  ret = galaxy100_i2c_write(GALAXY100_FANTRAY_I2C_BUS, GALAXY100_FANTRAY_CPLD_ADDR, info->fan1.pwm_reg, value, 1);
  if(ret < 0) {
    syslog(LOG_ERR, "%s: failed to set fan1 pwm 0x%x, value 0x%x", __func__, info->fan1.pwm_reg, value);
    return -1;
  }
  usleep(11000);

  ret = galaxy100_i2c_write(GALAXY100_FANTRAY_I2C_BUS, GALAXY100_FANTRAY_CPLD_ADDR, info->fan2.pwm_reg, value, 1);
  if(ret < 0) {
    syslog(LOG_ERR, "%s: failed to set fan2 pwm 0x%x, value 0x%x", __func__, info->fan2.pwm_reg, value);
    return -1;
  }
  usleep(11000);

  ret = galaxy100_i2c_write(GALAXY100_FANTRAY_I2C_BUS, GALAXY100_FANTRAY_CPLD_ADDR, info->fan3.pwm_reg, value, 1);
  if(ret < 0) {
    syslog(LOG_ERR, "%s: failed to set fan3 pwm 0x%x, value 0x%x", __func__, info->fan3.pwm_reg, value);
    return -1;
  }
  usleep(11000);

  return 0;
}
static int galaxy100_fab_is_present(int fan)
{
  int ret;
  int bits = 4 + fan;

  ret = galaxy100_i2c_read(GALAXY100_CMM_SYS_I2C_BUS, GALAXY100_CMM_SYS_CPLD_ADDR, GALAXY100_MODULE_PRESENT_REG, 1);
  if(ret < 0) {
    syslog(LOG_ERR, "%s: failed to read module present reg 0x%x", __func__, GALAXY100_MODULE_PRESENT_REG);
    return -1;
  }
  usleep(11000);
  if((ret >> bits) & 0x1 == 0x1) {
    /*not present*/
    syslog(LOG_ERR, "%s: FAB-%d not present", __func__, fan + 1);
    return 0;
  } else {
    return 1;
  }
}
static int galaxy100_lc_present_detect(void)
{
  int i = 0, ret;

  ret = galaxy100_i2c_read(GALAXY100_CMM_SYS_I2C_BUS, GALAXY100_CMM_SYS_CPLD_ADDR, GALAXY100_MODULE_PRESENT_REG, 1);
  if(ret < 0) {
    syslog(LOG_ERR, "%s: failed to read module present reg 0x%x", __func__, GALAXY100_MODULE_PRESENT_REG);
    return -1;
  }
  usleep(11000);
  //printf("%s: ret = 0x%x\n", __func__, ret);
  for(i = 0; i < GALAXY100_LC_NUM; i++) {
    if((ret >> i) & 0x1 == 0x1) {
      /*not present*/
      if(lc_status[i] == 0)
        printf("LC-%d is removed\n", i + 1);
      lc_status[i] = 1;
    } else {
      if(lc_status[i] == 1)
        printf("LC-%d is inserted\n", i + 1);
      lc_status[i] = 0;
    }
  }

  return 0;
}

static int galaxy100_scm_present_detect(void)
{
  int i = 0, ret;

  ret = galaxy100_i2c_read(GALAXY100_CMM_SYS_I2C_BUS, GALAXY100_CMM_SYS_CPLD_ADDR, GALAXY100_SCM_PRESENT_REG, 1);
  if(ret < 0) {
    syslog(LOG_ERR, "%s: failed to read module present reg 0x%x", __func__, GALAXY100_SCM_PRESENT_REG);
    return -1;
  }
  usleep(11000);
  //printf("%s: ret = 0x%x\n", __func__, ret);
  for(i = 0; i < GALAXY100_SCM_NUM; i++) {
    if((ret >> i) & 0x1 == 0x1) {
      /*not present*/
      if(scm_status[i] == 0)
        printf("SCM-%d is removed\n", i + 1);
      scm_status[i] = 1;
    } else {
      if(scm_status[i] == 1)
        printf("SCM-%d is inserted\n", i + 1);
      scm_status[i] = 0;
    }
  }

  return 0;
}

static int galaxy100_cmm_is_master(void)
{
  int i = 0, ret;

  ret = galaxy100_i2c_read(GALAXY100_CMM_SYS_I2C_BUS, GALAXY100_CMM_SYS_CPLD_ADDR, GALAXY100_CMM_STATUS_REG, 1);
  if(ret < 0) {
    syslog(LOG_ERR, "%s: failed to read cmm status reg 0x%x", __func__, GALAXY100_CMM_STATUS_REG);
    return -1;
  }
  usleep(11000);
  //printf("%s: ret = 0x%x\n", __func__, ret);
  if(ret & (0x1 << 3)) {
    return 0;//slave
  } else {
    return 1;//master
  }

  return 0;
}

static int galaxy100_chanel_reinit(void)
{
  int i;

  for(i = 0; i < BOARD_INFO_SIZE; i++) {
    struct galaxy100_board_info_stu *info;
    info = &galaxy100_board_info[i];
    if(info->alarm) {
      info->alarm->channel1.addr = GALAXY100_CMM_CHANNEL1_ADDR_BAKUP;
    }
    if(info->critical) {
      info->critical->channel1.addr = GALAXY100_CMM_CHANNEL1_ADDR_BAKUP;
    }
  }

  for(i = 0; i < FANTRAY_INFO_SIZE; i++) {
    struct galaxy100_fantray_info_stu *info;
    struct sensor_info  *channel;

    info = &galaxy100_fantray_info[i];
    channel = &info->channel_info;
    channel->channel1.addr = GALAXY100_CMM_CHANNEL1_ADDR_BAKUP;
  }
}



static int galaxy100_fan_is_okey(int fan)
{
  int ret;
  int error = 0;
  struct galaxy100_fantray_info_stu *info;
  struct sensor_info  *channel;

  info = &galaxy100_fantray_info[fan];
  channel = &info->channel_info;

  ret = galaxy100_fab_is_present(fan);
  if(ret == 0) {
    if(info->present == 1)
      printf("FAB-%d is removed\n", fan + 1);
    info->present = 0; //not preset
    return -1;
  } else if(ret == 1) {
    if(info->present == 0)
      printf("FAB-%d is inserted\n", fan + 1);
    info->present = 1;
  } else {
    return -1;
  }
  ret = galaxy100_channel_set(GALAXY100_FANTRAY_I2C_BUS, channel->channel1, channel->channel2, channel->channel3);
  if(ret < 0)
    return -1;

  ret = galaxy100_i2c_read(GALAXY100_FANTRAY_I2C_BUS, GALAXY100_FANTRAY_CPLD_ADDR, info->fan1.fan_status, 1);
  if(ret < 0) {
    syslog(LOG_ERR, "%s: failed to read fan1 status 0x%x", __func__, info->fan1.fan_status);
    error++;
  } else {
    usleep(11000);
    if(ret & 0x1) {
      if(info->fan1.present == 1)
        printf("FCB-%d fantray 1 is removed\n", fan + 1);
      info->fan1.present = 0;
      error++;
    } else {
      if(info->fan1.present == 0)
        printf("FCB-%d fantray 1 is inserted\n", fan + 1);
      else {
        if(ret > 1) {
          if(info->fan1.failed == 0)
            printf("FCB-%d fantray 1 is failed\n", fan + 1);
          info->fan1.failed = 1;
          error++;
        } else
          info->fan1.failed = 0;
      }
      info->fan1.present = 1;
    }
  }

  ret = galaxy100_i2c_read(GALAXY100_FANTRAY_I2C_BUS, GALAXY100_FANTRAY_CPLD_ADDR, info->fan2.fan_status, 1);
  if(ret < 0) {
    syslog(LOG_ERR, "%s: failed to read fan2 status 0x%x", __func__, info->fan2.fan_status);
    error++;
  } else {
    usleep(11000);
    if(ret & 0x1) {
      if(info->fan2.present == 1)
        printf("FCB-%d fantray 2 is removed\n", fan + 1);
      info->fan2.present = 0;
      error++;
    } else {
      if(info->fan2.present == 0)
        printf("FCB-%d fantray 2 is inserted\n", fan + 1);
      else {
        if(ret > 1) {
          if(info->fan2.failed == 0)
            printf("FCB-%d fantray 2 is failed\n", fan + 1);
          info->fan2.failed = 1;
          error++;
        } else
          info->fan2.failed = 0;
      }
      info->fan2.present = 1;
    }
  }

  ret = galaxy100_i2c_read(GALAXY100_FANTRAY_I2C_BUS, GALAXY100_FANTRAY_CPLD_ADDR, info->fan3.fan_status, 1);
  if(ret < 0) {
    syslog(LOG_ERR, "%s: failed to read fan3 status 0x%x", __func__, info->fan3.fan_status);
    error++;
  } else {
    usleep(11000);
    if(ret & 0x1) {
      if(info->fan3.present == 1)
        printf("FCB-%d fantray 3 is removed\n", fan + 1);
      info->fan3.present = 0;
      error++;
    } else {
      if(info->fan3.present == 0)
        printf("FCB-%d fantray 3 is inserted\n", fan + 1);
      else {
        if(ret > 1) {
          if(info->fan3.failed == 0)
            printf("FCB-%d fantray 3 is failed\n", fan + 1);
          info->fan3.failed = 1;
          error++;
        } else
          info->fan3.failed = 0;
      }
      info->fan3.present = 1;
    }
  }

  if(error)
    return -1;
  else
    return 0;
}

static int galaxy100_write_fan_led(int fan, const char *color)
{
  int ret, value;
  struct galaxy100_fantray_info_stu *info;
  struct sensor_info  *channel;

  info = &galaxy100_fantray_info[fan];
  channel = &info->channel_info;

  if(!strcmp(color, FAN_LED_BLUE))
    value = 1; //blue
  else
    value = 2; //red
  ret = galaxy100_fab_is_present(fan);
  if(ret == 0) {
    info->present = 0; //not preset
    return -1;
  } else if(ret == 1) {
    info->present = 1;
  } else {
    return -1;
  }

  ret = galaxy100_channel_set(GALAXY100_FANTRAY_I2C_BUS, channel->channel1, channel->channel2, channel->channel3);
  if(ret < 0)
    return -1;

  ret = galaxy100_i2c_read(GALAXY100_FANTRAY_I2C_BUS, GALAXY100_FANTRAY_CPLD_ADDR, info->fan1.fan_status, 1);
  if(ret < 0) {
    syslog(LOG_ERR, "%s: failed to read fan1 status 0x%x", __func__, info->fan1.fan_status);
    return -1;
  }
  usleep(11000);
  if((ret & 0x1) == 0 && ((ret > 1 && value == 2) || (ret == 0 && value == 1))) {
    ret = galaxy100_i2c_write(GALAXY100_FANTRAY_I2C_BUS, GALAXY100_FANTRAY_CPLD_ADDR, info->fan1.fan_led_reg, value, 1);
    if(ret < 0) {
      syslog(LOG_ERR, "%s: failed to set led 0x%x, value 0x%x", __func__, info->fan1.fan_led_reg, value);
      return -1;
    }
    usleep(11000);
  }

  ret = galaxy100_i2c_read(GALAXY100_FANTRAY_I2C_BUS, GALAXY100_FANTRAY_CPLD_ADDR, info->fan2.fan_status, 1);
  if(ret < 0) {
    syslog(LOG_ERR, "%s: failed to read fan2 status 0x%x", __func__, info->fan2.fan_status);
    return -1;
  }
  usleep(11000);
  if((ret & 0x1) == 0 && ((ret > 1 && value == 2) || (ret == 0 && value == 1))) {
    ret = galaxy100_i2c_write(GALAXY100_FANTRAY_I2C_BUS, GALAXY100_FANTRAY_CPLD_ADDR, info->fan2.fan_led_reg, value, 1);
    if(ret < 0) {
      syslog(LOG_ERR, "%s: failed to set led  0x%x, value 0x%x", __func__, info->fan2.fan_led_reg, value);
      return -1;
    }
    usleep(11000);
  }

  ret = galaxy100_i2c_read(GALAXY100_FANTRAY_I2C_BUS, GALAXY100_FANTRAY_CPLD_ADDR, info->fan3.fan_status, 1);
  if(ret < 0) {
    syslog(LOG_ERR, "%s: failed to read fan3 status 0x%x", __func__, info->fan3.fan_status);
    return -1;
  }
  usleep(11000);
  if((ret & 0x1) == 0 && ((ret > 1 && value == 2) || (ret == 0 && value == 1))) {
    ret = galaxy100_i2c_write(GALAXY100_FANTRAY_I2C_BUS, GALAXY100_FANTRAY_CPLD_ADDR, info->fan3.fan_led_reg, value, 1);
    if(ret < 0) {
      syslog(LOG_ERR, "%s: failed to set led  0x%x, value 0x%x", __func__, info->fan3.fan_led_reg, value);
      return -1;
    }
    usleep(11000);
  }

  return 1;
}
static int system_shutdown(const char *why)
{
  int ret;
  int i;

  syslog(LOG_EMERG, "Shutting down:  %s", why);
  for(i = 0; i < GALAXY100_PSUS; i++) {
    ret = galaxy100_i2c_write(GALAXY100_PSU_CHANNEL_I2C_BUS, GALAXY100_PSU_CHANNEL_I2C_ADDR, 0x0, 0x1 << i, 1);
    if(ret < 0) {
      syslog(LOG_ERR, "%s: failed to set PSU channel 0x%x, value 0x%x", __func__, GALAXY100_PSU_CHANNEL_I2C_ADDR, 0x1 << i);
      return -1;
    }
    usleep(11000);
    ret = galaxy100_i2c_write(GALAXY100_PSU_CHANNEL_I2C_BUS, GALAXY100_PSU_I2C_ADDR, 0x1, 0x0, 1);
    if(ret < 0) {
      syslog(LOG_ERR, "%s: failed to set PSU shutdown 0x1, value 0x0", __func__);
      return -1;
    }
  }
  stop_watchdog();

  sleep(2);
  exit(2);
}


void usage() {
  fprintf(stderr,
          "fand [-v] [-l <low-pct>] [-m <medium-pct>] "
          "[-h <high-pct>]\n"
          "\t[-b <temp-bottom>] [-t <temp-top>] [-r <report-temp>]\n\n"
          "\tlow-pct defaults to %d%% fan\n"
          "\tmedium-pct defaults to %d%% fan\n"
          "\thigh-pct defaults to %d%% fan\n"
          "\ttemp-bottom defaults to %dC\n"
          "\ttemp-top defaults to %dC\n"
          "\treport-temp defaults to every %d measurements\n\n"
          "fand compensates for uServer temperature reading %d degrees low\n"
          "kill with SIGUSR1 to stop watchdog\n",
          fan_low,
          fan_medium,
          fan_high,
          EXTERNAL_TEMPS(temp_bottom),
          EXTERNAL_TEMPS(temp_top),
          report_temp,
          EXTERNAL_TEMPS(USERVER_TEMP_FUDGE));
  exit(1);
}

int fan_speed_okay(const int fan, const int speed, const int slop) {
  return !galaxy100_fan_is_okey(fan);
}

/* Set fan speed as a percentage */

int write_fan_speed(const int fan, const int value) {
  /*
   * The hardware fan numbers for pwm control are different from
   * both the physical order in the box, and the mapping for reading
   * the RPMs per fan, above.
   */
  int unit = value * PWM_UNIT_MAX / 100;

  if (unit == PWM_UNIT_MAX)
    unit--;

  return galaxy100_set_fan(fan, unit);
}



/* Set up fan LEDs */

int write_fan_led(const int fan, const char *color) {
  return galaxy100_write_fan_led(fan, color);
}


/* Gracefully shut down on receipt of a signal */

void fand_interrupt(int sig)
{
  int fan;
  for (fan = 0; fan < total_fans; fan++) {
    write_fan_speed(fan + fan_offset, fan_max);
  }

  syslog(LOG_WARNING, "Shutting down fand on signal %s", strsignal(sig));
  if (sig == SIGUSR1) {
    stop_watchdog();
  }
  exit(3);
}

int main(int argc, char **argv) {
  /* Sensor values */
  int critical_temp;
  int alarm_temp;
  int old_temp = GALAXY100_RAISING_TEMP_HIGH;
  int raising_pwm;
  int falling_pwm;
  struct galaxy100_fantray_info_stu *info;
  int fan_speed = fan_medium;
  int failed_speed = 0;

  int bad_reads = 0;
  int fan_failure = 0;
  int fan_speed_changes = 0;
  int old_speed;
  int count = 0;

  int fan_bad[FANS];
  int fan;

  unsigned log_count = 0; // How many times have we logged our temps?
  int opt;
  int prev_fans_bad = 0;

  struct sigaction sa;

  sa.sa_handler = fand_interrupt;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);

  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGUSR1, &sa, NULL);

  // Start writing to syslog as early as possible for diag purposes.

  openlog("fand", LOG_CONS, LOG_DAEMON);

  while ((opt = getopt(argc, argv, "l:m:h:b:t:r:v")) != -1) {
    switch (opt) {
    case 'l':
      fan_low = atoi(optarg);
      break;
    case 'm':
      fan_medium = atoi(optarg);
      break;
    case 'h':
      fan_high = atoi(optarg);
      break;
    case 'b':
      temp_bottom = INTERNAL_TEMPS(atoi(optarg));
      break;
    case 't':
      temp_top = INTERNAL_TEMPS(atoi(optarg));
      break;
    case 'r':
      report_temp = atoi(optarg);
      break;
    case 'v':
      verbose = true;
      break;
    default:
      usage();
      break;
    }
  }

  if (optind > argc) {
    usage();
  }

  if (temp_bottom > temp_top) {
    fprintf(stderr,
            "Should temp-bottom (%d) be higher than "
            "temp-top (%d)?  Starting anyway.\n",
            EXTERNAL_TEMPS(temp_bottom),
            EXTERNAL_TEMPS(temp_top));
  }

  if (fan_low > fan_medium || fan_low > fan_high || fan_medium > fan_high) {
    fprintf(stderr,
            "fan RPMs not strictly increasing "
            "-- %d, %d, %d, starting anyway\n",
            fan_low,
            fan_medium,
            fan_high);
  }

  daemon(1, 0);

  if (verbose) {
    syslog(LOG_DEBUG, "Starting up;  system should have %d fans.",
           total_fans);
  }

  if(galaxy100_i2c_read(GALAXY100_CMM_SYS_I2C_BUS, GALAXY100_CMM_SYS_CPLD_ADDR,
                        GALAXY100_CMM_SLOTID_REG, 1) == 0x1) {
    // on CMM1
    galaxy100_chanel_reinit();
  }
  if(galaxy100_cmm_is_master() == 1) {
    for (fan = 0; fan < total_fans; fan++) {
      fan_bad[fan] = 0;
      write_fan_speed(fan + fan_offset, fan_speed);
      write_fan_led(fan + fan_offset, FAN_LED_BLUE);
    }
  }

  /* Start watchdog in manual mode */
  start_watchdog(0);

  /* Set watchdog to persistent mode so timer expiry will happen independent
   * of this process's liveliness. */
  set_persistent_watchdog(WATCHDOG_SET_PERSISTENT);

  sleep(5);  /* Give the fans time to come up to speed */

  while (1) {
    int max_temp;
    old_speed = fan_speed;

    /*if it is master, then run next*/
    if(galaxy100_cmm_is_master() != 1) {
      sleep(5);
      kick_watchdog();
      continue;
    }

    /* Read sensors */
    critical_temp = read_critical_max_temp();
    alarm_temp = read_alarm_max_temp();
    if ((critical_temp == BAD_TEMP || alarm_temp == BAD_TEMP)) {
      bad_reads++;
    }

    if (log_count++ % report_temp == 0) {
      syslog(LOG_DEBUG,
             "critical temp %d, alarm temp %d, fan speed %d, speed changes %d",
             critical_temp, alarm_temp, fan_speed, fan_speed_changes);
    }

    /* Protection heuristics */
    if(critical_temp > GALAXY100_SYSTEM_LIMIT) {
      system_shutdown("Critical temp limit reached");
    }

    /*
     * Calculate change needed -- we should eventually
     * do something more sophisticated, like PID.
     *
     * We should use the intake temperature to adjust this
     * as well.
     */

    /* Other systems use a simpler built-in table to determine fan speed. */
    raising_pwm = calculate_raising_fan_pwm(critical_temp);
    falling_pwm = calculate_falling_fan_pwm(critical_temp);
    if(old_temp <= critical_temp) {
      /*raising*/
      if(raising_pwm >= fan_speed) {
        fan_speed = raising_pwm;
      }
    } else {
      /*falling*/
      if(falling_pwm <= fan_speed ) {
        fan_speed = falling_pwm;
      }
    }
    old_temp = critical_temp;

    /*
     * Update fans only if there are no failed ones. If any fans failed
     * earlier, all remaining fans should continue to run at max speed.
     */
    if (fan_failure == 0 && fan_speed != old_speed) {
      syslog(LOG_NOTICE,
             "Fan speed changing from %d to %d",
             old_speed,
             fan_speed);
      fan_speed_changes++;
      for (fan = 0; fan < total_fans; fan++) {
        write_fan_speed(fan + fan_offset, fan_speed);
      }
    }

    /*
     * Wait for some change.  Typical I2C temperature sensors
     * only provide a new value every second and a half, so
     * checking again more quickly than that is a waste.
     *
     * We also have to wait for the fan changes to take effect
     * before measuring them.
     */

    sleep(5);
    galaxy100_lc_present_detect();
    galaxy100_scm_present_detect();


    /* Check fan RPMs */

    for (fan = 0; fan < total_fans; fan++) {
      /*
       * Make sure that we're within some percentage
       * of the requested speed.
       */
      if (fan_speed_okay(fan + fan_offset, fan_speed, FAN_FAILURE_OFFSET)) {
        if (fan_bad[fan] >= FAN_FAILURE_THRESHOLD) {
          write_fan_led(fan + fan_offset, FAN_LED_BLUE);
          syslog(LOG_CRIT,
                 "Fan %d has recovered",
                 fan);
        }
        fan_bad[fan] = 0;
      } else {
        info = &galaxy100_fantray_info[fan];
        if(info->present == 0 | info->fan1.failed == 1 | info->fan1.present == 0 |
           info->fan2.failed == 1 | info->fan2.present == 0 |
           info->fan3.failed == 1 | info->fan3.present == 0)
          fan_bad[fan]++;

      }
    }

    fan_failure = 0;
    for (fan = 0; fan < total_fans; fan++) {
      if (fan_bad[fan] >= FAN_FAILURE_THRESHOLD) {
        fan_failure++;
        write_fan_led(fan + fan_offset, FAN_LED_RED);
      }
    }

    if (fan_failure > 0) {
      if (prev_fans_bad != fan_failure) {
        syslog(LOG_CRIT, "%d fans failed", fan_failure);
      }

      /*
       * If fans are bad, we need to blast all of the
       * fans at 100%;  we don't bother to turn off
       * the bad fans, in case they are all that is left.
       *
       * Note that we have a temporary bug with setting fans to
       * 100% so we only do fan_max = 99%.
       */
      if (fan_failure > 0) {
        int not_present = 0;
        int fan_failed = 0;

        for (fan = 0; fan < total_fans; fan++) {
          info = &galaxy100_fantray_info[fan];
          if(info->present == 0)
            not_present++;
          if(info->fan1.failed == 1 | info->fan1.present == 0)
            fan_failed++;
          if(info->fan2.failed == 1 | info->fan2.present == 0)
            fan_failed++;
          if(info->fan3.failed == 1 | info->fan3.present == 0)
            fan_failed++;
        }
        if(fan_failed >= 6)
          count++;
        else if(not_present == 1 && fan_failed >= 3)
          count++;
        else if(not_present >= 2)
          count++;
        else
          count = 0;
        if (count >= FAN_SHUTDOWN_THRESHOLD) {
          system_shutdown("two FCB failed more than 5 mins");
        }
#if 0
        if(fan_failed == 1) {
          fan_speed += GALAXY100_FAN_ONEFAILED_RAISE_PEC;
          if(fan_speed > fan_max)
            fan_speed = fan_max;
        } else
          fan_speed = fan_max;
#endif
        fan_failed += not_present * 3;
        if(fan_failed > 0 && fan_failed <= 3) {
          if(fan_speed == GALAXY100_FAN_LOW) {
            failed_speed = galaxy100_fan_failed_control[fan_failed - 1].low_level;
          } else if(fan_speed == GALAXY100_FAN_MEDIUM) {
            failed_speed = galaxy100_fan_failed_control[fan_failed - 1].mid_level;
          } else if(fan_speed == GALAXY100_FAN_HIGH) {
            failed_speed = galaxy100_fan_failed_control[fan_failed - 1].high_level;
          } else if(fan_speed == GALAXY100_FAN_MAX) {
            failed_speed = galaxy100_fan_failed_control[fan_failed - 1].alarm_level;
          }
        } else {
          failed_speed = fan_max;
        }
        for (fan = 0; fan < total_fans; fan++) {
          write_fan_speed(fan + fan_offset, failed_speed);
        }
      }

      /*
       * Fans can be hot swapped and replaced; in which case the fan daemon
       * will automatically detect the new fan and (assuming the new fan isn't
       * itself faulty), automatically readjust the speeds for all fans down
       * to a more suitable rpm. The fan daemon does not need to be restarted.
       */
    } else if(prev_fans_bad != 0 && fan_failure == 0){
      old_temp = GALAXY100_RAISING_TEMP_HIGH;
      fan_speed = fan_medium;
      for (fan = 0; fan < total_fans; fan++) {
        write_fan_speed(fan + fan_offset, fan_speed);
      }
    }

    /* Suppress multiple warnings for similar number of fan failures. */
    prev_fans_bad = fan_failure;

    /* if everything is fine, restart the watchdog countdown. If this process
     * is terminated, the persistent watchdog setting will cause the system
     * to reboot after the watchdog timeout. */
    kick_watchdog();
  }
}
