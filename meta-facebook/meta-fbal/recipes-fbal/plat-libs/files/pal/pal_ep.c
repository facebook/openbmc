#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <syslog.h>
#include <openbmc/ipmb.h>
#include <openbmc/libgpio.h>
#include "pal.h"

#define GPIO_JBOG_PRSNT_1 "PRSNT_PCIE_CABLE_1_N"
#define GPIO_JBOG_PRSNT_2 "PRSNT_PCIE_CABLE_2_N"

static int
ep_ipmb_process(uint8_t ipmi_cmd, uint8_t netfn,
              uint8_t *txbuf, uint8_t txlen,
              uint8_t *rxbuf, uint8_t *rxlen ) {
  int ret = 0;
  uint16_t bmc_addr;

  ret = pal_get_bmc_ipmb_slave_addr(&bmc_addr, EP_I2C_BUS_NUMBER);
  if(ret != 0) {
    return ret;
  }

  return lib_ipmb_send_request(ipmi_cmd, netfn,
                               txbuf, txlen,
                               rxbuf, rxlen,
                               EP_I2C_BUS_NUMBER, EP_I2C_SLAVE_ADDR, bmc_addr);
}

static int
cmd_ep_set_system_mode(uint8_t mode) {
  uint8_t ipmi_cmd = CMD_OEM_ZION_SET_SYSTEM_MODE;
  uint8_t netfn = NETFN_OEM_ZION_REQ;
  uint8_t tbuf[8];
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen;

  tbuf[0] = mode;
  return ep_ipmb_process(ipmi_cmd, netfn, tbuf, 1, rbuf, &rlen);
}

static int
cmd_ep_sled_cycle(void) {
  uint8_t ipmi_cmd = CMD_CHASSIS_CONTROL;
  uint8_t netfn = NETFN_CHASSIS_REQ;
  uint8_t tbuf[8];
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen;

  tbuf[0] = 0xAC;
  return ep_ipmb_process(ipmi_cmd, netfn, tbuf, 1, rbuf, &rlen);
}

bool
is_ep_present() {
  int i, ret = 0;
  gpio_desc_t *gdesc;
  gpio_value_t val;
  const char *shadow[] = {
    GPIO_JBOG_PRSNT_1,
    GPIO_JBOG_PRSNT_2
  };

  for (i = 0; i < 2; i++) {
    gdesc = gpio_open_by_shadow(shadow[i]);
    if (gdesc) {
      if (gpio_get_value(gdesc, &val) < 0) {
	syslog(LOG_WARNING, "Get GPIO %s failed", shadow[i]);
	val = GPIO_VALUE_INVALID;
      }
      ret |= (val == GPIO_VALUE_LOW)? 1: 0;
      gpio_close(gdesc);
    }
  }

  return ret? true: false;
}

int
pal_ep_set_system_mode(uint8_t mode) {
  return is_ep_present()? cmd_ep_set_system_mode(mode): 0;
}

int
pal_ep_sled_cycle(void) {
  return is_ep_present()? cmd_ep_sled_cycle(): 0;
}
