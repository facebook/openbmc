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

#ifndef __NETLAKEMTP_COMMON_H__
#define __NETLAKEMTP_COMMON_H__

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
#define FRU_SERVER_BIN  "/tmp/fruid_server.bin"
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

#define CPLD_BUS 2
#define CPLD_ADDR 0x1E
#define CPLD_VER_REG 0x28002000
#define CPLD_REV_ID_REG 0x01
#define CPLD_REV_ID_BYTE 1

//In Netlake codebase, all definition used 0 base bus, 8 bit address
#define VR_BUS  0
#define VR_P1V8_STBY_ADDR 0xEC
#define VR_P1V05_STBY_ADDR 0x44
#define VR_PVNN_PCH_ADDR 0x22
#define VR_PVCCIN_ADDR 0xEC
#define VR_PVCCANCPU_ADDR 0xCC
#define VR_PVDDQ_ABC_CPU_ADDR 0x8A
#define VR_VOUT_MODE_REG 0x20

#define MTP_HSC_BUS 9
#define MTP_HSC_ADDR 0x80
#define MTP_PMON_CONFIG_ADDR 0xD4
#define MTP_HSC_EN_VOUT_LENGTH 3
#define MTP_HSC_POWER_CYCLE_REG 0xD9

#define CPLD_REV_ID_BUS 2
#define CPLD_REV_ID_ADDR 0x1E
#define CPLD_REV_ID_REG 0x01
#define CPLD_REV_ID_BYTE 1
#define CPLD_FW_REG_BUS 3
#define CPLD_FW_REG_ADDR 0x80
#define CPLD_VER_REG 0x28002000
#define CPLD_ADC_REG_BUS 4
#define CPLD_ADC_REG_ADDR 0x1E
#define CPLD_GET_REV_RETRY_TIME 3

// The following definition is specific to ICE-LAKE-D CPU without BIC.
// Any future platform that uses Netlake as COMe should consider
// using similar definition as below.
#define ME_BUS 1
#define NM_GLOBAL_POWER_STATISTICS_LENGTH 20

#define NVME_A_BUS 22
#define NVME_B_BUS 23
#define NVME_C_BUS 24
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

enum {
  FRU_ALL = 0,
  FRU_SERVER,
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

int netlakemtp_common_check_image_md5(const char* image_path, int cal_size, uint8_t *data);
int netlakemtp_common_check_image_signature(uint8_t* data);
bool netlakemtp_common_is_valid_img(const char* img_path, FW_IMG_INFO* img_info, uint8_t rev_id);
int netlakemtp_common_get_img_ver(const char* image_path, char* ver);
int netlakemtp_common_get_board_rev(uint8_t* rev_id);
int netlakemtp_common_linear11_convert(uint8_t *value_raw, float *value_linear11);
int netlakemtp_common_linear16_convert(uint8_t *value_raw, uint8_t mode, float *value_linear16);
int netlakemtp_common_me_ipmb_wrapper(uint8_t netfn, uint8_t cmd, uint8_t *txbuf, uint8_t txlen,
                                      uint8_t *rxbuf, uint8_t *rxlen);
int netlakemtp_common_me_get_fw_ver(ipmi_dev_id_t *dev_id);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __NETLAKEMTP_COMMON_H__ */
