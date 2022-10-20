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
#include <openbmc/libgpio.h>
#include <openbmc/pal.h>
#include <openbmc/nm.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/misc-utils.h>
#include <openbmc/peci_sensors.h>
#include <openbmc/pal_gpio.h>

#define POLL_TIMEOUT -1
#define POWER_ON_STR        "on"
#define POWER_OFF_STR       "off"
#define SERVER_POWER_CHECK(power_on_time)  \
do {                                       \
  uint8_t status = 0;                      \
  pal_get_server_power(FRU_MB, &status);   \
  if (status != SERVER_POWER_ON) {         \
    return;                                \
  }                                        \
  if (g_power_on_sec < power_on_time) {    \
    return;                                \
  }                                        \
}while(0)

#define TOUCH(path) \
{\
  int fd = creat(path, 0644);\
  if (fd) close(fd);\
}

static uint8_t g_caterr_irq = 0;
static bool g_mcerr_ierr_assert = false;
static int g_uart_switch_count = 0;
static long int g_reset_sec = 0;
static long int g_power_on_sec = 0;
static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t caterr_mutex = PTHREAD_MUTEX_INITIALIZER;
static gpio_value_t g_server_power_status = GPIO_VALUE_INVALID;
static bool g_cpu_pwrgd_trig = false;

enum {
  STBY,
  PS_ON,
  PS_ON_3S,
};

static bool
sgpio_valid_check(){
  int bit1 = gpio_get_value_by_shadow("CPLD_SGPIO_READY_ID0");
  int bit2 = gpio_get_value_by_shadow("CPLD_SGPIO_READY_ID1");
  int bit3 = gpio_get_value_by_shadow("CPLD_SGPIO_READY_ID2");
  int bit4 = gpio_get_value_by_shadow("CPLD_SGPIO_READY_ID3");
  if (
    bit1 ==  0 &&
    bit2 ==  1 &&
    bit3 ==  0 &&
    bit4 ==  1
  ) {
    return true;
  } else {
    return false;
  }
}

static void
log_gpio_change(gpiopoll_pin_t *desc, gpio_value_t value, useconds_t log_delay) {
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);
  assert(cfg);
  syslog(LOG_CRIT, "FRU: %d %s: %s - %s\n", FRU_MB, value ? "DEASSERT": "ASSERT",
         cfg->description, cfg->shadow);
}

static void
cpu_vr_hot_init(gpiopoll_pin_t *desc, gpio_value_t value) {
  if(value == GPIO_VALUE_LOW )
    log_gpio_change(desc, value, 0);

  return;
}

void cpu_skt_init(gpiopoll_pin_t *desc, gpio_value_t value) {
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
    log_gpio_change(desc, value, 0);
  }
}

static
void cpu0_pvccin_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check()) return;
  syslog(LOG_CRIT, "FRU: %d CPU0 PVCCIN VR HOT Warning %s\n", FRU_MB,
         curr ? "Deassertion": "Assertion");
}

static
void cpu1_pvccin_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check()) return;
  syslog(LOG_CRIT, "FRU: %d CPU1 PVCCIN VR HOT Warning %s\n", FRU_MB,
         curr ? "Deassertion": "Assertion");
}

static
void cpu0_pvccd_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check()) return;
  syslog(LOG_CRIT, "FRU: %d CPU0 PVCCD VR HOT Warning %s\n", FRU_MB,
         curr ? "Deassertion": "Assertion");
}

static
void cpu1_pvccd_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check()) return;
  syslog(LOG_CRIT, "FRU: %d CPU1 PVCCD VR HOT Warning %s\n", FRU_MB,
         curr ? "Deassertion": "Assertion");
}

static
void sml1_pmbus_alert_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check()) return;
  syslog(LOG_CRIT, "FRU: %d HSC OC Warning %s\n", FRU_MB,
         curr ? "Deassertion": "Assertion");
}

static
void oc_detect_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check()) return;
  syslog(LOG_CRIT, "FRU: %d HSC Surge Current Warning %s\n", FRU_MB,
         curr ? "Deassertion": "Assertion");
}

static
void uv_detect_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check()) return;
  syslog(LOG_CRIT, "FRU: %d HSC Under Voltage Warning %s\n", FRU_MB,
         curr ? "Deassertion": "Assertion");
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
static void pch_thermtrip_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
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
  log_gpio_change(desc, curr, 0);
}

//PROCHOT Handler
static void prochot_reason(char *reason)
{
  if (gpio_get_value_by_shadow(IRQ_UV_DETECT_N) == GPIO_VALUE_LOW)
    strcpy(reason, "UV");
  if (gpio_get_value_by_shadow(IRQ_OC_DETECT_N) == GPIO_VALUE_LOW)
    strcpy(reason, "OC");
  if (gpio_get_value_by_shadow(IRQ_HSC_ALERT_N) == GPIO_VALUE_LOW)
    strcpy(reason, "HSC PMBus alert");
}

static void cpu_prochot_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  char cmd[128] = {0};
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);
  if (!sgpio_valid_check()) return;
  assert(cfg);
  SERVER_POWER_CHECK(3);
  //LCD debug card critical SEL support
  strcat(cmd, "CPU FPH");
  if (curr) {
    strcat(cmd, " DEASSERT");
    syslog(LOG_CRIT, "FRU: %d DEASSERT: %s - %s\n", FRU_MB, cfg->description, cfg->shadow);
  } else {
    char reason[32] = "";
    strcat(cmd, " by ");
    prochot_reason(reason);
    strcat(cmd, reason);
    syslog(LOG_CRIT, "FRU: %d ASSERT: %s - %s (reason: %s)\n",
           FRU_MB, cfg->description, cfg->shadow, reason);
    strcat(cmd, " ASSERT");
  }
  pal_add_cri_sel(cmd);
}

static void thermtrip_add_cri_sel(const char *shadow_name, gpio_value_t curr)
{
  char cmd[128], log_name[16], log_descript[16] = "ASSERT\0";

  if (strcmp(shadow_name, FM_CPU0_THERMTRIP_N) == 0)
    sprintf(log_name, "CPU0");
  else if (strcmp(shadow_name, FM_CPU1_THERMTRIP_N) == 0)
    sprintf(log_name, "CPU1");

  if (curr == GPIO_VALUE_HIGH)
    sprintf(log_descript, "DEASSERT");

  sprintf(cmd, "FRU: %d %s thermtrip %s", FRU_MB, log_name, log_descript);
  pal_add_cri_sel(cmd);
}

static void cpu_thermtrip_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  if (!sgpio_valid_check()) return;
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);

  if (g_server_power_status != GPIO_VALUE_HIGH)
    return;

  thermtrip_add_cri_sel(cfg->shadow, curr);
  log_gpio_change(desc, curr, 0);
}

static void cpu_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  if (!sgpio_valid_check()) return;
  if (g_server_power_status != GPIO_VALUE_HIGH)
    return;
  log_gpio_change(desc, curr, 0);
}

static void mem_thermtrip_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  if (!sgpio_valid_check()) return;
  if (g_server_power_status != GPIO_VALUE_HIGH)
    return;
  log_gpio_change(desc, curr, 0);
}


// Generic Event Handler for GPIO changes
static void gpio_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  if (!sgpio_valid_check()) return;
  log_gpio_change(desc, curr, 0);
}

// Generic Event Handler for GPIO changes, but only logs event when MB is ON

static void gpio_event_pson_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  if (!sgpio_valid_check()) return;
  SERVER_POWER_CHECK(0);
  log_gpio_change(desc, curr, 0);
}

// Generic Event Handler for GPIO changes, but only logs event when MB is ON 3S
static void gpio_event_pson_3s_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  if (!sgpio_valid_check()) return;
  SERVER_POWER_CHECK(3);
  log_gpio_change(desc, curr, 0);
}

//Usb Debug Card Event Handler
static void
usb_dbg_card_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  log_gpio_change(desc, curr, 0);
}

//Reset Button Event Handler
static void
pwr_reset_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  log_gpio_change(desc, curr, 0);
}

//Power Button Event Handler
static void
pwr_button_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  pal_set_restart_cause(FRU_MB, RESTART_CAUSE_PWR_ON_PUSH_BUTTON);
  log_gpio_change(desc, curr, 0);
}

//CPU Power Ok Event Handler
static void
cpu_pwr_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check()) return;
  g_server_power_status = curr;
  g_cpu_pwrgd_trig = true;
  log_gpio_change(desc, curr, 0);
  reset_timer(&g_power_on_sec);
}

//IERR and MCERR Event Handler
static void
init_caterr(gpiopoll_pin_t *desc, gpio_value_t value) {
  uint8_t status = 0;
  pal_get_server_power(FRU_MB, &status);
  if (status && value == GPIO_VALUE_LOW) {
    g_caterr_irq++;
  }
}

static void
err_caterr_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  SERVER_POWER_CHECK(1);

  pthread_mutex_lock(&caterr_mutex);
  g_caterr_irq++;
  pthread_mutex_unlock(&caterr_mutex);
}

static int
ierr_mcerr_event_log(bool is_caterr, const char *err_type) {
  char temp_log[128] = {0};
  char temp_syslog[128] = {0};
  char cpu_str[32] = "";
  int num=0;
  int ret=0;

  ret = cmd_peci_get_cpu_err_num(PECI_CPU0_ADDR, &num, is_caterr);
  if (ret != 0) {
    syslog(LOG_ERR, "Can't Read MCA Log\n");
  }

  if (num == 2)
    strcpy(cpu_str, "0/1");
  else if (num != -1)
    sprintf(cpu_str, "%d", num);

  sprintf(temp_syslog, "ASSERT: CPU%s %s\n", cpu_str, err_type);
  sprintf(temp_log, "CPU%s %s", cpu_str, err_type);
  syslog(LOG_CRIT, "FRU: %d %s",FRU_MB, temp_syslog);
  pal_add_cri_sel(temp_log);

  return ret;
}

static void *
ierr_mcerr_event_handler() {
  uint8_t caterr_cnt = 0;
  gpio_value_t value;

  while (1) {
    if (g_caterr_irq > 0) {
      caterr_cnt++;
      if (caterr_cnt == 2) {
        if (g_caterr_irq == 1) {
          g_mcerr_ierr_assert = true;

          value = gpio_get_value_by_shadow(FM_CPU_CATERR_N);
          if (value == GPIO_VALUE_LOW) {
            ierr_mcerr_event_log(true, "IERR/CATERR");
          } else {
            ierr_mcerr_event_log(true, "MCERR/CATERR");
          }

          pthread_mutex_lock(&caterr_mutex);
          g_caterr_irq--;
          pthread_mutex_unlock(&caterr_mutex);
          caterr_cnt = 0;
          if (system("/usr/local/bin/autodump.sh &")) {
            syslog(LOG_WARNING, "Failed to start crashdump\n");
          }

        } else if (g_caterr_irq > 1) {
          while (g_caterr_irq > 1) {
            ierr_mcerr_event_log(true, "MCERR/CATERR");
            pthread_mutex_lock(&caterr_mutex);
            g_caterr_irq--;
            pthread_mutex_unlock(&caterr_mutex);
          }
          caterr_cnt = 1;
        }
      }
    }
    usleep(25000); //25ms
  }
  return NULL;
}

//Uart Select on DEBUG Card Event Handler
static void
uart_select_handle(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check()) return;
  g_uart_switch_count = 2;
  log_gpio_change(desc, curr, 0);
}

// Event Handler for GPIOF6 platform reset changes
static void platform_reset_handle(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  struct timespec ts;
  char value[MAX_VALUE_LEN];
  if (!sgpio_valid_check()) return;

  // Use GPIOF6 to filter some gpio logging
  reset_timer(&g_reset_sec);
  TOUCH("/tmp/rst_touch");

  clock_gettime(CLOCK_MONOTONIC, &ts);
  sprintf(value, "%ld", ts.tv_sec);

  if( curr == GPIO_VALUE_HIGH )
    kv_set("snr_pwron_flag", value, 0, 0);
  else
    kv_set("snr_pwron_flag", 0, 0, 0);

  log_gpio_change(desc, curr, 0);
}

static void
platform_reset_init(gpiopoll_pin_t *desc, gpio_value_t value) {
  struct timespec ts;
  char data[MAX_VALUE_LEN];

  clock_gettime(CLOCK_MONOTONIC, &ts);
  sprintf(data, "%ld", ts.tv_sec);

  if( value == GPIO_VALUE_HIGH )
    kv_set("snr_pwron_flag", data, 0, 0);
  else
    kv_set("snr_pwron_flag", 0, 0, 0);

  return;
}

// Thread for gpio timer
static void
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
  }

  return NULL;
}

// GPIO table to be monitored
static struct gpiopoll_config g_gpios[] = {
  // shadow, description, edge, handler, oneshot
  {FP_RST_BTN_IN_N,         "GPIOP2",   GPIO_EDGE_BOTH,    pwr_reset_handler,          NULL},
  {FP_PWR_BTN_IN_N,         "GPIOP0",   GPIO_EDGE_BOTH,    pwr_button_handler,         NULL},
  {FM_UARTSW_LSB_N,         "SGPIO106", GPIO_EDGE_BOTH,    uart_select_handle,         NULL},
  {FM_UARTSW_MSB_N,         "SGPIO108", GPIO_EDGE_BOTH,    uart_select_handle,         NULL},
  {FM_POST_CARD_PRES_N,     "GPIOZ6",   GPIO_EDGE_BOTH,    usb_dbg_card_handler,       NULL},
  {IRQ_UV_DETECT_N,         "SGPIO188", GPIO_EDGE_BOTH,    uv_detect_handler,          NULL},
  {IRQ_OC_DETECT_N,         "SGPIO178", GPIO_EDGE_BOTH,    oc_detect_handler,          NULL},
  {IRQ_HSC_FAULT_N,         "SGPIO36",  GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
  {IRQ_HSC_ALERT_N,         "SGPIO2",   GPIO_EDGE_BOTH,    sml1_pmbus_alert_handler,   NULL},
  {FM_CPU_CATERR_N,         "GPIOC5",   GPIO_EDGE_FALLING, err_caterr_handler,         init_caterr},
  {FM_CPU0_PROCHOT_N,       "SGPIO202", GPIO_EDGE_BOTH,    cpu_prochot_handler,        NULL},
  {FM_CPU1_PROCHOT_N,       "SGPIO186", GPIO_EDGE_BOTH,    cpu_prochot_handler,        NULL},
  {FM_CPU0_THERMTRIP_N,     "SGPIO136", GPIO_EDGE_FALLING, cpu_thermtrip_handler,      NULL},
  {FM_CPU1_THERMTRIP_N,     "SGPIO118", GPIO_EDGE_FALLING, cpu_thermtrip_handler,      NULL},
  {FM_CPU_ERR0_N,           "SGPIO144", GPIO_EDGE_FALLING, cpu_event_handler,          NULL},
  {FM_CPU_ERR1_N,           "SGPIO142", GPIO_EDGE_FALLING, cpu_event_handler,          NULL},
  {FM_CPU_ERR2_N,           "SGPIO140", GPIO_EDGE_FALLING, cpu_event_handler,          NULL},
  {FM_CPU0_MEM_HOT_N,       "SGPIO138", GPIO_EDGE_BOTH,    gpio_event_pson_3s_handler, NULL},
  {FM_CPU1_MEM_HOT_N,       "SGPIO124", GPIO_EDGE_BOTH,    gpio_event_pson_3s_handler, NULL},
  {FM_CPU0_MEM_THERMTRIP_N, "SGPIO122", GPIO_EDGE_FALLING, mem_thermtrip_handler,      NULL},
  {FM_CPU1_MEM_THERMTRIP_N, "SGPIO158", GPIO_EDGE_FALLING, mem_thermtrip_handler,      NULL},
  {FM_SYS_THROTTLE,         "SGPIO198", GPIO_EDGE_BOTH,    gpio_event_pson_handler,    NULL},
  {FM_PCH_THERMTRIP_N,      "SGPIO28",  GPIO_EDGE_BOTH,    pch_thermtrip_handler,      NULL},
  {FM_CPU0_VCCIN_VR_HOT,    "SGPIO154", GPIO_EDGE_BOTH,    cpu0_pvccin_handler,        cpu_vr_hot_init},
  {FM_CPU1_VCCIN_VR_HOT,    "SGPIO120", GPIO_EDGE_BOTH,    cpu1_pvccin_handler,        cpu_vr_hot_init},
  {FM_CPU0_VCCD_VR_HOT,     "SGPIO12",  GPIO_EDGE_BOTH,    cpu0_pvccd_handler,         cpu_vr_hot_init},
  {FM_CPU1_VCCD_VR_HOT,     "SGPIO18",  GPIO_EDGE_BOTH,    cpu1_pvccd_handler,         cpu_vr_hot_init},
  {RST_PLTRST_N,            "SGPIO200", GPIO_EDGE_BOTH,    platform_reset_handle,      platform_reset_init},
  {FM_LAST_PWRGD,           "SGPIO116", GPIO_EDGE_BOTH,    cpu_pwr_handler,            NULL},
  {FM_CPU0_SKTOCC,          "SGPIO112", GPIO_EDGE_BOTH,    gpio_event_handler,         cpu_skt_init},
  {FM_CPU1_SKTOCC,          "SGPIO114", GPIO_EDGE_BOTH,    gpio_event_handler,         cpu_skt_init},
};

int main(int argc, char **argv)
{
  int rc, pid_file;
  gpiopoll_desc_t *polldesc;
  pthread_t tid_ierr_mcerr_event;
  pthread_t tid_gpio_timer;

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

    //Create thread for IERR/MCERR event detect
    if (pthread_create(&tid_ierr_mcerr_event, NULL, ierr_mcerr_event_handler, NULL) < 0) {
      syslog(LOG_WARNING, "pthread_create for ierr_mcerr_event_handler\n");
      exit(1);
    }

    //Create thread for platform reset event filter check
    if (pthread_create(&tid_gpio_timer, NULL, gpio_timer, NULL) < 0) {
      syslog(LOG_WARNING, "pthread_create for platform_reset_filter_handler\n");
      exit(1);
    }

    polldesc = gpio_poll_open(g_gpios, sizeof(g_gpios)/sizeof(g_gpios[0]));
    if (!polldesc) {
      syslog(LOG_CRIT, "FRU: %d Cannot start poll operation on GPIOs", FRU_MB);
    } else {
      if (gpio_poll(polldesc, POLL_TIMEOUT)) {
        syslog(LOG_CRIT, "FRU: %d Poll returned error", FRU_MB);
      }
      gpio_poll_close(polldesc);
    }
  }

  return 0;
}
