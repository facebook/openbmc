/* Copyright 2020-present Facebook. All Rights Reserved.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __FBGC_GPIO_H__
#define __FBGC_GPIO_H__

#ifdef CONFIG_GRANDCANYON2
#define BOARD_ID_PIN_NUM 3
#endif

#include <stdint.h>
#include <openbmc/libgpio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gpio_info {
  char *shadow_name;
  char *pin_name;
  gpio_direction_t direction;
  gpio_value_t value;
} gpio_cfg;

// GPIO Expander GPIO pins
enum {
  GPIO_COMP_PRSNT_N = 0,
  GPIO_FAN_0_INS_N,
  GPIO_FAN_1_INS_N,
  GPIO_FAN_2_INS_N,
  GPIO_FAN_3_INS_N,
  GPIO_UIC_RMT_INS_N,
  GPIO_SCC_LOC_INS_N,
  GPIO_SCC_RMT_INS_N,
  GPIO_SCC_LOC_TYPE_0,
  GPIO_SCC_RMT_TYPE_0,
  GPIO_SCC_STBY_PGOOD,
  GPIO_SCC_FULL_PGOOD,
  GPIO_COMP_PGOOD,
  GPIO_DRAWER_CLOSED_N,
  GPIO_E1S_1_PRSNT_N,
  GPIO_E1S_2_PRSNT_N,
  GPIO_I2C_E1S_1_RST_N,
  GPIO_I2C_E1S_2_RST_N,
  GPIO_E1S_1_LED_ACT,
  GPIO_E1S_2_LED_ACT,
  GPIO_SCC_STBY_PWR_EN,
  GPIO_SCC_FULL_PWR_EN,
  GPIO_BMC_EXP_SOFT_RST_N,
  GPIO_UIC_COMP_BIC_RST_N,
  GPIO_E1S_1_3V3EFUSE_PGOOD,
  GPIO_E1S_2_3V3EFUSE_PGOOD,
#ifdef CONFIG_GRANDCANYON2
  GPIO_P12V_NIC_STATUS_N,   /* hack: NIC 12V fault_n (active-low) */
  GPIO_P3V3_NIC_STATUS_N,   /* hack: NIC 3V3 fault_n (active-low) */
#else
  GPIO_P12V_NIC_FAULT_N,
  GPIO_P3V3_NIC_FAULT_N,
#endif
  GPIO_SCC_POR_RST_N,
  GPIO_IOC_T7_SYS_PGOOD,
  GPIO_BMC_COMP_BLED,
  GPIO_BMC_COMP_YLED,

#ifdef CONFIG_GRANDCANYON2
  /* DVT additions (bus11 U58/U59; U126 on FDPB/RDPB; names differ by A/B side only) */
  GPIO_E1S_1_12VEFUSE_PGOOD,   /* A: E1SA_1_12VEFUSE_PGOOD / B: E1SB_1_12VEFUSE_PGOOD */
  GPIO_FM_BMC_RST_R_RTCRST,    /* A: FM_BMC_RST_A_R_RTCRST / B: FM_BMC_RST_B_R_RTCRST */
  GPIO_SMB_NIC_INA233_ALRT_N,  /* A: SMB_NIC_A_INA233_ALRT_N / B: SMB_NIC_B_INA233_ALRT_N */
  GPIO_E1S_1_12VEFUSE_FLT_R_N, /* A: E1SA_1_12VEFUSE_FLT_R_N / B: E1SB_1_12VEFUSE_FLT_R_N */
  GPIO_E1S_1_3V3EFUSE_FLT_R_N, /* A: E1SA_1_3V3EFUSE_FLT_R_N / B: E1SB_1_3V3EFUSE_FLT_R_N */
  GPIO_E1S_2_12VEFUSE_FLT_R_N, /* A: E1SA_2_12VEFUSE_FLT_R_N / B: E1SB_2_12VEFUSE_FLT_R_N */
  GPIO_E1S_2_3V3EFUSE_FLT_R_N, /* A: E1SA_2_3V3EFUSE_FLT_R_N / B: E1SB_2_3V3EFUSE_FLT_R_N */
  GPIO_E1S_2_12VEFUSE_PGOOD,   /* A: E1SA_2_12VEFUSE_PGOOD / B: E1SB_2_12VEFUSE_PGOOD */

  /* U126 A/B (FDPB/RDPB): common logical names, side-specific shadows */
  GPIO_ALRT_P12V_STBY_SCC_N,   /* same name on A/B */
  GPIO_P3V3_STBY_PG_R,         /* A: P3V3_STBY_A_PG_R / B: P3V3_STBY_B_PG_R */
  GPIO_P5V_1_PG,               /* A: P5V_A_1_PG / B: P5V_B_1_PG */
  GPIO_P5V_2_PG,               /* A: P5V_A_2_PG / B: P5V_B_2_PG */
  GPIO_P5V_3_PG,               /* A: P5V_A_3_PG / B: P5V_B_3_PG */
  GPIO_P5V_4_PG,               /* A: P5V_A_4_PG / B: P5V_B_4_PG */
  GPIO_EMPTY,
  GPIO_FM_HSC_FAULT_N,         /* same name on A/B */
#endif /* CONFIG_GRANDCANYON2 */

  /* upper bound for expander enum */
  MAX_GPIO_EXPANDER_GPIO_PINS,
};


// BMC GPIO pins
enum {
  GPIO_FPGA_CRCERROR = MAX_GPIO_EXPANDER_GPIO_PINS,
  GPIO_FPGA_NCONFIG,
  GPIO_BMC_SCC_FAULT_IN_R,
  GPIO_HSC_P12V_DPB_FAULT_N_IN_R,
  GPIO_HSC_COMP_FAULT_N_IN_R,
  GPIO_BMC_NIC_PWRBRK_R,
  GPIO_BMC_NIC_SMRST_N_R,
  GPIO_NIC_WAKE_BMC_N,
  GPIO_BMC_NIC_FULL_PWR_EN_R,
  GPIO_NIC_FULL_PWR_PG,
  GPIO_BOARD_REV_ID0,
  GPIO_BOARD_REV_ID1,
  GPIO_BOARD_REV_ID2,
  GPIO_EN_ASD_DEBUG,
  GPIO_DEBUG_RST_BTN_N,
  GPIO_E1S_1_P3V3_PG_R,
  GPIO_E1S_2_P3V3_PG_R,
  GPIO_BMC_FPGA_UART_SEL0_R,
  GPIO_BMC_FPGA_UART_SEL1_R,
  GPIO_BMC_FPGA_UART_SEL2_R,
  GPIO_BMC_FPGA_UART_SEL3_R,
  GPIO_DEBUG_BMC_UART_SEL_R,
  GPIO_DEBUG_PWR_BTN_N,
  GPIO_DEBUG_GPIO_BMC_2,
  GPIO_DEBUG_GPIO_BMC_3,
  GPIO_DEBUG_GPIO_BMC_4,
  GPIO_DEBUG_GPIO_BMC_5,
  GPIO_DEBUG_GPIO_BMC_6,
  GPIO_USB_OC_N1,
  GPIO_SCC_I2C_EN_R,
  GPIO_BMC_READY_R,
  GPIO_LED_POSTCODE_0,
  GPIO_LED_POSTCODE_1,
  GPIO_LED_POSTCODE_2,
  GPIO_LED_POSTCODE_3,
  GPIO_LED_POSTCODE_4,
  GPIO_LED_POSTCODE_5,
  GPIO_LED_POSTCODE_6,
  GPIO_LED_POSTCODE_7,
  GPIO_BMC_LOC_HEARTBEAT_R,
  GPIO_BIC_READY_IN,
  GPIO_COMP_STBY_PG_IN,
  GPIO_UIC_LOC_TYPE_IN,
  GPIO_UIC_RMT_TYPE_IN,
  GPIO_BMC_COMP_PWR_EN_R,
  GPIO_EXT_MINISAS_INS_OR_N_IN,
  GPIO_NIC_PRSNTB3_N,
  GPIO_FM_BMC_TPM_PRSNT_N,
  GPIO_DEBUG_CARD_PRSNT_N,
  GPIO_BMC_RST_BTN_N_R,
  GPIO_PCIE_COMP_UIC_RST_N,
  GPIO_BMC_COMP_SYS_RST_N_R,
  GPIO_BMC_PWRGD_NIC,
  GPIO_BMC_LED_STATUS_BLUE_EN_R,
  GPIO_BMC_LED_STATUS_YELLOW_EN_R,
  GPIO_BMC_LED_PWR_BTN_EN_R,
  GPIO_BMC_UIC_LOCATION_IN,
  MAX_GPIO_PINS,
};

static inline bool fbgc_is_grandcanyon2(void)
{
#ifdef CONFIG_GRANDCANYON2
    return true;
#else
    return false;
#endif
}

/* ---------------------------
 * Tables and active selection
 * --------------------------- */
#ifdef CONFIG_GRANDCANYON2
extern gpio_cfg gpio_expander_gpio_table_hack[];
extern gpio_cfg gpio_expander_gpio_table_dvt_a[];
extern gpio_cfg gpio_expander_gpio_table_dvt_b[];

extern gpio_cfg *gpio_expander_gpio_table;      /* active expander table pointer */
extern size_t    gpio_expander_gpio_table_size; /* active expander table size */

/* Initialize and select active expander table (hack/DVT-A/DVT-B) */
int fbgc_gpio_init(void);
#else
extern gpio_cfg gpio_expander_gpio_table[];
#endif /* CONFIG_GRANDCANYON2 */

extern gpio_cfg bmc_gpio_table[];

/* Get shadow name for given logical GPIO ID ("" if not mapped) */
const char * fbgc_get_gpio_name(uint8_t gpio);

/* BIC GPIO helpers (unchanged external API) */
uint8_t fbgc_get_bic_gpio_list_size(void);
const char * fbgc_get_bic_gpio_name(uint8_t gpio);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBGC_GPIO_H__ */
