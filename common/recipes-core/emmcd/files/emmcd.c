/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <syslog.h>
#include <string.h>
#include <pthread.h>
#include <sys/file.h>
#include <sys/types.h>

#include <openbmc/log.h>
#include <openbmc/misc-utils.h>

#define DAEMON_NAME          "emmcd"

#define EMMC_DEV_DIR         "/sys/bus/mmc/devices/mmc0:0001"
#define EMMC_PRE_EOL_FILE    EMMC_DEV_DIR "/pre_eol_info"
#define EMMC_LIFE_TIME_FILE  EMMC_DEV_DIR "/life_time"

/*
 * eMMC Pre EOL definitions.
 * Refer to JEDEC Standard, EXT_CSD[267] PRE_EOL_INFO for details.
 */
#define EMMC_EOL_TABLE                          \
   EOL_ENTRY(EMMC_EOL_NONE,    0, "undefined"), \
   EOL_ENTRY(EMMC_EOL_NORMAL,  1, "normal"),    \
   EOL_ENTRY(EMMC_EOL_WARNING, 2, "warning"),   \
   EOL_ENTRY(EMMC_EOL_URGENT,  3, "urgent")

#define EOL_ENTRY(_name, _val, _desc) _name = _val
enum {
  EMMC_EOL_TABLE,
  EMMC_EOL_TYPE_MAX,
};
#undef EOL_ENTRY
#define EOL_ENTRY(_name, _val, _desc) [_val] = _desc
static const char *emmc_eol_str[EMMC_EOL_TYPE_MAX] = {
  EMMC_EOL_TABLE,
};
#undef EOL_ENTRY

/*
 * eMMC estimated device life time definitions.
 * Refer to JEDEC Standard, EXT_CSD[269:268] DEVICE_LIFE_TIME_EST_TYP_A|B
 * for details.
 */
#define EMMC_LIFE_TIME_TABLE                                                \
  LIFE_TIME_ENTRY(EMMC_LIFE_TIME_NONE,      0,  "undefined"),               \
  LIFE_TIME_ENTRY(EMMC_LIFE_TIME_10PCT,     1,  "0%-10% life time used"),   \
  LIFE_TIME_ENTRY(EMMC_LIFE_TIME_20PCT,     2,  "10%-20% life time used"),  \
  LIFE_TIME_ENTRY(EMMC_LIFE_TIME_30PCT,     3,  "20%-30% life time used"),  \
  LIFE_TIME_ENTRY(EMMC_LIFE_TIME_40PCT,     4,  "30%-40% life time used"),  \
  LIFE_TIME_ENTRY(EMMC_LIFE_TIME_50PCT,     5,  "40%-50% life time used"),  \
  LIFE_TIME_ENTRY(EMMC_LIFE_TIME_60PCT,     6,  "50%-60% life time used"),  \
  LIFE_TIME_ENTRY(EMMC_LIFE_TIME_70PCT,     7,  "60%-70% life time used"),  \
  LIFE_TIME_ENTRY(EMMC_LIFE_TIME_80PCT,     8,  "70%-80% life time used"),  \
  LIFE_TIME_ENTRY(EMMC_LIFE_TIME_90PCT,     9,  "80%-90% life time used"),  \
  LIFE_TIME_ENTRY(EMMC_LIFE_TIME_100PCT,    10, "90%-100% life time used"), \
  LIFE_TIME_ENTRY(EMMC_LIFE_TIME_REACH_EOL, 11, "exceeded max life time")

#define LIFE_TIME_ENTRY(_name, _val, _desc) _name = _val
enum {
  EMMC_LIFE_TIME_TABLE,
  EMMC_LIFE_TIME_TYPE_MAX,
};
#undef LIFE_TIME_ENTRY
#define LIFE_TIME_ENTRY(_name, _val, _desc) [_val] = _desc
static const char *emmc_life_time_str[EMMC_LIFE_TIME_TYPE_MAX] = {
  EMMC_LIFE_TIME_TABLE,
};
#undef LIFE_TIME_ENTRY

#ifndef fallthrough
#define fallthrough do {} while (0)
#endif /* fallthrough */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof((_a)[0]))
#endif /* ARRAY_SIZE */

static int verbose_logging = 0;
#define EMMCD_VERBOSE(fmt, args...) \
  do {                              \
    if (verbose_logging)            \
      OBMC_INFO(fmt, ##args);       \
  } while (0)

static struct {
  unsigned int check_interval;
} emmcd_config = {
  .check_interval = 36000,  /* 10 hours */
};

static int read_file_helper(int fd, void *buf, size_t size)
{
  int ret;

  if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
    OBMC_ERROR(errno, "unable to update file (fd=%d) offset", fd);
    return -1;
  }

  ret = read(fd, buf, size);
  if (ret < 0) {
    OBMC_ERROR(errno, "failed to read file (fd=%d)", fd);
    return -1;
  } else if (ret == 0) {
    OBMC_WARN("file (fd=%d) is empty!", fd);
    errno = ENODATA;
    return -1;
  }

  return ret;
}

static int emmc_read_eol(int fd, unsigned long *eol)
{
  int ret;
  char buf[64];
  char *endptr;
  unsigned long val;

  ret = read_file_helper(fd, buf, sizeof(buf) - 1);
  if (ret < 0) {
    return -1;
  }
  buf[ret] = '\0';

  val = strtoul(buf, &endptr, 0);
  if (endptr == buf) {
    OBMC_WARN("Unexpected format in file (fd=%d): %s", fd, buf);
    return -1;
  }

  *eol = val;
  return 0;
}

static int emmc_read_life_time(int fd,
                               unsigned long *life_time_a,
                               unsigned long *life_time_b)
{
  int ret;
  char buf[64];
  char *nextptr, *endptr;
  unsigned long val_a, val_b;

  ret = read_file_helper(fd, buf, sizeof(buf) - 1);
  if (ret < 0) {
    return -1;
  }
  buf[ret] = '\0';

  val_a = strtoul(buf, &nextptr, 0);
  if (nextptr == buf || *nextptr == '\0') {
    OBMC_WARN("Unexpected format in file (fd=%d): %s", fd, buf);
    return -1;
  }

  nextptr++;
  val_b = strtoul(nextptr, &endptr, 0);
  if (endptr == nextptr) {
    OBMC_WARN("Unexpected format in file (fd=%d): %s", fd, buf);
    return -1;
  }

  *life_time_a = val_a;
  *life_time_b = val_b;
  return 0;
}

static const char* eol_val_to_str(unsigned long eol)
{
  if ((eol < ARRAY_SIZE(emmc_eol_str)) &&
      (emmc_eol_str[eol] != NULL)) {
    return emmc_eol_str[eol];
  }

  return "reserved";
}

static const char* life_time_val_to_str(unsigned long life_time)
{
  if ((life_time < ARRAY_SIZE(emmc_life_time_str)) &&
      (emmc_life_time_str[life_time] != NULL)) {
    return emmc_life_time_str[life_time];
  }

  return "reserved";
}

static int parse_emmc_life_time(unsigned long eol,
                                unsigned long life_time_a,
                                unsigned long life_time_b)
{
  if ((eol > EMMC_EOL_NORMAL) ||
      (life_time_a > EMMC_LIFE_TIME_50PCT) ||
      (life_time_b > EMMC_LIFE_TIME_50PCT)) {
    OBMC_CRIT("eMMC ALERT: eol=%s, life_time_a=(%s), life_time_b=(%s)",
              eol_val_to_str(eol), life_time_val_to_str(life_time_a),
              life_time_val_to_str(life_time_b));
  } else if ((life_time_a > EMMC_LIFE_TIME_20PCT) ||
             (life_time_b > EMMC_LIFE_TIME_20PCT)) {
    OBMC_WARN("eMMC WARNING: eol=%s, life_time_a=(%s), life_time_b=(%s)",
              eol_val_to_str(eol), life_time_val_to_str(life_time_a),
              life_time_val_to_str(life_time_b));

    /* Increase check frequency to every 1 hour */
    emmcd_config.check_interval = 3600;
  } else {
    EMMCD_VERBOSE("eMMC health: eol=%s, life_time_a=(%s), life_time_b=(%s)",
                  eol_val_to_str(eol), life_time_val_to_str(life_time_a),
                  life_time_val_to_str(life_time_b));
  }

  return 0;
}

static void *emmc_life_time_monitor(void* unused)
{
  int eol_fd, life_time_fd;

  EMMCD_VERBOSE("life_time_monitor thread starts running");

  /*
   * open files.
   */
  eol_fd = open(EMMC_PRE_EOL_FILE, O_RDONLY);
  if (eol_fd < 0) {
    OBMC_ERROR(errno, "failed to open %s", EMMC_PRE_EOL_FILE);
    fallthrough;
  }
  life_time_fd = open(EMMC_LIFE_TIME_FILE, O_RDONLY);
  if (life_time_fd < 0) {
    OBMC_ERROR(errno, "failed to open %s", EMMC_LIFE_TIME_FILE);
    fallthrough;
  }

  /*
   * Terminate the thread if neither file can be opened.
   */
  if ((eol_fd < 0) && (life_time_fd < 0)) {
    return NULL;
  }

  while (1) {
    int error = 0;
    unsigned long eol = EMMC_EOL_TYPE_MAX;
    unsigned long life_time_a = EMMC_LIFE_TIME_TYPE_MAX;
    unsigned long life_time_b = EMMC_LIFE_TIME_TYPE_MAX;

    /*
     * Terminate the thread if none of the data can be fetched.
     */
    if (emmc_read_eol(eol_fd, &eol) < 0) {
      error++;
    }
    if (emmc_read_life_time(life_time_fd, &life_time_a, &life_time_b) < 0) {
      error++;
    }
    if (error == 2) {
      break;
    }

    /*
     * Termiate the thread if none of these states are supported by the
     * eMMC device.
     */
    if ((eol == EMMC_EOL_NONE) &&
        (life_time_a == EMMC_LIFE_TIME_NONE) &&
        (life_time_b == EMMC_LIFE_TIME_NONE)) {
      OBMC_WARN("device life time is not reported by eMMC!");
      break;
    }

    parse_emmc_life_time(eol, life_time_a, life_time_b);

    sleep(emmcd_config.check_interval);
  }

  if (life_time_fd >= 0)
    close(life_time_fd);
  if (eol_fd >= 0)
    close(eol_fd);
  return NULL;
}

static void dump_usage(const char *prog_name)
{
  int i;
  struct {
    const char *opt;
    const char *desc;
  } options[] = {
    {"-h|--help", "print this help message"},
    {"-v|--verbose", "enable verbose logging"},
    {NULL, NULL},
  };

  printf("Usage: %s [options]\n", prog_name);
  for (i = 0; options[i].opt != NULL; i++) {
    printf("    %-18s - %s\n", options[i].opt, options[i].desc);
  }
}

int main (int argc, char * const argv[])
{
  int i, ret;
  struct option long_opts[] = {
    {"help",    no_argument, NULL, 'h'},
    {"verbose", no_argument, NULL, 'v'},
    {NULL,      0,           NULL, 0},
  };
  struct {
    const char *name;
    pthread_t tid;
    void* (*func)(void *args);
    void *args;
  } monitor_threads[] = {
    {
      .name = "life_time_monitor",
      .func = emmc_life_time_monitor,
      .args = NULL,
    },

    /* This is the last entry */
    {
      .name = NULL,
      .func = NULL,
      .args = NULL,
    },
  };

  while (1) {
    int opt_index = 0;

    ret = getopt_long(argc, argv, "hv", long_opts, &opt_index);
    if (ret == -1)
      break; /* end of arguments */

    switch (ret) {
    case 'h':
      dump_usage(argv[0]);
      return 0;

    case 'v':
      verbose_logging = 1;
      break;

    default:
      return -1;
    }
  } /* while */

  /*
   * Make sure only 1 instance of emmcd is running.
   */
  if (single_instance_lock(DAEMON_NAME) < 0) {
    if (errno == EWOULDBLOCK) {
      syslog(LOG_ERR, "Another %s instance is running. Exiting..\n",
             DAEMON_NAME);
    } else {
      syslog(LOG_ERR, "unable to ensure single %s instance: %s\n",
             DAEMON_NAME, strerror(errno));
    }

    return -1;
  }

  /*
   * Initialize logging utilities.
   */
  obmc_log_init(DAEMON_NAME, LOG_INFO, 0);
  obmc_log_set_syslog(LOG_CONS, LOG_DAEMON);
  obmc_log_unset_std_stream();

  OBMC_INFO("%s started", DAEMON_NAME);

  /*
   * Creating monitoring threads.
   */
  for (i = 0; monitor_threads[i].name != NULL; i++) {
    OBMC_INFO("creating %s thread", monitor_threads[i].name);
    ret = pthread_create(&monitor_threads[i].tid, NULL,
                         monitor_threads[i].func, monitor_threads[i].args);
    if (ret != 0) {
      OBMC_ERROR(ret, "failed to create %s thread", monitor_threads[i].name);
      break;
    }
  }

  while (--i >= 0) {
    ret = pthread_join(monitor_threads[i].tid, NULL);
    if (ret != 0) {
      OBMC_ERROR(ret, "failed to join %s thread", monitor_threads[i].name);
    }
  }

  return 0;
}
