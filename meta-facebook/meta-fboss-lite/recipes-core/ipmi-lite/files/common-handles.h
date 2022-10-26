/* Copyright (c) Meta Platforms, Inc. and affiliates.
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
 */

#pragma once
#include <errno.h>
#include <openbmc/ipmi.h>
#include <openbmc/log.h>
#include <openbmc/misc-utils.h>
#include <openbmc/obmc-i2c.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#define MAX_ETH_IF_SIZE 16
#define MAX_NUM_SLOT 1

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof((_a)[0]))
#endif
#define CRASHDUMP_BIN "/usr/local/bin/autodump.sh"

#define RUN_SHELL_CMD(_cmd)                              \
  do {                                                   \
    int _ret = system(_cmd);                             \
    if (_ret != 0)                                       \
      OBMC_WARN("'%s' command returned %d", _cmd, _ret); \
  } while (0)

#ifndef BIT
#define BIT(value, index) ((value >> index) & 1)
#endif

#define SETBIT(x, y) (x | (1ULL << y))
#define GETBIT(x, y) ((x & (1ULL << y)) > y)
#define CLEARBIT(x, y) (x & (~(1ULL << y)))
#define GETMASK(y) (1ULL << y)
#define SETMASK(y) (1ULL << y)

#define IPMI_ERR(err, fmt, args...) \
  syslog(LOG_ERR, "[%s ]" fmt ": %s", __func__, ##args, strerror(err))

#ifdef DEBUG
#define IPMI_DBG(fmt, args...) \
  syslog(LOG_DEBUG, "[%s %d]" fmt, __FILE__, __LINE__, ##args)
#else
#define IPMI_DBG(fmt, args...)
#endif

// GUID
#define GUID_SIZE 16

typedef struct _dimm_info {
  uint8_t sled;
  uint8_t socket;
  uint8_t channel;
  uint8_t slot;
} _dimm_info;

// for power restore policy
enum {
  POWER_CFG_OFF = 0,
  POWER_CFG_LPS,
  POWER_CFG_ON,
  POWER_CFG_UKNOWN,
};

// For power control/status
enum {
  SERVER_POWER_OFF,
  SERVER_POWER_ON,
  SERVER_POWER_CYCLE,
  SERVER_POWER_RESET,
  SERVER_GRACEFUL_SHUTDOWN,
  SERVER_12V_OFF,
  SERVER_12V_ON,
  SERVER_12V_CYCLE,
  SERVER_GLOBAL_RESET,
  SERVER_GRACEFUL_POWER_ON,
  SERVER_FORCE_POWER_ON,
  SERVER_FORCE_12V_ON,
};

enum {
  OS_BOOT = 0x1F,
};

enum {
  UNIFIED_PCIE_ERR = 0x0,
  UNIFIED_MEM_ERR = 0x1,
  UNIFIED_UPI_ERR = 0x2,
  UNIFIED_IIO_ERR = 0x3,
  UNIFIED_POST_ERR = 0x8,
  UNIFIED_PCIE_EVENT = 0x9,
  UNIFIED_MEM_EVENT = 0xA,
  UNIFIED_UPI_EVENT = 0xB,
  UNIFIED_BOOT_GUARD = 0xC,
};

enum {
  MEMORY_TRAINING_ERR = 0x0,
  MEMORY_CORRECTABLE_ERR = 0x1,
  MEMORY_UNCORRECTABLE_ERR = 0x2,
  MEMORY_CORR_ERR_PTRL_SCR = 0x3,
  MEMORY_UNCORR_ERR_PTRL_SCR = 0x4,
};

enum {
  UPI_INIT_ERR = 0x0,
};

enum {
  PCIE_DPC = 0x0,
};

enum {
  MEM_PPR = 0x0,
  MEM_NO_DIMM = 0x7,
};

// Enum for get the event sensor name in processing SEL
enum {
  SYSTEM_EVENT = 0xE9,
  THERM_THRESH_EVT = 0x7D,
  BUTTON = 0xAA,
  POWER_STATE = 0xAB,
  CRITICAL_IRQ = 0xEA,
  POST_ERROR = 0x2B,
  MACHINE_CHK_ERR = 0x40,
  PCIE_ERR = 0x41,
  IIO_ERR = 0x43,
  SMN_ERR = 0x44, // FBND
  USB_ERR = 0x45, // FBND
  PSB_ERR = 0x46, // FBND
  MEMORY_ECC_ERR = 0X63,
  MEMORY_ERR_LOG_DIS = 0X87,
  PROCHOT_EXT = 0X51,
  PWR_ERR = 0X56,
  CATERR_A = 0xE6, // W100, FBTP
  CATERR_B = 0xEB, // FBY2, FBTTN, LIGHTNING ,YOSEMITE
  CPU_DIMM_HOT = 0xB3,
  SOFTWARE_NMI = 0x90,
  CPU0_THERM_STATUS = 0x1C,
  CPU1_THERM_STATUS = 0x1D,
  CPU2_THERM_STATUS = 0x1E,
  CPU3_THERM_STATUS = 0x1F,
  ME_POWER_STATE = 0x16,
  SPS_FW_HEALTH = 0x17,
  NM_EXCEPTION_A = 0x18, // W100, FBTP, FBY2, YOSEMITE, FBTTN
  PCH_THERM_THRESHOLD = 0x8,
  NM_HEALTH = 0x19,
  NM_CAPABILITIES = 0x1A,
  NM_THRESHOLD = 0x1B,
  PWR_THRESH_EVT = 0x3B,
  MSMI = 0xE7,
  HPR_WARNING = 0xC5,
};

/* Sensors on SCM */
enum {
  SCM_SENSOR_OUTLET_LOCAL_TEMP = 0x02,
  SCM_SENSOR_OUTLET_REMOTE_TEMP = 0x03,
  SCM_SENSOR_INLET_LOCAL_TEMP = 0x04,
  SCM_SENSOR_INLET_REMOTE_TEMP = 0x06,
  SCM_SENSOR_HSC_VOLT = 0x0a,
  SCM_SENSOR_HSC_CURR = 0x0b,
  SCM_SENSOR_HSC_POWER = 0x0c,
  /* Sensors on COM-e */
  BIC_SENSOR_MB_OUTLET_TEMP = 0x01,
  BIC_SENSOR_MB_INLET_TEMP = 0x07,
  BIC_SENSOR_PCH_TEMP = 0x08,
  BIC_SENSOR_VCCIN_VR_TEMP = 0x80,
  BIC_SENSOR_1V05MIX_VR_TEMP = 0x81,
  BIC_SENSOR_SOC_TEMP = 0x05,
  BIC_SENSOR_SOC_THERM_MARGIN = 0x09,
  BIC_SENSOR_VDDR_VR_TEMP = 0x82,
  BIC_SENSOR_SOC_DIMMA0_TEMP = 0xB4,
  BIC_SENSOR_SOC_DIMMB0_TEMP = 0xB6,
  BIC_SENSOR_SOC_PACKAGE_PWR = 0x2C,
  BIC_SENSOR_VCCIN_VR_POUT = 0x8B,
  BIC_SENSOR_VDDR_VR_POUT = 0x8D,
  BIC_SENSOR_SOC_TJMAX = 0x30,
  BIC_SENSOR_P3V3_MB = 0xD0,
  BIC_SENSOR_P12V_MB = 0xD2,
  BIC_SENSOR_P1V05_PCH = 0xD3,
  BIC_SENSOR_P3V3_STBY_MB = 0xD5,
  BIC_SENSOR_P5V_STBY_MB = 0xD6,
  BIC_SENSOR_PV_BAT = 0xD7,
  BIC_SENSOR_PVDDR = 0xD8,
  BIC_SENSOR_P1V05_MIX = 0x8E,
  BIC_SENSOR_1V05MIX_VR_CURR = 0x84,
  BIC_SENSOR_VDDR_VR_CURR = 0x85,
  BIC_SENSOR_VCCIN_VR_CURR = 0x83,
  BIC_SENSOR_VCCIN_VR_VOL = 0x88,
  BIC_SENSOR_VDDR_VR_VOL = 0x8A,
  BIC_SENSOR_P1V05MIX_VR_VOL = 0x89,
  BIC_SENSOR_P1V05MIX_VR_POUT = 0x8C,
  BIC_SENSOR_INA230_POWER = 0x29,
  BIC_SENSOR_INA230_VOL = 0x2A,
  BIC_SENSOR_SYSTEM_STATUS = 0x10, // Discrete
  BIC_SENSOR_PROC_FAIL = 0x65, // Discrete
  BIC_SENSOR_SYS_BOOT_STAT = 0x7E, // Discrete
  BIC_SENSOR_VR_HOT = 0xB2, // Discrete
  BIC_SENSOR_CPU_DIMM_HOT = 0xB3, // Discrete
  BIC_SENSOR_SPS_FW_HLTH = 0x17, // Event-only
  BIC_SENSOR_POST_ERR = 0x2B, // Event-only
  BIC_SENSOR_POWER_THRESH_EVENT = 0x3B, // Event-only
  BIC_SENSOR_MACHINE_CHK_ERR = 0x40, // Event-only
  BIC_SENSOR_PCIE_ERR = 0x41, // Event-only
  BIC_SENSOR_OTHER_IIO_ERR = 0x43, // Event-only
  BIC_SENSOR_PROC_HOT_EXT = 0x51, // Event-only
  BIC_SENSOR_POWER_ERR = 0x56, // Event-only
  BIC_SENSOR_MEM_ECC_ERR = 0x63, // Event-only
  BIC_SENSOR_CAT_ERR = 0xEB, // Event-only
};

typedef int (*_get_plat_guid)(uint8_t, char*);
typedef int (*_set_plat_guid)(uint8_t, char*);
typedef int (*_is_fru_prsnt)(uint8_t, uint8_t*);
typedef int (*_get_board_id)(uint8_t, uint8_t*, uint8_t, uint8_t*, uint8_t*);
typedef int (*_get_80port_record)(uint8_t, uint8_t*, size_t, size_t*);
typedef int (*_get_fru_name)(uint8_t, char*);
typedef int (*_post_handle)(uint8_t, uint8_t);
typedef int (*_set_server_power)(uint8_t, uint8_t);
typedef bool (*_is_fw_update_ongoing_system)(void);
typedef int (*_is_test_board)(void);

typedef struct _ipmi_helper_funcs_t {
  _get_plat_guid get_plat_guid;
  _set_plat_guid set_plat_guid;
  _is_fru_prsnt is_fru_prsnt;
  _get_board_id get_board_id;
  _get_80port_record get_80port_record;
  _get_fru_name get_fru_name;
  _post_handle post_handle;
  _set_server_power set_server_power;
  _is_fw_update_ongoing_system is_fw_update_ongoing_system;
  _is_test_board is_test_board;
} ipmi_helper_funcs_t;

extern ipmi_helper_funcs_t* g_fv;

int ipmi_get_event_sensor_name(uint8_t fru, uint8_t* sel, char* name);
int ipmi_get_poss_pcie_config(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t req_len,
    uint8_t* res_data,
    uint8_t* res_len);
int ipmi_get_boot_order(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t* boot,
    uint8_t* res_len);
int ipmi_set_boot_order(
    uint8_t slot,
    uint8_t* boot,
    uint8_t* res_data,
    uint8_t* res_len);
uint8_t ipmi_set_power_restore_policy(
    uint8_t slot,
    uint8_t* pwr_policy,
    uint8_t* res_data);
void ipmi_update_ts_sled(void);
int ipmi_get_sysfw_ver(uint8_t slot, uint8_t* ver);
// int ipmi_parse_sel(uint8_t fru, uint8_t *sel, char *error_log);
int ipmi_chassis_control(uint8_t slot, uint8_t* req_data, uint8_t req_len);
int ipmi_bmc_reboot(int cmd);
int ipmi_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t* event_data);
int ipmi_is_crashdump_ongoing(uint8_t fru);
int ipmi_is_cplddump_ongoing(uint8_t fru);
int ipmi_parse_oem_sel(uint8_t fru, uint8_t* sel, char* error_log);
int ipmi_parse_oem_unified_sel(uint8_t fru, uint8_t* sel, char* error_log);
int ipmi_get_server_power(uint8_t slot_id, uint8_t* status);
int ipmi_get_plat_guid(uint8_t fru, char* guid);
int ipmi_set_plat_guid(uint8_t fru, char* str);
int ipmi_set_sysfw_ver(uint8_t slot, uint8_t* ver);
int ipmi_set_ppin_info(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t req_len,
    uint8_t* res_data,
    uint8_t* res_len);
void ipmi_get_chassis_status(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t* res_data,
    uint8_t* res_len);
int ipmi_is_slot_server(uint8_t fru);
uint8_t ipmi_set_slot_power_policy(uint8_t* pwr_policy, uint8_t* res_data);
int ipmi_get_fruid_path(uint8_t fru, char* path);
bool ipmi_parse_sel_helper(uint8_t fru, uint8_t* sel, char* error_log);
//
// not overridenable APIs:
//
int set_restart_cause(uint8_t slot, uint8_t restart_cause);
int get_restart_cause(uint8_t slot, uint8_t* restart_cause);
void add_cri_sel(char* str);
void populate_guid(char* guid, char* str);
int set_guid(char* guid);
int max_slot_num(int* num);
int run_command(const char* cmd);
void init_func_vector();
void set_plat_funcs();
void msleep(int msec);
