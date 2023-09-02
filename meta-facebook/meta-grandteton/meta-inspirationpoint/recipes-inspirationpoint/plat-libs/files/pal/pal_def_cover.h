#ifndef __HAL_PALDEF_COVER_H__
#define __HAL_PALDEF_COVER_H__

//BUS number
#define HSC_BUS_NUM               (6)

#define MAX_DIMM_NUM              (24)
#define PER_CPU_DIMM_NUMBER_MAX   (MAX_DIMM_NUM/MAX_CPU_CNT)

#define MAX_NUM_RETIMERS          (8)

//GPIO EVENT Cover
#define IRQ_UV_DETECT_N          "UV_ALERT_N"
#define IRQ_OC_DETECT_N          "OC_ALERT_N"
#define IRQ_HSC_FAULT_N          "IRQ_HSC_FAULT_R1_N"
#define IRQ_HSC_ALERT_N          "IRQ_SML1_PMBUS_ALERT_N"
#define FM_CPU0_PROCHOT_N        "FM_CPU0_PROCHOT_R_N"
#define FM_CPU1_PROCHOT_N        "FM_CPU1_PROCHOT_R_N"
#define FM_CPU0_THERMTRIP_N      "CPU0_THERMTRIP_R_N"
#define FM_CPU1_THERMTRIP_N      "CPU1_THERMTRIP_R_N"
#define FM_CPU_ERR0_N            "FM_CPU0_SMERR_N"
#define FM_CPU_ERR1_N            "FM_CPU1_SMERR_N"
#define FM_SYS_THROTTLE          "FM_SYS_THROTTLE_R_N"
#define FM_CPU0_SKTOCC           "FM_PRSNT_CPU0_R_N"
#define FM_CPU1_SKTOCC           "FM_PRSNT_CPU1_R_N"
#define FM_BIOS_POST_CMPLT       "FM_BIOS_POST_CMPLT_BUF_R_N"
#define FM_CPU0_PWR_FAIL         "P0_PWR_ERR"
#define FM_CPU1_PWR_FAIL         "P1_PWR_ERR"
#define FM_UV_ADR_TRIGGER        "FM_UV_ADR_TRIGGER_R_N"
#define FM_PVDDCR_CPU0_P0_PMALERT   "PVDDCR_CPU0_P0_PMALERT"
#define FM_PVDDCR_CPU0_P1_PMALERT   "PVDDCR_CPU0_P1_PMALERT"
#define FM_PVDDCR_CPU1_P0_SMB_ALERT "PVDDCR_CPU1_P0_SMB_ALERT"
#define FM_PVDDCR_CPU1_P1_SMB_ALERT "PVDDCR_CPU1_P1_SMB_ALERT"
#define FM_CPU0_PRSNT            "FM_PRSNT_CPU0_R_N"
#define FM_CPU1_PRSNT            "FM_PRSNT_CPU1_R_N"
#define FM_OCP0_PRSNT            "PRSNT_OCP_SFF_N"
#define FM_OCP1_PRSNT            "PRSNT_OCP_V3_2_N"
#define FM_E1S0_PRSNT            "E1S_0_PRSNT_R_N"
#define FM_PVDD11_S3_P0_OCP      "FM_PVDD11_S3_P0_OCP_N"
#define FM_PVDD11_S3_P1_OCP      "FM_PVDD11_S3_P1_OCP_N"
#define PVDD11_S3_P0_PMALERT     "PVDD11_S3_P0_PMALERT"
#define PVDD11_S3_P1_PMALERT     "PVDD11_S3_P1_PMALERT"
#define RST_PERST_N              "RST_PERST_CPUx_SWB_N"
#define PEX_FW_VER_UPDATE        "RST_PERST_CPUx_SWB_N"
#define APML_CPU0_ALERT          "APML_CPU0_ALERT_R_N"
#define APML_CPU1_ALERT          "APML_CPU1_ALERT_R_N"
#define PWR_FALUT_ALERT          "CPLD_PWR_FAULT_ALERT"
#define RESERVED_GPIO            "NULL"

//GPIO Power Control Cover
#define FM_LAST_PWRGD            "FM_PWRGD_CPU1_PWROK"
#define RST_PLTRST_N             "FM_RST_CPU1_RESETL_N"
#define RST_KB_RESET_N           "RST_KB_RESET_N"
#define KB_RESET_EN              "KB_RESET_EN"

#endif
