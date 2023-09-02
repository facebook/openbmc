#include <stdio.h>
#include <syslog.h>
#include <openbmc/libgpio.h>
#include "pal.h"


int
pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t snr_num = sel[11];
  bool parsed = true;

  strcpy(error_log, "");
  switch(snr_num) {
    default:
      parsed = false;
      break;
  }

  if (parsed == true) {
    return 0;
  }

  pal_parse_sel_helper(fru, sel, error_log);
  return 0;
}

int
pal_clear_psb_cache(void) {
  return 0;
}

int
pal_check_psb_error(uint8_t head, uint8_t last) {
  return 0;
}

int pal_udbg_get_frame_total_num() {
  return 3;
}

int
pal_parse_oem_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t index = sel[11] & 0x01;
  char* yellow_track[] = {
    "Corrected Cache Error",
    "Persistent Cache Fault"
  };

  // error_log size 256
  snprintf(error_log, 256, "%s", yellow_track[index]);

  return 0;
}

bool
fru_presence_ext(uint8_t fru_id, uint8_t *status) {
  gpio_value_t gpio_value;

  switch (fru_id) {
    case FRU_NIC0:
      gpio_value = gpio_get_value_by_shadow(FM_OCP0_PRSNT);
      *status = (gpio_value == GPIO_VALUE_HIGH) ? FRU_PRSNT : FRU_NOT_PRSNT;
      return true;
    case FRU_NIC1:
      gpio_value = gpio_get_value_by_shadow(FM_OCP1_PRSNT);
      *status = (gpio_value == GPIO_VALUE_HIGH) ? FRU_PRSNT : FRU_NOT_PRSNT;
      return true;
    default:
      *status = FRU_PRSNT;
      return true;
  }
}

int
pal_set_rst_btn(uint8_t slot, uint8_t status) {
  int ret;
  gpio_desc_t *gdesc = NULL;
  gpio_value_t val;

  if (slot != FRU_MB) {
    return -1;
  }

  gdesc = gpio_open_by_shadow(FP_RST_BTN_OUT_N);
  if (gdesc == NULL)
    return -1;

  val = status? GPIO_VALUE_HIGH: GPIO_VALUE_LOW;
  ret = gpio_set_value(gdesc, val);
  if (ret != 0)
    goto error;

error:
  gpio_close(gdesc);
  return ret;
}

int
pal_toggle_rst_btn(uint8_t slot) {
  int ret;
  ret = pal_set_rst_btn(FRU_MB, 0);
  // some server miss to detect a quick pulse, so delay 100ms
  // between low to high
  msleep(100);
  ret |= pal_set_rst_btn(FRU_MB, 1);
  return ret;
}

void hgx_pwr_limit_mon (void) {
  return;
}
