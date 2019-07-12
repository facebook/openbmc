/*
 * sensord
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
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <sys/un.h>
#include <sys/file.h>
#include <openbmc/pal.h>
#include <openbmc/libgpio.h>
#include <openbmc/ocp-dbg-lcd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define POWER_ON_STR        "on"
#define POWER_OFF_STR       "off"

//#define SMI_DEBUG

#define TOUCH(path) \
{\
  int fd = creat(path, 0644);\
  if (fd) close(fd);\
}

extern int usleep(useconds_t usec);
extern void daemon(int, int);

#define POLL_TIMEOUT -1 /* Forever */
static uint8_t CATERR_irq = 0;
static uint8_t MSMI_irq = 0;
static long int reset_sec = 0, power_on_sec = 0;
static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;

static bool MCERR_IERR_assert = false;
static int g_uart_switch_count = 0;

static void set_fault_led(bool on)
{
  static gpio_desc_t *gpio = NULL;
  if (!gpio) {
    gpio = gpio_open_by_shadow("BMC_FAULT_N"); // GPIOU5
    if (!gpio)
      return;
  }
  gpio_set_value(gpio, on ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH);
}

static void trigger_ppin(void)
{
  static bool is_ppin_triggered = false;
  static gpio_desc_t *gpio = NULL;
  if (!gpio) {
    gpio = gpio_open_by_shadow("BMC_PPIN"); // GPIOB5
    if (!gpio)
      return;
  }
  if( !is_ppin_triggered )
  {
    gpio_set_value(gpio, GPIO_VALUE_LOW);
    usleep(1000);
    gpio_set_value(gpio, GPIO_VALUE_HIGH);
    is_ppin_triggered = true;
  }
}

static inline long int reset_timer(long int *val) {
  pthread_mutex_lock(&timer_mutex);
  *val = 0;
  pthread_mutex_unlock(&timer_mutex);
  return *val;
}
static inline long int increase_timer(long int *val) {
  pthread_mutex_lock(&timer_mutex);
  (*val)++;
  pthread_mutex_unlock(&timer_mutex);
  return *val;
}
static inline long int decrease_timer(long int *val) {
  pthread_mutex_lock(&timer_mutex);
  (*val)--;
  pthread_mutex_unlock(&timer_mutex);
  return *val;
}

struct delayed_log {
  useconds_t usec;
  char msg[1024];
};
// Thread for delay event
static void *
delay_log(void *arg)
{
  struct delayed_log* log = (struct delayed_log*)arg;

  pthread_detach(pthread_self());

  if (arg) {
    usleep(log->usec);
    syslog(LOG_CRIT, log->msg);

    free(arg);
  }

  pthread_exit(NULL);
}

static void log_gpio_change(gpiopoll_pin_t *gp, gpio_value_t value, useconds_t log_delay)
{
  const struct gpiopoll_config *cfg = gpio_poll_get_config(gp);
  assert(cfg);
  if (log_delay == 0) {
    syslog(LOG_CRIT, "%s: %s - %s\n", value ? "DEASSERT": "ASSERT", cfg->description, cfg->shadow);
  } else {
    pthread_t tid_delay_log;
    struct delayed_log *log = (struct delayed_log *)malloc(sizeof(struct delayed_log));
    if (log) {
      log->usec = log_delay;
      snprintf(log->msg, 1024, "%s: %s - %s\n", value ? "DEASSERT" : "ASSERT", cfg->description, cfg->shadow);
      if (pthread_create(&tid_delay_log, NULL, delay_log, (void *)log)) {
        free(log);
        log = NULL;
      }
    }
    if (!log) {
      syslog(LOG_CRIT, "%s: %s - %s\n", value ? "DEASSERT": "ASSERT", cfg->description, cfg->shadow);
    }
  }
}

// On some board versions, we need to reset the IO expander
// as a software workaround for issues in the CPLD.
static void usb_debug_card_check_hotfix(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t value)
{
  static uint8_t odm_id = 0xff;
  static uint8_t board_rev = 0xff;

  if (odm_id == 0xff) {
    pal_get_platform_id(&odm_id);
    odm_id = odm_id & 0x1;
  }
  if (board_rev == 0xff) {
    pal_get_board_rev_id(&board_rev);
  }

  if (odm_id == 0 && board_rev <= BOARD_REV_DVT) {
    if(value == GPIO_VALUE_HIGH)
      usb_dbg_reset();
  }
  log_gpio_change(desc, value, 0);
}

// Event Handler for GPIOR5 platform reset changes
static void platform_reset_event_handle(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  // Use GPIOR5 to filter some gpio logging
  reset_timer(&reset_sec);
  TOUCH("/tmp/rst_touch");
  log_gpio_change(gp, curr, 0);
  if (MCERR_IERR_assert == true) {
    pal_second_crashdump_chk();
    MCERR_IERR_assert = false;
  }
}

static void cpu0_thermtrip(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  if (curr == GPIO_VALUE_HIGH) {
    pal_add_cri_sel("CPU0 thermtrip DEASSERT");
  } else {
    pal_add_cri_sel("CPU0 thermtrip ASSERT");
  }
  log_gpio_change(desc, curr, 0);
}

static void cpu1_thermtrip(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  if (curr == GPIO_VALUE_HIGH) {
    pal_add_cri_sel("CPU1 thermtrip DEASSERT");
  } else {
    pal_add_cri_sel("CPU1 thermtrip ASSERT");
  }
  log_gpio_change(desc, curr, 0);
}

static void irq_uv_detect(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  log_gpio_change(gp, curr, 20*1000);
}

static void power_ok(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  reset_timer(&power_on_sec);
  log_gpio_change(gp, curr, 0);
}

static void slps4(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  pal_check_power_sts();
}

//FM_UARTSW_MSB_N, FM_UARTSW_LSB_N
static void uart_switch(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  g_uart_switch_count = 2;
  pal_uart_switch_for_led_ctrl();
}

static void pwr_on_push_button(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  pal_set_restart_cause(FRU_MB, RESTART_CAUSE_PWR_ON_PUSH_BUTTON);
  log_gpio_change(gp, curr, 0);
}

static void pwr_on_reset(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  pal_set_restart_cause(FRU_MB, RESTART_CAUSE_RESET_PUSH_BUTTON);
  log_gpio_change(desc, curr, 0);
}

// Generic Event Handler for GPIO changes
static void gpio_event_handle(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  log_gpio_change(gp, curr, 0);
}

static void
*smi_timer()
{

  int smi_timeout_count = 1;  
  int smi_timeout_threshold = 90;
  bool is_issue_event = false;
  gpio_desc_t *gpio = gpio_open_by_shadow("FM_BIOS_SMI_ACTIVE_N"); // GPIOG7
  gpio_value_t value;

  if (!gpio) {
    syslog(LOG_CRIT, "SMI PIN monitoring not functional");
    return NULL;
  }
 
#ifdef SMI_DEBUG
  syslog(LOG_WARNING, "[%s][%lu] Timer is started.\n", __func__, pthread_self());
  syslog(LOG_WARNING, "[%s] Get GPIO Num: %d", __func__, pin_num);
#endif
  
  while(1)
  {
    if (gpio_get_value(gpio, &value)) {
      syslog(LOG_WARNING, "Could not get current SMI GPIO value");
      sleep(1);
      continue;
    }
    if ( GPIO_VALUE_LOW == value) 
    {
      smi_timeout_count++;
    }
    else if ( GPIO_VALUE_HIGH == value)
    {
      smi_timeout_count = 0;
    }

#ifdef SMI_DEBUG
    syslog(LOG_WARNING, "[%s][%lu] smi_timeout_count[%d] == smi_timeout_threshold[%d]\n", __func__, pthread_self(), smi_timeout_count, smi_timeout_threshold);
#endif
    
    if ( smi_timeout_count == smi_timeout_threshold )
    {
      syslog(LOG_CRIT, "ASSERT: GPIOG7-FM_BIOS_SMI_ACTIVE_N\n");
      is_issue_event = true;
    }
    else if ( (true == is_issue_event) && (0 == smi_timeout_count) )
    {
      syslog(LOG_CRIT, "DEASSERT: GPIOG7-FM_BIOS_SMI_ACTIVE_N\n");
      is_issue_event = false;
    }

    //sleep periodically.
    sleep(1);
#ifdef SMI_DEBUG
    syslog(LOG_WARNING, "[%s][%lu] count=%d\n", __func__, pthread_self(), smi_timeout_count);
#endif
  }

  return NULL;
}

#define SERVER_POWER_CHECK(power_on_time)  \
  do {                                     \
    uint8_t status = 0;                    \
    pal_get_server_power(FRU_MB, &status); \
    if (status != SERVER_POWER_ON) {       \
      return;                              \
    }                                      \
    if (power_on_sec < power_on_time) {    \
      return;                              \
    }                                      \
  }while(0)

static gpio_value_t gpio_get(const char *shadow)
{
  gpio_value_t value = GPIO_VALUE_INVALID;
  gpio_desc_t *desc = gpio_open_by_shadow(shadow);
  if (!desc) {
    syslog(LOG_CRIT, "Open failed for GPIO: %s\n", shadow);
    return GPIO_VALUE_INVALID;
  }
  if (gpio_get_value(desc, &value)) {
    syslog(LOG_CRIT, "Get failed for GPIO: %s\n", shadow);
    value = GPIO_VALUE_INVALID;
  }
  gpio_close(desc);
  return value;
}

static void prochot_reason(char *reason)
{
  if (gpio_get("IRQ_UV_DETECT_N") == GPIO_VALUE_LOW)
    strcpy(reason, "UV");
  if (gpio_get("IRQ_OC_DETECT_N") == GPIO_VALUE_LOW)
    strcpy(reason, "OC");
  if (gpio_get("FM_HSC_TIMER_EXP_N") == GPIO_VALUE_LOW)
    strcpy(reason, "timer exp");
  if (gpio_get("IRQ_SML1_PMBUS_ALERT_N") == GPIO_VALUE_LOW)
    strcpy(reason, "PMBus alert");
}

static void cpu_prochot(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  char cmd[128] = {0};
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);
  assert(cfg);
  SERVER_POWER_CHECK(3);
  //LCD debug card critical SEL support
  strcat(cmd, "CPU FPH");
  if (curr) {
    strcat(cmd, " DEASSERT");
    syslog(LOG_CRIT, "DEASSERT: %s - %s\n",
      cfg->description, cfg->shadow);
  } else {
    char reason[32] = "";
    strcat(cmd, " by ");
    prochot_reason(reason);
    strcat(cmd, reason);
    syslog(LOG_CRIT, "ASSERT: %s - %s (reason: %s)\n",
      cfg->description, cfg->shadow, reason);
    strcat(cmd, " ASSERT");
  }
  pal_add_cri_sel(cmd);
}

void caterr(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  SERVER_POWER_CHECK(3);
  CATERR_irq++;
}

void msmi(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  SERVER_POWER_CHECK(3);
  MSMI_irq++;
}

void server_power_on_errors(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  SERVER_POWER_CHECK(3);
  log_gpio_change(desc, curr, 0);
}

void server_power_on_errors_late(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  SERVER_POWER_CHECK(5);
  log_gpio_change(desc, curr, 0);
}

// Generic Event Handler for GPIO changes, but only logs event when MB is ON
static void gpio_event_handle_power(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  SERVER_POWER_CHECK(0);
  log_gpio_change(gp, curr, 0);
}

static void gpio_event_handle_thermtrip(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  uint8_t status = 0;

  if (gpio_get("RST_BMC_PLTRST_BUF_N") == GPIO_VALUE_LOW)
    return;
  pal_get_server_power(FRU_MB, &status);
  if (status != SERVER_POWER_ON)
    return;
  // Filter false positives during reset.
  if (reset_sec < 10)
    return;
  log_gpio_change(gp, curr, 0);
}

// Generic Event Handler for GPIO changes, but only logs event when PLTRST_N is high and power on
static void gpio_event_handle_PLTRST(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr)
{
  uint8_t status = 0;

  if (gpio_get("RST_BMC_PLTRST_BUF_N") == GPIO_VALUE_LOW)
    return;
  pal_get_server_power(FRU_MB, &status);
  if (status != SERVER_POWER_ON)
    return;
  log_gpio_change(gp, curr, 0);
}

void cpu_present(gpiopoll_pin_t *gpdesc, gpio_value_t value)
{
  log_gpio_change(gpdesc, value, 0);
}

void por_caterr(gpiopoll_pin_t *gpdesc, gpio_value_t value)
{
  uint8_t status = 0;
  pal_get_server_power(FRU_MB, &status);
  if (status && value == GPIO_VALUE_LOW) {
    CATERR_irq++;
  }
}

void por_msmi(gpiopoll_pin_t *gpdesc, gpio_value_t value)
{
  uint8_t status = 0;
  pal_get_server_power(FRU_MB, &status);
  if (status && value == GPIO_VALUE_LOW) {
    MSMI_irq++;
  }
}

// GPIO table to be monitored
static struct gpiopoll_config g_gpios[] = {
  // shadow, description, edge, handler, oneshot
  {"PWRGD_SYS_PWROK", "GPIOB6", GPIO_EDGE_BOTH, power_ok, NULL},
  {"IRQ_PVDDQ_GHJ_VRHOT_LVT3_N", "GPIOB7", GPIO_EDGE_BOTH, gpio_event_handle_power, NULL},
  {"FM_CPU_ERR0_LVT3_BMC_N", "GPIOD6", GPIO_EDGE_BOTH, server_power_on_errors, NULL},
  {"FM_CPU_ERR1_LVT3_BMC_N", "GPIOD7", GPIO_EDGE_BOTH, server_power_on_errors, NULL},
  {"RST_SYSTEM_BTN_N", "GPIOE0", GPIO_EDGE_BOTH, pwr_on_reset, NULL},
  {"FM_PWR_BTN_N", "GPIOE2", GPIO_EDGE_BOTH, pwr_on_push_button, NULL},
  {"FP_NMI_BTN_N", "GPIOE4", GPIO_EDGE_BOTH, gpio_event_handle, NULL},
  {"FM_CPU0_PROCHOT_LVT3_BMC_N", "GPIOE6", GPIO_EDGE_BOTH, cpu_prochot, NULL},
  {"FM_CPU1_PROCHOT_LVT3_BMC_N", "GPIOE7", GPIO_EDGE_BOTH, cpu_prochot, NULL},
  {"IRQ_PVDDQ_ABC_VRHOT_LVT3_N", "GPIOF0", GPIO_EDGE_BOTH, gpio_event_handle_power, NULL},
  {"IRQ_PVCCIN_CPU0_VRHOT_LVC3_N", "GPIOF2", GPIO_EDGE_BOTH, gpio_event_handle_power, NULL},
  {"IRQ_PVCCIN_CPU1_VRHOT_LVC3_N", "GPIOF3", GPIO_EDGE_BOTH, gpio_event_handle_power, NULL},
  {"IRQ_PVDDQ_KLM_VRHOT_LVT3_N", "GPIOF4", GPIO_EDGE_BOTH, gpio_event_handle_power, NULL},
  {"FM_CPU_ERR2_LVT3_N", "GPIOG0", GPIO_EDGE_BOTH, server_power_on_errors, NULL},
  {"FM_CPU_CATERR_LVT3_N", "GPIOG1", GPIO_EDGE_FALLING, caterr, por_caterr},
  {"FM_PCH_BMC_THERMTRIP_N", "GPIOG2", GPIO_EDGE_BOTH, gpio_event_handle_thermtrip, NULL},
  {"FM_CPU0_SKTOCC_LVT3_N", "GPIOG3", GPIO_EDGE_BOTH, gpio_event_handle_power, cpu_present},
  {"FM_CPU0_FIVR_FAULT_LVT3_N", "GPIOI0", GPIO_EDGE_BOTH, gpio_event_handle_PLTRST, NULL},
  {"FM_CPU1_FIVR_FAULT_LVT3_N", "GPIOI1", GPIO_EDGE_BOTH, gpio_event_handle_PLTRST, NULL},
  {"IRQ_UV_DETECT_N", "GPIOL0", GPIO_EDGE_BOTH, irq_uv_detect, NULL},
  {"IRQ_OC_DETECT_N", "GPIOL1", GPIO_EDGE_BOTH, gpio_event_handle, NULL},
  {"FM_HSC_TIMER_EXP_N", "GPIOL2", GPIO_EDGE_BOTH, gpio_event_handle, NULL},
  {"FM_MEM_THERM_EVENT_PCH_N", "GPIOL4", GPIO_EDGE_BOTH, gpio_event_handle_power, NULL},
  {"FM_CPU0_RC_ERROR_N", "GPIOM0", GPIO_EDGE_BOTH, gpio_event_handle_power, NULL},
  {"FM_CPU1_RC_ERROR_N", "GPIOM1", GPIO_EDGE_BOTH, gpio_event_handle_power, NULL},
  {"FM_CPU0_THERMTRIP_LATCH_LVT3_N", "GPIOM4", GPIO_EDGE_BOTH, cpu0_thermtrip, NULL},
  {"FM_CPU1_THERMTRIP_LATCH_LVT3_N", "GPIOM5", GPIO_EDGE_BOTH, cpu1_thermtrip, NULL},
  {"FM_CPU_MSMI_LVT3_N", "GPION3", GPIO_EDGE_FALLING, msmi, por_msmi},
  {"FM_POST_CARD_PRES_BMC_N", "GPIOQ6", GPIO_EDGE_BOTH, usb_debug_card_check_hotfix, NULL},
  {"RST_BMC_PLTRST_BUF_N", "GPIOR5", GPIO_EDGE_BOTH, platform_reset_event_handle, NULL},
  {"FM_THROTTLE_N", "GPIOS0", GPIO_EDGE_BOTH, server_power_on_errors_late, NULL},
  {"H_CPU0_MEMABC_MEMHOT_LVT3_BMC_N", "GPIOX4", GPIO_EDGE_BOTH, gpio_event_handle_power, NULL},
  {"H_CPU0_MEMDEF_MEMHOT_LVT3_BMC_N", "GPIOX5", GPIO_EDGE_BOTH, server_power_on_errors, NULL},
  {"H_CPU1_MEMGHJ_MEMHOT_LVT3_BMC_N", "GPIOX6", GPIO_EDGE_BOTH, gpio_event_handle_power, NULL},
  {"H_CPU1_MEMKLM_MEMHOT_LVT3_BMC_N", "GPIOX7", GPIO_EDGE_BOTH, gpio_event_handle_power, NULL},
  {"FM_SLPS4_N", "GPIOY1", GPIO_EDGE_FALLING, slps4, NULL},
  {"IRQ_PVDDQ_DEF_VRHOT_LVT3_N", "GPIOZ2", GPIO_EDGE_BOTH, gpio_event_handle_power, NULL},
  {"FM_CPU1_SKTOCC_LVT3_N", "GPIOAA0", GPIO_EDGE_BOTH, gpio_event_handle_power, cpu_present},
  {"IRQ_SML1_PMBUS_ALERT_N", "GPIOAA1", GPIO_EDGE_BOTH, gpio_event_handle, NULL},
  {"IRQ_HSC_FAULT_N", "GPIOAB0", GPIO_EDGE_BOTH, gpio_event_handle, NULL},
  {"IRQ_DIMM_SAVE_LVT3_N", "GPIOD4", GPIO_EDGE_BOTH, gpio_event_handle_power, NULL},
  {"FM_UARTSW_LSB_N", "GPIOQ4", GPIO_EDGE_BOTH, uart_switch, NULL},
  {"FM_UARTSW_MSB_N", "GPIOQ5", GPIO_EDGE_BOTH, uart_switch, NULL},
};

// Thread for gpio timer
static void *
gpio_timer() {
  uint8_t status = 0;
  uint8_t fru = 1;
  long int pot;
  char str[MAX_VALUE_LEN] = {0};
  int tread_time = 0 ;

  while (1) {
    sleep(1);

    pal_get_server_power(fru, &status);
    if (status == SERVER_POWER_ON) {
      increase_timer(&reset_sec);
      pot = increase_timer(&power_on_sec);
    } else {
      pot = decrease_timer(&power_on_sec);
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
      trigger_ppin();
    }
    if ( g_uart_switch_count > 0) {
      if ( --g_uart_switch_count == 0 )
        pal_mmap(AST_GPIO_BASE, UARTSW_OFFSET, UARTSW_BY_DEBUG, 0);
    }
  }

  return NULL;
}

static void ierr_mcerr_event_log(bool is_caterr, const char *err_type)
{
  char temp_log[128] = {0};
  char temp_syslog[128] = {0};
  char cpu_str[32] = "";
  int CPU_num = pal_CPU_error_num_chk( is_caterr );

  if (CPU_num == 2)
    strcpy(cpu_str, "0/1");
  else if (CPU_num != -1)
    sprintf(cpu_str, "%d", CPU_num);

  sprintf(temp_syslog, "ASSERT: CPU%s %s\n", cpu_str, err_type);
  sprintf(temp_log, "CPU%s %s", cpu_str, err_type);
  syslog(LOG_CRIT, temp_syslog);
  pal_add_cri_sel(temp_log);
}

// Thread for IERR/MCERR event detect
static void *
ierr_mcerr_event_handler() {
  uint8_t CATERR_ierr_time_count = 0;
  uint8_t MSMI_ierr_time_count = 0;
  gpio_value_t value;
  gpio_desc_t *caterr = gpio_open_by_shadow("FM_CPU_CATERR_LVT3_N");
  gpio_desc_t *msmi = gpio_open_by_shadow("FM_CPU_MSMI_LVT3_N");
  if (!caterr) {
    return NULL;
  }
  if (!msmi) {
    gpio_close(caterr);
    return NULL;
  }

  while (1) {
    if (CATERR_irq > 0) {
      CATERR_ierr_time_count++;
      if (CATERR_ierr_time_count == 2) {
        if (CATERR_irq == 1) {
          //FM_CPU_CATERR_LVT3_N
          MCERR_IERR_assert = true;
          if (gpio_get_value(caterr, &value)) {
            syslog(LOG_CRIT, "Getting CATERR GPIO failed");
            break;
          }
          if (value == GPIO_VALUE_LOW) {
            ierr_mcerr_event_log(true, "IERR/CATERR");
          } else {
            ierr_mcerr_event_log(true, "MCERR/CATERR");
          }
          CATERR_irq--;
          CATERR_ierr_time_count = 0;
          set_fault_led(true);
          system("/usr/local/bin/autodump.sh &");
        } else if (CATERR_irq > 1) {
          while (CATERR_irq > 1) {
            ierr_mcerr_event_log(true, "MCERR/CATERR");
            CATERR_irq = CATERR_irq - 1;
          }
          CATERR_ierr_time_count = 1;
        }
      }
    }

    if (MSMI_irq > 0) {
      MSMI_ierr_time_count++;
      if (MSMI_ierr_time_count == 2) {
        if (MSMI_irq == 1) {
          //FM_CPU_MSMI_LVT3_N
          MCERR_IERR_assert = true;
          if (gpio_get_value(msmi, &value)) {
            syslog(LOG_CRIT, "Getting MSMI GPIO failed");
            break;
          }
          if (value == GPIO_VALUE_LOW) {
            ierr_mcerr_event_log(false, "IERR/MSMI");
          } else {
            ierr_mcerr_event_log(false, "MCERR/MSMI");
          }
          MSMI_irq--;
          MSMI_ierr_time_count = 0;
          set_fault_led(true);
          system("/usr/local/bin/autodump.sh &");
        } else if (MSMI_irq > 1) {
          while (MSMI_irq > 1) {
            ierr_mcerr_event_log(false, "MCERR/MSMI");
            MSMI_irq = MSMI_irq - 1;
          }
          MSMI_ierr_time_count = 1;
        }
      }
    }
    usleep(25000); //25ms
  }
  gpio_close(caterr);
  gpio_close(msmi);
  return NULL;
}

int
main(int argc, char **argv) {
  int rc, pid_file;
  pthread_t tid_ierr_mcerr_event;
  pthread_t tid_gpio_timer;
  pthread_t tid_smi_timer;
  gpiopoll_desc_t *polldesc;

  pid_file = open("/var/run/gpiod.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      syslog(LOG_ERR, "Another gpiod instance is running...\n");
      exit(-1);
    }
  } else {

    daemon(0,1);
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

    //Create thread for SMI check
    if (pthread_create(&tid_smi_timer, NULL, smi_timer, NULL) < 0) 
    {
      syslog(LOG_WARNING, "pthread_create for smi_handler fail\n");
      exit(1);
    }   

    polldesc = gpio_poll_open(g_gpios, sizeof(g_gpios)/sizeof(g_gpios[0]));
    if (!polldesc) {
      syslog(LOG_CRIT, "Cannot start poll operation on GPIOs");
    } else {
      if (gpio_poll(polldesc, POLL_TIMEOUT)) {
        syslog(LOG_CRIT, "Poll returned error");
      }
      gpio_poll_close(polldesc);
    }
  }

  return 0;
}
