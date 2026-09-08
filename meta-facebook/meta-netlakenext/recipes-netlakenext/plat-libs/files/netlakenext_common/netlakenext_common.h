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

#ifndef __NETLAKENEXT_COMMON_H__
#define __NETLAKENEXT_COMMON_H__

#include <stdbool.h>
#include <stdint.h>
#include <openssl/md5.h>
#include <sys/stat.h>
#include <openbmc/ipmi.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EEPROM_PATH     "/sys/bus/i2c/devices/%d-00%X/eeprom"
#define COMMON_FRU_PATH "/tmp/fruid_%s.bin"
#define FRU_SERVER_BIN  "/tmp/fruid_fboss.bin" // MB FRU uses Meta FBOSS EEPROM format
#define FRU_BMC_BIN  "/tmp/fruid_bmc.bin"
#define FRU_NIC_BIN  "/tmp/fruid_nic.bin"

#define SERVER_FRU_ADDR  0x56
#define BMC_FRU_ADDR  0x56

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#endif

#define SERVER_FRU_ADDR 0x56
#define BMC_FRU_ADDR 0x56
#define NIC_FRU_ADDR 0x50

#define I2C_RETRY_TIME 3

#define CPLD_BUS_2 2
#define CPLD_ADDR_BUS_2 0x1E
#define CPLD_BUS_4 4
#define CPLD_ADDR_BUS_4 0x3E
#define CPLD_SYS_CONFIG_REG_REG 0x01
#define CPLD_OCP_THERMTRIP_REG 0x07
#define CPLD_REV_ID_BIT (0x07)
#define CPLD_VR_SOURCE_BIT (0x18)
#define CPLD_SPI_MUX_REG 0x0A
#define CPLD_MISC_CTRL_SIG_REG 0x0C
#define CPLD_KBRST_BIT (0x10)
#define CPLD_ADC_ADDR_BUS_4 0x1E
#define CPLD_FW_BUS 3
#define CPLD_FW_ADDR 0x80
#define CPLD_FW_VER_REG 0x28002000
#define CPLD_RETRY_TIME 3
#define CPLD_REG_BYTE 1

//In Netlake codebase, all definition used 0 base bus, 8 bit address
#define VR_BUS  0
#define VR_PVDDCR_BUS 20
#define VR_PVDDCR_SOC_BUS 20
#define VR_PVDD_MISC_BUS 21
// PVDDCR and PVDDCR_SOC share the same PMBus address
#define VR_PVDDCR_ADDR 0x40
#define VR_PVDDCR_SOC_ADDR 0x40
#define VR_PVDD_MISC_ADDR 0x42
#define VR_PAGE_REG 0x00
#define VR_PAGE_0 0x00
#define VR_PAGE_1 0x01
#define VR_VOUT_MODE_REG 0x20
#define VR_MFR_VOUT_SCALE_LOOP_REG 0x29
#define VR_STATUS_BYTE_REG 0x78
#define VR_STATUS_WORD_REG 0x79
#define VR_STATUS_IOUT_REG 0x7B
#define VR_MFR_ID_REG 0x99
#define VR_MFR_ID_MAX_LEN 6
#define VR_MFR_ID_MPS 0x4D5053
#define VR_MFR_ID_MPS_LEN 3
#define VR_MFR_ID_INF 0x4946
#define VR_MFR_ID_INF_LEN 2
#define VR_MFR_ID_RNS 0x00000000
#define VR_MFR_ID_RNS_LEN 4
#define VR_RETRY_TIME 3

#define MTP_HSC_BUS 9
#define MTP_HSC_ADDR 0x80
#define MTP_PMON_CONFIG_ADDR 0xD4
#define MTP_HSC_EN_VOUT_LENGTH 3
#define MTP_HSC_POWER_CYCLE_REG 0xD9
#define MTP_HSC_SAMPLE_AVG_1 0x16
#define MTP_HSC_SAMPLE_AVG_2 0x3F

#define DIMM_BUS 4
#define DIMMA_ADDR 0xA0
#define DIMMB_ADDR 0xA2
#define DIMM_TEMP_LEN 1
#define DIMM_PAGE_OFFSET 0x0b
#define DIMM_PAGE0 0x00

#define PMICA_ADDR 0x90
#define PMICB_ADDR 0x92
#define PMIC_ADC_REG 0x30
#define PMIC_TOTAL_PWR 0x1A
#define PMIC_PWR_SELECT 0x1B
#define PMIC_VDD_READ 0x0C

#define INA230_BUS 4
#define INA230_ADDR 0x80
#define INA230_CONFIG 0x00
#define INA230_POWER 0x03
#define INA230_IOUT 0x04
#define INA230_CALIBRATION 0x05
#define LSB_INA230_CONFIG 0x4E
#define MSB_INA230_CONFIG 0xDF
#define LSB_INA230_DEFAULT_CALIBRATION 0x14
#define MSB_INA230_DEFAULT_CALIBRATION 0x00
#define INA230_GET_DATA_LEN 2

#define NVME_B_BUS 23
#define NVME_E1S_BUS 24
#define NVME_D_BUS 25
#define NVME_ADDR 0xD4
#define NVME_GET_STATUS_CMD 0x00
#define NVME_GET_STATUS_LEN 8
#define NVME_TEMP_REG 0x03

#define NIC_BUS 8
#define NIC_ADDR 0x3E
#define NIC_INFO_TEMP_CMD 0x01
#define NIC_TEMP_LEN 1
#define NIC_TEMP_RETRY_TIME 5
#define MAX_NIC_TEMPERATURE 130

#define MAX_PATH_LEN 128  // include the string terminal

#define MD5_SIZE              (16)
#define PLAT_SIG_SIZE         (16)
#define FW_VER_SIZE           (4)
#define IMG_MD5_OFFSET        (0x0)
#define IMG_SIGNATURE_OFFSET  ((IMG_MD5_OFFSET) + MD5_SIZE)
#define IMG_FW_VER_OFFSET     ((IMG_SIGNATURE_OFFSET) + PLAT_SIG_SIZE)
#define IMG_IDENTIFY_OFFSET   ((IMG_FW_VER_OFFSET) + FW_VER_SIZE)
#define IMG_POSTFIX_SIZE      (MD5_SIZE + PLAT_SIG_SIZE + FW_VER_SIZE + 1)

#define MD5_READ_BYTES     (1024)

#define REVISION_ID(x)  (((x >> 4) & 0x07) + 1)

#define HIGH_STR            "1"
#define LOW_STR             "0"
#define PWR_GOOD_KV_KEY     "power_good_status"
#define POST_CMPLT_KV_KEY   "post_complete_status"
#define SKIP_BMC_FWUPD_PRECHK_KV_KEY   "skip_bmc_fwupd_precheck"
#define ADDC_INIT_KV_KEY    "addc_init"
#define VR_DUMP_KV_KEY      "vr_dump_in_progress"

enum pmbus_rw_size {
  PMBUS_RW_BYTE  = 1,
  PMBUS_RW_WORD  = 2,
};

enum {
  FRU_ALL = 0,
  FRU_SERVER, // MB FRU uses Meta FBOSS EEPROM format
  FRU_BMC,
  FRU_PDB,
  FRU_FIO,
  FRU_NIC,
  FRU_CNT,
};

enum fru_bus{
  I2C_SERVER_BUS = 4,
  I2C_NIC_BUS = 8,
  I2C_BMC_BUS = 9,
};

enum fw_rev {
  FW_REV_POC = 0,
  FW_REV_EVT,
  FW_REV_DVT,
  FW_REV_PVT,
  FW_REV_MP ,
};

enum board_rev {
  EVT = 0,
  EVT2 = 1,
  EVT3 = 2,
  PreDVT = 3,
  DVT = 4,
  PVT = 5,
  MP = 6,
  BOARD_REV_COUNT
};

enum vr_sku {
  MPS = 0,
  INFINEON,
  RENESAS,
};

enum {
  FORCE_UPDATE_UNSET = 0x0,
  FORCE_UPDATE_SET,
};

typedef struct {
  uint8_t md5_sum[MD5_SIZE];
  uint8_t plat_sig[PLAT_SIG_SIZE];
  uint8_t version[FW_VER_SIZE];
  uint8_t err_proof;
} FW_IMG_INFO;

int netlakenext_common_check_image_md5(const char* image_path, int cal_size, uint8_t *data);
int netlakenext_common_check_image_signature(uint8_t* data);
bool netlakenext_common_is_valid_img(const char* img_path, FW_IMG_INFO* img_info, uint8_t rev_id);
int netlakenext_common_i2c_transfer(uint8_t bus, uint8_t addr, uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t rlen);
int netlakenext_common_get_img_ver(const char* image_path, char* ver);
int netlakenext_get_cpld_data(int bus, uint8_t addr, uint8_t reg, uint8_t* value);
int netlakenext_common_get_sys_cfg(uint8_t* sys_cfg);
int netlakenext_common_get_vr_sku(uint8_t* sku);
int netlakenext_common_linear11_convert(uint8_t *value_raw, float *value_linear11);
int netlakenext_common_linear16_convert(uint8_t *value_raw, uint8_t mode, float *value_linear16);
void netlakenext_vr_dump(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __NETLAKENEXT_COMMON_H__ */
