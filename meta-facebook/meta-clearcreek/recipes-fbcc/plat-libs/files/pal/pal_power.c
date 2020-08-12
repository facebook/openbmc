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
