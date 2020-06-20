/*
 * fand
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <dirent.h>
#include <assert.h>
#include <ctype.h>
#include <linux/limits.h>
#include <linux/version.h>
#include <facebook/wedge_eeprom.h>

#include <openbmc/watchdog.h>
#include <openbmc/libgpio.h>
#include <openbmc/misc-utils.h>

#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE 8
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof((_a)[0]))
#endif

#define HWMON_SYSFS_DIR "/sys/class/hwmon"

#define MAX_FANS 4

/* Sensor definitions */

#define INTERNAL_TEMPS(x) ((x) * 1000) // stored as C * 1000
#define EXTERNAL_TEMPS(x) ((x) / 1000)

/*
 * The sensor for the uServer CPU is not on the CPU itself, so it reads
 * a little low.  We are special casing this, but we should obviously
 * be thinking about a way to generalize these tweaks, and perhaps
 * the entire configuration.  JSON file?
 */

#define USERVER_TEMP_FUDGE INTERNAL_TEMPS(10)

#define BAD_TEMP INTERNAL_TEMPS(-60)

#define BAD_READ_THRESHOLD 4    /* How many times can reads fail */
#define FAN_FAILURE_THRESHOLD 4 /* How many times can a fan fail */
#define FAN_SHUTDOWN_THRESHOLD 20 /* How long fans can be failed before */
                                  /* we just shut down the whole thing. */

#define LM75_DIR "/sys/bus/i2c/drivers/lm75/"
#define PANTHER_PLUS_DIR "/sys/bus/i2c/drivers/fb_panther_plus/"

#define INTAKE_TEMP_DEVICE LM75_DIR "3-0048"
#define CHIP_TEMP_DEVICE LM75_DIR "3-0049"
#define EXHAUST_TEMP_DEVICE LM75_DIR "3-004a"
#define USERVER_TEMP_DEVICE PANTHER_PLUS_DIR "4-0040"

#define REV_ID_BITS  3
#define SLOT_ID_BITS 4

static const char *board_rev_gpios[REV_ID_BITS] = {
    "BOARD_REV_ID0",
    "BOARD_REV_ID1",
    "BOARD_REV_ID2",
};

static const char *slot_id_gpios[SLOT_ID_BITS] = {
    "BP_SLOT_ID0",
    "BP_SLOT_ID1",
    "BP_SLOT_ID2",
    "BP_SLOT_ID3",
};

#define PWM_TACHO_DIR_LEGACY "/sys/devices/platform/ast_pwm_tacho.0"

#define PWM_UNIT_MAX 96
#define FAN_READ_RPM_FORMAT "tacho%d_rpm"

#define FAN_LED_RED  GPIO_VALUE_LOW
#define FAN_LED_BLUE GPIO_VALUE_HIGH

static struct {
    const char *shadow;
    gpio_desc_t *gdesc;
} fan_led_gpios[MAX_FANS] = {
    {"FAN_LED_GPIOG5", NULL},
    {"FAN_LED_GPIOG6", NULL},
    {"FAN_LED_GPIOG7", NULL},
    {"FAN_LED_GPIOJ0", NULL},
};

#define REPORT_TEMP 720  /* Report temp every so many cycles */

/* Sensor limits and tuning parameters */

#define INTAKE_LIMIT INTERNAL_TEMPS(60)
#define SWITCH_LIMIT INTERNAL_TEMPS(80)
#define USERVER_LIMIT INTERNAL_TEMPS(90)

#define TEMP_TOP INTERNAL_TEMPS(70)
#define TEMP_BOTTOM INTERNAL_TEMPS(40)

/*
 * Toggling the fan constantly will wear it out (and annoy anyone who
 * can hear it), so we'll only turn down the fan after the temperature
 * has dipped a bit below the point at which we'd otherwise switch
 * things up.
 */

#define COOLDOWN_SLOP INTERNAL_TEMPS(6)

#define WEDGE_FAN_LOW 35
#define WEDGE_FAN_MEDIUM 50
#define WEDGE_FAN_HIGH 70
#define WEDGE_FAN_MAX 99

#define SIXPACK_FAN_LOW 35
#define SIXPACK_FAN_MEDIUM 55
#define SIXPACK_FAN_HIGH 75
#define SIXPACK_FAN_MAX 99

// Tacho offset between front and rear fans:
#define REAR_FAN_OFFSET 4
#define BACK_TO_BACK_FANS

struct fan_controller {
  const char *name;
  char pwm_tacho_dir[PATH_MAX];

  /*
   * Mapping physical to hardware addresses for fans;  it's different for
   * RPM measuring and PWM setting, naturally.  Doh.
   */
  const char *fan_pwm_map[MAX_FANS];
  const char *fan_tacho_map[2 * MAX_FANS]; /* 4 front and 4 rear */

  int (*init)(struct fan_controller *controller);
  void (*cleanup)(struct fan_controller *controller);
  int (*fan_rpm_read)(struct fan_controller *controller, int fan, int *rpm);
  int (*fan_pwm_write)(struct fan_controller *controller, int fan, int duty);
};

static int fan_speed_sysfs_read(struct fan_controller *ctrl, int fan, int *rpm);
static int fan_pwm_write_41(struct fan_controller *ctrl, int fan, int duty);
static int fan_controller_init_41(struct fan_controller *ctrl);

/*
 * "fan_controller_41" is intended to be used in kernel 4.1.x.
 */
static struct fan_controller fan_controller_41 = {
  .name = "fan_controller_41",
  .pwm_tacho_dir = {},

  /*
   * 1 pwm drives 2 fans (front and rear), so we have 4 pwm for total 8 fans.
   */
  .fan_pwm_map = {
    [0] = "pwm7",
    [1] = "pwm6",
    [2] = "pwm0",
    [3] = "pwm1",
  },

  /*
   * fan [0-3] are the 4 front fans and fan [4-7] are the corresponding
   * rear fans.
   */
  .fan_tacho_map = {
    [0] = "tacho3_rpm",
    [1] = "tacho2_rpm",
    [2] = "tacho0_rpm",
    [3] = "tacho1_rpm",
    [4] = "tacho7_rpm",
    [5] = "tacho6_rpm",
    [6] = "tacho4_rpm",
    [7] = "tacho5_rpm",
  },

  .init = fan_controller_init_41,
  .cleanup = NULL,
  .fan_rpm_read = fan_speed_sysfs_read,
  .fan_pwm_write = fan_pwm_write_41,
};

static int fan_pwm_write_5x(struct fan_controller *ctrl, int fan, int duty);
static int fan_controller_init_5x(struct fan_controller *ctrl);

/*
 * "fan_controller_5x" is intended to be used in kernel 5.x.
 */
static struct fan_controller fan_controller_5x = {
  .name = "fan_controller_5x",
  .pwm_tacho_dir = {},
  /*
   * 1 pwm drives 2 fans (front and rear) and that's why there are 4 pwm
   * for 8 fans.
   */
  .fan_pwm_map = {
    [0] = "pwm8",
    [1] = "pwm7",
    [2] = "pwm1",
    [3] = "pwm2",
  },

  /*
   * fan [0-3] are the 4 front fans and fan [4-7] are the corresponding
   * rear fans.
   */
  .fan_tacho_map = {
    [0] = "fan1_input",
    [1] = "fan2_input",
    [2] = "fan3_input",
    [3] = "fan4_input",
    [4] = "fan5_input",
    [5] = "fan6_input",
    [6] = "fan7_input",
    [7] = "fan8_input",
  },

  .init = fan_controller_init_5x,
  .cleanup = NULL,
  .fan_rpm_read = fan_speed_sysfs_read,
  .fan_pwm_write = fan_pwm_write_5x,
};

static struct fan_controller *active_fan_ctrl = &fan_controller_41;

/*
 * The measured RPM of the fans doesn't match linearly to the requested
 * rate.  In addition, there are coaxially mounted fans, so the rear fans
 * feed into the front fans.  The rear fans will run slower since they're
 * grabbing still air, and the front fants are getting an extra boost.
 *
 * We'd like to measure the fan RPM and compare it to the expected RPM
 * so that we can detect failed fans, so we have a table (derived from
 * hardware testing):
 */

struct rpm_to_pct_map {
  uint pct;
  uint rpm;
};

struct rpm_to_pct_map rpm_front_map[] = {{30, 6150},
                                         {35, 7208},
                                         {40, 8195},
                                         {45, 9133},
                                         {50, 10017},
                                         {55, 10847},
                                         {60, 11612},
                                         {65, 12342},
                                         {70, 13057},
                                         {75, 13717},
                                         {80, 14305},
                                         {85, 14869},
                                         {90, 15384},
                                         {95, 15871},
                                         {100, 16095}};
#define FRONT_MAP_SIZE (sizeof(rpm_front_map) / sizeof(struct rpm_to_pct_map))

struct rpm_to_pct_map rpm_rear_map[] = {{30, 3911},
                                        {35, 4760},
                                        {40, 5587},
                                        {45, 6434},
                                        {50, 7295},
                                        {55, 8187},
                                        {60, 9093},
                                        {65, 10008},
                                        {70, 10949},
                                        {75, 11883},
                                        {80, 12822},
                                        {85, 13726},
                                        {90, 14690},
                                        {95, 15516},
                                        {100, 15897}};
#define REAR_MAP_SIZE (sizeof(rpm_rear_map) / sizeof(struct rpm_to_pct_map))

/*
 * Mappings from temperatures recorded from sensors to fan speeds;
 * note that in some cases, we want to be able to look at offsets
 * from the CPU temperature margin rather than an absolute temperature,
 * so we use ints.
 */

struct temp_to_pct_map {
  int temp;
  unsigned speed;
};

#define FAN_FAILURE_OFFSET 30

int fan_low = WEDGE_FAN_LOW;
int fan_medium = WEDGE_FAN_MEDIUM;
int fan_high = WEDGE_FAN_HIGH;
int fan_max = WEDGE_FAN_MAX;
int total_fans = MAX_FANS;
int fan_offset = 0;

int temp_bottom = TEMP_BOTTOM;
int temp_top = TEMP_TOP;

int report_temp = REPORT_TEMP;
bool verbose = false;

#define LOG_VERBOSE(fmt, args...)    \
  do {                               \
    if (verbose)                     \
      syslog(LOG_INFO, fmt, ##args); \
  } while (0)

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

static int read_fan_speed(int fan, int *rpm)
{
  int rc;

  assert(active_fan_ctrl != NULL);
  assert(active_fan_ctrl->fan_rpm_read != NULL);

  rc = active_fan_ctrl->fan_rpm_read(active_fan_ctrl, fan, rpm);
  if (rc == 0) {
    LOG_VERBOSE("fan %d, current speed (rpm): %d", fan, *rpm);
  }

  return rc;
}

static int write_fan_speed(int fan, int percent)
{
  int rc;

  assert(active_fan_ctrl != NULL);
  assert(active_fan_ctrl->fan_pwm_write != NULL);

  rc = active_fan_ctrl->fan_pwm_write(active_fan_ctrl, fan, percent);
  if (rc == 0) {
    LOG_VERBOSE("fan %d, pwm is set to %d percent", fan, percent);
  }

  return rc;
}

static int fan_controller_init_41(struct fan_controller *controller)
{
  snprintf(controller->pwm_tacho_dir, sizeof(controller->pwm_tacho_dir),
           "%s", PWM_TACHO_DIR_LEGACY);
  return 0;
}

/*
 * Read an integer from the beginning of the file.
 * Return 0 for success, or errno on failures.
 */
static int device_read_integer(const char *pathname, int *value)
{
  FILE *fp;
  int rc, saved_errno;

  fp = fopen(pathname, "r");
  if (fp == NULL) {
    return errno;
  }

  rc = fscanf(fp, "%d", value);
  if (rc != 1) {
    saved_errno = errno;
    fclose(fp);  /* ignore errors */
    return saved_errno;
  }

  fclose(fp);
  return 0;
}

/*
 * Write an integer to the beginning of the given file.
 * Return 0 for success, or errno on failures.
 */
int device_write_integer(const char *pathname, int value) {
  FILE *fp;
  int rc, saved_errno;
  char data[64];

  fp = fopen(pathname, "w");
  if (!fp) {
    return errno;
  }

  snprintf(data, sizeof(data), "%d", value);
  rc = fputs(data, fp);
  if (rc < 0) {
    saved_errno = errno;
    fclose(fp); /* ignore errors */
    return saved_errno;
  }

  fclose(fp);
  return 0;
}

static int file_read_line(const char *pathname, char *buf, size_t size)
{
  int len;
  FILE *fp;

  fp = fopen(pathname, "r");
  if (fp == NULL) {
    return errno;
  }

  buf[0] = '\0';
  if (fgets(buf, size, fp) != NULL) {
    len = strlen(buf);

    /* delete the newline character */
    if (len > 0 && isspace(buf[len - 1])) {
      buf[len - 1] = '\0';
    }
  }

  fclose(fp);
  return 0;
}

static int hwmon_lookup_by_name(const char *name, char *buf, size_t size)
{
  DIR *dir;
  int rc = 0;
  int found = 0;
  struct dirent *dent;
  char hwmon_name[NAME_MAX];
  char hwmon_path[PATH_MAX];

  dir = opendir(HWMON_SYSFS_DIR);
  if (dir == NULL)
    return errno;

  while (1) {
    errno = 0;
    dent = readdir(dir);
    if (dent == NULL) { /* Error or end of directory */
      if (errno != 0) {
        rc = errno;
      }
      break;
    }

    if (!str_startswith(dent->d_name, "hwmon")) {
      continue;
    }

    snprintf(hwmon_path, sizeof(hwmon_path), "%s/%s/name",
             HWMON_SYSFS_DIR, dent->d_name);
    if (!path_isfile(hwmon_path)) {
      continue;
    }
    if (file_read_line(hwmon_path, hwmon_name, sizeof(hwmon_name)) != 0) {
      continue;
    }

    if (strcmp(hwmon_name, name) == 0) {
      snprintf(buf, size, "%s/%s", HWMON_SYSFS_DIR, dent->d_name);
      found = 1;
      break;
    }
  } /* while */

  closedir(dir);
  if (found == 0 && rc == 0) {
    rc = ENOENT;
  }
  return rc;
}

static int fan_controller_init_5x(struct fan_controller *controller)
{
  int rc;
  char hwmon_path[PATH_MAX];
  const char *name = "aspeed_pwm_tacho";

  rc = hwmon_lookup_by_name(name, hwmon_path, sizeof(hwmon_path));
  if (rc != 0) {
    syslog(LOG_WARNING, "failed to find hwmon node for %s: %s",
           name, strerror(rc));
    return rc;
  }

  snprintf(controller->pwm_tacho_dir, sizeof(controller->pwm_tacho_dir),
           "%s", hwmon_path);
  return 0;
}

int read_temp(const char *device, int *value) {
  int rc;
  char full_name[PATH_MAX];

  /* We set an impossible value to check for errors */
  *value = BAD_TEMP;
  snprintf(full_name, sizeof(full_name), "%s/temp1_input", device);

  rc = device_read_integer(full_name, value);

  /**
   * In the latest Linux kernel, lm75 temperature sysfs file is moved from
   * the device directory to device/hwmon/hwmon???. The ??? is the index of
   * hwmon device created in the system, which depends on the order when the
   * device is registered with hwmon.
   * For temperature reported by Facebook kernel module, i.e. fb_panther_plus
   * or cpld, we still keep the temperature sysfs directly under the device
   * directory.
   * This code will try to read from the sysfs directory first. If it fails
   * with ENOENT, the code will try to probe device/hwmon/hwmon???.
   */

  if (rc != 0) {
    DIR *dir = NULL;
    struct dirent *ent;

    snprintf(full_name, sizeof(full_name), "%s/hwmon", device);
    dir = opendir(full_name);
    if (dir != NULL) {
      while ((ent = readdir(dir)) != NULL) {
        if (strstr(ent->d_name, "hwmon")) {
          // found the correct 'hwmon??' directory
          snprintf(full_name, sizeof(full_name), "%s/hwmon/%s/temp1_input",
                   device, ent->d_name);
          rc = device_read_integer(full_name, value);
          break;
        }
      }
      closedir(dir);
    } /* if (dir != NULL) */
  } /* if (rc != 0) */

  if (rc != 0) {
    syslog(LOG_INFO, "failed to read temperature from %s: %s",
           full_name, strerror(rc));
  }

  return rc;
}

static int parse_gpio_ids(const char **shadows, size_t size, unsigned int *id)
{
    gpio_desc_t *gdesc;
    gpio_value_t value;
    unsigned int i, bit;

    assert(size < sizeof(*id) * BITS_PER_BYTE);

    *id = 0;
    for (i = 0; i < size; i++) {
        gdesc = gpio_open_by_shadow(shadows[i]);
        if (gdesc == NULL)
            return -1;

        if (gpio_get_value(gdesc, &value) != 0) {
            gpio_close(gdesc);
            return -1;
        }

        if (gpio_close(gdesc) != 0)
            return -1;

        bit = (value == GPIO_VALUE_HIGH) ? 1 : 0;
        *id |= (bit << i);
    }

    return 0;
}

static int read_ids(unsigned int *rev_id, unsigned int *board_id) {
  int status;

  status = parse_gpio_ids(board_rev_gpios, ARRAY_SIZE(board_rev_gpios), rev_id);
  if (status != 0) {
    syslog(LOG_INFO, "failed to read rev_id");
    return status;
  }

  status = parse_gpio_ids(slot_id_gpios, ARRAY_SIZE(slot_id_gpios), board_id);
  if (status != 0) {
    syslog(LOG_INFO, "failed to read board_id");
  }
  return status;
}

bool is_two_fan_board(bool verbose) {
  struct wedge_eeprom_st eeprom;
  /* Retrieve the board type from EEPROM */
  if (wedge_eeprom_parse(NULL, &eeprom) == 0) {
    /* able to parse EEPROM */
    if (verbose) {
      syslog(LOG_INFO, "board type is %s", eeprom.fbw_location);
    }
    /* only WEDGE is NOT two-fan board */
    return strncasecmp(eeprom.fbw_location, "wedge",
                       sizeof(eeprom.fbw_location));
  } else {
    int status;
    unsigned int board_id = 0;
    unsigned int rev_id = 0;
    /*
     * Could not parse EEPROM. Most likely, it is an old HW without EEPROM.
     * In this case, use board ID to distinguish if it is wedge or 6-pack.
     */
    status = read_ids(&rev_id, &board_id);
    if (verbose) {
      syslog(LOG_INFO, "rev ID %d, board id %d", rev_id, board_id);
    }
    if (status == 0 && board_id != 0xf) {
      return true;
    } else {
      return false;
    }
  }
}

/*
 * Read fan speed, used in kernel 4.1.x and 5.x.
 */
static int fan_speed_sysfs_read(struct fan_controller *controller, int fan,
                                int *rpm)
{
  int rc;
  char pathname[PATH_MAX];

  assert(fan >= 0 && fan < ARRAY_SIZE(controller->fan_tacho_map));

  snprintf(pathname, sizeof(pathname), "%s/%s",
           controller->pwm_tacho_dir, controller->fan_tacho_map[fan]);
  rc = device_read_integer(pathname, rpm);
  if (rc != 0) {
      syslog(LOG_WARNING, "failed to read from %s: %s",
             pathname, strerror(rc));
  }

  return rc;
}

/* Return fan speed as a percentage of maximum -- not necessarily linear. */

int fan_rpm_to_pct(const struct rpm_to_pct_map *table,
                   const int table_len,
                   int rpm) {
  int i;

  for (i = 0; i < table_len; i++) {
    if (table[i].rpm > rpm) {
      break;
    }
  }

  /*
   * If the fan RPM is lower than the lowest value in the table,
   * we may have a problem -- fans can only go so slow, and it might
   * have stopped.  In this case, we'll return an interpolated
   * percentage, as just returning zero is even more problematic.
   */

  if (i == 0) {
    return (rpm * table[i].pct) / table[i].rpm;
  } else if (i == table_len) { // Fell off the top?
    return table[i - 1].pct;
  }

  // Interpolate the right percentage value:

  int percent_diff = table[i].pct - table[i - 1].pct;
  int rpm_diff = table[i].rpm - table[i - 1].rpm;
  int fan_diff = table[i].rpm - rpm;

  return table[i].pct - (fan_diff * percent_diff / rpm_diff);
}

int fan_speed_okay(const int fan, const int speed, const int slop) {
  int front_rpm, rear_rpm;
  int front_pct, rear_pct;
  int okay;

  /*
   * The hardware fan numbers are different from the physical order
   * in the box, so we have to map them:
   */

  front_rpm = 0;
  read_fan_speed(fan, &front_rpm);
  front_pct = fan_rpm_to_pct(rpm_front_map, FRONT_MAP_SIZE, front_rpm);
#ifdef BACK_TO_BACK_FANS
  rear_rpm = 0;
  read_fan_speed(fan + REAR_FAN_OFFSET, &rear_rpm);
  rear_pct = fan_rpm_to_pct(rpm_rear_map, REAR_MAP_SIZE, rear_rpm);
#endif


  /*
   * If the fans are broken, the measured rate will be rather
   * different from the requested rate, and we can turn up the
   * rest of the fans to compensate.  The slop is the percentage
   * of error that we'll tolerate.
   *
   * XXX:  I suppose that we should only measure negative values;
   * running too fast isn't really a problem.
   */

#ifdef BACK_TO_BACK_FANS
  okay = (abs(front_pct - speed) * 100 / speed < slop &&
          abs(rear_pct - speed) * 100 / speed < slop);
#else
  okay = (abs(front_pct - speed) * 100 / speed < slop);
#endif

  if (!okay || verbose) {
    syslog(!okay ? LOG_WARNING : LOG_INFO,
#ifdef BACK_TO_BACK_FANS
           "fan %d rear %d (%d%%), front %d (%d%%), expected %d",
#else
           "fan %d %d RPM (%d%%), expected %d",
#endif
           fan,
#ifdef BACK_TO_BACK_FANS
           rear_rpm,
           rear_pct,
#endif
           front_rpm,
           front_pct,
           speed);
  }

  return okay;
}

/* Set fan speed as a percentage */

static int fan_pwm_write_41(struct fan_controller *controller, int fan,
                            int percent)
{
  int rc = 0;
  const char *pwm;
  char pathname[PATH_MAX];

  /*
   * The hardware fan numbers for pwm control are different from
   * both the physical order in the box, and the mapping for reading
   * the RPMs per fan, above.
   */
  pwm = controller->fan_pwm_map[fan % ARRAY_SIZE(controller->fan_pwm_map)];

  if (percent == 0) {
    snprintf(pathname, sizeof(pathname), "%s/%s_en",
             controller->pwm_tacho_dir, pwm);
    rc = device_write_integer(pathname, percent);
    if (rc != 0) {
      syslog(LOG_WARNING, "failed to write %d to %s: %s",
             percent, pathname, strerror(rc));
    }
  } else {
    int i, unit;
    struct {
      const char *key;
      int value;
    } pwm_kv_map[4];

    unit = percent * PWM_UNIT_MAX / 100;
    if (unit == PWM_UNIT_MAX)
      unit = 0;

    pwm_kv_map[0] = {"type", 0};
    pwm_kv_map[1] = {"rising", 0};
    pwm_kv_map[2] = {"falling", unit};
    pwm_kv_map[3] = {"en", 1};

    for (i = 0; i < ARRAY_SIZE(pwm_kv_map); i++) {
      snprintf(pathname, sizeof(pathname), "%s/%s_%s",
               controller->pwm_tacho_dir, pwm, pwm_kv_map[i].key);
      rc = device_write_integer(pathname, pwm_kv_map[i].value);
      if (rc != 0) {
        syslog(LOG_WARNING, "failed to write %d to %s: %s",
               pwm_kv_map[i].value, pathname, strerror(rc));
        break;
      }
    } /* for */
  }

  return rc;
}

static int fan_pwm_write_5x(struct fan_controller *controller, int fan,
                            int percent)
{
  int rc, pwm_value;
  char pathname[PATH_MAX];

  /*
   * Duty cycle from 0 to 100% with 1/256 resolution incremental.
   */
  pwm_value = percent * 256 / 100;

  fan %= ARRAY_SIZE(controller->fan_pwm_map);
  snprintf(pathname, sizeof(pathname), "%s/%s",
           controller->pwm_tacho_dir, controller->fan_pwm_map[fan]);

  rc = device_write_integer(pathname, pwm_value);
  if (rc != 0) {
    syslog(LOG_WARNING, "failed to write %d to %s: %s",
           pwm_value, pathname, strerror(rc));
  }

  return rc;
}

/* Set up fan LEDs */

static int update_fan_led(int fan, gpio_value_t color) {
    const char *shadow;

    assert(fan >= 0 && fan < MAX_FANS);

    if (fan_led_gpios[fan].gdesc == NULL) {
        shadow = fan_led_gpios[fan].shadow;
        fan_led_gpios[fan].gdesc = gpio_open_by_shadow(shadow);
        if (fan_led_gpios[fan].gdesc == NULL)
            return -1;
    }

    return gpio_set_value(fan_led_gpios[fan].gdesc, color);
}

static int set_gpio_state(const char *shadow, gpio_value_t value)
{
    int error;
    gpio_desc_t *gdesc;

    gdesc = gpio_open_by_shadow(shadow);
    if (gdesc == NULL)
        return -1;

    error = gpio_set_direction(gdesc, GPIO_DIRECTION_OUT);
    if (error != 0)
        goto exit;

    error = gpio_set_value(gdesc, value);

exit:
    if (gpio_close(gdesc) != 0)
        return -1;
    return error;
}

int server_shutdown(const char *why) {
  int fan;
  for (fan = 0; fan < total_fans; fan++) {
    write_fan_speed(fan + fan_offset, fan_max);
  }

  syslog(LOG_EMERG, "Shutting down:  %s", why);
  if (set_gpio_state("BMC_PWR_BTN_OUT_N", GPIO_VALUE_LOW) != 0)
      syslog(LOG_EMERG, "failed to shutdown userver: %s", strerror(errno));

  /*
   * Putting T2 in reset generates a non-maskable interrupt to uS,
   * the kernel running on uS might panic depending on its version.
   * sleep 5s here to make sure uS is completely down.
   */
  sleep(5);

  if (set_gpio_state("T2_POWER_UP", GPIO_VALUE_HIGH) != 0) {
    /*
     * We're here because something has gone badly wrong.  If we
     * didn't manage to shut down the T2, cut power to the whole box,
     * using the PMBus OPERATION register.  This will require a power
     * cycle (removal of both power inputs) to recover.
     *
     * Note: When BMC drives T2_POWER_UP to high, MAX16050 will be disabled
     * by its EN pin. Then output signal EN2 on MAX16050 will shut down power
     * VDD3_3V which provides power to I2C_2464_S08, I2C_PCF8574 and pull-up
     * resistors on this I2C bus 6. The FRU EEPROM is not accessible anymore.
     */
    syslog(LOG_EMERG, "T2 power off failed;  turning off via ADM1278");
    system("rmmod adm1275");
    system("i2cset -y 12 0x10 0x01 00");
  }

  /*
   * We have to stop the watchdog, or the system will be automatically
   * rebooted some seconds after fand exits (and stops kicking the
   * watchdog).
   */

  stop_watchdog();

  sleep(2);
  exit(2);
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

  int intake_temp;
  int exhaust_temp;
  int switch_temp;
  int userver_temp;

  int fan_speed = fan_high;
  int bad_reads = 0;
  int fan_failure = 0;
  int fan_speed_changes = 0;
  int old_speed;

  int fan_bad[MAX_FANS];
  int fan;

  unsigned log_count = 0; // How many times have we logged our temps?
  int opt;
  int prev_fans_bad = 0;

  struct sigaction sa;

  k_version_t k_version;

  sa.sa_handler = fand_interrupt;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);

  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGUSR1, &sa, NULL);

  // Start writing to syslog as early as possible for diag purposes.

  openlog("fand", LOG_CONS, LOG_DAEMON);

  if (is_two_fan_board(false)) {
    /* Alternate, two fan configuration */
    total_fans = 2;
    fan_offset = 2; /* fan 3 is the first */

    fan_low = SIXPACK_FAN_LOW;
    fan_medium = SIXPACK_FAN_MEDIUM;
    fan_high = SIXPACK_FAN_HIGH;
    fan_max = SIXPACK_FAN_MAX;
    fan_speed = fan_high;
  }

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
  LOG_VERBOSE("temperature settings: bottom=%d, top=%d",
              temp_bottom, temp_top);

  if (fan_low > fan_medium || fan_low > fan_high || fan_medium > fan_high) {
    fprintf(stderr,
            "fan RPMs not strictly increasing "
            "-- %d, %d, %d, starting anyway\n",
            fan_low,
            fan_medium,
            fan_high);
  }
  LOG_VERBOSE("fan speed settings: low=%d, medium=%d, high=%d",
              fan_low, fan_medium, fan_high);

  /*
   * Determine which fan controller to be used based on kernel version.
   */
  k_version = get_kernel_version();
  if (k_version > KERNEL_VERSION(4, 1, 51)) {
    active_fan_ctrl = &fan_controller_5x;
  }
  LOG_VERBOSE("Setting up %s", active_fan_ctrl->name);
  if (active_fan_ctrl->init != NULL) {
    if (active_fan_ctrl->init(active_fan_ctrl) != 0) {
      syslog(LOG_CRIT, "unable to initialize %s!", active_fan_ctrl->name);
      /*
       * fall through regardless of init results, because we wish fand
       * can still read temp sensors and shutdown the chassis in case temp
       * is too high.
       */
    }
  }

  daemon(1, 0);

  LOG_VERBOSE("Starting up;  system should have %d fans.", total_fans);

  for (fan = 0; fan < total_fans; fan++) {
    fan_bad[fan] = 0;
    write_fan_speed(fan + fan_offset, fan_speed);
    update_fan_led(fan + fan_offset, FAN_LED_BLUE);
  }

  /* Start watchdog in manual mode */
  open_watchdog(0, 0);

  /* Set watchdog to persistent mode so timer expiry will happen independent
   * of this process's liveliness. */
  watchdog_disable_magic_close();

  sleep(5);  /* Give the fans time to come up to speed */

  while (1) {
    int max_temp;
    old_speed = fan_speed;

    LOG_VERBOSE("checking system temperature..");
    /* Read sensors */
    read_temp(INTAKE_TEMP_DEVICE, &intake_temp);
    read_temp(EXHAUST_TEMP_DEVICE, &exhaust_temp);
    read_temp(CHIP_TEMP_DEVICE, &switch_temp);
    read_temp(USERVER_TEMP_DEVICE, &userver_temp);

    /*
     * uServer can be powered down, but all of the rest of the sensors
     * should be readable at any time.
     */

    /* TODO(vineelak) : Add userver_temp too , in case we fail to read temp */
    if ((intake_temp == BAD_TEMP || exhaust_temp == BAD_TEMP ||
         switch_temp == BAD_TEMP)) {
      bad_reads++;
    }

    if (bad_reads > BAD_READ_THRESHOLD) {
      server_shutdown("Some sensors couldn't be read");
    }

    if (log_count++ % report_temp == 0) {
      syslog(LOG_DEBUG,
             "Temp intake %d, switch %d, "
             " userver %d, exhaust %d, "
             "fan speed %d, speed changes %d",
             intake_temp,
             switch_temp,
             userver_temp,
             exhaust_temp,
             fan_speed,
             fan_speed_changes);
    }

    /* Protection heuristics */

    if (intake_temp > INTAKE_LIMIT) {
      server_shutdown("Intake temp limit reached");
    }

    if (switch_temp > SWITCH_LIMIT) {
      server_shutdown("T2 temp limit reached");
    }

    if (userver_temp + USERVER_TEMP_FUDGE > USERVER_LIMIT) {
      syslog(LOG_DEBUG,
             "Temp intake %d, switch %d, "
             " userver %d, exhaust %d, "
             "fan speed %d, speed changes %d",
             intake_temp,
             switch_temp,
             userver_temp,
             exhaust_temp,
             fan_speed,
             fan_speed_changes);

      server_shutdown("uServer temp limit reached");
    }

    /*
     * Calculate change needed -- we should eventually
     * do something more sophisticated, like PID.
     *
     * We should use the intake temperature to adjust this
     * as well.
     */

    /* Other systems use a simpler built-in table to determine fan speed. */

    if (switch_temp > userver_temp + USERVER_TEMP_FUDGE) {
      max_temp = switch_temp;
    } else {
      max_temp = userver_temp + USERVER_TEMP_FUDGE;
    }

    LOG_VERBOSE("checking/adjusting fan speed..");
    /*
     * If recovering from a fan problem, spin down fans gradually in case
     * temperatures are still high. Gradual spin down also reduces wear on
     * the fans.
     */
    if (fan_speed == fan_max) {
      if (fan_failure == 0) {
        fan_speed = fan_high;
      }
    } else if (fan_speed == fan_high) {
      if (max_temp + COOLDOWN_SLOP < temp_top) {
        fan_speed = fan_medium;
      }
    } else if (fan_speed == fan_medium) {
      if (max_temp > temp_top) {
        fan_speed = fan_high;
      } else if (max_temp + COOLDOWN_SLOP < temp_bottom) {
        fan_speed = fan_low;
      }
    } else {/* low */
      if (max_temp > temp_bottom) {
        fan_speed = fan_medium;
      }
    }

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

    /* Check fan RPMs */

    for (fan = 0; fan < total_fans; fan++) {
      /*
       * Make sure that we're within some percentage
       * of the requested speed.
       */
      if (fan_speed_okay(fan + fan_offset, fan_speed, FAN_FAILURE_OFFSET)) {
        if (fan_bad[fan] > FAN_FAILURE_THRESHOLD) {
          update_fan_led(fan + fan_offset, FAN_LED_BLUE);
          syslog(LOG_CRIT,
                 "Fan %d has recovered",
                 fan);
        }
        fan_bad[fan] = 0;
      } else {
        fan_bad[fan]++;
      }
    }

    fan_failure = 0;
    for (fan = 0; fan < total_fans; fan++) {
      if (fan_bad[fan] > FAN_FAILURE_THRESHOLD) {
        fan_failure++;
        update_fan_led(fan + fan_offset, FAN_LED_RED);
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

      fan_speed = fan_max;
      for (fan = 0; fan < total_fans; fan++) {
        write_fan_speed(fan + fan_offset, fan_speed);
      }

      /*
       * On Wedge, we want to shut down everything if none of the fans
       * are visible, since there isn't automatic protection to shut
       * off the server or switch chip.  On other platforms, the CPUs
       * generating the heat will automatically turn off, so this is
       * unnecessary.
       */

      if (fan_failure == total_fans) {
        int count = 0;
        for (fan = 0; fan < total_fans; fan++) {
          if (fan_bad[fan] > FAN_SHUTDOWN_THRESHOLD)
            count++;
        }
        if (count == total_fans) {
          server_shutdown("all fans are bad for more than 12 cycles");
        }
      }

      /*
       * Fans can be hot swapped and replaced; in which case the fan daemon
       * will automatically detect the new fan and (assuming the new fan isn't
       * itself faulty), automatically readjust the speeds for all fans down
       * to a more suitable rpm. The fan daemon does not need to be restarted.
       */
    }

    /* Suppress multiple warnings for similar number of fan failures. */
    prev_fans_bad = fan_failure;

    /* if everything is fine, restart the watchdog countdown. If this process
     * is terminated, the persistent watchdog setting will cause the system
     * to reboot after the watchdog timeout. */
    LOG_VERBOSE("kicking watchdog");
    kick_watchdog();
  }

  if (active_fan_ctrl->cleanup != NULL) {
    active_fan_ctrl->cleanup(active_fan_ctrl);
  }
  return 0;
}
