/*
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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

#ifndef __PAL_H__
#define __PAL_H__

#include <openbmc/obmc-pal.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <openbmc/kv.h>
#include <openbmc/ipmi.h>
#include <stdbool.h>
#include <facebook/bic.h>

#define BIT(value, index) ((value >> index) & 1)

#define MAX_NODES     1
#define MAX_NUM_FRUS  5
#define BMC_READY_N   28
#define IPMB_BUS 0
#define BOARD_REV_EVTA 4
#define BOARD_REV_EVTB 0

extern const char pal_fru_list[];

enum {
  BIC_MODE_NORMAL = 0x01,
  BIC_MODE_UPDATE = 0x0F,
};

enum {
  SERVER_POWER_OFF,
  SERVER_POWER_ON,
  SERVER_POWER_CYCLE,
  SERVER_POWER_RESET,
  SERVER_GRACEFUL_SHUTDOWN,
  SERVER_12V_OFF,
  SERVER_12V_ON,
  SERVER_12V_CYCLE,
};

enum {
  FRU_ALL = 0,
  FRU_MB = 1,
};

typedef struct _sensor_info_t {
  bool valid;
  sdr_full_t sdr;
} sensor_info_t;

int pal_is_fru_prsnt(uint8_t fru, uint8_t *status);
int pal_handle_oem_1s_intr(uint8_t slot, uint8_t *data);
void pal_inform_bic_mode(uint8_t fru, uint8_t mode);
int pal_get_plat_sku_id(void);
int pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
int pal_get_fru_list(char *list);
int pal_is_scm_prsnt(uint8_t *status);
int pal_is_debug_card_prsnt(uint8_t *status);
int pal_post_enable(uint8_t slot);
int pal_post_disable(uint8_t slot);
int pal_post_get_last(uint8_t slot, uint8_t *status);
int pal_post_handle(uint8_t slot, uint8_t status);
int pal_set_last_pwr_state(uint8_t fru, char *state);
int pal_get_last_pwr_state(uint8_t fru, char *state);
int pal_set_com_pwr_btn_n(char *status);
int pal_get_board_rev(int *rev);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
