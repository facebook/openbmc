#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <syslog.h>
#include <openbmc/ipmb.h>
#include <openbmc/libgpio.h>
#include "pal.h"
#include "pal_sbmc.h"

#define GPIO_JBOG_PRSNT_1 "PRSNT_PCIE_CABLE_1_N"
#define GPIO_JBOG_PRSNT_2 "PRSNT_PCIE_CABLE_2_N"

static int
cc_ipmb_process(uint8_t ipmi_cmd, uint8_t netfn,
              uint8_t *txbuf, uint8_t txlen,
              uint8_t *rxbuf, uint8_t *rxlen ) {
  int ret = 0;
  uint16_t bmc_addr;

  ret = pal_get_bmc_ipmb_slave_addr(&bmc_addr, CC_I2C_BUS_NUMBER);
  if(ret != 0) {
    return ret;
  }

  return lib_ipmb_send_request(ipmi_cmd, netfn,
                               txbuf, txlen,
                               rxbuf, rxlen,
                               CC_I2C_BUS_NUMBER, CC_I2C_SLAVE_ADDR, bmc_addr);
}

static int
cmd_cc_sled_cycle(void) {
  uint8_t ipmi_cmd = CMD_CHASSIS_CONTROL;
  uint8_t netfn = NETFN_CHASSIS_REQ;
  uint8_t tbuf[8];
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen;
  uint8_t mode;
  uint8_t pos;
  int ret;

  ret = pal_get_host_system_mode(&mode);
  if (ret != 0) {
    syslog(LOG_WARNING,"%s Wrong get system mode\n", __func__);
    return ret;
  }

  ret = pal_get_mb_position (&pos);
  if (ret != 0) {
    syslog(LOG_WARNING,"%s Wrong get position\n", __func__);
    return ret;
  }

  tbuf[0] = 0xAC;

  if( mode == MB_4S_MODE && pos == MB_ID0 ) {
    return cmd_mb0_bridge_to_cc(CMD_CHASSIS_CONTROL, NETFN_CHASSIS_REQ, tbuf, 1);
  }

  return cc_ipmb_process(ipmi_cmd, netfn, tbuf, 1, rbuf, &rlen);
}

bool
is_cc_present() {
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
pal_cc_sled_cycle(void) {
  return is_cc_present()? cmd_cc_sled_cycle(): 0;
}

int
pal_cc_get_lan_config(uint8_t sel, uint8_t *buf, uint8_t *rlen)
{
  uint8_t netfn = NETFN_TRANSPORT_REQ;
  uint8_t ipmi_cmd = CMD_TRANSPORT_GET_LAN_CONFIG;
  uint8_t req[2] = {0x0, sel};
  uint8_t resp[MAX_IPMI_MSG_SIZE];

  if (!is_cc_present())
    return -1;
  if (cc_ipmb_process(ipmi_cmd, netfn, req, 2, resp, rlen))
    return -1;
  if (*rlen <= 1)
    return -1;
  *rlen = *rlen - 1;
  memcpy(buf, resp + 1, *rlen);
  return 0;
}

int
pal_cc_set_usb_ch(uint8_t dev, uint8_t mb) {
  uint8_t netfn = NETFN_OEM_ZION_REQ;
  uint8_t ipmi_cmd = CMD_OEM_ZION_SET_USB_PATH;
  uint8_t tbuf[8];
  uint8_t rbuf[8] = {0x00};
  uint8_t rlen;

  tbuf[0] = dev;
  tbuf[1] = mb;

  return cc_ipmb_process(ipmi_cmd, netfn, tbuf, 2, rbuf, &rlen);
}

