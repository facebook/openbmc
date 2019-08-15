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
#include <dirent.h>
#ifdef CONFIG_GALAXY100
#include <fcntl.h>
#endif

#include <openbmc/watchdog.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/misc-utils.h>
#include <openbmc/obmc-pmbus.h>

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
#define FAN_PRESENT 0
#define FAN_ALIVE 0

#define CMM_BMC_HEART_BEAT "cmm_bmc_heart_beat"
#define CLEAR_CMM_BMC_HEART_BEAT 1

#define PATH_CACHE_SIZE 256

#define log_error(fmt, args...) \
  syslog(LOG_ERR, "%s" fmt ": %s", __func__, ##args, strerror(errno))
#define log_warn(fmt, args...) \
  syslog(LOG_WARNING, "%s" fmt ": %s", __func__, ##args, strerror(errno))

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof((_a)[0]))
#endif

struct sensor_info_sysfs {
  char* prefix;
  char* suffix;
  uchar temp;
  int (*read_sysfs)(struct sensor_info_sysfs *sensor);
  char path_cache[PATH_CACHE_SIZE];
};

struct fan_info_stu_sysfs {
  char *front_fan_prefix;
  char *rear_fan_prefix;
  char *pwm_prefix;
  char *fan_led_prefix;
  uchar present;
  uchar failed;
};

struct psu_device_info {
  struct {
    char *filename;
    int value;
  } sysfs_shutdown;

  i2c_id_t devices[GALAXY100_PSUS];
};

struct galaxy100_board_info_stu_sysfs {
  const char *name;
  uint slot_id;
  struct sensor_info_sysfs *critical;
  struct sensor_info_sysfs *alarm;
};

struct galaxy100_fantray_info_stu_sysfs {
  const char *name;
  int present;
  struct sensor_info_sysfs channel_info;
  struct fan_info_stu_sysfs fan1;
  struct fan_info_stu_sysfs fan2;
  struct fan_info_stu_sysfs fan3;
};

struct galaxy100_fan_failed_control_stu {
  int failed_num;
  int low_level;
  int mid_level;
  int high_level;
  int alarm_level;
};

static int read_temp_sysfs(sensor_info_sysfs *sensor);
static int read_critical_max_temp(void);
static int read_alarm_max_temp(void);
static int calculate_falling_fan_pwm(int temp);
static int calculate_raising_fan_pwm(int temp);

static int galaxy100_set_fan_sysfs(int fan, int value);
static int galaxy100_fan_is_okey_sysfs(int fan);
static int galaxy100_write_fan_led_sysfs(int fan, const char *color);
static int galaxy100_fab_is_present_sysfs(int fan);
static int galaxy100_lc_present_detect(void);
static int galaxy100_scm_present_detect(void);
static int galaxy100_cmm_is_master(void);
static int galaxy100_chanel_reinit_sysfs(void);

/*CMM info*/
struct sensor_info_sysfs cmm_sensor_critical_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/3-0048",
  .suffix = "temp1_input",
  .temp = 0,
  .read_sysfs = &read_temp_sysfs,
};
struct sensor_info_sysfs cmm_sensor_alarm_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/3-0049",
  .suffix = "temp1_input",
  .temp = 0,
  .read_sysfs = &read_temp_sysfs,
};

struct sensor_info_sysfs scm_fab_1_sensor_critical_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/113-0048",
  .suffix = "temp1_input",
  .temp = 0,
  .read_sysfs = &read_temp_sysfs,
};
struct sensor_info_sysfs scm_fab_2_sensor_critical_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/129-0048",
  .suffix = "temp1_input",
  .temp = 0,
  .read_sysfs = &read_temp_sysfs,
};
struct sensor_info_sysfs scm_fab_3_sensor_critical_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/145-0048",
  .suffix = "temp1_input",
  .temp = 0,
  .read_sysfs = &read_temp_sysfs,
};
struct sensor_info_sysfs scm_fab_4_sensor_critical_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/161-0048",
  .suffix = "temp1_input",
  .temp = 0,
  .read_sysfs = &read_temp_sysfs,
};
struct sensor_info_sysfs scm_lc_1_sensor_critical_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/49-0048",
  .suffix = "temp1_input",
  .temp = 0,
  .read_sysfs = &read_temp_sysfs,
};
struct sensor_info_sysfs scm_lc_2_sensor_critical_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/65-0048",
  .suffix = "temp1_input",
  .temp = 0,
  .read_sysfs = &read_temp_sysfs,
};
struct sensor_info_sysfs scm_lc_3_sensor_critical_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/81-0048",
  .suffix = "temp1_input",
  .temp = 0,
  .read_sysfs = &read_temp_sysfs,
};
struct sensor_info_sysfs scm_lc_4_sensor_critical_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/97-0048",
  .suffix = "temp1_input",
  .temp = 0,
  .read_sysfs = &read_temp_sysfs,
};

//
// No read function within the structure is used.
// as most of fantray read is done by concaternating
// fan name after fantray directory name.
// (also hwmon scanning is not required)
// Therefore it is not provided here
//
/* fantran info*/
struct sensor_info_sysfs fantray1_channel_info = {
  .prefix = "/sys/bus/i2c/drivers/fancpld/171-0033",
  .suffix = "",
  .temp = 0,
  .read_sysfs = NULL,
};
struct sensor_info_sysfs fantray2_channel_info = {
  .prefix = "/sys/bus/i2c/drivers/fancpld/179-0033",
  .suffix = "",
  .temp = 0,
  .read_sysfs = NULL,
};
struct sensor_info_sysfs fantray3_channel_info = {
  .prefix = "/sys/bus/i2c/drivers/fancpld/187-0033",
  .suffix = "",
  .temp = 0,
  .read_sysfs = NULL,
};
struct sensor_info_sysfs fantray4_channel_info = {
  .prefix = "/sys/bus/i2c/drivers/fancpld/195-0033",
  .suffix = "",
  .temp = 0,
  .read_sysfs = NULL,
};

struct fan_info_stu_sysfs fan1_info = {
  .front_fan_prefix = "fan1_input",
  .rear_fan_prefix = "fan2_input",
  .pwm_prefix = "fantray1_pwm",
  .fan_led_prefix = "fantray1_led_ctrl",
  .present = 1,
  .failed = 0,
};
struct fan_info_stu_sysfs fan2_info = {
  .front_fan_prefix = "fan3_input",
  .rear_fan_prefix = "fan4_input",
  .pwm_prefix = "fantray2_pwm",
  .fan_led_prefix = "fantray2_led_ctrl",
  .present = 1,
  .failed = 0,
};
struct fan_info_stu_sysfs fan3_info = {
  .front_fan_prefix = "fan5_input",
  .rear_fan_prefix = "fan6_input",
  .pwm_prefix = "fantray3_pwm",
  .fan_led_prefix = "fantray3_led_ctrl",
  .present = 1,
  .failed = 0,
};

/************board and fantray info*****************/
struct galaxy100_board_info_stu_sysfs galaxy100_board_info[] = {
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
struct galaxy100_fantray_info_stu_sysfs galaxy100_fantray_info[] = {
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

struct psu_device_info galaxy100_psu_info = {
  .sysfs_shutdown = {
    .filename = "control/shutdown",
    .value = 1,
  },

  .devices = {
    [0] = {
      .bus = 24,
      .addr = 0x10,
    },
    [1] = {
      .bus = 25,
      .addr = 0x10,
    },
    [2] = {
      .bus = 26,
      .addr = 0x10,
    },
    [3] = {
      .bus = 27,
      .addr = 0x10,
    },
  },
};



#define BOARD_INFO_SIZE (sizeof(galaxy100_board_info) \
                        / sizeof(struct galaxy100_board_info_stu_sysfs))
#define FANTRAY_INFO_SIZE (sizeof(galaxy100_fantray_info) \
                        / sizeof(struct galaxy100_fantray_info_stu_sysfs))

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

/*
 * Helper function to probe directory, and make full path
 * Will probe directory structure, then make a full path
 * using "<prefix>/hwmon/hwmonxxx/<suffix>"
 * returns < 0, if hwmon directory does not exist or something goes wrong
 */
int assemble_sysfs_path(const char* prefix, const char* suffix,
                        char* full_name, int buffer_size)
{
  int rc = 0;
  int dirname_found = 0;
  char temp_str[PATH_CACHE_SIZE];
  DIR *dir = NULL;
  struct dirent *ent;

  if (full_name == NULL)
    return -1;

  snprintf(temp_str, (buffer_size - 1), "%s/hwmon", prefix);
  dir = opendir(temp_str);
  if (dir == NULL) {
    rc = ENOENT;
    goto close_dir_out;
  }

  while ((ent = readdir(dir)) != NULL) {
    if (strstr(ent->d_name, "hwmon")) {
      // found the correct 'hwmon??' directory
      snprintf(full_name, buffer_size, "%s/%s/%s",
               temp_str, ent->d_name, suffix);
      dirname_found = 1;
      break;
    }
  }

close_dir_out:
  if (dir != NULL) {
    closedir(dir);
  }

  if (dirname_found == 0) {
    rc = ENOENT;
  }

  return rc;
}

// Functions for reading from sysfs stub
static int read_sysfs_raw_internal(const char *device, char *value, int log)
{
  FILE *fp;
  int rc, err;

  fp = fopen(device, "r");
  if (!fp) {
    if (log) {
      err = errno;
      syslog(LOG_INFO, "failed to open device %s for read: %s",
             device, strerror(err));
      errno = err;
    }
    return -1;
  }

  rc = fscanf(fp, "%s", value);
  fclose(fp);

  if (rc != 1) {
    if (log) {
      err = errno;
      syslog(LOG_INFO, "failed to read device %s: %s",
             device, strerror(err));
      errno = err;
    }
    return -1;
  }

  return 0;
}

static int read_sysfs_raw(char *sysfs_path, char *buffer)
{
  return read_sysfs_raw_internal(sysfs_path, buffer, 1);
}

// Returns 0 for success, or -1 on failures.
static int read_sysfs_int(char *sysfs_path, int *buffer)
{
  int rc;
  char readBuf[PATH_CACHE_SIZE];

  if (sysfs_path == NULL || buffer == NULL) {
    errno = EINVAL;
    return -1;
  }

  rc = read_sysfs_raw(sysfs_path, readBuf);
  if (rc == 0)
  {
    if (strstr(readBuf, "0x") ||
        strstr(readBuf, "0X"))
      sscanf(readBuf, "%x", buffer);
    else
      sscanf(readBuf, "%d", buffer);
  }
  return rc;
}

// Functions for writing to system stub
static int write_sysfs_raw_internal(const char *device, char *value, int log)
{
  FILE *fp;
  int rc, err;

  fp = fopen(device, "w");
  if (!fp) {
    if (log) {
      err = errno;
      syslog(LOG_INFO, "failed to open device %s for write : %s",
             device, strerror(err));
      errno = err;
    }
    return err;
  }

  rc = fputs(value, fp);
  fclose(fp);

  if (rc < 0) {
    if (log) {
      err = errno;
      syslog(LOG_INFO, "failed to write to device %s", device);
      errno = err;
    }
    return -1;
  }

  return 0;
}

static int write_sysfs_raw(char *sysfs_path, char* buffer)
{
  return write_sysfs_raw_internal(sysfs_path, buffer, 1);
}

// Returns 0 for success, or -1 on failures.
static int write_sysfs_int(char *sysfs_path, int buffer)
{
  int rc;
  char writeBuf[PATH_CACHE_SIZE];

  if (sysfs_path == NULL) {
    errno = EINVAL;
    return -1;
  }

  snprintf(writeBuf, PATH_CACHE_SIZE, "%d", buffer);
  return write_sysfs_raw(sysfs_path, writeBuf);
}

static int read_temp_sysfs(struct sensor_info_sysfs *sensor)
{
  int ret;
  int value;
  char fullpath[PATH_CACHE_SIZE];
  bool use_cache = false;
  int cache_str_len = 0;

  if (sensor == NULL)
  {
    syslog(LOG_NOTICE, "sensor is null\n");
    return -1;
  }
  // Check if cache is available
  if (sensor->path_cache != NULL)
  {
    cache_str_len = strlen(sensor->path_cache);
    if (cache_str_len != 0)
      use_cache = true;
  }

  if (use_cache == false)
  {
    // No cached value yet. Calculate the full path first
    ret = assemble_sysfs_path(sensor->prefix, sensor->suffix, fullpath, sizeof(fullpath));
    if(ret != 0)
    {
      syslog(LOG_NOTICE, "%s: I2C bus %s not available. Failed reading %s\n", __FUNCTION__, sensor->prefix, sensor->suffix);
      return -1;
    }
    // Update cache, if possible.
    if (sensor->path_cache != NULL)
      snprintf(sensor->path_cache, (PATH_CACHE_SIZE - 1), "%s", fullpath);
      use_cache = true;
  }

  /*
   * By the time control reaches here, use_cache is always true
   * or this function already returned -1. So assume the cache is always on
   */
  ret = read_sysfs_int(sensor->path_cache, &value);

  /*  Note that Kernel sysfs stub pre-converts raw value in xxxxx format,
   *  which is equivalent to xx.xxx degree - all we need to do is to divide
   *  the read value by 1000
  */
  if (ret < 0)
    value = ret;
  else
    value = value / 1000;

  if (value < 0) {
    log_error("failed to read temperature bus %s", fullpath);
    return -1;
  }

  usleep(11000);
  return value;
}

static int read_critical_max_temp(void)
{
  int i;
  int temp, max_temp = 0;
  struct galaxy100_board_info_stu_sysfs *info;

  for(i = 0; i < BOARD_INFO_SIZE; i++) {
    info = &galaxy100_board_info[i];
    if(info->critical) {
      temp = read_temp_sysfs(info->critical);
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
  struct galaxy100_board_info_stu_sysfs *info;

  for(i = 0; i < BOARD_INFO_SIZE; i++) {
    info = &galaxy100_board_info[i];
    if(info->alarm) {
      temp = read_temp_sysfs(info->alarm);
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

// Note that the fan number here is 0-based
static int galaxy100_set_fan_sysfs(int fan, int value)
{
  int ret;
  struct galaxy100_fantray_info_stu_sysfs *info;
  struct sensor_info_sysfs *channel;

  info = &galaxy100_fantray_info[fan];
  channel = &info->channel_info;
  char fullpath[PATH_CACHE_SIZE];

  ret = galaxy100_fab_is_present_sysfs(fan);
  if(ret == 0) {
    info->present = 0; //not preset
    return -1;
  } else if(ret == 1) {
    info->present = 1;
  } else {
    return -1;
  }

  snprintf(fullpath, PATH_CACHE_SIZE, "%s/%s", channel->prefix, "fantray1_pwm");
  ret = write_sysfs_int(fullpath, value);
  if(ret < 0) {
    log_error("failed to set fan %s...%s, value %#x",
              channel->prefix, "fantray1_pwm", value);
    return -1;
  }
  usleep(11000);

  snprintf(fullpath, PATH_CACHE_SIZE, "%s/%s", channel->prefix, "fantray2_pwm");
  ret = write_sysfs_int(fullpath, value);
  if(ret < 0) {
    log_error("failed to set fan %s...%s, value %#x",
              channel->prefix, "fantray2_pwm", value);
    return -1;
  }
  usleep(11000);

  snprintf(fullpath, PATH_CACHE_SIZE, "%s/%s", channel->prefix, "fantray3_pwm");
  ret = write_sysfs_int(fullpath, value);
  if(ret < 0) {
    log_error("failed to set fan %s...%s, value 0x%x",
              channel->prefix, "fantray3_pwm", value);
    return -1;
  }
  usleep(11000);

  return 0;
}

/*
 * Fan number here is 0-based
 * Note that 1 means present, unlike LC presence detection
 */
static int galaxy100_fab_is_present_sysfs(int fan)
{
  int ret;
  int fan_1base = fan + 1;
  char buf[PATH_CACHE_SIZE];
  int rc = 0;

  snprintf(buf, PATH_CACHE_SIZE, "/sys/bus/i2c/drivers/cmmcpld/13-003e/fc%d_present", fan_1base);

  rc = read_sysfs_int(buf, &ret);
  if(rc < 0) {
    log_error("failed to read module present reg %#x",
    GALAXY100_MODULE_PRESENT_REG);
    return -1;
  }

  usleep(11000);

  if (ret != 0) {
    syslog(LOG_ERR, "%s: FAB-%d not present", __func__, fan + 1);
    return 0;
  } else {
    return 1;
  }

  // Make compiler happy
  return 0;
}

// Note that 0 means present, unlike FC detection function
static int galaxy100_lc_present_detect(void)
{
  int i = 0, ret;
  int lc_1base = 0;
  char buf[PATH_CACHE_SIZE];
  int rc = 0;

  for(i = 0; i < GALAXY100_LC_NUM; i++) {
    lc_1base = i + 1;
    snprintf(buf, PATH_CACHE_SIZE, "/sys/bus/i2c/drivers/cmmcpld/13-003e/lc%d_present", lc_1base);
    rc = read_sysfs_int(buf, &ret);
    usleep(11000);
    if (rc == 0)
    {
      if (ret != 0) {
        /*not present*/
        if(lc_status[i] == 0)
        syslog(LOG_NOTICE, "LC-%d is removed\n", i + 1);
        lc_status[i] = 1;
      } else {
        if(lc_status[i] == 1)
          syslog(LOG_NOTICE, "LC-%d is inserted\n", i + 1);
        lc_status[i] = 0;
      }
    } else
    {
      log_warn("Unable to detect LC presence of LC%d(1-based)", lc_1base);
      lc_status[i] = 1;
    }
  }

  return 0;
}

static int galaxy100_scm_present_detect(void)
{
  int i = 0, ret;
  int scm_1base = 0;
  char buf[PATH_CACHE_SIZE];
  int rc = 0;

  //printf("%s: ret = 0x%x\n", __func__, ret);
  for(i = 0; i < GALAXY100_SCM_NUM; i++) {

    if ((i == 0) || (i == 1))
      // SCM 0, 1 are for FC 1 and 2
      snprintf(buf, PATH_CACHE_SIZE,
               "/sys/bus/i2c/drivers/cmmcpld/13-003e/scm_fc%d_present",
               i + 1);
    else if ((i >= 2) && (i <= 5))
      // SCM 2 to 5 are for LC1 to 4
      snprintf(buf, PATH_CACHE_SIZE,
               "/sys/bus/i2c/drivers/cmmcpld/13-003e/scm_lc%d_present",
               i - 1);
    else
      // SCM 6 and 7 are for FC 3 and 4
      snprintf(buf, PATH_CACHE_SIZE,
               "/sys/bus/i2c/drivers/cmmcpld/13-003e/scm_fc%d_present",
               i - 3);

    rc = read_sysfs_int(buf, &ret);
    usleep(11000);

    if (rc == 0)
    {
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
    } else
    {
      printf("Unable to detect LC presence of SCM%d(0-based)!\n", i);
      scm_status[i] = 1;
    }
  }

  return 0;
}

static int galaxy100_cmm_is_master(void)
{
  int i = 0, ret;
  char buf[PATH_CACHE_SIZE];
  int rc = 0;
  snprintf(buf, PATH_CACHE_SIZE, "/sys/bus/i2c/drivers/cmmcpld/13-003e/cmm_role");
  rc = read_sysfs_int(buf, &ret);
  if(rc < 0) {
    log_error("failed to read cmm status reg %#x", GALAXY100_CMM_STATUS_REG);
    return -1;
  }
  usleep(11000);

  // In new function, it's a bit tricky.
  // Readvalue : 0 --> We are master --> Function returns 1
  // Readvalue : 1 --> We are slave  --> Function returns 0

  if (ret != 0) // Read value says we are NOT master
    return 0;   // return 0, meaning we are NOT master

  return 1; // Otherwise, return 1, meaning we ARE master
}

/*
 * SYSFS already takes care of this logic - finding the right mux
 * to access fans. So nothing needs to be done here.
 */
static int galaxy100_chanel_reinit_sysfs(void)
{
  return 0;
}

/*
 * Given fan tray number, Check fan presence status,
 * and return error if any missing or problematic
 * Again, fan here is 0 based.
 */
static int galaxy100_fan_is_okey_sysfs(int fan)
{
  int ret;
  int error = 0;
  struct galaxy100_fantray_info_stu_sysfs *info;
  struct sensor_info_sysfs  *channel;

  info = &galaxy100_fantray_info[fan];
  channel = &info->channel_info;
  int rc = 0;

  char fullpath[PATH_CACHE_SIZE];

  // First, make sure FC is present, before checking every fan behind it.
  ret = galaxy100_fab_is_present_sysfs(fan);
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

  snprintf(fullpath, PATH_CACHE_SIZE, "%s/fantray%d_present",
           channel->prefix, 1);
  rc = read_sysfs_int(fullpath, &ret);
  if ((ret < 0)|| (rc < 0))
  {
    log_error("failed to read fan1 status %s", fullpath);
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
        snprintf(fullpath, PATH_CACHE_SIZE, "%s/fantray%d_alive",
                 channel->prefix, 1);
        rc = read_sysfs_int(fullpath, &ret);
        if((rc != 0) || (ret != 0)) {
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


  snprintf(fullpath, PATH_CACHE_SIZE, "%s/fantray%d_present",
           channel->prefix, 2);
  rc = read_sysfs_int(fullpath, &ret);
  if ((ret < 0)|| (rc < 0))
  {
    log_error("failed to read fan2 status %s", fullpath);
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
        snprintf(fullpath, PATH_CACHE_SIZE, "%s/fantray%d_alive",
                 channel->prefix, 2);
        rc = read_sysfs_int(fullpath, &ret);
        if((rc != 0) || (ret != 0)) {
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


  snprintf(fullpath, PATH_CACHE_SIZE, "%s/fantray%d_present",
           channel->prefix, 3);
  rc = read_sysfs_int(fullpath, &ret);
  if ((ret < 0)|| (rc < 0))
  {
    log_error("failed to read fan3 status %s", fullpath);
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
        snprintf(fullpath, PATH_CACHE_SIZE, "%s/fantray%d_alive",
                 channel->prefix, 3);
        rc = read_sysfs_int(fullpath, &ret);
        if((rc != 0) || (ret != 0)) {
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


static int galaxy100_write_fan_led_sysfs(int fan, const char *color)
{
  int ret, value;
  struct galaxy100_fantray_info_stu_sysfs *info;
  struct sensor_info_sysfs  *channel;
  int rc = 0;
  int fan_alive = 0;
  int fan_present = 0;

  info = &galaxy100_fantray_info[fan];
  channel = &info->channel_info;

  int fan_1base = 0;
  char fullpath[PATH_CACHE_SIZE];

  if(!strcmp(color, FAN_LED_BLUE))
    value = 1; //blue
  else
    value = 2; //red
  ret = galaxy100_fab_is_present_sysfs(fan);
  if(ret == 0) {
    info->present = 0; //not preset
    return -1;
  } else if(ret == 1) {
    info->present = 1;
  } else {
    return -1;
  }

  for (fan_1base = 1; fan_1base <=3; fan_1base++)
  {
    // First, make sure fan tray is  present.
    snprintf(fullpath, PATH_CACHE_SIZE, "%s/fantray%d_present",
             channel->prefix, fan_1base);
    rc = read_sysfs_int(fullpath, &fan_present);
    if (rc < 0) {
      log_error("fantray %d not present (%s : %d)",
                fan_1base, fullpath, ret);
      return -1;
    }
    // Second, make sure it's in good condition
    snprintf(fullpath, PATH_CACHE_SIZE, "%s/fantray%d_alive",
             channel->prefix, fan_1base);
    rc = read_sysfs_int(fullpath, &fan_alive);
    if (rc < 0) {
      log_error("fantray %d is in failure state (%s : %d)",
                fan_1base, fullpath, ret);
      return -1;
    }
    // If the state matches with what we want to write, write to led reg
    if (fan_present == FAN_PRESENT)
    {
      if (((fan_alive == FAN_ALIVE) && (value == 1))|| // ALIVE and write BLUE
          ((fan_alive != FAN_ALIVE) && (value == 2)))  // BAD AND write RED
      {
        // Third, write value
        snprintf(fullpath, PATH_CACHE_SIZE, "%s/fantray%d_led_ctrl",
                 channel->prefix, fan_1base);
        write_sysfs_int(fullpath, value);
        usleep(11000);
        // Finally, read back, and make sure the value is there.
        rc = read_sysfs_int(fullpath, &ret);
        if ((rc < 0) || (ret != value)) {
          log_error("failed to set led %s, value %#x "
                    "(rc = %d, ret = %d, value = %d)\n",
                    fullpath, value, rc, ret, value);
          return -1;
        }
      }
    }
  }
  return 1;
}

/*
 * Clear CMM_BMC watchdog timer in every fancpld.
 * (Do the best effort)
 */
void clear_fancpld_watchdog_timer() {
  char sysfs_path_str[PATH_CACHE_SIZE];
  int rc = 0, fancpld_inst = 0;
  /*
   * galaxy100_fantray_info has NULL as its last entry, so the actual number of
   * fans in this list is FANTRAY_INFO_SIZE - 1.
   */
  for (fancpld_inst = 0;  fancpld_inst < (FANTRAY_INFO_SIZE - 1); fancpld_inst++)
  {
    snprintf(sysfs_path_str, PATH_CACHE_SIZE,
             "%s/%s",
             galaxy100_fantray_info[fancpld_inst].channel_info.prefix,
             CMM_BMC_HEART_BEAT);
    rc = write_sysfs_int(sysfs_path_str, CLEAR_CMM_BMC_HEART_BEAT);
    if (rc < 0)
      log_warn("failed to clear watchdog in inst%d (%s)",
               fancpld_inst, sysfs_path_str);
  }
}

/*
 * Shutdown system via pfe3000's "shutdown" sysfs node.
 */
static int system_shutdown_sysfs(void)
{
  int i, ret;
  i2c_id_t *psu_dev;
  char i2c_suid[NAME_MAX], pathname[PATH_MAX];

  for(i = 0; i < ARRAY_SIZE(galaxy100_psu_info.devices); i++) {
    psu_dev = &galaxy100_psu_info.devices[i];
    i2c_sysfs_suid_gen(i2c_suid, sizeof(i2c_suid),
                       psu_dev->bus, psu_dev->addr);
    path_join(pathname, sizeof(pathname), I2C_SYSFS_DEVICES,
              i2c_suid, galaxy100_psu_info.sysfs_shutdown.filename, NULL);

    if (!path_exists(pathname)) {
        errno = ENOENT;
        return -1;
    }

    ret = write_sysfs_int(pathname, galaxy100_psu_info.sysfs_shutdown.value);
    if(ret < 0) {
      log_error("failed to turn off psu device %d-00%x via sysfs: %s",
                psu_dev->bus, psu_dev->addr, strerror(errno));
      return -1;
    }
  }

  return 0;
}

/*
 * Shutdown system by sending pmbus command.
 */
static int system_shutdown_pmbus(void)
{
  int i, ret;
  int page = 0;
  i2c_id_t *psu_dev;
  pmbus_dev_t *pmdev;

  for (i = 0; i < ARRAY_SIZE(galaxy100_psu_info.devices); i++) {
    psu_dev = &galaxy100_psu_info.devices[i];

    pmdev = pmbus_device_open(psu_dev->bus, psu_dev->addr,
                              PMBUS_FLAG_FORCE_CLAIM);
    if (pmdev == NULL) {
      log_error("failed to open psu device %d-00%x: %s",
                psu_dev->bus, psu_dev->addr, strerror(errno));
      continue;
    }

    ret = pmbus_write_byte_data(pmdev, page,
                                PMBUS_OPERATION, PMBUS_OPERATION_OFF);
    if (ret != 0) {
      log_error("failed to turn off psu device %d-00%x via pmbus: %s",
                psu_dev->bus, psu_dev->addr, strerror(errno));
      /* fall through */
    }

    pmbus_device_close(pmdev);
  }

  return 0;
}

static void system_shutdown(const char *why)
{
  syslog(LOG_EMERG, "Shutting down:  %s", why);

  if (system_shutdown_sysfs() != 0) {
    system_shutdown_pmbus();
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
  return !galaxy100_fan_is_okey_sysfs(fan);
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

  return galaxy100_set_fan_sysfs(fan, unit);
}



/* Set up fan LEDs */

int write_fan_led(const int fan, const char *color) {
  return galaxy100_write_fan_led_sysfs(fan, color);
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

/*
 * Initialize path cache by writing 0-length string
 */
void init_path_cache()
{
  int i = 0;
  // Temp Sensor datastructure
  for (i = 0; i < BOARD_INFO_SIZE; i++)
  {
    if (galaxy100_board_info[i].alarm != NULL)
      snprintf(galaxy100_board_info[i].alarm->path_cache, PATH_CACHE_SIZE, "");
    if (galaxy100_board_info[i].critical != NULL)
      snprintf(galaxy100_board_info[i].critical->path_cache, PATH_CACHE_SIZE, "");
  }

  // This cache is not used currently, but will initialize anyway,
  // just in case something changes in the future.
  for (i = 0; i < FANTRAY_INFO_SIZE; i++)
  {
    snprintf(galaxy100_fantray_info[i].channel_info.path_cache,
            PATH_CACHE_SIZE, "");
  }

  return;
}

int main(int argc, char **argv) {
  /* Sensor values */
  int critical_temp;
  int alarm_temp;
  int old_temp = GALAXY100_RAISING_TEMP_HIGH;
  int raising_pwm;
  int falling_pwm;
  struct galaxy100_fantray_info_stu_sysfs *info;
  int fan_speed = fan_medium;
  int failed_speed = 0;

  int bad_reads = 0;
  int fan_failure = 0;
  int fan_speed_changes = 0;
  int old_speed;
  int count = 0;

  int fan_bad[FANS];
  int fan;

  int sysfs_rc = 0;
  int sysfs_value = 0;

  unsigned log_count = 0; // How many times have we logged our temps?
  int opt;
  int prev_fans_bad = 0;

  struct sigaction sa;

  // Initialize path cache
  init_path_cache();

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

  sysfs_rc = read_sysfs_int("/sys/bus/i2c/drivers/cmmcpld/13-003e/slotid",
                             &sysfs_value);
  if ((sysfs_rc == 0) &&(sysfs_value == 0x1)) {
    // on CMM1
    galaxy100_chanel_reinit_sysfs();
  }
  if(galaxy100_cmm_is_master() == 1) {
    for (fan = 0; fan < total_fans; fan++) {
      fan_bad[fan] = 0;
      write_fan_speed(fan + fan_offset, fan_speed);
      write_fan_led(fan + fan_offset, FAN_LED_BLUE);
    }
  }

  /* Start watchdog in manual mode */
  open_watchdog(0, 0);

  /* Set watchdog to persistent mode so timer expiry will happen independent
   * of this process's liveliness. */
  watchdog_disable_magic_close();

  /* We pet the watchdog here once, so that fan cpld will stop
   * forcing the fan speed to be 50% - this needs to be done before
   * we set fan speed to appropriate values for the first time
   * in the very first pass of the while loop below.
   */
  clear_fancpld_watchdog_timer();

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

    usleep(11000);
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
             "critical temp %d, alarm temp %d, fan speed %d, speed changes %d",
             critical_temp, alarm_temp, fan_speed, fan_speed_changes);
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
          if(info->present == 0) {
            not_present++;
            // Make sure that all fans on a FAB is marked as absent if that
            // fab itself is absent. Thus, one FAB down is equal to
            // three FANs down
            info->fan1.present = 0;
            info->fan2.present = 0;
            info->fan3.present = 0;
          }
          if(info->fan1.failed == 1 | info->fan1.present == 0)
            fan_failed++;
          if(info->fan2.failed == 1 | info->fan2.present == 0)
            fan_failed++;
          if(info->fan3.failed == 1 | info->fan3.present == 0)
            fan_failed++;
        }
        if(fan_failed >= 6)
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

    /*
     * Do the best effort to CMM_BMC_HEARTBEAT counter in every fancplds.
     * The watchdog timer expires in 500 secondds, if we don't clear the
     * counter
     */
    clear_fancpld_watchdog_timer();

    /* if everything is fine, restart the watchdog countdown. If this process
     * is terminated, the persistent watchdog setting will cause the system
     * to reboot after the watchdog timeout. */
    kick_watchdog();
  }
}
