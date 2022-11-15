#include <safe_str_lib.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/pal_def.h>

#include "jtag_handler.h"
#include "logging.h"
#include "target_handler.h"

extern struct gpiopoll_config pin_gpios[];

static const ASD_LogStream stream = ASD_LogStream_Pins;
static const ASD_LogOption option = ASD_LogOption_None;

int pin_get_gpio(uint8_t fru, Target_Control_GPIO *gpio)
{
    if (gpio == NULL) {
        return -1;
    }
    return gpio_get_value_by_shadow(gpio->name);
}

int pin_set_gpio(uint8_t fru, Target_Control_GPIO *gpio, int val)
{
    if (gpio == NULL) {
        return -1;
    }

    if (gpio_set_value_by_shadow(gpio->name, val)) {
        return -1;
    }
    if (gpio->fd == BMC_TCK_MUX_SEL) {
        msleep(1);
    }

    return 0;
}

STATUS platform_init(Target_Control_Handle *state)
{
    if (state == NULL) {
        return ST_ERR;
    }

    strcpy_s(state->gpios[BMC_TCK_MUX_SEL].name,
             sizeof(state->gpios[BMC_TCK_MUX_SEL].name), FM_JTAG_TCK_MUX_SEL);
    state->gpios[BMC_TCK_MUX_SEL].number = BMC_TCK_MUX_SEL;
    state->gpios[BMC_TCK_MUX_SEL].fd = BMC_TCK_MUX_SEL;

    strcpy_s(state->gpios[BMC_PREQ_N].name,
             sizeof(state->gpios[BMC_PREQ_N].name), FM_BMC_PREQ_N);
    state->gpios[BMC_PREQ_N].number = BMC_PREQ_N;
    state->gpios[BMC_PREQ_N].fd = BMC_PREQ_N;
    state->gpios[BMC_PREQ_N].active_low = true;

    strcpy_s(state->gpios[BMC_PRDY_N].name,
             sizeof(state->gpios[BMC_PRDY_N].name), IRQ_BMC_PRDY_N);
    state->gpios[BMC_PRDY_N].number = BMC_PRDY_N;
    state->gpios[BMC_PRDY_N].active_low = true;

    strcpy_s(state->gpios[BMC_JTAG_SEL].name,
             sizeof(state->gpios[BMC_JTAG_SEL].name), FM_JTAG_BMC_MUX_SEL);
    state->gpios[BMC_JTAG_SEL].number = BMC_JTAG_SEL;
    state->gpios[BMC_JTAG_SEL].fd = BMC_JTAG_SEL;

    strcpy_s(state->gpios[BMC_CPU_PWRGD].name,
             sizeof(state->gpios[BMC_CPU_PWRGD].name), FM_LAST_PWRGD);
    state->gpios[BMC_CPU_PWRGD].number = BMC_CPU_PWRGD;

    strcpy_s(state->gpios[BMC_PLTRST_B].name,
             sizeof(state->gpios[BMC_PLTRST_B].name), RST_PLTRST_N);
    state->gpios[BMC_PLTRST_B].number = BMC_PLTRST_B;
    state->gpios[BMC_PLTRST_B].active_low = true;

    strcpy_s(state->gpios[BMC_SYSPWROK].name,
             sizeof(state->gpios[BMC_SYSPWROK].name), PWRGD_SYS_PWROK);
    state->gpios[BMC_SYSPWROK].number = BMC_SYSPWROK;
    state->gpios[BMC_SYSPWROK].fd = BMC_SYSPWROK;

    strcpy_s(state->gpios[BMC_PWR_DEBUG_N].name,
             sizeof(state->gpios[BMC_PWR_DEBUG_N].name), FM_BMC_CPU_PWR_DEBUG_N);
    state->gpios[BMC_PWR_DEBUG_N].number = BMC_PWR_DEBUG_N;
    state->gpios[BMC_PWR_DEBUG_N].fd = BMC_PWR_DEBUG_N;
    state->gpios[BMC_PWR_DEBUG_N].active_low = true;

    strcpy_s(state->gpios[BMC_XDP_PRST_IN].name,
             sizeof(state->gpios[BMC_XDP_PRST_IN].name), DBP_PRESENT_N);
    state->gpios[BMC_XDP_PRST_IN].number = BMC_XDP_PRST_IN;
    state->gpios[BMC_XDP_PRST_IN].active_low = true;

    strcpy_s(pin_gpios[JTAG_PLTRST_EVENT].shadow,
             sizeof(pin_gpios[JTAG_PLTRST_EVENT].shadow), RST_PLTRST_N);
    strcpy_s(pin_gpios[JTAG_PWRGD_EVENT].shadow,
             sizeof(pin_gpios[JTAG_PWRGD_EVENT].shadow), FM_LAST_PWRGD);
    strcpy_s(pin_gpios[JTAG_PRDY_EVENT].shadow,
             sizeof(pin_gpios[JTAG_PRDY_EVENT].shadow), IRQ_BMC_PRDY_N);
    strcpy_s(pin_gpios[JTAG_XDP_PRESENT_EVENT].shadow,
             sizeof(pin_gpios[JTAG_XDP_PRESENT_EVENT].shadow), DBP_PRESENT_N);

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

    if (gpio_set_value_by_shadow(FM_JTAG_BMC_MUX_SEL, 1)) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Setting FM_JTAG_BMC_MUX_SEL failed");
    }

    if (gpio_set_value_by_shadow(FM_JTAG_TCK_MUX_SEL, chain)) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Setting FM_JTAG_TCK_MUX_SEL failed");
    }
    msleep(1);

    return state;
}
