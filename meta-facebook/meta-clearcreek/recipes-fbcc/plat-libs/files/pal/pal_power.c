#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <string.h>
#include <ctype.h>
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include "pal.h"

#define GPIO_POWER "FM_BMC_PWRBTN_OUT_R_N"
#define GPIO_POWER_GOOD "PWRGD_SYS_PWROK"
#define GPIO_CPU0_POWER_GOOD "PWRGD_CPU0_LVC3"
#define GPIO_POWER_RESET "RST_BMC_RSTBTN_OUT_R_N"
#define GPIO_RESET_BTN_IN "FP_BMC_RST_BTN_N"

#define DELAY_POWER_ON 1
#define DELAY_POWER_OFF 6
#define DELAY_GRACEFUL_SHUTDOWN 1
#define DELAY_POWER_CYCLE 10

// Return the front panel's Reset Button status
int
pal_get_rst_btn(uint8_t *status) {
  int ret = -1;
  gpio_value_t value;
  gpio_desc_t *desc = gpio_open_by_shadow(GPIO_RESET_BTN_IN);
  if (!desc) {
    return -1;
  }
  if (0 == gpio_get_value(desc, &value)) {
    *status = value == GPIO_VALUE_HIGH ? 0 : 1;
    ret = 0;
  }
  gpio_close(desc);
  return ret;
}

int pal_get_server_power(uint8_t fru, uint8_t *status)
{
  gpio_desc_t *desc;
  gpio_value_t value;

  if (fru != FRU_MB)
    return -1;

  desc = gpio_open_by_shadow("SYS_PWR_READY");
  if (!desc) {
#ifdef DEBUG
    syslog(LOG_WARNING, "Open GPIO SYS_PWR_READY failed");
#endif
    return -1;
  }

  if (gpio_get_value(desc, &value) < 0) {
    syslog(LOG_WARNING, "Get GPIO SYS_PWR_READY failed");
    value = GPIO_VALUE_INVALID;
  }
  gpio_close(desc);

  *status = (value == GPIO_VALUE_HIGH)? SERVER_POWER_ON: SERVER_POWER_OFF;

  return 0;
}

static int server_power_on()
{
  int ret = -1;
  gpio_desc_t *gpio = gpio_open_by_shadow("BMC_IPMI_PWR_ON");

  if (!gpio) {
    return -1;
  }
  if (gpio_set_value(gpio, GPIO_VALUE_HIGH)) {
    goto bail;
  }
  if (gpio_set_value(gpio, GPIO_VALUE_LOW)) {
    goto bail;
  }
  sleep(1);
  if (gpio_set_value(gpio, GPIO_VALUE_HIGH)) {
    goto bail;
  }
  sleep(2);

  ret = 0;
bail:
  gpio_close(gpio);
  return ret;
}

static int server_power_off()
{
  int ret = -1;
  gpio_desc_t *gpio = gpio_open_by_shadow("BMC_IPMI_PWR_ON");

  if (!gpio) {
    return -1;
  }

  if (gpio_set_value(gpio, GPIO_VALUE_HIGH)) {
    goto bail;
  }
  if (gpio_set_value(gpio, GPIO_VALUE_LOW)) {
    goto bail;
  }
  sleep(1);
  if (gpio_set_value(gpio, GPIO_VALUE_HIGH)) {
    goto bail;
  }

  ret = 0;
bail:
  gpio_close(gpio);
  return ret;
}

// Power Off, Power On, or Power Cycle
int pal_set_server_power(uint8_t fru, uint8_t cmd)
{
  uint8_t status;

  if (pal_get_server_power(fru, &status) < 0) {
    return -1;
  }

  switch (cmd) {
    case SERVER_POWER_ON:
        return status == SERVER_POWER_ON? 1: server_power_on();
      break;

    case SERVER_POWER_OFF:
        return status == SERVER_POWER_OFF? 1: server_power_off();
      break;

    case SERVER_POWER_CYCLE:
      if (status == SERVER_POWER_ON) {
        if (server_power_off())
          return -1;

        sleep(DELAY_POWER_CYCLE);

        return server_power_on();
      } else if (status == SERVER_POWER_OFF) {
        return server_power_on();
      }
      break;

    default:
      return -1;
  }

  return 0;
}

bool pal_is_server_off()
{
  uint8_t status;

  if (pal_get_server_power(FRU_MB, &status) < 0)
    return false;

  return status == SERVER_POWER_OFF? true: false;
}
