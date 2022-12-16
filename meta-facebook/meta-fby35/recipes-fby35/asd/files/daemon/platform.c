#include <safe_str_lib.h>
#include <facebook/bic_ipmi.h>
#include <facebook/bic_xfer.h>

#include "jtag_handler.h"
#include "logging.h"
#include "target_handler.h"

extern uint8_t pin_gpios[];

static const ASD_LogStream stream = ASD_LogStream_Pins;
static const ASD_LogOption option = ASD_LogOption_None;

STATUS jtag_ipmb_wrapper(uint8_t fru, uint8_t netfn, uint8_t cmd,
                         uint8_t *txbuf, size_t txlen,
                         uint8_t *rxbuf, size_t *rxlen)
{
    if (txbuf == NULL || rxbuf == NULL) {
        return ST_ERR;
    }

    if (bic_ipmb_wrapper(fru, netfn, cmd, txbuf, txlen, rxbuf, (uint8_t *)rxlen)) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "bic_ipmb_wrapper failed, slot%u\n", fru);
        return ST_ERR;
    }

    return ST_OK;
}

int pin_get_gpio(uint8_t fru, Target_Control_GPIO *gpio)
{
    uint8_t val = 0;

    if (gpio == NULL) {
        return -1;
    }

    if (bic_get_one_gpio_status(fru, gpio->number, &val)) {
        return -1;
    }

    return val;
}

int pin_set_gpio(uint8_t fru, Target_Control_GPIO *gpio, int val)
{
    if (gpio == NULL) {
        return -1;
    }

    if (bic_set_gpio(fru, gpio->number, val)) {
        return -1;
    }

    return 0;
}

STATUS platform_init(Target_Control_Handle *state)
{
    if (state == NULL) {
        return ST_ERR;
    }

    strcpy_s(state->gpios[BMC_TCK_MUX_SEL].name,
             sizeof(state->gpios[BMC_TCK_MUX_SEL].name), "FM_JTAG_TCK_MUX_SEL_R");
    state->gpios[BMC_TCK_MUX_SEL].number = FM_JTAG_TCK_MUX_SEL_R;
    state->gpios[BMC_TCK_MUX_SEL].fd = BMC_TCK_MUX_SEL;

    strcpy_s(state->gpios[BMC_PREQ_N].name,
             sizeof(state->gpios[BMC_PREQ_N].name), "DBP_CPU_PREQ_BIC_N");
    state->gpios[BMC_PREQ_N].number = DBP_CPU_PREQ_BIC_N;
    state->gpios[BMC_PREQ_N].fd = BMC_PREQ_N;
    state->gpios[BMC_PREQ_N].active_low = true;

    strcpy_s(state->gpios[BMC_PRDY_N].name,
             sizeof(state->gpios[BMC_PRDY_N].name), "H_BMC_PRDY_BUF_N");
    state->gpios[BMC_PRDY_N].number = H_BMC_PRDY_BUF_N;
    state->gpios[BMC_PRDY_N].active_low = true;

    strcpy_s(state->gpios[BMC_JTAG_SEL].name,
                sizeof(state->gpios[BMC_JTAG_SEL].name), "BMC_JTAG_SEL_R");
    state->gpios[BMC_JTAG_SEL].number = BMC_JTAG_SEL_R;
    state->gpios[BMC_JTAG_SEL].fd = BMC_JTAG_SEL;

    strcpy_s(state->gpios[BMC_CPU_PWRGD].name,
             sizeof(state->gpios[BMC_CPU_PWRGD].name), "PWRGD_CPU_LVC3");
    state->gpios[BMC_CPU_PWRGD].number = PWRGD_CPU_LVC3;

    strcpy_s(state->gpios[BMC_PLTRST_B].name,
             sizeof(state->gpios[BMC_PLTRST_B].name), "RST_PLTRST_BUF_N");
    state->gpios[BMC_PLTRST_B].number = RST_PLTRST_BUF_N;
    state->gpios[BMC_PLTRST_B].active_low = true;

    strcpy_s(state->gpios[BMC_SYSPWROK].name,
             sizeof(state->gpios[BMC_SYSPWROK].name), "PWRGD_SYS_PWROK");
    state->gpios[BMC_SYSPWROK].number = PWRGD_SYS_PWROK;
    state->gpios[BMC_SYSPWROK].fd = BMC_SYSPWROK;

    strcpy_s(state->gpios[BMC_PWR_DEBUG_N].name,
             sizeof(state->gpios[BMC_PWR_DEBUG_N].name), "FBRK_N_R");
    state->gpios[BMC_PWR_DEBUG_N].number = FBRK_N_R;
    state->gpios[BMC_PWR_DEBUG_N].fd = BMC_PWR_DEBUG_N;
    state->gpios[BMC_PWR_DEBUG_N].active_low = true;

    strcpy_s(state->gpios[BMC_DEBUG_EN_N].name,
             sizeof(state->gpios[BMC_DEBUG_EN_N].name), "FM_BMC_DEBUG_ENABLE_N");
    state->gpios[BMC_DEBUG_EN_N].number = FM_BMC_DEBUG_ENABLE_N;
    state->gpios[BMC_DEBUG_EN_N].fd = BMC_DEBUG_EN_N;
    state->gpios[BMC_DEBUG_EN_N].active_low = true;

    strcpy_s(state->gpios[BMC_XDP_PRST_IN].name,
             sizeof(state->gpios[BMC_XDP_PRST_IN].name), "FM_DBP_PRESENT_N");
    state->gpios[BMC_XDP_PRST_IN].number = FM_DBP_PRESENT_N;
    state->gpios[BMC_XDP_PRST_IN].active_low = true;

    pin_gpios[JTAG_PLTRST_EVENT] = RST_PLTRST_BUF_N;
    pin_gpios[JTAG_PWRGD_EVENT] = PWRGD_CPU_LVC3;
    pin_gpios[JTAG_PRDY_EVENT] = H_BMC_PRDY_BUF_N;
    pin_gpios[JTAG_XDP_PRESENT_EVENT] = FM_DBP_PRESENT_N;

    return ST_OK;
}

JTAG_Handler* SoftwareJTAGHandler(uint8_t fru)
{
    scanChain chain;
    JTAG_Handler *state = JTAGHandler();

    if (!state) {
        return NULL;
    }

    chain = (fru & 0x80) ? SCAN_CHAIN_1 : SCAN_CHAIN_0;
    state->active_chain = &state->chains[chain];
    state->fru = fru & 0x7F;
    state->msg_flow = JFLOW_BMC;

    if (bic_set_gpio(state->fru, BMC_JTAG_SEL_R, 1)) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Setting BMC_JTAG_SEL_R failed, slot%u", state->fru);
    }

    if (bic_set_gpio(state->fru, FM_JTAG_TCK_MUX_SEL_R, chain)) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Setting FM_JTAG_TCK_MUX_SEL_R failed, slot%u", state->fru);
    }

    return state;
}
