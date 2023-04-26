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

#ifndef __FBY35_COMMON_H__
#define __FBY35_COMMON_H__

#include <stdbool.h>
#include <openssl/evp.h>
#include <sys/stat.h>

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

#define FRU_ID_COMPONENT_VER_KEY "slot%d_%s_ver"

#define NEW_VER_KEY "_new_ver"
#define FRU_STR_COMPONENT_NEW_VER_KEY "%s_%s" NEW_VER_KEY
#define FRU_STR_CPLD_NEW_VER_KEY "%s_cpld" NEW_VER_KEY
#define FRU_ID_CPLD_NEW_VER_KEY "slot%d_cpld" NEW_VER_KEY

#define NIC_CPLD_BUS 9
#define BB_CPLD_BUS  12
#define BB_CPLD_ADDR 0x0f
#define SB_CPLD_ADDR 0x0f

#define NIC_FRU_BUS     8
#define CLASS1_FRU_BUS 11
#define CLASS2_FRU_BUS 11
#define BMC_FRU_ADDR 0x54
#define BB_FRU_ADDR  0x51
#define NICEXP_FRU_ADDR 0x51
#define NIC_FRU_ADDR 0x50
#define DPV2_FRU_ADDR 0x51
#define PROT_FRU_ADDR 0x50
#define VPDB_VR_BUS 11
#define VPDB_VR_ADDR 0xb4
#define I2C_PATH "/sys/class/i2c-dev/i2c-%d/device/new_device"
#define EEPROM_PATH "/sys/bus/i2c/devices/%d-00%X/eeprom"
#define MAX_FRU_PATH_LEN 128
#define FRU_SIZE        512

#define CPLD_REG_SB_CLASS           0x00
#define CPLD_REG_UART_MUX           0x01
#define CPLD_REG_BIC_READY          0x02
#define CPLD_REG_M2_PRSNT           0x05
#define CPLD_REG_REV_ID             0x07
#define CPLD_REG_BB_REV             0x08
#define CPLD_REG_PCH_BIC_PWR_FAULT  0x09
#define CPLD_REG_CPU_PWR_FAULT      0x0A
#define CPLD_REG_RISER              0x0D
#define CPLD_REG_SB_BIC_BOOT_STRAP  0x10
#define CPLD_REG_BOARD              0x11
#define CPLD_REG_CPLD_STATUS        0x13
#define CPLD_REG_1OU_BIC_BOOT_STRAP 0x22
#define CPLD_REG_PROT               0x23

#define BIC_SRST_SHADOW    "BIC_SRST_N_IO_EXP_R_SLOT%u"
#define BIC_EXTRST_SHADOW  "BIC_EXTRST_N_IO_EXP_R_SLOT%u"
#define BIC_FWSPICK_SHADOW "BIC_FWSPICK_IO_EXP_SLOT%u"

#define SLOT_SENSOR_LOCK "/var/run/slot%d_sensor.lock"

extern const char *slot_usage;

#define MAX_NUM_FRUS 9

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

#define SYS_FAN_EVENT        0x14
#define SYS_BB_FW_UPDATE     0x15

#define MAX_BYPASS_DATA_LEN  256
#define UNKNOWN_SLOT 0xFF

#define BIOS_IMG_SIZE             (0x4000000)         //64MB
#define CL_BIOS_IMG_INFO_OFFSET   (0x2FEF000)
#define HD_BIOS_IMG_INFO_OFFSET   (0x00004000)
#define MD5_SIZE                  (16)
#define PLAT_SIG_SIZE             (16)
#define FW_VER_SIZE               (13)
#define ERR_PROOF_SIZE            (3)
#define IMG_MD5_OFFSET            (0x0)
#define IMG_PROJECT_CODE_OFFSET   (IMG_MD5_OFFSET + MD5_SIZE)
#define IMG_FW_VER_OFFSET         (IMG_PROJECT_CODE_OFFSET + PLAT_SIG_SIZE)
#define IMG_SIGNED_INFO_SIZE      (MD5_SIZE + PLAT_SIG_SIZE + FW_VER_SIZE + ERR_PROOF_SIZE + MD5_SIZE)

#define MD5_READ_BYTES     (1024)

#define REVISION_ID(x)  (x & 0x0f)
#define BOARD_ID(x)     (x & 0x0f)
#define COMPONENT_ID(x) (x >> 7)

#define FRU_DEVICE_BUS(fru) ((fru) + 3)

#define KEY_BB_HSC_TYPE "bb_hsc_type"

#define IANA_ID_SIZE 3

// BB cpld reg
#define SLED_STATUS_REG 0x0D
#define FRUID_0 0

#define ASPEED_PID "7ec80010000"
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
  FRU_OCPDBG    = 9,
  FRU_AGGREGATE = 0xff, //sensor-util will call pal_get_fru_name(). Add this virtual fru for sensor-util.
};

enum {
  UART_SELECT_BMC = 0x0,
  UART_SELECT_SLOT1,
  UART_SELECT_SLOT2,
  UART_SELECT_SLOT3,
  UART_SELECT_SLOT4,
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
  DEV_ID0_3OU = 0xA,
  DEV_ID1_3OU = 0xB,
  DEV_ID2_3OU = 0xC,
  DEV_ID3_3OU = 0xD,
  DEV_ID0_4OU = 0xE,
  DEV_ID1_4OU = 0xF,
  DEV_ID2_4OU = 0x10,
  DEV_ID3_4OU = 0x11,
  DEV_ID4_4OU = 0x12,
  BOARD_1OU = 0x13,
  BOARD_2OU = 0x14,
  BOARD_3OU = 0x15,
  BOARD_4OU = 0x16,
  BOARD_2OU_X8 = 0x17,
  BOARD_2OU_X16 = 0x18,
  BOARD_PROT = 0x19,
  MAX_NUM_DEVS,

  DEV_ID5_2OU,
  DEV_ID6_2OU,
  DEV_ID7_2OU,
  DEV_ID8_2OU,
  DEV_ID9_2OU,
  DEV_ID10_2OU,
  DEV_ID11_2OU,
  DEV_ID12_2OU,
  DEV_ID13_2OU,
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
  FRU_ID_2OU_X8     = 25,
  FRU_ID_2OU_X16    = 26,
  FRU_ID_PROT       = 27,
  FRU_ID_BOARD_3OU  = 28,
  FRU_ID_BOARD_4OU  = 29,
};

enum {
  SLOT1_BUS = 0,
  SLOT2_BUS = 1,
  SLOT3_BUS = 2,
  SLOT4_BUS = 3,

  OCP_DBG_I2C_BUS    = 9,
};

enum {
  // BOARD_ID [3:0]
  NIC_BMC = 0x06,
  BB_BMC  = 0x07,
};

// 1OU Board ID
enum {
  VF_1U = 0x07,
  OP_1U = 0x08,
  RF_1U = 0x0A,
};

// 2OU Board type
enum {
  M2_BOARD = 0x01,
  E1S_BOARD = 0x02,
  DPV2_X8_BOARD = 0x07,
  DPV2_X16_BOARD = 0x70,
  DPV2_BOARD = 0x77,
  UNKNOWN_BOARD = 0xFF,
};

// 1OU card type
enum {
  TYPE_1OU_SI_TEST_CARD = 0x0,
  TYPE_1OU_EXP_WITH_6_M2,
  TYPE_1OU_RAINBOW_FALLS,
  TYPE_1OU_VERNAL_FALLS_WITH_TI,  // TI BIC
  TYPE_1OU_VERNAL_FALLS_WITH_AST, // AST1030 BIC
  TYPE_1OU_KAHUNA_FALLS,
  TYPE_1OU_WAIMANO_FALLS,
  TYPE_1OU_EXP_WITH_NIC,
  TYPE_1OU_OLMSTEAD_POINT,
  TYPE_1OU_ABSENT = 0xFE,
  TYPE_1OU_UNKNOWN = 0xFF,
};

// 2OU card type
enum {
  TYPE_2OU_DPV2_8 = 0x07, // DPV2x8
  TYPE_2OU_DPV2_16 = 0x70, // DPV2x16
  TYPE_2OU_DPV2_8_16 = 0x77, // DPV2x8 + DPV2x16
  TYPE_2OU_ABSENT = 0xFE,
  TYPE_2OU_UNKNOWN = 0xFF,
};

// Server type
enum {
  SERVER_TYPE_CL = 0x0,
  SERVER_TYPE_HD = 0x2,
  SERVER_TYPE_GL = 0x4,
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

enum fan_mode {
  FAN_MANUAL_MODE = 0,
  FAN_AUTO_MODE,
};

enum sel_dir {
  SEL_ASSERT = 0x6F,
  SEL_DEASSERT = 0xEF,
};

enum comp_id {
  COMP_CPLD = 1,
  COMP_BIC  = 2,
  COMP_BIOS = 3,
  COMP_CXL  = 4,
};

enum fw_rev {
  FW_REV_POC = 0,
  FW_REV_EVT,
  FW_REV_DVT,
  FW_REV_PVT,
  FW_REV_MP ,
};

enum brd_rev {
  BB_REV_POC1   = 0,
  BB_REV_POC2   = 1,
  BB_REV_EVT    = 2,
  BB_REV_EVT2   = 3,
  BB_REV_EVT3   = 4,
  BB_REV_DVT    = 5,
  BB_REV_DVT_1C = 6,
  BB_REV_PVT    = 7,
  BB_REV_MP     = 8,

  SB_REV_POC     = 0,
  SB_REV_EVT     = 1,
  SB_REV_EVT2    = 2,
  SB_REV_EVT3    = 3,
  SB_REV_EVT_MPS = 4,
  SB_REV_DVT     = 5,
  SB_REV_DVT_MPS = 6,
  SB_REV_PVT     = 7,
  SB_REV_PVT_MPS = 8,
  SB_REV_MP      = 9,
  SB_REV_MP_MPS  = 10,

  HD_REV_POC  = 0,
  HD_REV_EVT  = 1,
  HD_REV_EVT1 = 2,
  HD_REV_EVT2 = 3,
  HD_REV_DVT  = 4,
  HD_REV_DVT1 = 5,
  HD_REV_DVT2 = 6,
  HD_REV_DVT3 = 7,
  HD_REV_PVT  = 8,

  OP_REV_EVT = 0,
  OP_REV_DVT = 1,
  OP_REV_PVT = 2,
  OP_REV_MP = 3,

  GL_REV_POC = 0,
  GL_REV_EVT = 1,
  GL_REV_DVT = 3,
  GL_REV_PVT = 5,
  GL_REV_MP = 6,

  UNKNOWN_REV = 0xFF,
};

enum board_id {
  BOARD_ID_SB = 1,
  BOARD_ID_BB = 2,
  BOARD_ID_RF = 3,
  BOARD_ID_HD = 4,
  BOARD_ID_OP = 5,
  BOARD_ID_NIC_EXP = 6,
  BOARD_ID_VF = 7,
  BOARD_ID_GL = 8,
  BOARD_ID_NF = 9,
};

enum {
  FW_CPLD = 1,
  FW_SB_BIC,
  FW_ME,
  FW_BIC_RCVY,
  FW_VR_VCCIN,         //5
  FW_VR_VCCD,
  FW_VR_VCCINFAON,
  FW_VR_VDDCRCPU0,
  FW_VR_VDDCRCPU1,
  FW_VR_VDD11S3,       //10
  FW_VR,
  FW_BIOS,
  FW_BIOS_SPIB,//Flash on PRoT Module
  FW_PROT,
  FW_PROT_SPIB,//Flash on PRoT Module
  FW_1OU_BIC,
  FW_1OU_BIC_RCVY,     //15
  FW_1OU_CPLD,
  FW_2OU_BIC,
  FW_2OU_BIC_RCVY,
  FW_2OU_CPLD,
  FW_BB_BIC,           //20
  FW_BB_CPLD,
  FW_1OU_CXL,
  FW_1OU_VR,
  FW_1OU_VR_V9_ASICA,
  FW_1OU_VR_VDDQAB,    //25
  FW_1OU_VR_VDDQCD,
  FW_2OU_3V3_VR1,
  FW_2OU_3V3_VR2,
  FW_2OU_3V3_VR3,
  FW_2OU_1V8_VR,       //30
  FW_2OU_PESW_VR,
  FW_2OU_PESW_CFG_VER,
  FW_2OU_PESW_FW_VER,
  FW_2OU_PESW_BL0_VER,
  FW_2OU_PESW_BL1_VER, //35
  FW_2OU_PESW_PART_MAP0_VER,
  FW_2OU_PESW_PART_MAP1_VER,
  FW_2OU_PESW,
  FW_2OU_M2_DEV0,
  FW_2OU_M2_DEV1,      //40
  FW_2OU_M2_DEV2,
  FW_2OU_M2_DEV3,
  FW_2OU_M2_DEV4,
  FW_2OU_M2_DEV5,
  FW_2OU_M2_DEV6,      //45
  FW_2OU_M2_DEV7,
  FW_2OU_M2_DEV8,
  FW_2OU_M2_DEV9,
  FW_2OU_M2_DEV10,
  FW_2OU_M2_DEV11,     //50
  FW_3OU_BIC,
  FW_3OU_BIC_RCVY,
  FW_4OU_BIC,
  FW_4OU_BIC_RCVY,
  FW_1OU_RETIMER,      //55
  FW_3OU_RETIMER,
  FW_VR_VCCIN_EHV,
  FW_VR_VCCD_HV,
  FW_VR_VCCINF,
  FW_VPDB_VR,
  // last id
  FW_COMPONENT_LAST_ID,
};

enum {
  // BB HSC / Halfdome HSC
  HSC_ADM1278 = 0,
  HSC_LTC4286,
  HSC_MP5990,
  // MEDUSA HSC
  MEDUSA_HSC_LTC4287,
  MEDUSA_HSC_LTC4282,
  MEDUSA_HSC_ADM1272,
  HSC_UNKNOWN,
};

enum {
  PRESENT = 0,
  NOT_PRESENT = 1,
};

enum {
  KEY_CLEAR = 0,
  KEY_SET,
};

enum {
  CARD_TYPE_1OU = 0x0,
  CARD_TYPE_2OU,
};

enum POWER_LOCK {
  UNLOCK = 0x0,
  LOCK
};

typedef enum {
  SLOT_NOT_PRESENT = 0,
  SLOT_PRESENT,
} PRESENT_STATUS;

enum SB_CPLD_STATUS {
  CPLD_VALID = 0x0,
};

typedef enum {
  CPLD_READ = 0,
  CPLD_WRITE = 1,
} CPLD_ACTION;

typedef enum {
  ISOLATED_ENABLE = 0,
  ISOLATED_DISABLE = 1,
} SMBUS_ISOLATED_STATUS;

static const char *gpio_server_prsnt[] =
{
  "",
  "PRSNT_MB_BMC_SLOT1_BB_N",
  "PRSNT_MB_SLOT2_BB_N",
  "PRSNT_MB_BMC_SLOT3_BB_N",
  "PRSNT_MB_SLOT4_BB_N"
};

static const char *gpio_server_stby_pwr_sts[] =
{
  "",
  "PWROK_STBY_BMC_SLOT1_R",
  "PWROK_STBY_BMC_SLOT2",
  "PWROK_STBY_BMC_SLOT3_R",
  "PWROK_STBY_BMC_SLOT4"
};

static const char *gpio_server_i2c_isolated[] =
{
  "",
  "FM_BMC_SLOT1_ISOLATED_EN_R",
  "FM_BMC_SLOT2_ISOLATED_EN_R",
  "FM_BMC_SLOT3_ISOLATED_EN_R",
  "FM_BMC_SLOT4_ISOLATED_EN_R"
};

typedef struct {
  uint8_t iana_id[IANA_ID_SIZE];
  uint8_t bypass_intf;
  uint8_t bypass_data[MAX_BYPASS_DATA_LEN];
} BYPASS_MSG;

typedef struct {
  uint8_t iana_id[IANA_ID_SIZE];
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

typedef struct {
  uint8_t md5_sum[MD5_SIZE];
  uint8_t plat_sig[PLAT_SIG_SIZE];
  uint8_t version[FW_VER_SIZE];
  uint8_t err_proof[ERR_PROOF_SIZE];
  uint8_t md5_sum_second[MD5_SIZE];
} FW_IMG_INFO;

int fby35_common_set_fru_i2c_isolated(uint8_t fru, uint8_t val);
int fby35_common_is_bic_ready(uint8_t fru, uint8_t *val);
int fby35_common_server_stby_pwr_sts(uint8_t fru, uint8_t *val);
int fby35_common_server_smbus_isolated(uint8_t fru, uint8_t *val);
int fby35_common_get_bmc_location(uint8_t *id);
int fby35_common_get_fru_id(char *str, uint8_t *fru);
int fby35_common_check_slot_id(uint8_t fru);
int fby35_common_get_slot_id(char *str, uint8_t *fru);
int fby35_common_get_bus_id(uint8_t slot_id);
int fby35_common_is_fru_prsnt(uint8_t fru, uint8_t *val);
int fby35_common_get_slot_type(uint8_t fru);
int fby35_common_get_sb_rev(uint8_t fru);
int fby35_common_get_1ou_m2_prsnt(uint8_t fru);
int fby35_common_get_sb_pch_bic_pwr_fault(uint8_t fru);
int fby35_common_get_sb_cpu_pwr_fault(uint8_t fru);
int fby35_common_get_sb_bic_boot_strap(uint8_t fru);
int fby35_common_get_bb_rev(void);
int fby35_common_crashdump(uint8_t fru, bool ierr, bool platform_reset);
int fby35_common_dev_id(char *str, uint8_t *dev);
int fby35_common_dev_name(uint8_t dev, char *str);
int fby35_common_get_2ou_board_type(uint8_t fru, uint8_t *board_type);
int fby35_common_fscd_ctrl (uint8_t mode);
int fby35_common_check_image_signature(uint8_t board_id, uint8_t *data);
int fby35_common_get_img_ver(const char* image_path, char* ver, uint8_t comp);
int fby35_common_check_image_md5(const char* image_path, int cal_size, uint8_t *data, bool is_first, uint8_t comp, uint8_t board_id);
uint8_t fby35_zero_checksum_calculate(uint8_t *buf, uint8_t len);
bool fby35_is_zero_checksum_valid(uint8_t *buf, uint8_t len);
bool fby35_common_is_valid_img(const char* img_path, uint8_t comp, uint8_t board_id, uint8_t rev_id);
int fby35_common_get_bb_hsc_type(uint8_t* type);
bool fby35_common_is_prot_card_prsnt(uint8_t fru) ;
bool fby35_common_is_prot_auth_complete(uint8_t fru) ;
int copy_eeprom_to_bin(const char *eeprom_file, const char *bin_file);
bool fby35_common_is_sb_cpld_valid(uint8_t fru);
int fby35_common_sb_set_amber_led(uint8_t fru, uint8_t *tbuf, uint8_t tlen);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBY35_COMMON_H__ */
