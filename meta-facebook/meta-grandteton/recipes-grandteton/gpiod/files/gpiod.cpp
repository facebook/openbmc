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
#include <thread>
#include <chrono>
#include <sys/types.h>
#include <sys/file.h>
#include <future>
#include <pthread.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/misc-utils.h>
#include <openbmc/pal.h>
#include <openbmc/pal_def.h>
#include <openbmc/pal_common.h>
#include <libpldm-oem/pldm.h>
#include <libpldm-oem/pal_pldm.hpp>
#include <openbmc/hgx.h>
#include "gpiod.h"


#define TOUCH(path) \
{\
  int fd = creat(path, 0644);\
  if (fd) close(fd);\
}

static int g_uart_switch_count = 0;
static long int g_reset_sec = 0;
static long int g_power_on_sec = 0;
static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static gpio_value_t g_server_power_status = GPIO_VALUE_INVALID;

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

struct gpiopoll_ioex_config iox_gpios[] = {
  {FAN_BP1_PRSNT, "FAN_BP1",     IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN_BP2_PRSNT, "FAN_BP2",     IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN0_PRSNT,    "FAN0",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN1_PRSNT,    "FAN1",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN2_PRSNT,    "FAN2",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN3_PRSNT,    "FAN3",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN4_PRSNT,    "FAN4",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN5_PRSNT,    "FAN5",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN6_PRSNT,    "FAN6",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN7_PRSNT,    "FAN7",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN8_PRSNT,    "FAN8",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN9_PRSNT,    "FAN9",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN10_PRSNT,   "FAN10",       IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN11_PRSNT,   "FAN11",       IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN12_PRSNT,   "FAN12",       IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN13_PRSNT,   "FAN13",       IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN14_PRSNT,   "FAN14",       IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN15_PRSNT,   "FAN15",       IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_G, "SWB_CABLE_G", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_B, "SWB CABLE_B", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_C, "SWB_CABLE_C", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_F, "SWB_CABLE_F", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_H, "GPU_CABLE_H", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_A, "GPU_CABLE_A", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_D, "GPU_CABLE_D", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_E, "GPU_CABLE_E", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FM_HS1_EN_BUSBAR_BUF, "HPDB_HS1_BUSBAR_EN", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, enable_init, enable_handle},
  {FM_HS2_EN_BUSBAR_BUF, "HPDB_HS2_BUSBAR_EN", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, enable_init, enable_handle},
};

struct gpiopoll_ioex_config gta_iox_gpios[] = {
  {FAN_BP1_PRSNT, "FAN_BP1",     IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN_BP2_PRSNT, "FAN_BP2",     IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN0_PRSNT,    "FAN0",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN1_PRSNT,    "FAN1",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN2_PRSNT,    "FAN2",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN3_PRSNT,    "FAN3",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN4_PRSNT,    "FAN4",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN5_PRSNT,    "FAN5",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN6_PRSNT,    "FAN6",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN7_PRSNT,    "FAN7",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN8_PRSNT,    "FAN8",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN9_PRSNT,    "FAN9",        IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN10_PRSNT,   "FAN10",       IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN11_PRSNT,   "FAN11",       IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN12_PRSNT,   "FAN12",       IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN13_PRSNT,   "FAN13",       IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN14_PRSNT,   "FAN14",       IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FAN15_PRSNT,   "FAN15",       IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_G, "ACB_CABLE_G", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_B, "ACB CABLE_B", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {CABLE_PRSNT_A, "MEB_CABLE_A", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, present_init, present_handle},
  {FM_HS1_EN_BUSBAR_BUF, "HPDB_HS1_BUSBAR_EN", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, enable_init, enable_handle},
  {FM_HS2_EN_BUSBAR_BUF, "HPDB_HS2_BUSBAR_EN", IOEX_GPIO_STANDBY, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, enable_init, enable_handle},
};

struct gpiopoll_config gpios_common_list[] = {
  // shadow, description, edge, handler, oneshot
  {FP_RST_BTN_IN_N,      "GPIOP2",         GPIO_EDGE_BOTH,    pwr_reset_handler,    NULL},
  {FP_PWR_BTN_IN_N,      "GPIOP0",         GPIO_EDGE_BOTH,    pwr_button_handler,   NULL},
  {FM_UARTSW_LSB_N,      "SGPIO106",       GPIO_EDGE_BOTH,    uart_select_handle,   NULL},
  {FM_UARTSW_MSB_N,      "SGPIO108",       GPIO_EDGE_BOTH,    uart_select_handle,   NULL},
  {FM_POST_CARD_PRES_N,  "GPIOZ6",         GPIO_EDGE_BOTH,    usb_dbg_card_handler, NULL},
  {FP_AC_PWR_BMC_BTN,    "GPIO18A0",       GPIO_EDGE_BOTH,    gpio_event_handler,   NULL},
  {BIC_READY,            "SGPIO32",        GPIO_EDGE_BOTH,    bic_ready_handler,    bic_ready_init},
  {GPU_FPGA_THERM_OVERT, "SGPIO40",        GPIO_EDGE_FALLING, nv_event_handler,     NULL},
  {GPU_FPGA_DEVIC_OVERT, "SGPIO42",        GPIO_EDGE_FALLING, nv_event_handler,     NULL},
  {PEX_FW_VER_UPDATE,    "PEX_VER_UPDATE", GPIO_EDGE_RISING,  pex_fw_ver_handle,    NULL},
  {FM_SMB_2_ALERT_GPU,   "SGPIO208",       GPIO_EDGE_BOTH,    sgpio_event_handler,  NULL},
  {FM_SMB_1_ALERT_GPU,   "SGPIO210",       GPIO_EDGE_BOTH,    sgpio_event_handler,  NULL},
  {BIOS_TPM_PRESENT_IN,  "GPIOO5",         GPIO_EDGE_BOTH,    tpm_sync_handler,     NULL},
};

bool server_power_check(uint8_t power_on_time) {
  uint8_t status = 0;

  pal_get_server_power(FRU_MB, &status);
  if (status != SERVER_POWER_ON || g_power_on_sec < power_on_time) {
    return false;
  }
  return true;
}

static void*
delay_get_firmware(void* arg) {
  unsigned int* delay = (unsigned int*)arg;
  pthread_detach(pthread_self());
  syslog(LOG_INFO, "Get swb firmware: start delay(%u).", *delay);
  sleep(*delay);
  pal_pldm_get_firmware_parameter(SWB_BUS_ID, SWB_BIC_EID);
  free(delay);
  delay = NULL;
  pthread_exit(NULL);
}

static
void bic_get_firmware (unsigned int _delay) {
  unsigned int *delay = (unsigned int*) malloc (sizeof(unsigned int));
  pthread_t tid_delay_get_firmware;

  if (delay == NULL) {
    syslog(LOG_WARNING, "Get swb firmware failed.");
    return;
  }

  *delay = _delay;
  if (*delay == 0) {
    pal_pldm_get_firmware_parameter(SWB_BUS_ID, SWB_BIC_EID);
    free(delay);
    delay = NULL;
  } else {
    if (pthread_create(&tid_delay_get_firmware, NULL, delay_get_firmware, (void*)delay)) {
      syslog(LOG_WARNING, "pthread_create for delay_get_firmware");
      free(delay);
      delay = NULL;
    }
  }
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
                     const char* str,
                     const char* usrdef_str) {
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);
  struct delayed_log *log = (struct delayed_log *)malloc(sizeof(struct delayed_log));
  pthread_t tid_delay_log;
  assert(cfg);

  if(!log) {
    syslog(LOG_CRIT, "FRU: %d %s: %s - %s\n", fru, value ? "DEASSERT": "ASSERT", cfg->description, cfg->shadow);
    return;
  }

  memset(log->msg, 0, sizeof(log->msg));

  if (usrdef_str != NULL) {
    // User provides the entire log message to print.
    snprintf(log->msg, sizeof(log->msg), "%s\n", usrdef_str);
  } else if (str != NULL) {
    // User provides event name which had an assertion/deassertion.
    snprintf(log->msg, sizeof(log->msg), "FRU: %d %s: %s\n",
        fru, str, value ? "Deassertion": "Assertion");
  } else {
    // Infer the event name from the GPIO description
    snprintf(log->msg, sizeof(log->msg), "FRU: %d %s: %s - %s\n",
             fru,
             value ? "DEASSERT" : "ASSERT",
             cfg->description,
             cfg->shadow);
  }

  if (log_delay == 0) {
    syslog(LOG_CRIT, "%s", log->msg);
    free(log);
    log = NULL;
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
    log_gpio_change(FRU_MB, desc, value, 0, NULL, NULL);

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
    prev_value = (gpio_value_t)atoi(kvalue);
    if(value != prev_value ) {
      log_gpio_change(FRU_MB, desc, value, 0, NULL, NULL);
    }
  }
  snprintf(kvalue, sizeof(kvalue), "%d", value);
  kv_set(key, kvalue, 0, KV_FPERSIST);
}

void
cpu0_pvccin_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  const char* str = "CPU0 PVCCIN VR HOT Warning";
  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, str, NULL);
}

void
cpu1_pvccin_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  const char* str = "CPU1 PVCCIN VR HOT Warning";
  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, str, NULL);
}

void
cpu0_pvccd_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  const char* str = "CPU0 PVCCD VR HOT Warning";
  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, str, NULL);
}

void
cpu1_pvccd_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  const char* str = "CPU1 PVCCD VR HOT Warning";
  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, str, NULL);
}

void
sml1_pmbus_alert_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  const char* str = "HSC OC Warning";
  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, str, NULL);
}

void
oc_detect_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  const char* str = "HSC Surge Current Warning";
  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, str, NULL);
}

void
uv_detect_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  const char* str = "HSC Under Voltage Warning";
  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, str, NULL);
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

  log_gpio_change(FRU_MB, desc, curr, 0, NULL, NULL);
}

void prochot_reason(char *reason)
{
  if (gpio_get_value_by_shadow(IRQ_UV_DETECT_N) == GPIO_VALUE_LOW)
    strcpy(reason, "UV");
  if (gpio_get_value_by_shadow(IRQ_OC_DETECT_N) == GPIO_VALUE_LOW)
    strcpy(reason, "OC");
}

void
cpu_prochot_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  char cmd[128] = {0};
  char str[1024] = {0};
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);

  if (!sgpio_valid_check()) return;
  assert(cfg);

  if(!server_power_check(3))
    return;

  //LCD debug card critical SEL support
  strcat(cmd, "CPU FPH");
  if (curr) {
    log_gpio_change(FRU_MB, desc, curr, 0, NULL, NULL);
    strcat(cmd, " DEASSERT");
  } else {
    char reason[32] = "";
    strcat(cmd, " by ");
    prochot_reason(reason);
    strcat(cmd, reason);

    snprintf(str, sizeof(str), "FRU: %d ASSERT: %s - %s (reason: %s)",
           FRU_MB, cfg->description, cfg->shadow, reason);
    log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, NULL, str);
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
  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, NULL, NULL);
}

void
cpu_error_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  if (g_server_power_status != GPIO_VALUE_HIGH)
    return;

  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, NULL, NULL);
}

void
mem_thermtrip_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  if (g_server_power_status != GPIO_VALUE_HIGH)
    return;

  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, NULL, NULL);
}


// Generic Event Handler for GPIO changes
void
sgpio_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, NULL, NULL);
}

// Generic Event Handler for GPIO changes
void
gpio_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  log_gpio_change(FRU_MB, desc, curr, 0, NULL, NULL);
}

// Generic Event Handler for GPIO changes
void
tpm_sync_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  gpio_set_value_by_shadow(BIOS_TPM_PRESENT_OUT, curr);
  log_gpio_change(FRU_MB, desc, curr, 0, NULL, NULL);
}

// Generic Event Handler for GPIO changes, but only logs event when MB is ON

void
gpio_event_pson_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  if (g_server_power_status != GPIO_VALUE_HIGH)
    return;

  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, NULL, NULL);
}

// Generic Event Handler for GPIO changes, but only logs event when MB is ON 3S
void
gpio_event_pson_3s_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  if(!server_power_check(3))
    return;

  log_gpio_change(FRU_MB, desc, curr, 0, NULL, NULL);
}

//Usb Debug Card Event Handler
void
usb_dbg_card_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  syslog(LOG_CRIT, "Debug Card %s\n", curr ? "Extraction": "Insertion");
  log_gpio_change(FRU_MB, desc, curr, 0, NULL, NULL);
}

//Reset Button Event Handler
void
pwr_reset_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  log_gpio_change(FRU_MB, desc, curr, 0, NULL, NULL);
}

//Power Button Event Handler
void
pwr_button_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  pal_set_restart_cause(FRU_MB, RESTART_CAUSE_PWR_ON_PUSH_BUTTON);
  log_gpio_change(FRU_MB, desc, curr, 0, NULL, NULL);
}

//CPU Power Ok Event Handler
void
pwr_good_init(gpiopoll_pin_t *desc, gpio_value_t value) {
  g_server_power_status = value;
  return;
}

void
pwr_good_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  g_server_power_status = curr;
  log_gpio_change(FRU_MB, desc, curr, 0, NULL, NULL);
  reset_timer(&g_power_on_sec);

}

//Uart Select on DEBUG Card Event Handler
void
uart_select_handle(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  g_uart_switch_count = 2;
  log_gpio_change(FRU_MB, desc, curr, 0, NULL, NULL);
  pal_uart_select_led_set();

}

void
pex_fw_ver_handle(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  bic_get_firmware(15);
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

  pal_clear_psb_cache();

  log_gpio_change(FRU_MB, desc, curr, 0, NULL, NULL);
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
*gpio_timer(void *) {
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
          pal_set_last_pwr_state(fru, (char *)POWER_ON_STR);
          syslog(LOG_INFO, "last pwr state updated to on\n");
        }
      } else {
        // wait until PowerOnTime < -2 to make sure it's not AC lost
        // Handle corner case during sled-cycle due to BMC residual electricity (1.2sec)
        if (pot < -2 && strncmp(str, POWER_OFF_STR, strlen(POWER_OFF_STR)) != 0) {
          pal_set_last_pwr_state(fru, (char *)POWER_OFF_STR);
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

static
void iox_log_gpio_change(uint8_t fru,
                     char* desc,
                     useconds_t log_delay,
                     const char* str) {

  struct delayed_log *log = (struct delayed_log *)malloc(sizeof(struct delayed_log));
  pthread_t tid_delay_log;

  memset(log->msg, 0, sizeof(log->msg));
  snprintf(log->msg, sizeof(log->msg), "FRU: %d , %s %s", fru, str, desc);

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
present_init (char* desc, gpio_value_t value) {
  if (value == GPIO_VALUE_HIGH) {
    const char* str = "not present";
    iox_log_gpio_change(FRU_MB, desc, 0, str);
  }
}

void
present_handle (char* desc, gpio_value_t value) {
  const char* str = value? "not present": "present";
  iox_log_gpio_change(FRU_MB, desc, 0, str);
}

void
enable_init (char* desc, gpio_value_t value) {
  if (value == GPIO_VALUE_LOW) {
    const char* str = "disable";
    iox_log_gpio_change(FRU_MB, desc, DEFER_LOG_TIME, str);
  }
}

void
enable_handle (char* desc, gpio_value_t value) {
  const char* str = value? "enable": "disable";
  iox_log_gpio_change(FRU_MB, desc, DEFER_LOG_TIME, str);
}

void
*iox_gpio_handle(void *)
{
  int i, status;
  int size = pal_is_artemis() ? ARRAY_SIZE(gta_iox_gpios) : ARRAY_SIZE(iox_gpios);
  int init_flag[size];
  gpiopoll_ioex_config *iox_gpio_table = pal_is_artemis() ? gta_iox_gpios : iox_gpios;

  for (i = 0; i < size; ++i)
    init_flag[i] = false;

  while (1) {
    if (pal_get_server_power(FRU_MB, (uint8_t*)&status) < 0)
      status = -1;
    ioex_table_polling_once(iox_gpio_table, init_flag, size, status);
    sleep(1);
  }

  return NULL;
}

int get_gpios_common_list(struct gpiopoll_config** list) {

  uint8_t cnt = sizeof(gpios_common_list)/sizeof(gpios_common_list[0]);
  *list = gpios_common_list;

  return cnt;
}


static int
set_pldm_event_receiver()
{
  // pldmd-util -b 3 -e 0x0a raw 0x02 0x04 0x00 0x00 0x08 0x00 0x00
  uint8_t tbuf[MAX_TXBUF_SIZE] = {0};
  uint8_t* rbuf = (uint8_t *) NULL;
  uint8_t tlen = 0;
  size_t  rlen = 0;
  int rc;

  struct pldm_msg* pldmbuf = (struct pldm_msg *)tbuf;
  pldmbuf->hdr.request = 1;
  pldmbuf->hdr.type    = PLDM_PLATFORM;
  pldmbuf->hdr.command = PLDM_SET_EVENT_RECEIVER;
  tlen = PLDM_HEADER_SIZE;
  tbuf[tlen++] = 0x00; // eventMessageGlobalEnable, 0x00: Disable
  tbuf[tlen++] = 0x00; // transportProtocolType,    0x00: MCTP
  tbuf[tlen++] = 0x08; // eventReceiverAddressInfo  0x08: EID
  tbuf[tlen++] = 0x00; // heartbeatTimer
  tbuf[tlen++] = 0x00; // heartbeatTimer

  rc = oem_pldm_send_recv(SWB_BUS_ID, SWB_BIC_EID, tbuf, tlen, &rbuf, &rlen);
  if (rc == PLDM_SUCCESS) {
    syslog(LOG_INFO, "Set PLDM event receiver success.");
  }

  if (rbuf) {
    free(rbuf);
  }

  return rc;
}

void
bic_ready_init(gpiopoll_pin_t *desc, gpio_value_t value)
{
  set_pldm_event_receiver();

  if (value == GPIO_VALUE_HIGH)
    log_gpio_change(FRU_MB, desc, value, 0, NULL, NULL);
}

void
bic_ready_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  if (!sgpio_valid_check())
    return;
  if (curr == GPIO_VALUE_LOW) {
    bic_get_firmware(1);
    set_pldm_event_receiver();
  }
  log_gpio_change(FRU_MB, desc, curr, 0, NULL, NULL);
}

static void hmc_ready(bool startup) {
  using namespace std::literals;
  static std::future<void> worker;
  auto hmc_provision = [startup] {
    using namespace std::literals;
    syslog(LOG_INFO, "HGX: Syncing time to HMC");
    while (1) {
      try {
        hgx::syncTime();
        break;
      } catch (std::exception& e) {
        syslog(LOG_WARNING, "HGX: Failed to get to HMC: %s", e.what());
        std::this_thread::sleep_for(1s);
      }
    }
  };
  // Create the future thread only if we have an invalid future (First
  // call of this function since start-up) or if the previous future
  // has completed and a 0s wait for it succeeds. Else if the wait fails,
  // the previous future is still running, so dont create another.
  if (!worker.valid() || worker.wait_for(0s) == std::future_status::ready) {
    worker = std::async(std::launch::async, hmc_provision);
  } else {
    syslog(LOG_WARNING, "HGX: Previous provision thread is still running");
  }
}

static void
hmc_ready_mon() {
  using namespace std::literals;
  static std::future<void> worker;
  auto hmc_ready_mon_thr = [] {
    constexpr auto timeout = 6min;
    constexpr auto poll_time = 5s;
    for (int i = 0; i < timeout / poll_time; i++) {
      std::this_thread::sleep_for(poll_time);
      auto val = gpio_get_value_by_shadow("GPU_BASE_HMC_READY_ISO_R");
      if (val == GPIO_VALUE_HIGH) {
        return;
      }
    }
    syslog(LOG_CRIT, "FRU: %d HMC_READY - ASSERT Timed out", FRU_HGX);
  };
  if (!worker.valid() || worker.wait_for(0s) == std::future_status::ready) {
    worker = std::async(std::launch::async, hmc_ready_mon_thr);
  } else {
    syslog(LOG_WARNING, "HGX: Previous HMC Ready waiter thread is still running");
  }
}

void
hmc_ready_init(gpiopoll_pin_t *desc, gpio_value_t value)
{
  if (value == GPIO_VALUE_HIGH) {
    hmc_ready(true);
  } else {
    hmc_ready_mon();
  }
}

void
hmc_ready_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  if (curr == GPIO_VALUE_HIGH) {
    hmc_ready(false);
  } else {
    hmc_ready_mon();
  }
}

void
nv_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  if(curr == GPIO_VALUE_LOW)
    if (system("/usr/local/bin/dump-nv-reg.sh &"))
      syslog(LOG_WARNING, "Failed to start NVDIA reg dump\n");

  log_gpio_change(FRU_MB, desc, curr, DEFER_LOG_TIME, NULL, NULL);
}


int main(int argc, char **argv)
{
  int rc, pid_file;
  gpiopoll_desc_t *polldesc;
  //Get Common GPIOS list
  struct gpiopoll_config* gpios_common_data = NULL;
  int gpios_common_cnt = get_gpios_common_list(&gpios_common_data);
  //Get Platform GPIOS list
  struct gpiopoll_config* gpios_plat_data = NULL;
  int gpios_plat_cnt = get_gpios_plat_list(&gpios_plat_data);

  uint8_t cnt = gpios_common_cnt + gpios_plat_cnt;

  pid_file = open("/var/run/gpiod.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if (rc) {
    if (EWOULDBLOCK == errno) {
      syslog(LOG_ERR, "Another gpiod instance is running...\n");
      exit(-1);
    }
  } else {
    openlog("gpiod", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "gpiod: daemon started");

    if(gpiod_plat_thread_create())
      exit(1);

    struct gpiopoll_config* list = (struct gpiopoll_config *)malloc(cnt * sizeof(struct gpiopoll_config));
    if(list == NULL) {
      syslog(LOG_WARNING, "gpiopoll config list memeory alloc fail\n");
      exit(1);
    }
    memcpy(list, gpios_common_data, gpios_common_cnt * sizeof(struct gpiopoll_config));
    memcpy(list+gpios_common_cnt, gpios_plat_data, gpios_plat_cnt * sizeof(struct gpiopoll_config));

    polldesc = gpio_poll_open(list, cnt);
    if (!polldesc) {
      syslog(LOG_CRIT, "FRU: %d Cannot start poll operation on GPIOs", FRU_MB);
    } else {
      if (gpio_poll(polldesc, POLL_TIMEOUT)) {
        syslog(LOG_CRIT, "FRU: %d Poll returned error", FRU_MB);
      }
      gpio_poll_close(polldesc);
    }

    if(list != NULL)
      free(list);
  }

  gpiod_plat_thread_finish();
  return 0;
}
