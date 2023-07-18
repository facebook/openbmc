#include <facebook/bic_ipmi.h>
#include <facebook/bic_xfer.h>
#include <safe_str_lib.h>
#include "../include/logging.h"
#include "jtag_handler.h"
#include "target_handler.h"

extern uint8_t pin_gpios[];

static const ASD_LogStream stream = ASD_LogStream_Pins;
static const ASD_LogOption option = ASD_LogOption_None;

STATUS jtag_ipmb_wrapper(
    uint8_t fru,
    uint8_t netfn,
    uint8_t cmd,
    uint8_t* txbuf,
    size_t txlen,
    uint8_t* rxbuf,
    size_t* rxlen) {
  if (txbuf == NULL || rxbuf == NULL) {
    return ST_ERR;
  }

  if (bic_data_send(
          fru, netfn, cmd, txbuf, txlen, rxbuf, (uint8_t*)rxlen, NONE_INTF)) {
    ASD_log(
        ASD_LogLevel_Error,
        stream,
        option,
        "bic_data send failed, slot%u, netfn: %d, cmd: %d\n",
        fru,
        netfn,
        cmd);
    return ST_ERR;
  }

  return ST_OK;
}

int pin_get_gpio(uint8_t fru, Target_Control_GPIO* gpio) {
  uint8_t val = 0;

  if (gpio == NULL) {
    return -1;
  }

  if (bic_get_one_gpio_status(fru, gpio->number, &val)) {
    return -1;
  }

  return val;
}

int pin_set_gpio(uint8_t fru, Target_Control_GPIO* gpio, int val) {
  if (gpio == NULL) {
    return -1;
  }

  if (bic_set_gpio(fru, gpio->number, val)) {
    return -1;
  }

  return 0;
}

STATUS platform_init(Target_Control_Handle* state) {
  enum gl_bic_gpio {
    BIC_GPIO_H_BMC_PRDY_BUF_N = 26,
    BIC_GPIO_PWRGD_CPU_LVC3 = 32,
    BIC_GPIO_RST_PLTRST_BUF_N = 39,
    BIC_GPIO_FM_DBP_PRESENT_N = 44,
  };

  if (state == NULL) {
    return ST_ERR;
  }

  strcpy_s(
      state->gpios[BMC_PREQ_N].name,
      sizeof(state->gpios[BMC_PREQ_N].name),
      "DBP_CPU_PREQ_BIC_N");
  state->gpios[BMC_PREQ_N].number = GL_DBP_CPU_PREQ_BIC_N;
  state->gpios[BMC_PREQ_N].fd = BMC_PREQ_N;
  state->gpios[BMC_PREQ_N].active_low = true;

  strcpy_s(
      state->gpios[BMC_PRDY_N].name,
      sizeof(state->gpios[BMC_PRDY_N].name),
      "H_BMC_PRDY_BUF_N");
  state->gpios[BMC_PRDY_N].number = GL_H_BMC_PRDY_BUF_N;
  state->gpios[BMC_PRDY_N].active_low = true;

  strcpy_s(
      state->gpios[BMC_JTAG_SEL].name,
      sizeof(state->gpios[BMC_JTAG_SEL].name),
      "BMC_JTAG_SEL_R");
  state->gpios[BMC_JTAG_SEL].number = GL_BMC_JTAG_SEL_R;
  state->gpios[BMC_JTAG_SEL].fd = BMC_JTAG_SEL;

  strcpy_s(
      state->gpios[BMC_CPU_PWRGD].name,
      sizeof(state->gpios[BMC_CPU_PWRGD].name),
      "PWRGD_CPU_LVC3");
  state->gpios[BMC_CPU_PWRGD].number = GL_PWRGD_CPU_LVC3;

  strcpy_s(
      state->gpios[BMC_PLTRST_B].name,
      sizeof(state->gpios[BMC_PLTRST_B].name),
      "RST_PLTRST_BUF_N");
  state->gpios[BMC_PLTRST_B].number = GL_RST_PLTRST_BUF_N;
  state->gpios[BMC_PLTRST_B].active_low = true;

  strcpy_s(
      state->gpios[BMC_SYSPWROK].name,
      sizeof(state->gpios[BMC_SYSPWROK].name),
      "PWRGD_CPU_LVC3");
  state->gpios[BMC_SYSPWROK].number = GL_PWRGD_CPU_LVC3;
  state->gpios[BMC_SYSPWROK].fd = BMC_SYSPWROK;

  strcpy_s(
      state->gpios[BMC_PWR_DEBUG_N].name,
      sizeof(state->gpios[BMC_PWR_DEBUG_N].name),
      "FBRK_R_N");
  state->gpios[BMC_PWR_DEBUG_N].number = GL_FBRK_R_N;
  state->gpios[BMC_PWR_DEBUG_N].fd = BMC_PWR_DEBUG_N;
  state->gpios[BMC_PWR_DEBUG_N].active_low = true;

  strcpy_s(
      state->gpios[BMC_DEBUG_EN_N].name,
      sizeof(state->gpios[BMC_DEBUG_EN_N].name),
      "FM_BMC_DEBUG_ENABLE_R_N");
  state->gpios[BMC_DEBUG_EN_N].number = GL_FM_BMC_DEBUG_ENABLE_R_N;
  state->gpios[BMC_DEBUG_EN_N].fd = BMC_DEBUG_EN_N;
  state->gpios[BMC_DEBUG_EN_N].active_low = true;

  strcpy_s(
      state->gpios[BMC_XDP_PRST_IN].name,
      sizeof(state->gpios[BMC_XDP_PRST_IN].name),
      "FM_DBP_PRESENT_N");
  state->gpios[BMC_XDP_PRST_IN].number = GL_FM_DBP_PRESENT_N;
  state->gpios[BMC_XDP_PRST_IN].active_low = true;

  pin_gpios[JTAG_PLTRST_EVENT] = BIC_GPIO_RST_PLTRST_BUF_N;
  pin_gpios[JTAG_PWRGD_EVENT] = BIC_GPIO_PWRGD_CPU_LVC3;
  pin_gpios[JTAG_PRDY_EVENT] = BIC_GPIO_H_BMC_PRDY_BUF_N;
  pin_gpios[JTAG_XDP_PRESENT_EVENT] = BIC_GPIO_FM_DBP_PRESENT_N;

  return ST_OK;
}

JTAG_Handler* SoftwareJTAGHandler(uint8_t fru) {
  scanChain chain;
  JTAG_Handler* state = JTAGHandler();
  int server_type = SERVER_TYPE_NONE;
  uint8_t bmc_jtag_sel_pin = 0;

  if (!state) {
    return NULL;
  }

  chain = (fru & 0x80) ? SCAN_CHAIN_1 : SCAN_CHAIN_0;
  state->active_chain = &state->chains[chain];
  state->fru = fru & 0x7F;
  state->msg_flow = JFLOW_BMC;

  ASD_log(
      ASD_LogLevel_Info,
      stream,
      option,
      "Setting BMC_JTAG_SEL_R, slot%u",
      state->fru);
  if (bic_set_gpio(state->fru, GL_BMC_JTAG_SEL_R, 1)) {
    ASD_log(
        ASD_LogLevel_Error,
        stream,
        option,
        "Setting BMC_JTAG_SEL_R failed, slot%u",
        state->fru);
  }

  return state;
}
