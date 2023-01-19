/*
 * sensord
 *
 * Copyright 2022-present Facebook. All Rights Reserved.
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
#include <syslog.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/file.h>
#include <pthread.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/misc-utils.h>
#include <openbmc/pal.h>
#include <openbmc/pal_def.h>
#include <openbmc/pal_common.h>
#include "gpiod.h"


#define TOUCH(path) \
{\
  int fd = creat(path, 0644);\
  if (fd) close(fd);\
}

extern struct gpiopoll_config gpios_list[];
extern const uint8_t gpios_cnt;

static int g_uart_switch_count = 0;
static long int g_reset_sec = 0;
static long int g_power_on_sec = 0;
static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static gpio_value_t g_server_power_status = GPIO_VALUE_INVALID;
static bool g_cpu_pwrgd_trig = false;

enum {
  IOEX_GPIO_STANDBY,
  IOEX_GPIO_POWER_ON,
  IOEX_GPIO_POWER_ON_3S,
};

/* For GPIOs on IO expander don't have edge attr. */
struct gpiopoll_ioex_config {
  char shadow[32];
  char desc[32];
  char pwr_st;
  gpio_edge_t edge;
  gpio_value_t last;
  void (*init)(char* desc, gpio_value_t value);
  void (*handler)(char* desc, gpio_value_t value);
};

struct delayed_log {
  useconds_t usec;
  char msg[1024];
};

bool server_power_check(uint8_t power_on_time) {
  uint8_t status = 0;

  pal_get_server_power(FRU_MB, &status);
  if (status != SERVER_POWER_ON || g_power_on_sec < power_on_time) {
    return false;
  }
  return true;
}

// Thread for delay event
static void*
delay_log(void *arg) {
  struct delayed_log* log = (struct delayed_log*)arg;

  pthread_detach(pthread_self());
  if (log) {
    usleep(log->usec);
    syslog(LOG_CRIT, "Record time delay %dms, %s",log->usec/1000, log->msg);
    free(log);
  }

  pthread_exit(NULL);
}

static
void log_gpio_change(uint8_t fru,
                     gpiopoll_pin_t *desc,
                     gpio_value_t value,
                     useconds_t log_delay,
                     const char* str) {
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);
  struct delayed_log *log = (struct delayed_log *)malloc(sizeof(struct delayed_log));
  pthread_t tid_delay_log;
  assert(cfg);

  if(!log) {
    syslog(LOG_CRIT, "%s: %s - %s\n", value ? "DEASSERT": "ASSERT", cfg->description, cfg->shadow);
    return;
  }

  memset(log->msg, 0, sizeof(log->msg));
  if(str == NULL) {
    snprintf(log->msg, sizeof(log->msg), "%s: %s - %s\n",
             value ? "DEASSERT" : "ASSERT", cfg->description, cfg->shadow);
  } else {
    snprintf(log->msg, sizeof(log->msg), "FRU: %d %s: %s\n",
             fru, str, value ? "Deassertion": "Assertion");
  }

  if (log_delay == 0) {
    syslog(LOG_CRIT, "%s", log->msg);
  } else {
    log->usec = log_delay;
    if (pthread_create(&tid_delay_log, NULL, delay_log, (void *)log)) {
      free(log);
      log = NULL;
    }
  }
}

void
gpio_present_init (gpiopoll_pin_t *desc, gpio_value_t value)  {
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);

  if (value == GPIO_VALUE_HIGH)
    syslog(LOG_CRIT, "FRU: %d , %s - %s not present.", FRU_MB, cfg->description, cfg->shadow);
}

void
gpio_present_handler (gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);

  syslog(LOG_CRIT, "FRU: %d , %s - %s %s.", FRU_MB, cfg->description, cfg->shadow,
          (curr == GPIO_VALUE_HIGH) ? "not present": "present");
}

void
cpu_vr_hot_init(gpiopoll_pin_t *desc, gpio_value_t value) {
  if(value == GPIO_VALUE_LOW )
    log_gpio_change(FRU_MB, desc, value, 0, NULL);

  return;
}

void
cpu_skt_init(gpiopoll_pin_t *desc, gpio_value_t value) {
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);
  int cpu_id = strcmp(cfg->shadow, FM_CPU0_SKTOCC) == 0 ? 0 : 1;
  char key[MAX_KEY_LEN] = {0};
  char kvalue[MAX_VALUE_LEN] = {0};

  gpio_value_t prev_value = GPIO_VALUE_INVALID;
  snprintf(key, sizeof(key), "cpu%d_skt_status", cpu_id);
  if (kv_get(key, kvalue, NULL, KV_FPERSIST) == 0) {
    prev_value = atoi( kvalue);
  }
  snprintf(kvalue, sizeof(kvalue), "%d", value);
  kv_set(key, kvalue, 0, KV_FPERSIST);
  if(value != prev_value ) {
    log_gpio_change(FRU_MB, desc, value, 0, NULL);
  }
}

void
cpu0_pvccin_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  const char* str = "CPU0 PVCCIN VR HOT Warning";
  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, str);
}

void
cpu1_pvccin_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  const char* str = "CPU1 PVCCIN VR HOT Warning";
  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, str);
}

void
cpu0_pvccd_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  const char* str = "CPU0 PVCCD VR HOT Warning";
  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, str);
}

void
cpu1_pvccd_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  const char* str = "CPU1 PVCCD VR HOT Warning";
  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, str);
}

void
sml1_pmbus_alert_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  const char* str = "HSC OC Warning";
  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, str);
}

void
oc_detect_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  const char* str = "HSC Surge Current Warning";
  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, str);
}

void
uv_detect_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  char* str = "HSC Under Voltage Warning";
  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, str);
}

static inline long int
reset_timer(long int *val) {
  pthread_mutex_lock(&timer_mutex);
  *val = 0;
  pthread_mutex_unlock(&timer_mutex);
  return *val;
}

static inline long int
increase_timer(long int *val) {
  pthread_mutex_lock(&timer_mutex);
  (*val)++;
  pthread_mutex_unlock(&timer_mutex);
  return *val;
}

static inline long int
decrease_timer(long int *val) {
  pthread_mutex_lock(&timer_mutex);
  (*val)--;
  pthread_mutex_unlock(&timer_mutex);
  return *val;
}

//PCH Thermtrip Handler
void
pch_thermtrip_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  uint8_t status = 0;
  gpio_value_t value;
  if (!sgpio_valid_check()) return;

  value = gpio_get_value_by_shadow(RST_PLTRST_N);
  if(value < 0 || value == GPIO_VALUE_LOW) {
    return;
  }

  pal_get_server_power(FRU_MB, &status);
  if (status != SERVER_POWER_ON)
    return;
  // Filter false positives during reset.
  if (g_reset_sec < 10)
    return;

  log_gpio_change(FRU_MB, desc, curr, 0, NULL);
}

void
cpu_prochot_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  char cmd[128] = {0};
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);

  if (!sgpio_valid_check()) return;
  assert(cfg);

  if(!server_power_check(3))
    return;

  log_gpio_change(FRU_MB, desc, curr, 0, NULL);

  //LCD debug card critical SEL support
  strcat(cmd, "CPU FPH");
  if (curr) {
    strcat(cmd, " DEASSERT");
  } else {
    strcat(cmd, " ASSERT");
  }
  pal_add_cri_sel(cmd);
}

static
void thermtrip_add_cri_sel(const char *shadow_name, gpio_value_t curr)
{
  char cmd[128], log_name[16], log_descript[16] = "ASSERT\0";

  if (strcmp(shadow_name, FM_CPU0_THERMTRIP_N) == 0)
    snprintf(log_name, sizeof(log_name), "CPU0");
  else if (strcmp(shadow_name, FM_CPU1_THERMTRIP_N) == 0)
    snprintf(log_name, sizeof(log_name), "CPU1");

  if (curr == GPIO_VALUE_HIGH)
    snprintf(log_descript, sizeof(log_descript), "DEASSERT");

  snprintf(cmd, sizeof(cmd), "FRU: %d %s thermtrip %s", FRU_MB, log_name, log_descript);
  pal_add_cri_sel(cmd);
}

void
cpu_thermtrip_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);

  if (!sgpio_valid_check())
    return;

  if (g_server_power_status != GPIO_VALUE_HIGH)
    return;

  thermtrip_add_cri_sel(cfg->shadow, curr);
  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, NULL);
}

void
cpu_error_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  if (g_server_power_status != GPIO_VALUE_HIGH)
    return;

  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, NULL);
}

void
mem_thermtrip_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  if (g_server_power_status != GPIO_VALUE_HIGH)
    return;

  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, NULL);
}


// Generic Event Handler for GPIO changes
void
gpio_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, NULL);
}

// Generic Event Handler for GPIO changes, but only logs event when MB is ON

void
gpio_event_pson_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  if (g_server_power_status != GPIO_VALUE_HIGH)
    return;

  log_gpio_change(FRU_MB, desc, curr, 0, NULL);
}

// Generic Event Handler for GPIO changes, but only logs event when MB is ON 3S
void
gpio_event_pson_3s_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  if(!server_power_check(3))
    return;

  log_gpio_change(FRU_MB, desc, curr, 0, NULL);
}

//Usb Debug Card Event Handler
void
usb_dbg_card_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  syslog(LOG_CRIT, "Debug Card %s\n", curr ? "Extraction": "Insertion");
  log_gpio_change(FRU_MB, desc, curr, 0, NULL);
}

//Reset Button Event Handler
void
pwr_reset_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  log_gpio_change(FRU_MB, desc, curr, 0, NULL);
}

//Power Button Event Handler
void
pwr_button_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  pal_set_restart_cause(FRU_MB, RESTART_CAUSE_PWR_ON_PUSH_BUTTON);
  log_gpio_change(FRU_MB, desc, curr, 0, NULL);
}

//CPU Power Ok Event Handler
void
pwr_good_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  g_server_power_status = curr;
  g_cpu_pwrgd_trig = true;
  log_gpio_change(FRU_MB, desc, curr, 0, NULL);
  reset_timer(&g_power_on_sec);
}

//Uart Select on DEBUG Card Event Handler
void
uart_select_handle(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  g_uart_switch_count = 2;
  log_gpio_change(FRU_MB, desc, curr, 0, NULL);
  pal_uart_select_led_set();

}

// Event Handler for GPIOF6 platform reset changes
void
platform_reset_handle(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  struct timespec ts;
  char value[MAX_VALUE_LEN];
  if (!sgpio_valid_check()) return;

  // Use GPIOF6 to filter some gpio logging
  reset_timer(&g_reset_sec);
  TOUCH("/tmp/rst_touch");

  clock_gettime(CLOCK_MONOTONIC, &ts);
  snprintf(value, sizeof(value), "%ld", ts.tv_sec);

  if( curr == GPIO_VALUE_HIGH )
    kv_set("snr_pwron_flag", value, 0, 0);
  else
    kv_set("snr_pwron_flag", 0, 0, 0);

  log_gpio_change(FRU_MB, desc, curr, 0, NULL);
}

void
platform_reset_init(gpiopoll_pin_t *desc, gpio_value_t value) {
  struct timespec ts;
  char data[MAX_VALUE_LEN];

  clock_gettime(CLOCK_MONOTONIC, &ts);
  snprintf(data, sizeof(data), "%ld", ts.tv_sec);

  if( value == GPIO_VALUE_HIGH )
    kv_set("snr_pwron_flag", data, 0, 0);
  else
    kv_set("snr_pwron_flag", 0, 0, 0);

  return;
}

// Thread for gpio timer
void
*gpio_timer() {
  uint8_t status = 0;
  uint8_t fru = FRU_MB;
  long int pot;
  char str[MAX_VALUE_LEN] = {0};
  int tread_time = 0;

  while (1) {
    sleep(1);

    pal_get_server_power(fru, &status);
    if (status == SERVER_POWER_ON) {
      increase_timer(&g_reset_sec);
      pot = increase_timer(&g_power_on_sec);
    } else {
      pot = decrease_timer(&g_power_on_sec);
    }

    // Wait power-on.sh finished, then update pwr_server_last_state
    if (tread_time < 20) {
      tread_time++;
    } else {
      if (pal_get_last_pwr_state(fru, str) < 0)
        str[0] = '\0';
      if (status == SERVER_POWER_ON) {
        if (strncmp(str, POWER_ON_STR, strlen(POWER_ON_STR)) != 0) {
          pal_set_last_pwr_state(fru, POWER_ON_STR);
          syslog(LOG_INFO, "last pwr state updated to on\n");
        }
      } else {
        // wait until PowerOnTime < -2 to make sure it's not AC lost
        // Handle corner case during sled-cycle due to BMC residual electricity (1.2sec)
        if (pot < -2 && strncmp(str, POWER_OFF_STR, strlen(POWER_OFF_STR)) != 0) {
          pal_set_last_pwr_state(fru, POWER_OFF_STR);
          syslog(LOG_INFO, "last pwr state updated to off\n");
        }
      }
    }

    //Show Uart Debug Select Number 2sec
    if (g_uart_switch_count > 0) {
      --g_uart_switch_count;

      if ( g_uart_switch_count == 0)
        pal_postcode_select(POSTCODE_BY_HOST);
    }
  }

  return NULL;
}

static void
ioex_table_polling_once(struct gpiopoll_ioex_config *ioex_gpios,
                        int *init_flags, int size, int power_status)
{
  int i, assert;
  gpio_value_t curr;

  for (i = 0; i < size; ++i)
  {
    if(ioex_gpios[i].pwr_st != IOEX_GPIO_STANDBY) {
      if (power_status < 0 || power_status == SERVER_POWER_OFF) {
        continue;
      }
    }

    curr = gpio_get_value_by_shadow(ioex_gpios[i].shadow);
    if(curr == ioex_gpios[i].last) {
      continue;
    }

    if (init_flags[i] == false && ioex_gpios[i].init != NULL) {
        ioex_gpios[i].init(ioex_gpios[i].desc, curr);
        ioex_gpios[i].last = curr;
        init_flags[i] = true;
        continue;
      }

    switch (ioex_gpios[i].edge) {
      case GPIO_EDGE_FALLING:
        assert = (curr == GPIO_VALUE_LOW);
        break;
      case GPIO_EDGE_RISING:
        assert = (curr == GPIO_VALUE_HIGH);
        break;
      case GPIO_EDGE_BOTH:
        assert = true;
        break;
      default:
        assert = false;
        break;
    }

    if (assert && (ioex_gpios[i].handler != NULL)) {
      ioex_gpios[i].handler(ioex_gpios[i].desc, curr);
    }
    ioex_gpios[i].last = curr;
  }
}

static void
present_init (char* desc, gpio_value_t value) {
  if (value == GPIO_VALUE_HIGH)
    syslog(LOG_CRIT, "FRU: %d , %s not present.", FRU_MB, desc);
}

static void
present_handle (char* desc, gpio_value_t value) {
  syslog(LOG_CRIT, "FRU: %d , %s %s.", FRU_MB, desc,
          (value == GPIO_VALUE_HIGH) ? "not present": "present");
}

static void
enable_init (char* desc, gpio_value_t value) {
  if (value == GPIO_VALUE_LOW)
    syslog(LOG_CRIT, "FRU: %d , %s disable.", FRU_MB, desc);
}

static void
enable_handle (char* desc, gpio_value_t value) {
  syslog(LOG_CRIT, "FRU: %d , %s %s.", FRU_MB, desc,
          (value == GPIO_VALUE_HIGH) ? "enable": "disable");
}

struct gpiopoll_ioex_config iox_gpios[] = {
  {FAN_BP0_PRSNT,  "FAN_BP0",      IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN_BP1_PRSNT,  "FAN_BP1",      IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN0_PRSNT,     "FAN0",         IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN1_PRSNT,     "FAN1",         IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN2_PRSNT,     "FAN2",         IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN3_PRSNT,     "FAN3",         IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN4_PRSNT,     "FAN4",         IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN5_PRSNT,     "FAN5",         IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN6_PRSNT,     "FAN6",         IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN7_PRSNT,     "FAN7",         IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN8_PRSNT,     "FAN8",         IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN9_PRSNT,     "FAN9",         IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN10_PRSNT,    "FAN10",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN11_PRSNT,    "FAN11",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN12_PRSNT,    "FAN12",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN13_PRSNT,    "FAN13",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN14_PRSNT,    "FAN14",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN15_PRSNT,    "FAN15",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_C,  "SWB_CABLE_C",  IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_B1, "SWB CABLE_B1", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_B2, "SWB_CABLE_B2", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_B3, "SWB_CABLE_B3", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_D,  "GPU_CABLE_D",  IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_A1, "GPU_CABLE_A1", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_A2, "GPU_CABLE_A2", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_A3, "GPU_CABLE_A3", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FM_HS1_EN_BUSBAR_BUF, "HPDB_HS1_BUSBAR_EN", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, enable_init, enable_handle},
  {FM_HS2_EN_BUSBAR_BUF, "HPDB_HS2_BUSBAR_EN", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, enable_init, enable_handle},
};

void
*iox_gpio_handle()
{
  int i, status;
  int size = sizeof(iox_gpios)/sizeof(iox_gpios[0]);
  int init_flag[size];

  for (i = 0; i < size; ++i)
    init_flag[i] = false;

  while (1) {
    if (pal_get_server_power(FRU_MB, (uint8_t*)&status) < 0)
      status = -1;
    ioex_table_polling_once(iox_gpios, init_flag, size, status);
    sleep(1);
  }

  return NULL;
}


