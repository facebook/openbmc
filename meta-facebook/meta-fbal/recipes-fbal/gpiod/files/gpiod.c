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
static uint8_t g_msmi_irq = 0;
static bool g_mcerr_ierr_assert = false;
static int g_uart_switch_count = 0;
static long int g_reset_sec = 0;
static long int g_power_on_sec = 0;
static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t caterr_mutex = PTHREAD_MUTEX_INITIALIZER;
static gpio_value_t g_server_power_status = GPIO_VALUE_INVALID;
static bool g_cpu_pwrgd_trig = false;

// For monitoring GPIOs on IO expender
struct gpioexppoll_config {
  char shadow[32];
  char desc[64];
  char pwr_st;
  gpio_edge_t edge;
  gpio_value_t last;
  gpio_value_t curr;
  void (*init_gpio)(char* shadow, char* desc, gpio_value_t value);
  void (*handler)(char* shadow, char* desc, gpio_value_t value);
};

// For thermaltrip config
struct cpld_register_config {
  char shadow[32];
  uint8_t addr;
  uint8_t offset;
};

enum {
  STBY,
  PS_ON,
  PS_ON_3S,
};

void cpu_vr_hot_init(char* shadow, char* desc, gpio_value_t value) {
  if(value == GPIO_VALUE_LOW ) {
    syslog(LOG_CRIT, "FRU: %d %s: %s - %s\n", FRU_MB, value ? "DEASSERT": "ASSERT", desc, shadow);
  }
  return;
}

void cpu_skt_init(char* shadow, char* desc, gpio_value_t value) {
  int cpu_id = strcmp(shadow, "FM_CPU0_SKTOCC_LVT3_PLD_N") == 0 ? 0 : 1;
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
    syslog(LOG_CRIT, "FRU: %d %s: %s - %s\n", FRU_MB, value ? "DEASSERT": "ASSERT", desc, shadow);
  }
}

void ioex_gpios_event_handle(char* shadow, char* desc, gpio_value_t value) {
  syslog(LOG_CRIT, "FRU: %d %s: %s - %s\n", FRU_MB, value ? "DEASSERT": "ASSERT", desc, shadow);
  return;
}

static
void cpu0_pvqq_abc_handler(char* shadow, char* desc, gpio_value_t value) {
  syslog(LOG_CRIT, "FRU: %d CPU0 PVDDQ ABC VR HOT Warning %s\n", FRU_MB,
         value ? "Deassertion": "Assertion");
}

static
void cpu0_pvqq_def_handler(char* shadow, char* desc, gpio_value_t value) {
  syslog(LOG_CRIT, "FRU: %d CPU0 PVDDQ DEF VR HOT Warning %s\n", FRU_MB,
         value ? "Deassertion": "Assertion");
}

static
void cpu1_pvqq_abc_handler(char* shadow, char* desc, gpio_value_t value) {
  syslog(LOG_CRIT, "FRU: %d CPU1 PVDDQ ABC VR HOT Warning %s\n", FRU_MB,
         value ? "Deassertion": "Assertion");
}

static
void cpu1_pvqq_def_handler(char* shadow, char* desc, gpio_value_t value) {
  syslog(LOG_CRIT, "FRU: %d CPU1 PVDDQ DEF VR HOT Warning %s\n", FRU_MB,
         value ? "Deassertion": "Assertion");
}

static
void cpu0_pvccin_vr_hot_handler(char* shadow, char* desc, gpio_value_t value) {
  syslog(LOG_CRIT, "FRU: %d CPU0 PVCCIN VR HOT Warning %s\n", FRU_MB,
         value ? "Deassertion": "Assertion");
}

static
void cpu1_pvccin_vr_hot_handler(char* shadow, char* desc, gpio_value_t value) {
  syslog(LOG_CRIT, "FRU: %d CPU1 PVCCIN VR HOT Warning %s\n", FRU_MB,
         value ? "Deassertion": "Assertion");
}

static
void cpu0_pvccin_pwr_in_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  syslog(LOG_CRIT, "FRU: %d CPU0 PVCCIN POWER FAULT %s\n", FRU_MB,
         curr ? "Deassertion": "Assertion");
}

static
void cpu1_pvccin_pwr_in_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  syslog(LOG_CRIT, "FRU: %d CPU1 PVCCIN POWER FAULT %s\n", FRU_MB,
         curr ? "Deassertion": "Assertion");
}


static
void sml1_pmbus_alert_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  syslog(LOG_CRIT, "FRU: %d HSC OC Warning %s\n", FRU_MB,
         curr ? "Deassertion": "Assertion");
}

static
void oc_detect_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  syslog(LOG_CRIT, "FRU: %d HSC Surge Current Warning %s\n", FRU_MB,
         curr ? "Deassertion": "Assertion");
}

static
void uv_detect_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  syslog(LOG_CRIT, "FRU: %d HSC Under Voltage Warning %s\n", FRU_MB,
         curr ? "Deassertion": "Assertion");
}

static
void hsc_timer_exp_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  syslog(LOG_CRIT, "FRU: %d HSC OCP Fault Warning %s\n", FRU_MB,
         curr ? "Deassertion": "Assertion");
}

//PCA9539 Address 0XEE
struct gpioexppoll_config ioex0_gpios[] = {
  {"IRQ_PVCCIN_CPU0_VRHOT_LVC3_N", "ExIO_0", PS_ON, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, GPIO_VALUE_INVALID, cpu_vr_hot_init, cpu0_pvccin_vr_hot_handler},
  {"IRQ_PVCCIN_CPU1_VRHOT_LVC3_N", "ExIO_1", PS_ON, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, GPIO_VALUE_INVALID, cpu_vr_hot_init, cpu1_pvccin_vr_hot_handler},
  {"IRQ_PVDDQ_ABC_CPU0_VRHOT_LVC3_N", "ExIO_2", PS_ON, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, GPIO_VALUE_INVALID, cpu_vr_hot_init, cpu0_pvqq_abc_handler},
  {"IRQ_PVDDQ_DEF_CPU0_VRHOT_LVC3_N", "ExIO_3", PS_ON, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, GPIO_VALUE_INVALID, cpu_vr_hot_init, cpu0_pvqq_def_handler},
  {"IRQ_PVDDQ_ABC_CPU1_VRHOT_LVC3_N", "ExIO_4", PS_ON, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, GPIO_VALUE_INVALID, cpu_vr_hot_init, cpu1_pvqq_abc_handler},
  {"IRQ_PVDDQ_DEF_CPU1_VRHOT_LVC3_N", "ExIO_5", PS_ON, GPIO_EDGE_BOTH, GPIO_VALUE_INVALID, GPIO_VALUE_INVALID, cpu_vr_hot_init, cpu1_pvqq_def_handler},
  {"FM_CPU0_SKTOCC_LVT3_PLD_N", "ExIO_6", STBY, GPIO_EDGE_RISING, GPIO_VALUE_INVALID, GPIO_VALUE_INVALID, cpu_skt_init, ioex_gpios_event_handle},
  {"FM_CPU1_SKTOCC_LVT3_PLD_N", "ExIO_7", STBY, GPIO_EDGE_RISING, GPIO_VALUE_INVALID, GPIO_VALUE_INVALID, cpu_skt_init, ioex_gpios_event_handle},
};

static gpio_value_t gpio_get(const char *shadow)
{
  gpio_value_t value = GPIO_VALUE_INVALID;
  gpio_desc_t *desc = gpio_open_by_shadow(shadow);
  if (!desc) {
    syslog(LOG_CRIT, "FRU: %d Open failed for GPIO: %s\n", FRU_MB, shadow);
    return GPIO_VALUE_INVALID;
  }
  if (gpio_get_value(desc, &value)) {
    syslog(LOG_CRIT, "FRU: %d Get failed for GPIO: %s\n", FRU_MB, shadow);
    value = GPIO_VALUE_INVALID;
  }
  gpio_close(desc);
  return value;
}

/*
static int gpio_set(const char *shadow, gpio_value_t val)
{
  gpio_desc_t *desc = gpio_open_by_shadow(shadow);

  if (!desc) {
    syslog(LOG_CRIT, "Open failed for GPIO: %s\n", shadow);
    return -1;
  }
  if (gpio_set_value(desc, val)) {
    syslog(LOG_CRIT, "Set failed for GPIO: %s\n", shadow);
    gpio_close(desc);
    return -1;
  }
  gpio_close(desc);
  return 0;
}
*/

static void* ioex0_monitor()
{
  int i;
  uint8_t status;
  uint8_t assert = false;
  static uint8_t init_flag[8];
  gpio_value_t curr;

  while (1) {

    for (i = 0; i < 8; i++) {
      if(ioex0_gpios[i].pwr_st != STBY) {
        if (pal_get_server_power(FRU_MB, &status) < 0 || status == SERVER_POWER_OFF) {
          continue;
        }
      }

      curr = gpio_get(ioex0_gpios[i].shadow);
      if(curr == ioex0_gpios[i].last) {
        continue;
      }

      if(init_flag[i] != true) {
        if(ioex0_gpios[i].init_gpio != NULL) {
          ioex0_gpios[i].init_gpio(ioex0_gpios[i].shadow, ioex0_gpios[i].desc, curr);
        }
        ioex0_gpios[i].last = curr;
        init_flag[i] = true;
#ifdef DEBUG
        syslog(LOG_DEBUG, "gpio %s initial value=%x\n", ioex0_gpios[i].shadow, curr);
#endif
        continue;
      }

#ifdef DEBUG
      syslog(LOG_DEBUG, "edge gpio %s value=%x\n", ioex0_gpios[i].shadow, curr);
#endif
      switch (ioex0_gpios[i].edge) {
      case GPIO_EDGE_FALLING:
        if(curr == GPIO_VALUE_LOW) {
          assert = true;
        } else {
          assert = false;
        }
        break;
      case GPIO_EDGE_RISING:
        if(curr == GPIO_VALUE_HIGH) {
          assert = true;
        } else {
          assert = false;
        }
        break;
      case GPIO_EDGE_BOTH:
        assert = true;
        break;
      default:
        assert = false;
        break;
      }

      if((assert == true) && (ioex0_gpios[i].handler != NULL)) {
        ioex0_gpios[i].handler(ioex0_gpios[i].shadow, ioex0_gpios[i].desc, curr);
      }
      ioex0_gpios[i].last = curr;
    }
    sleep(1);
  }
  return NULL;
}

static void
log_gpio_change(gpiopoll_pin_t *desc, gpio_value_t value, useconds_t log_delay) {
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);
  assert(cfg);
  syslog(LOG_CRIT, "FRU: %d %s: %s - %s\n", FRU_MB, value ? "DEASSERT": "ASSERT",
         cfg->description, cfg->shadow);
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

  value = gpio_get("RST_PLTRST_BMC_N");
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

//FIVR Fault Handler
static void fivr_fault_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  uint8_t status = 0;
  gpio_value_t value;

  value = gpio_get("RST_PLTRST_BMC_N");
  if(value < 0 || value == GPIO_VALUE_LOW) {
    return;
  }

  pal_get_server_power(FRU_MB, &status);
  if (status != SERVER_POWER_ON)
    return;
  log_gpio_change(desc, curr, 0);
}

//PROCHOT Handler
static void prochot_reason(char *reason)
{
  if (gpio_get("IRQ_UV_DETECT_N") == GPIO_VALUE_LOW)
    strcpy(reason, "UV");
  if (gpio_get("IRQ_OC_DETECT_N") == GPIO_VALUE_LOW)
    strcpy(reason, "OC");
  if (gpio_get("FM_HSC_TIMER_EXP_N") == GPIO_VALUE_LOW)
    strcpy(reason, "timer exp");
  if (gpio_get("IRQ_SML1_PMBUS_BMC_ALERT_N") == GPIO_VALUE_LOW)
    strcpy(reason, "PMBus alert");
}

static void cpu_prochot_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  char cmd[128] = {0};
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);
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

static struct cpld_register_config cpld_registers[] = {
  {"FM_CPU0_THERMTRIP_LVT3_PLD_N", 0x1e, 0x20}, // GPIOA1
  {"FM_CPU1_THERMTRIP_LVT3_PLD_N", 0x1e, 0x21}, // GPIOD0
  {"FM_MEM_THERM_EVENT_CPU0_LVT3_N", 0x1e, 0x22}, // GPIOB0
  {"FM_MEM_THERM_EVENT_CPU1_LVT3_N", 0x1e, 0x23}, // GPIOB1
  {"FM_CPU_CATERR_LVT3_N", 0x16, 0x10}, // GPIOZ0
  {"FM_CPU_MSMI_LVT3_N", 0x16, 0x11}, // GPIOZ2
  {"FM_CPU_ERR0_LVT3_N", 0x16, 0x12}, // GPIOF0
  {"FM_CPU_ERR1_LVT3_N", 0x16, 0x13}, // GPIOF1
  {"FM_CPU_ERR2_LVT3_N", 0x16, 0x14}, // GPIOF2
};

static void get_cpld_register_data (const char *shadow, uint8_t *addr, uint8_t *offset) {
  int i, config_size = sizeof(cpld_registers) / sizeof(cpld_registers[0]);
  for (i = 0; i < config_size; i++) {
    if (strcmp(shadow, cpld_registers[i].shadow) == 0) {
      *addr = cpld_registers[i].addr;
      *offset = cpld_registers[i].offset;
      return;
    }
  }
}

static int cpld_register_check(const char *shadow)
{
  int fd = 0, retCode = -1;
  uint8_t bus = 4;
  uint8_t addr = 0x00;
  uint8_t offset = 0x00;
  uint8_t tlen, rlen;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  get_cpld_register_data(shadow, &addr, &offset);
  if (addr == 0 || offset == 0) {
    syslog(LOG_WARNING, "There is no cpld register can check with %s\n", shadow);
    return retCode;
  }

  fd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open %d", __func__, bus);
    return retCode;
  }

  tbuf[0] = (uint8_t)offset;
  tlen = 1;
  rlen = 1;

  retCode = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
  if (retCode == -1) {
    syslog(LOG_WARNING, "%s bus=%x slavaddr=%x offset=%x\n", __func__, bus, addr >> 1, offset);
  } else {
    retCode = (int)rbuf[0];
  }
  i2c_cdev_slave_close(fd);
  return retCode;
}

static void thermtrip_add_cri_sel(const char *shadow_name, gpio_value_t curr)
{
  char cmd[128], log_name[16], log_descript[16] = "ASSERT\0";

  if (strcmp(shadow_name, "FM_CPU0_THERMTRIP_LVT3_PLD_N") == 0)
    sprintf(log_name, "CPU0");
  else if (strcmp(shadow_name, "FM_CPU1_THERMTRIP_LVT3_PLD_N") == 0)
    sprintf(log_name, "CPU1");

  if (curr == GPIO_VALUE_HIGH)
    sprintf(log_descript, "DEASSERT");

  sprintf(cmd, "FRU: %d %s thermtrip %s", FRU_MB, log_name, log_descript);
  pal_add_cri_sel(cmd);
}

static void cpu_thermtrip_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);

  if (g_server_power_status != GPIO_VALUE_HIGH)
    return;

  if (cpld_register_check(cfg->shadow) <= 0)
    return;

  thermtrip_add_cri_sel(cfg->shadow, curr);
  log_gpio_change(desc, curr, 0);
}

static void cpu_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  int cpld_register_count = 0;
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);

  if (g_server_power_status != GPIO_VALUE_HIGH)
    return;

  cpld_register_count = cpld_register_check(cfg->shadow);
  if (cpld_register_count == 0)
    return;

  assert(cfg);
  syslog(LOG_CRIT, "FRU: %d %s: %s - %s, %d time%s\n",
  FRU_MB, curr ? "DEASSERT": "ASSERT", cfg->description, cfg->shadow,
  cpld_register_count, cpld_register_count > 1 ? "s": "");
}

static void mem_thermtrip_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  if (g_server_power_status != GPIO_VALUE_HIGH)
    return;
  log_gpio_change(desc, curr, 0);
}


// Generic Event Handler for GPIO changes
static void gpio_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  log_gpio_change(desc, curr, 0);
}

// Generic Event Handler for GPIO changes, but only logs event when MB is ON

static void gpio_event_pson_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  SERVER_POWER_CHECK(0);
  log_gpio_change(desc, curr, 0);
}

// Generic Event Handler for GPIO changes, but only logs event when MB is ON 3S
static void gpio_event_pson_3s_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
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

static int
dimm_mux_recover_needed(int fd, uint8_t addr) {
  int ret = 0;
  uint8_t spd;
  uint8_t tbuf[8] = {0};
  uint8_t rbuf[8] = {0};

  // check if DIMM_MUX is abnormal
  do {
    // DIMM_MUX
    if ((ret = retry_cond(!i2c_rdwr_msg_transfer(fd, addr, tbuf, 0, rbuf, 1), 2, 10))) {
      syslog(LOG_WARNING, "FRU: %d Failed to read DIMM_MUX status", FRU_MB);
      break;
    }
    if (rbuf[0] & 0xF) {
      syslog(LOG_WARNING, "FRU: %d Unexpected DIMM_MUX status %02x", FRU_MB, rbuf[0]);
      ret = 1;
      break;
    }

    // SPD
    for (spd = 0xa0; spd <= 0xa8; spd += 2) {
      if (!retry_cond(!i2c_rdwr_msg_transfer(fd, spd, tbuf, 0, rbuf, 1), 1, 10)) {
        syslog(LOG_WARNING, "FRU: %d Found unexpected SPD %02x", FRU_MB, spd);
        ret = 1;
        break;
      }
    }
  } while (0);

  return ret;
}

static int
dimm_mux_check(void) {
  int fd;
  int ret = 0, rc_need = 0, retry = 1;
  uint8_t bus = 4, addr = 0xe6;
  uint8_t tbuf[8] = {0};
  uint8_t rbuf[8] = {0};

  fd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open i2c-%d", __func__, bus);
    return -1;
  }

  do {
    if (!(rc_need = dimm_mux_recover_needed(fd, addr))) {
      break;
    }

    tbuf[0] = 0;
    if ((ret = retry_cond(!i2c_rdwr_msg_transfer(fd, addr, tbuf, 1, rbuf, 0), 2, 10))) {
      syslog(LOG_WARNING, "FRU: %d Failed to write DIMM_MUX", FRU_MB);
      break;
    }

    if (!(rc_need = dimm_mux_recover_needed(fd, addr))) {
      syslog(LOG_CRIT, "FRU: %d Cleared DIMM_MUX successfully", FRU_MB);
      pal_set_server_power(FRU_MB, SERVER_POWER_RESET);
      break;
    }
  } while (--retry >= 0);

  if (ret && (rc_need > 0)) {
    syslog(LOG_CRIT, "FRU: %d Failed to clear DIMM_MUX", FRU_MB);
  }

  i2c_cdev_slave_close(fd);
  return ret;
}

//CPU Power Ok Event Handler
static void
cpu_pwr_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  g_server_power_status = curr;
  g_cpu_pwrgd_trig = true;
  log_gpio_change(desc, curr, 0);
  reset_timer(&g_power_on_sec);

  // workaround for unexpected DIMM_MUX status
  if (g_server_power_status == GPIO_VALUE_HIGH) {
    dimm_mux_check();
  }
}

static void
init_cpu_pwr(gpiopoll_pin_t *desc, gpio_value_t value) {
  g_server_power_status = value;

  // workaround for unexpected DIMM_MUX status
  if (g_server_power_status == GPIO_VALUE_HIGH) {
    dimm_mux_check();
  }
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
init_msmi(gpiopoll_pin_t *desc, gpio_value_t value) {
  uint8_t status = 0;
  pal_get_server_power(FRU_MB, &status);
  if (status && value == GPIO_VALUE_LOW) {
    g_msmi_irq++;
  }
}

static void
err_caterr_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);

  SERVER_POWER_CHECK(1);

  if (cpld_register_check(cfg->shadow) == 0)
    return;

  pthread_mutex_lock(&caterr_mutex);
  g_caterr_irq++;
  pthread_mutex_unlock(&caterr_mutex);
}

static void
err_msmi_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);

  SERVER_POWER_CHECK(1);

  if (cpld_register_check(cfg->shadow) <= 0)
    return;

  pthread_mutex_lock(&caterr_mutex);
  g_msmi_irq++;
  pthread_mutex_unlock(&caterr_mutex);
}

static int
ierr_mcerr_event_log(bool is_caterr, const char *err_type) {
  char temp_log[128] = {0};
  char temp_syslog[128] = {0};
  char cpu_str[32] = "";
  int num=0;
  int ret=0;

  ret = cmd_peci_get_cpu_err_num(&num, is_caterr);
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
  uint8_t msmi_cnt = 0;
  gpio_value_t value;
  gpio_desc_t *caterr = gpio_open_by_shadow("FM_CPU_CATERR_LVT3_N");
  gpio_desc_t *msmi = gpio_open_by_shadow("FM_CPU_MSMI_LVT3_N");

  if (!caterr) {
    gpio_close(caterr);
    return NULL;
  }
  if (!msmi) {
    gpio_close(msmi);
    return NULL;
  }

  while (1) {
    if (g_caterr_irq > 0) {
      caterr_cnt++;
      if (caterr_cnt == 2) {
        if (g_caterr_irq == 1) {
          g_mcerr_ierr_assert = true;

          if (gpio_get_value(caterr, &value)) {
            syslog(LOG_CRIT, "FRU: %d Getting CATERR GPIO failed", FRU_MB);
            break;
          }

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

    if (g_msmi_irq > 0) {
      msmi_cnt++;
      if (msmi_cnt == 2) {
        if (g_msmi_irq == 1) {
          g_mcerr_ierr_assert = true;

          if (gpio_get_value(msmi, &value)) {
            syslog(LOG_CRIT, "FRU: %d Getting MSMI GPIO failed", FRU_MB);
            break;
          }

          if (value == GPIO_VALUE_LOW) {
            ierr_mcerr_event_log(false, "IERR/MSMI");
          } else {
            ierr_mcerr_event_log(false, "MCERR/MSMI");
          }

          pthread_mutex_lock(&caterr_mutex);
          g_msmi_irq--;
          pthread_mutex_unlock(&caterr_mutex);
          msmi_cnt = 0;
          if (system("/usr/local/bin/autodump.sh &")) {
            syslog(LOG_WARNING, "Failed to start crashdump\n");
          }

        } else if (g_msmi_irq > 1) {
          while (g_msmi_irq > 1) {
            ierr_mcerr_event_log(false, "MCERR/MSMI");
            pthread_mutex_lock(&caterr_mutex);
            g_msmi_irq--;
            pthread_mutex_unlock(&caterr_mutex);
          }
          msmi_cnt = 1;
        }
      }
    }
    usleep(25000); //25ms
  }

  gpio_close(caterr);
  gpio_close(msmi);
  return NULL;
}

//Uart Select on DEBUG Card Event Handler
static void
uart_select_handle(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
   g_uart_switch_count = 2;
   log_gpio_change(desc, curr, 0);
   pal_uart_select_led_set();
}

// Event Handler for GPIOF6 platform reset changes
static void platform_reset_handle(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  struct timespec ts;
  char value[MAX_VALUE_LEN];

  // Use GPIOF6 to filter some gpio logging
  reset_timer(&g_reset_sec);
  TOUCH("/tmp/rst_touch");

  clock_gettime(CLOCK_MONOTONIC, &ts);
  sprintf(value, "%lld", ts.tv_sec);

  if( curr == GPIO_VALUE_HIGH )
    kv_set("snr_pwron_flag", value, 0, 0);
  else
    kv_set("snr_pwron_flag", 0, 0, 0);

  log_gpio_change(desc, curr, 0);
}

static void
upi_detect_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
}

static void
upi_init_handler(gpiopoll_pin_t *desc, gpio_value_t value) {
  int ret;
  uint8_t mode;

  ret = pal_get_host_system_mode(&mode);
  if ( ret != 0 )
    syslog(LOG_WARNING, "%s get mode fail\n", __func__);


  //2S can't plug in UPI board, 4S can't lose UPI borad
  if ( mode == MB_2S_MODE && value == GPIO_VALUE_LOW)
      log_gpio_change(desc, value, 0);
  else if ( (mode == MB_4S_EP_MODE || mode == MB_4S_EX_MODE) &&  value == GPIO_VALUE_HIGH )
      log_gpio_change(desc, value, 0);
}

static void
platform_reset_init(gpiopoll_pin_t *desc, gpio_value_t value) {
  struct timespec ts;
  char data[MAX_VALUE_LEN];

  clock_gettime(CLOCK_MONOTONIC, &ts);
  sprintf(data, "%lld", ts.tv_sec);

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

    //Show Uart Debug Select Number 2sec
    if (g_uart_switch_count > 0) {
      if (--g_uart_switch_count == 0)
        pal_postcode_select(POSTCODE_BY_HOST);
    }
  }

  return NULL;
}

// Thread for gpio timer
static void
*power_fail_monitor() {
  static uint8_t init_cache=0;
  static uint8_t retry=1;

  while (1) {
    if ( init_cache == 0 ) {
      if ( g_power_on_sec > 0 ) {
        g_cpu_pwrgd_trig = true;
        init_cache = 1;
      }
    }

    if ( g_cpu_pwrgd_trig == true ) {
      sleep(2);
      while ( pal_check_cpld_power_fail()) {
        if( retry == 0) {
          syslog(LOG_CRIT, "Check CPLD Power Fail Falure\n");
          break;
        }
        retry--;
        sleep(1);
      }
      g_cpu_pwrgd_trig = false;
      retry=1;
    }
    sleep(2);
  }
  return NULL;
}


// GPIO table to be monitored
static struct gpiopoll_config g_gpios[] = {
  // shadow, description, edge, handler, oneshot
  {"FP_BMC_RST_BTN_N", "GPIOE0", GPIO_EDGE_BOTH, pwr_reset_handler, NULL},
  {"FM_BMC_PWR_BTN_R_N", "GPIOE2", GPIO_EDGE_BOTH, pwr_button_handler, NULL},
  {"FM_UARTSW_LSB_N", "GPIOL0", GPIO_EDGE_BOTH, uart_select_handle, NULL},
  {"FM_UARTSW_MSB_N", "GPIOL1", GPIO_EDGE_BOTH, uart_select_handle, NULL},
  {"FM_POST_CARD_PRES_BMC_N", "GPIOQ6", GPIO_EDGE_BOTH, usb_dbg_card_handler, NULL},
  {"FM_PCH_BMC_THERMTRIP_N", "GPIOG2", GPIO_EDGE_BOTH, pch_thermtrip_handler, NULL},
  {"RST_PLTRST_BMC_N", "GPIOF6", GPIO_EDGE_BOTH, platform_reset_handle, platform_reset_init},
  {"IRQ_UV_DETECT_N", "GPIOM0", GPIO_EDGE_BOTH, uv_detect_handler, NULL},
  {"IRQ_OC_DETECT_N", "GPIOM1", GPIO_EDGE_BOTH, oc_detect_handler, NULL},
  {"IRQ_HSC_FAULT_N", "GPIOL2", GPIO_EDGE_BOTH, gpio_event_handler, NULL},
  {"IRQ_SML1_PMBUS_BMC_ALERT_N", "GPIOAA1", GPIO_EDGE_BOTH, sml1_pmbus_alert_handler, NULL},
  {"FM_CPU_CATERR_LVT3_N", "GPIOZ0", GPIO_EDGE_FALLING, err_caterr_handler, init_caterr},
  {"FM_CPU_MSMI_LVT3_N", "GPIOZ2", GPIO_EDGE_FALLING, err_msmi_handler, init_msmi},
  {"FM_CPU0_THERMTRIP_LVT3_PLD_N", "GPIOA1", GPIO_EDGE_FALLING, cpu_thermtrip_handler, NULL},
  {"FM_CPU1_THERMTRIP_LVT3_PLD_N", "GPIOD0", GPIO_EDGE_FALLING, cpu_thermtrip_handler, NULL},
  {"FM_CPU0_MEMHOT_OUT_N", "GPIOU5", GPIO_EDGE_BOTH, gpio_event_pson_3s_handler, NULL},
  {"FM_CPU1_MEMHOT_OUT_N", "GPIOL3", GPIO_EDGE_BOTH, gpio_event_pson_3s_handler, NULL},
  {"FM_CPU0_FIVR_FAULT_LVT3_PLD", "GPIOB2", GPIO_EDGE_BOTH, fivr_fault_handler, NULL},
  {"FM_CPU1_FIVR_FAULT_LVT3_PLD", "GPIOB3", GPIO_EDGE_BOTH, fivr_fault_handler, NULL},
  {"FM_CPU_ERR0_LVT3_N", "GPIOF0", GPIO_EDGE_FALLING, cpu_event_handler, NULL},
  {"FM_CPU_ERR1_LVT3_N", "GPIOF1", GPIO_EDGE_FALLING, cpu_event_handler, NULL},
  {"FM_CPU_ERR2_LVT3_N", "GPIOF2", GPIO_EDGE_FALLING, cpu_event_handler, NULL},
  {"FM_MEM_THERM_EVENT_CPU0_LVT3_N", "GPIOB0", GPIO_EDGE_FALLING, mem_thermtrip_handler, NULL},
  {"FM_MEM_THERM_EVENT_CPU1_LVT3_N", "GPIOB1", GPIO_EDGE_FALLING, mem_thermtrip_handler, NULL},
  {"FM_SYS_THROTTLE_LVC3", "GPIOR7", GPIO_EDGE_BOTH, gpio_event_pson_handler, NULL},
  {"IRQ_DIMM_SAVE_LVT3_N", "GPION4", GPIO_EDGE_BOTH, gpio_event_pson_handler, NULL},
  {"FM_HSC_TIMER_EXP_N", "GPIOM2", GPIO_EDGE_BOTH, hsc_timer_exp_handler, NULL},
  {"FM_CPU0_PROCHOT_LVT3_BMC_N", "GPIOB6", GPIO_EDGE_BOTH, cpu_prochot_handler, NULL},
  {"FM_CPU1_PROCHOT_LVT3_BMC_N", "GPIOB7", GPIO_EDGE_BOTH, cpu_prochot_handler, NULL},
  {"FM_PVCCIN_CPU0_PWR_IN_ALERT_N", "GPIOAA2", GPIO_EDGE_FALLING, cpu0_pvccin_pwr_in_handler, NULL},
  {"FM_PVCCIN_CPU1_PWR_IN_ALERT_N", "GPIOAA3", GPIO_EDGE_FALLING, cpu1_pvccin_pwr_in_handler, NULL},
  {"PWRGD_CPU0_LVC3", "GPIOZ1", GPIO_EDGE_BOTH, cpu_pwr_handler, init_cpu_pwr},
  {"PRSNT_UPI_BD_1_N", "GPIOO5", GPIO_EDGE_BOTH, upi_detect_handler, upi_init_handler},
  {"PRSNT_UPI_BD_2_N", "GPIOO6", GPIO_EDGE_BOTH, upi_detect_handler, upi_init_handler},
  {"PRSNT_UPI_BD_3_N", "GPIOO7", GPIO_EDGE_BOTH, upi_detect_handler, upi_init_handler},
  {"PRSNT_UPI_BD_4_N", "GPIOP0", GPIO_EDGE_BOTH, upi_detect_handler, upi_init_handler},
};

int main(int argc, char **argv)
{
  int rc, pid_file;
  gpiopoll_desc_t *polldesc;
  pthread_t tid_ierr_mcerr_event;
  pthread_t tid_gpio_timer;
  pthread_t tid_ioex0_monitor;
  pthread_t tid_power_fail_monitor;

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

    if (pthread_create(&tid_ioex0_monitor, NULL, ioex0_monitor, NULL) < 0) {
      syslog(LOG_CRIT, "FRU: %d pthread_create for io expender monitor error", FRU_MB);
      exit(1);
    }

    if (pthread_create(&tid_power_fail_monitor, NULL, power_fail_monitor, NULL) < 0) {
      syslog(LOG_CRIT, "FRU: %d pthread_create for power fail monitor error", FRU_MB);
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
