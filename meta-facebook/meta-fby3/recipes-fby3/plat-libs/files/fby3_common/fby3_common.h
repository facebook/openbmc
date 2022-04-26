/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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

#ifndef __FBY3_COMMON_H__
#define __FBY3_COMMON_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef GETBIT
#define GETBIT(x, y)  ((x & (1ULL << y)) > y)
#endif

#define BIC_CACHED_PID "/var/run/bic-cached_%d.lock"

#define SOCK_PATH_ASD_BIC "/tmp/asd_bic_socket"
#define SOCK_PATH_JTAG_MSG "/tmp/jtag_msg_socket"

#define FRU_NIC_BIN    "/tmp/fruid_nic.bin"
#define FRU_BMC_BIN    "/tmp/fruid_bmc.bin"
#define FRU_BB_BIN     "/tmp/fruid_bb.bin"
#define FRU_NICEXP_BIN "/tmp/fruid_nicexp.bin"
#define FRU_SLOT_BIN   "/tmp/fruid_slot%d.bin"
#define FRU_DEV_PATH   "/tmp/fruid_slot%d_dev%d.bin"

#define SLOT_BUS_BASE 3
#define BB_CPLD_BUS 12
#define NIC_CPLD_BUS 9
#define CPLD_ADDRESS 0x1E
#define BB_CPLD_BOARD_REV_ID_REGISTER 0x08
#define SB_CPLD_BOARD_REV_ID_REGISTER 0x07

#define NIC_FRU_BUS     8
#define CLASS1_FRU_BUS 11
#define CLASS2_FRU_BUS 10
#define BMC_FRU_ADDR 0x54
#define BB_FRU_ADDR  0x51
#define NICEXP_FRU_ADDR 0x51
#define NIC_FRU_ADDR 0x50
#define I2C_PATH "/sys/class/i2c-dev/i2c-%d/device/new_device"
#define EEPROM_PATH "/sys/bus/i2c/devices/%d-00%X/eeprom"

#define CPLD_BOARD_OFFSET  0x0D

#define SLOT_SENSOR_LOCK "/var/run/slot%d_sensor.lock"

extern const char *slot_usage;

#define MAX_NUM_FRUS 13
#define MAX_NUM_EXPS 4

#define MAX_SYS_REQ_LEN  128  //include the string terminal
#define MAX_SYS_RESP_LEN 128  //include the string terminal

#define SEL_RECORD_ID_LEN    2
#define SEL_TIMESTAMP_LEN    4
#define SEL_GENERATOR_ID_LEN 2
#define SEL_EVENT_DATA_LEN   3
#define SEL_SYS_EVENT_RECORD 0x2
#define SEL_IPMI_V2_REV      0x4
#define SEL_SNR_TYPE_FAN     0x4
#define SEL_SNR_TYPE_FW_STAT 0xF
#define SEL_SNR_TYPE_PWR_LOCK 0x10
#define SEL_SNR_TYPE_ACK_SEL  0x11

#define SYS_FAN_EVENT        0x14
#define SYS_BB_FW_UPDATE     0x15
#define SYS_SEL_ACK          0x18
#define SYS_SLED_PWR_LOCK    0x60

#define MAX_BYPASS_DATA_LEN  256
#define IANA_LEN 3
#define UNKNOWN_SLOT 0xFF
#define DUAL_DEV_BIC_ID0 15

#define FRU_NAME_2U_CWC "slot1-2U-exp"
#define FRU_NAME_2U_TOP "slot1-2U-top"
#define FRU_NAME_2U_BOT "slot1-2U-bot"

#define DEV_NAME_2U_TOP "slot1-2U-top"
#define DEV_NAME_2U_BOT "slot1-2U-bot"
#define DEV_NAME_2U_CWC "slot1-2U-cwc"
#define DEV_NAME_2U "slot1-2U"

enum {
  FRU_ALL       = 0,
  FRU_SLOT1     = 1,
  FRU_SLOT2     = 2,
  FRU_SLOT3     = 3,
  FRU_SLOT4     = 4,
  FRU_BB        = 5,
  FRU_NIC       = 6,
  FRU_BMC       = 7,
  FRU_NICEXP    = 8, //the fru is used when bmc is located on class 2
  FRU_EXP_BASE  = 9,
  FRU_2U        = 9,
  FRU_CWC       = 10,
  FRU_2U_TOP    = 11,
  FRU_2U_BOT    = 12,
  FRU_2U_SLOT3  = 13,
  FRU_AGGREGATE = 0xff, //sensor-util will call pal_get_fru_name(). Add this virtual fru for sensor-util.
};

enum {
  DEV_ID0_1OU = 0x1,
  DEV_ID1_1OU = 0x2,
  DEV_ID2_1OU = 0x3,
  DEV_ID3_1OU = 0x4,
  DEV_ID0_2OU = 0x5,
  DEV_ID1_2OU = 0x6,
  DEV_ID2_2OU = 0x7,
  DEV_ID3_2OU = 0x8,
  DEV_ID4_2OU = 0x9,
  DEV_ID5_2OU = 0xA,
  DEV_ID6_2OU = 0xB,
  DEV_ID7_2OU = 0xC,
  DEV_ID8_2OU = 0xD,
  DEV_ID9_2OU = 0xE,
  DEV_ID10_2OU = 0xF,
  DEV_ID11_2OU = 0x10,
  DEV_ID12_2OU = 0x11,
  DEV_ID13_2OU = 0x12,
  BOARD_1OU,
  BOARD_2OU,
  MAX_NUM_DEVS,
  BOARD_2OU_TOP = MAX_NUM_DEVS,
  BOARD_2OU_BOT,
  BOARD_2OU_CWC,
  DUAL_DEV_ID0_2OU,
  DUAL_DEV_ID1_2OU,
  DUAL_DEV_ID2_2OU,
  DUAL_DEV_ID3_2OU,
  DUAL_DEV_ID4_2OU,
  DUAL_DEV_ID5_2OU,

  MAX_NUM_DEVS_CWC,
};

enum {
  FRU_ID_SERVER     = 0,
  FRU_ID_BMC        = 1,
  FRU_ID_NIC        = 2,
  FRU_ID_BB         = 3,
  FRU_ID_NICEXP     = 4,
  FRU_ID_BOARD_1OU  = 5,
  FRU_ID_BOARD_2OU  = 6,
  FRU_ID_1OU_DEV0   = 7,
  FRU_ID_1OU_DEV1   = 8,
  FRU_ID_1OU_DEV2   = 9,
  FRU_ID_1OU_DEV3   = 10,
  FRU_ID_2OU_DEV0   = 11,
  FRU_ID_2OU_DEV1   = 12,
  FRU_ID_2OU_DEV2   = 13,
  FRU_ID_2OU_DEV3   = 14,
  FRU_ID_2OU_DEV4   = 15,
  FRU_ID_2OU_DEV5   = 16,
  FRU_ID_2OU_DEV6   = 17,
  FRU_ID_2OU_DEV7   = 18,
  FRU_ID_2OU_DEV8   = 19,
  FRU_ID_2OU_DEV9   = 20,
  FRU_ID_2OU_DEV10  = 21,
  FRU_ID_2OU_DEV11  = 22,
  FRU_ID_2OU_DEV12  = 23,
  FRU_ID_2OU_DEV13  = 24,
  FRU_ID_BOARD_2OU_TOP = 30,
  FRU_ID_BOARD_2OU_BOT = 31,
  FRU_ID_BOARD_2OU_CWC = 32,
};

enum {
  IPMB_SLOT1_I2C_BUS = 0,
  IPMB_SLOT2_I2C_BUS = 1,
  IPMB_SLOT3_I2C_BUS = 2,
  IPMB_SLOT4_I2C_BUS = 3,
};

enum {
  // BOARD_ID [0:3] 
  NIC_BMC = 0x09, // 1001
  BB_BMC  = 0x0E, // 1110
  DVT_BB_BMC  = 0x07, // 0111
  EDSFF_1U = 0x07,  // 0111
};

// Server type
enum {
  SERVER_TYPE_DL = 0x0,
  SERVER_TYPE_NONE = 0xFF,
};

enum {
  UTIL_EXECUTION_OK = 0,
  UTIL_EXECUTION_FAIL = -1,
};

enum {
  STATUS_PRSNT = 0,
  STATUS_NOT_PRSNT,
  STATUS_ABNORMAL,
};

// 2OU Board type
enum {
  M2_BOARD = 0x01,
  E1S_BOARD = 0x02,
  GPV3_MCHP_BOARD = 0x03,
  GPV3_BRCM_BOARD = 0x00,
  DP_RISER_BOARD = 0x06,
  CWC_MCHP_BOARD = 0x04,
  UNKNOWN_BOARD = 0xff,
};

enum {
  HSC_DET_ADM1278 = 0,
  HSC_DET_LTC4282,
  HSC_DET_MP5990,
  HSC_DET_ADM1276,
};

enum fan_mode {
  FAN_MANUAL_MODE = 0,
  FAN_AUTO_MODE,
};

enum service_act {
  SV_STOP = 0,
  SV_START,
};

enum sel_dir {
  SEL_ASSERT = 0x6F,
  SEL_DEASSERT = 0xEF,
};

const static char *gpio_server_prsnt[] =
{
  "",
  "PRSNT_MB_BMC_SLOT1_BB_N",
  "PRSNT_MB_BMC_SLOT2_BB_N",
  "PRSNT_MB_BMC_SLOT3_BB_N",
  "PRSNT_MB_BMC_SLOT4_BB_N"
};

const static char *gpio_server_stby_pwr_sts[] =
{
  "",
  "PWROK_STBY_BMC_SLOT1",
  "PWROK_STBY_BMC_SLOT2",
  "PWROK_STBY_BMC_SLOT3",
  "PWROK_STBY_BMC_SLOT4"
};

const static char *gpio_server_i2c_isolated[] =
{
  "",
  "FM_BMC_SLOT1_ISOLATED_EN_R",
  "FM_BMC_SLOT2_ISOLATED_EN_R",
  "FM_BMC_SLOT3_ISOLATED_EN_R",
  "FM_BMC_SLOT4_ISOLATED_EN_R"
};

typedef struct {
  uint8_t iana_id[IANA_LEN];
  uint8_t bypass_intf;
  uint8_t bypass_data[MAX_BYPASS_DATA_LEN];
} BYPASS_MSG;

typedef struct {
  uint8_t iana_id[IANA_LEN];
  uint8_t bypass_intf;
} BYPASS_MSG_HEADER;

typedef struct {
  uint8_t netfn;
  uint8_t cmd;
  uint8_t record_id[SEL_RECORD_ID_LEN];
  uint8_t record_type;
  uint8_t timestamp[SEL_TIMESTAMP_LEN];
  uint8_t slave_addr;
  uint8_t channel_num;
  uint8_t rev;
  uint8_t snr_type;
  uint8_t snr_num;
  uint8_t event_dir_type;
  uint8_t event_data[SEL_EVENT_DATA_LEN];
} IPMI_SEL_MSG;

typedef struct {
  uint8_t index;
} GET_MB_INDEX_RESP;

typedef struct {
  uint8_t type;
  uint8_t slot;
  uint8_t mode;
} FAN_SERVICE_EVENT;

typedef struct {
  uint8_t type;
  uint8_t component;
} BB_FW_UPDATE_EVENT;

int fby3_common_set_fru_i2c_isolated(uint8_t fru, uint8_t val);
int fby3_common_is_bic_ready(uint8_t fru, uint8_t *val);
int fby3_common_server_stby_pwr_sts(uint8_t fru, uint8_t *val);
int fby3_common_get_bmc_location(uint8_t *id);
int fby3_common_get_fru_id(char *str, uint8_t *fru);
int fby3_common_check_slot_id(uint8_t fru);
int fby3_common_exp_get_num_devs(uint8_t fru, uint8_t *num);
int fby3_common_get_slot_id(char *str, uint8_t *fru);
int fby3_common_get_bus_id(uint8_t slot_id);
int fby3_common_is_fru_prsnt(uint8_t fru, uint8_t *val);
int fby3_common_get_slot_type(uint8_t fru);
int fby3_common_crashdump(uint8_t fru, bool ierr, bool platform_reset);
int fby3_common_dev_id(char *str, uint8_t *dev);
int fby3_common_dev_name(uint8_t dev, char *str);
int fby3_common_get_2ou_board_type(uint8_t fru_id, uint8_t *board_type);
int fby3_common_get_exp_dev_id(char *str, uint8_t *dev);
int fby3_common_exp_dev_name(uint8_t dev, char *str);
int fby3_common_get_bb_board_rev(uint8_t *rev);
int fby3_common_get_sb_board_rev(uint8_t slot_id, uint8_t *rev);
int fby3_common_get_hsc_bb_detect(uint8_t *id);
int fby3_common_fscd_ctrl(uint8_t mode);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBY3_COMMON_H__ */
