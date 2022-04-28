#ifndef __PAL_POWER_H__
#define __PAL_POWER_H__

bool is_server_off(void);
int pal_is_bmc_por(void);

#define GPIO_POWER_BTN_OUT      "SYS_BMC_PWRBTN_OUT"
#define GPIO_CPU_POWER_GOOD     "PWRGD_CPUPWRGD_LVC2_R1"
#define RST_BMC_RSTBTN_OUT_N    "RST_BMC_RSTBTN_OUT_R_N"
#define GPIO_RESET_BTN_IN       "ID_RST_BTN_BMC_IN"
#define PWRGD_SYS_PWROK         "PWRGD_SYS_PWROK_R"
#endif

