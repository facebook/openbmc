/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#ifndef __FBGC_COMMON_H__
#define __FBGC_COMMON_H__

#include <stdbool.h>
#include <stdint.h>
#include <openssl/md5.h>
#include <openbmc/ipmi.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EEPROM_PATH              "/sys/bus/i2c/devices/%d-00%X/eeprom"
#define COMMON_FRU_PATH          "/tmp/fruid_%s.bin"
#define COMMON_FAN_FRU_PATH      "/mnt/data/fruid_%s.bin"
#define COMMON_TMP_FRU_PATH      "/tmp/tfruid_%s.bin"
#define FRU_BMC_BIN              "/tmp/fruid_bmc.bin"
#define FRU_UIC_BIN              "/tmp/fruid_uic.bin"
#define FRU_NIC_BIN              "/tmp/fruid_nic.bin"
#define FRU_IOCM_BIN             "/tmp/fruid_iocm.bin"

#define BMC_FRU_ADDR  0x54
#define UIC_FRU_ADDR  0x50
#define NIC_FRU_ADDR  0x50
#define IOCM_FRU_ADDR 0x50


//UIC FPGA slave address (8-bit)
#define UIC_FPGA_SLAVE_ADDR 0x1e
#define UIC_FPGA_SLAVE_AC_POWER_OFFSET 0x00 //GC2

//BS FPGA slave address (8-bit)
#define BS_FPGA_SLAVE_ADDR 0x1e

//ES FPGA slave address (7-bit)
#define I2C_ES_FPGA_BUS 3
#define ES_FPGA_SLAVE_ADDR 0x0f
#define ES_FPGA_SLAVE_DC_POWER_OFFSET 0x22 //GC2

// Expander slave address (7-bit)
#define EXPANDER_SLAVE_ADDR    0x71

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#endif

#define MIN(a , b)      ((a) < (b) ? (a) : (b))
#define MAX(a , b)      ((a) > (b) ? (a) : (b))

#define SERVER_SENSOR_LOCK "/var/run/sensor_read_server.lock"
#define POWER_UTIL_LOCK "/var/run/power-util_%d.lock"

#define MAX_PATH_LEN 128  // include the string terminal
#define E1S0_IOCM_PRESENT_BIT   (1 << 0)
#define E1S1_IOCM_PRESENT_BIT   (1 << 1)

#define I2C_BASE           0x1e78a000
#define I2C_BASE_INTERVAL  0x80
#define I2C_BASE_MAP(bus)  (I2C_BASE + (((bus) + 1) * I2C_BASE_INTERVAL))

#define CHASSIS_TYPE_BIT_0(value)   (value << 0)
#define CHASSIS_TYPE_BIT_1(value)   (value << 1)
#define CHASSIS_TYPE_BIT_2(value)   (value << 2)
#define CHASSIS_TYPE_BIT_3(value)   (value << 3)

#define WAIT_POWER_STATUS_CHANGE_TIME 30 // second

//                 UIC_LOC_TYPE_IN   UIC_RMT_TYPE_IN   SCC_LOC_TYPE_0   SCC_RMT_TYPE_0
// Type 5                        0                 0                0                0
// Type 7 Headnode               0                 1                0                1

#define CHASSIS_TYPE_5_VALUE (CHASSIS_TYPE_BIT_3(0) | CHASSIS_TYPE_BIT_2(0) | CHASSIS_TYPE_BIT_1(0) | CHASSIS_TYPE_BIT_0(0)) // 0000b
#define CHASSIS_TYPE_7_VALUE (CHASSIS_TYPE_BIT_3(0) | CHASSIS_TYPE_BIT_2(1) | CHASSIS_TYPE_BIT_1(0) | CHASSIS_TYPE_BIT_0(1)) // 0101b


#define MAX_SYS_CMD_REQ_LEN  100  // include the string terminal
#define MAX_SYS_CMD_RESP_LEN 100  // include the string terminal

#define IPMI_NETFN_SHIFT(netfn) ((netfn) << 2)

#define SOCK_PATH_ASD_BIC "/tmp/asd_bic_socket_1"
#define SOCK_PATH_JTAG_MSG "/tmp/jtag_msg_socket_1"

#ifdef CONFIG_GRANDCANYON2
#define BOARD_ID_PIN_NUM 4
#else
#define BOARD_ID_PIN_NUM 3
#endif

// For IOC Daemon
#define SOCK_PATH_IOC      "ioc_socket_%d"
#define MAX_SOCK_PATH_SIZE (64)

#define IOC_FW_VER_SIZE    (8)
#define TIMEOUT_IOC        (5) //Unit: second

#define MAX_POSTCODE_LEN    256
#define POST_CODE_FILE      "/tmp/post_code_buffer.bin"
#define LAST_POST_CODE_FILE "/tmp/last_post_code_buffer.bin"


#define SKU_UIC_ID_SIZE    2
#define SKU_UIC_TYPE_SIZE  4
#define SKU_SIZE           (SKU_UIC_ID_SIZE + SKU_UIC_TYPE_SIZE)
#define MAX_SKU_VALUE      (1 << SKU_SIZE)
#define SYSTEM_INFO        "system_info"

#define MD5_READ_BYTES     (1024)

#define PLAT_SIG_SIZE      (16)
//The delay of power control
#define DELAY_DC_POWER_CYCLE 5
#define DELAY_DC_POWER_OFF 6
#define DELAY_GRACEFUL_SHUTDOWN 1
#define DELAY_DC_POWER_ON 1
#define DELAY_RESET 1

#define PWR_CTRL_ACT_CNT 3

#define MAX_RETRY        (3)

#define BS_FPGA_BOARD_REV_ID_OFFSET (0x07)
#define ES_FPGA_BOARD_REV_ID_OFFSET (0x08)

#define UIC_FPGA_UART_BRIDGING_OFFSET (0x13)

#ifdef CONFIG_GRANDCANYON2
#define IANA_ID_SIZE 3
#endif

#define GRANDCANYON2 "Grand Canyon V2.0"
#define GRANDCANYON  "Grand Canyon"
#define MD5_OFFSET                (0x0)
#define MD5_SIZE                  (16)
#define PLAT_SIG_SIZE             (16)
#define FW_VER_SIZE               (13)
#define ERR_PROOF_SIZE            (3)
#define SUR_SIZE           (MD5_SIZE + PLAT_SIG_SIZE + FW_VER_SIZE + ERR_PROOF_SIZE)
#define SUR_TOTAL_SIZE     (SUR_SIZE+MD5_SIZE)
#define SUR_SIG_OFFSET     (MD5_OFFSET + MD5_SIZE)
#define SUR_FW_VER_OFFSET  (SUR_SIG_OFFSET + PLAT_SIG_SIZE)
#define SUR_ERR_OFFSET     (SUR_FW_VER_OFFSET + FW_VER_SIZE)
#define SUR_MD5_2_OFFSET   (SUR_ERR_OFFSET + ERR_PROOF_SIZE)

#define BIOS_SUR_OFFSET        0x02FEF000
#define BIOS_SUR_END           0x02FEFFFF
#define BIOS_MD5_SKIP_SIZE     0x1000

extern const char *board_stage[];

enum {
  FW_BIC_FPGA = 1,
  FW_BIC,
  FW_ME,
  FW_BIC_BOOTLOADER,
  FW_VR,
  FW_BIOS,
  FW_BIC_RECOVERY,
  FW_BMC_FPGA
};

enum {
  NOT_MASKED= 0,
  MASKED = 1,
};

enum {
  FRU_ALL = 0,
  FRU_SERVER,
  FRU_BMC,
  FRU_UIC,
  FRU_DPB,
  FRU_SCC,
  FRU_NIC,
  FRU_E1S_IOCM,
  FRU_FAN0,
  FRU_FAN1,
  FRU_FAN2,
  FRU_FAN3,
#ifdef CONFIG_GRANDCANYON2
  FRU_PTB,
#endif
  FRU_CNT,
};

// AC Power status
enum {
  STAT_AC_OFF = 0,
  STAT_AC_ON = 1,
};

// DC power status
enum {
  STAT_DC_OFF = 0,
  STAT_DC_ON = 1,
};

enum {
  CHASSIS_TYPE5 = 0,
  CHASSIS_TYPE7,
};

enum {
  I2C_SYS_HSC_BUS = 1,
  I2C_BIC_BUS = 2,
  I2C_BS_FPGA_BUS = 3,
  I2C_UIC_BUS = 4,
  I2C_UIC_FPGA_BUS = 5,
  I2C_BSM_BUS = 6,
  I2C_DBG_CARD_BUS = 7,
  I2C_NIC_BUS = 8,
  I2C_IOEXP_BUS = 9,
  I2C_EXP_BUS = 10,
  I2C_T5IOC_BUS = 11,
  I2C_T5E1S0_T7IOC_BUS = 12,  // T5: E1.S 1; T7: IOC
  I2C_T5E1S1_T7IOC_BUS = 13,  // T5: E1.S 2; T7: IOCM FRU, Voltage sensor, Temp sensor
  I2C_SCC_BUS = 14,
  I2C_TPM_BUS = 15,  // Reserved

};

// IPMB payload ID
enum {
  PAYLOAD_BIC = 1,
  PAYLOAD_DBG_CARD = 2,
  PAYLOAD_EXP = 3,
};

// server 12v power status
enum {
  STAT_12V_OFF = 0,
  STAT_12V_ON = 1,
};

// system stage
enum {
  STAGE_PRE_EVT = 0,
  STAGE_EVT,
  STAGE_DVT,
  STAGE_PVT,
  STAGE_MP
};

#ifdef CONFIG_GRANDCANYON2
// UIC board ID stage
enum {
  UIC_STAGE_PRE_EVT = 0,
  UIC_STAGE_EVT = 1,
  UIC_STAGE_DVT = 2,
  UIC_STAGE_DVT3 = 3,
  UIC_STAGE_HACK = 6,
  UIC_STAGE_PVT_MAX15090 = 8,
  UIC_STAGE_PVT_TPS25974_RS31332_INA233 = 9,
  UIC_STAGE_MP_MAX15090 = 10,
  UIC_STAGE_MP_TPS25974_RS31332_INA233 = 11
};
#else
// UIC board ID stage
enum {
  UIC_STAGE_PRE_EVT = 0,
  UIC_STAGE_EVT,
  UIC_STAGE_DVT,
  UIC_STAGE_DVT3,
  UIC_STAGE_PVT,
  UIC_STAGE_PVT3,
  UIC_STAGE_MP
};
#endif

// GC2 board ID stage
enum {
  ES_STAGE_POC = 0,
  ES_STAGE_DVT = 2,
  ES_STAGE_PVT = 3,
  ES_STAGE_MP  = 4
};

enum {
  DEV_ID0_E1S = 0x1,
  DEV_ID1_E1S = 0x2,
  MAX_NUM_DEVS,
};

typedef struct {
  unsigned char netfn_lun;
  unsigned char cmd;
} ipmi_req_t_common_header;

typedef struct {
  uint8_t cc;
  ipmi_dev_id_t ipmi_dev_id;
} me_get_dev_id_res;

typedef struct {
  uint8_t cc;
  uint8_t data[];
} me_xmit_res;

typedef struct _platformInformation {
  char uicId[SKU_UIC_ID_SIZE];
  char uicType[SKU_UIC_TYPE_SIZE];
} platformInformation;

// Card Type
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
  TYPE_1OU_NIAGARA_FALLS,
  TYPE_1OU_ABSENT = 0xFE,
  TYPE_1OU_UNKNOWN = 0xFF,
};

typedef enum {
  SV_STOP = 0,
  SV_START,
  SV_STATUS,
} svc_mode_t;

enum board_id {
  BOARD_ID_SB = 1,
  BOARD_ID_UIC = 2,
};

enum component_id {
  COMP_CPLD = 1,
  COMP_BIC  = 2,
  COMP_BIOS = 3,
};

typedef struct {
  uint32_t version;
  uint8_t board_id;
  uint8_t fru_stage;
  uint8_t comp_id;
} sur_error_proof_info_t;

typedef struct __attribute__((packed)) {
  uint8_t md5_1[MD5_SIZE];
  char    plat_sig[PLAT_SIG_SIZE];
  uint8_t version[FW_VER_SIZE];
  uint8_t err_proof[ERR_PROOF_SIZE];
  uint8_t md5_2[MD5_SIZE];
} sur_signed_info_t;

typedef enum {
  GPIO_NAME_TYPE_UNKNOWN = -1,
  GPIO_NAME_TYPE_HACK = 0,
  GPIO_NAME_TYPE_UIC_A,
  GPIO_NAME_TYPE_UIC_B,
} gpio_name_type_t;

typedef struct {
  const char *hack_name;
  const char *uic_a_name;
  const char *uic_b_name;
} gpio_shadow_name_map_t;

typedef enum {
  GPIO_SHADOW_ID_COMP_PRSNT_N = 0,
  GPIO_SHADOW_ID_SCC_STBY_PGOOD,
  GPIO_SHADOW_ID_SCC_FULL_PGOOD,
  GPIO_SHADOW_ID_COMP_PGOOD,
  GPIO_SHADOW_ID_E1S_1_PRSNT_N,
  GPIO_SHADOW_ID_E1S_2_PRSNT_N,
  GPIO_SHADOW_ID_I2C_E1S_1_RST_N,
  GPIO_SHADOW_ID_I2C_E1S_2_RST_N,
  GPIO_SHADOW_ID_E1S_1_LED_ACT,
  GPIO_SHADOW_ID_E1S_2_LED_ACT,
  GPIO_SHADOW_ID_SCC_STBY_PWR_EN,
  GPIO_SHADOW_ID_SCC_FULL_PWR_EN,
  GPIO_SHADOW_ID_BMC_EXP_SOFT_RST_N,
  GPIO_SHADOW_ID_UIC_COMP_BIC_RST_N,
  GPIO_SHADOW_ID_E1S_1_3V3EFUSE_PGOOD,
  GPIO_SHADOW_ID_E1S_2_3V3EFUSE_PGOOD,
  GPIO_SHADOW_ID_P12V_NIC_STATUS_N,
  GPIO_SHADOW_ID_P3V3_NIC_STATUS_N,
  GPIO_SHADOW_ID_SCC_POR_RST_N,
  GPIO_SHADOW_ID_BMC_COMP_BLED,
  GPIO_SHADOW_ID_MAX,

} gpio_shadow_id_t;

int fbgc_common_get_chassis_type(uint8_t *type);
void msleep(int msec);
int fbgc_common_server_stby_pwr_sts(uint8_t *val);
uint8_t cal_crc8(uint8_t crc, uint8_t const *data, uint8_t len);
uint8_t hex_c2i(const char c);
int string_2_byte(const char* c);
bool start_with(const char *s, const char *p);
int split(char **dst, char *src, char *delim, int max_size);
int fbgc_common_get_system_stage(uint8_t *stage);
int check_image_md5(const char* image_path, int cal_size, uint32_t md5_offset);
int check_image_signature(const char* image_path, uint32_t sig_offset);
int get_server_board_revision_id(uint8_t* board_rev_id, uint8_t board_rev_id_len);
int fbgc_common_dev_id(char *str, uint8_t *dev);
bool fbgc_common_is_grandcanyon2(void);
int sv_control(const char *service, svc_mode_t mode);
int check_image_md5_at_offset(const char* image_path, off_t start_offs, int cal_size, uint32_t md5_offset, uint8_t is_masked);
int fbgc_common_get_img_sur_info(const char *img_path, uint8_t comp, sur_error_proof_info_t *img_info);
int fbgc_common_validate_img(const char *img_path, uint8_t comp, uint8_t expected_board_id, uint8_t board_rev_id);
int fbgc_common_get_gpio_name_type(gpio_name_type_t *type);
const char *fbgc_common_get_gpio_shadow_name(gpio_shadow_id_t id);
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBGC_COMMON_H__ */
