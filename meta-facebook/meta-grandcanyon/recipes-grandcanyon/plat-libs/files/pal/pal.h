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

#ifndef __PAL_H__
#define __PAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include <openbmc/ncsi.h>
#include <openbmc/nl-wrapper.h>
#include <openbmc/phymem.h>
#include <openbmc/nvme-mi.h>
#include <facebook/fbgc_common.h>
#include <facebook/fbgc_fruid.h>
#include <facebook/bic.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include "pal_sensors.h"
#include "pal_power.h"

#define MAX_NUM_FRUS    (FRU_CNT-1)
#define MAX_NODES       1
#define FRUID_SIZE      512
#define CUSTOM_FRU_LIST 1

#define MAX_FRU_CMD_STR   16

#define GUID_SIZE 16
#define OFFSET_SYS_GUID 0x17F0
#define OFFSET_DEV_GUID 0x1800

// ADS1015
#define IOCM_ADS1015_DRIVER_BIND_NAME     "ti_ads1015"
#define IOCM_ADS1015_DRIVER_NAME          "ads1015"
#define IOCM_ADS1015_BIND_DIR             "/sys/bus/i2c/drivers/ads1015/13-0049/"
#define IOCM_ADS1015_ADDR                 (49) // unit: hex

// IOCM TMP75
#define IOCM_TMP75_DEVICE_DIR             "/sys/class/i2c-dev/i2c-13/device/13-004a"
#define IOCM_TMP75_DEVICE_NAME            "tmp75"
#define IOCM_TMP75_ADDR                   (0x4a)

#define SKU_UIC_ID_SIZE       2
#define SKU_UIC_TYPE_SIZE     4
#define SKU_SIZE              (SKU_UIC_ID_SIZE + SKU_UIC_TYPE_SIZE)
#define MAX_SKU_VALUE         (1 << SKU_SIZE)

// IOCM EEPROM
#define IOCM_EEPROM_BIND_DIR         "/sys/bus/i2c/drivers/at24/13-0050"
#define IOCM_EEPROM_DRIVER_NAME      "at24"
#define IOCM_EEPROM_ADDR             (50) //unit: hex

#define MAX_NUM_OF_BOARD_REV_ID_GPIO     3

#define MAX_NUM_ERR_CODES                    256
#define MAX_NUM_EXP_ERR_CODES                100
#define MAX_NUM_ERR_CODES_ARRAY              32
#define MAX_NUM_EXP_ERR_CODES_ARRAY          13
#define BMC_ERR_CODE_START_NUM               0xCE
#define ERR_CODE_BIN                         "/tmp/error_code.bin"

#define ERR_CODE_ENABLE                  1
#define ERR_CODE_DISABLE                 0

#define ERR_CODE_CPU_UTILIZA             0xCE
#define ERR_CODE_MEM_UTILIZA             0xCF
#define ERR_CODE_ECC_RECOVERABLE         0xE0
#define ERR_CODE_ECC_UNRECOVERABLE       0xE1

// For FRUs missing error code
#define ERR_CODE_SERVER_MISSING          0xE2
#define ERR_CODE_SCC_MISSING             0xE3
#define ERR_CODE_NIC_MISSING             0xE4
#define ERR_CODE_E1S_MISSING             0xE5
#define ERR_CODE_IOCM_MISSING            0xE6

// For I2C bus crash error code
#define MAX_NUM_I2C_BUS                  16
#define ERR_CODE_I2C_CRASH_BASE          0xE7

// For BMC health error code
#define ERR_CODE_SERVER_HEALTH           0xF7
#define ERR_CODE_UIC_HEALTH              0xF8
#define ERR_CODE_DPB_HEALTH              0xF9
#define ERR_CODE_SCC_HEALTH              0xFA
#define ERR_CODE_NIC_HEALTH              0xFB
#define ERR_CODE_BMC_REMOTE_HB_HEALTH    0xFC
#define ERR_CODE_SCC_LOCAL_HB_HEALTH     0xFD
#define ERR_CODE_SCC_REMOTE_HB_HEALTH    0xFE
// Host Interface Control Register
#define LPC_CTR_BASE        0x1E789000
#define HICR9_ADDR          0x098
#define HICRA_ADDR          0x09C

#define ROUTE_UART2_TO_IO2  (0)
#define ROUTE_UART6_TO_IO6  ((1 << 9) | (1 << 11))
#define ROUTE_IO2_TO_IO6    ((1 << 9) | (1 << 10))
#define ROUTE_IO6_TO_IO2    ((1 << 3) | (1 << 4) | (1 << 5))
#define NVME_SMART_WARNING_MASK          0x1F  // check bit 0~4
#define NVME_STATUS_MASK                 0x28  // check bit 3, 5
#define NVME_STATUS_NORMAL               0x28  // bit3 = 1, bit5 = 1

#define MAX_SERIAL_NUM_SIZE               20

#define SERVER_CRASHDUMP_PID_PATH        "/var/run/autodump.pid"
#define SERVER_CRASHDUMP_KV_KEY          "server_crashdump"

typedef enum {
  STATUS_LED_OFF,
  STATUS_LED_YELLOW,
  STATUS_LED_BLUE,
} status_led_color;

typedef enum {
  ID_E1S0_LED,
  ID_E1S1_LED,
} e1s_led_id;

extern const char pal_fru_list_print[];
extern const char pal_fru_list_rw[];
extern const char pal_fru_list_sensor_history[];
extern const char pal_fru_list[];
extern const char pal_pwm_list[];
extern const char pal_tach_list[];
extern const char pal_server_list[];

extern size_t pal_pwm_cnt;
extern size_t pal_tach_cnt;

enum {
  BOOT_DEVICE_IPV4     = 0x1,
  BOOT_DEVICE_HDD      = 0x2,
  BOOT_DEVICE_CDROM    = 0x3,
  BOOT_DEVICE_OTHERS   = 0x4,
  BOOT_DEVICE_IPV6     = 0x9,
  BOOT_DEVICE_RESERVED = 0xff,
};

enum {
  FRU_ABSENT           = 0,
  FRU_PRESENT,
};

enum {
  PCIE_CONFIG_TYPE5    = 0x6,
  PCIE_CONFIG_TYPE7    = 0x8,
};

typedef struct {
	uint8_t pcie_cfg;
	uint8_t completion_code;
} get_pcie_config_response;

typedef struct _platformInformation {
  char uicId[SKU_UIC_ID_SIZE];
  char uicType[SKU_UIC_TYPE_SIZE];
} platformInformation;

typedef struct {
  uint8_t target;
  uint8_t data[MAX(MAX_IPMB_REQ_LEN, sizeof(NCSI_NL_MSG_T))];
} bypass_cmd;

typedef struct {
  uint8_t netdev;
  uint8_t channel;
  uint8_t cmd;
  unsigned char data[NCSI_MAX_PAYLOAD];
} bypass_ncsi_req;

typedef struct {
  uint8_t payload_id;
  uint8_t netfn;
  uint8_t cmd;
  uint8_t target;
  uint8_t bypass_netfn;
  uint8_t bypass_cmd;
} bypass_ipmi_header;

typedef struct {
  uint8_t payload_id;
  uint8_t netfn;
  uint8_t cmd;
  uint8_t target;
} bypass_me_header;

typedef struct {
  uint8_t cc;
} bypass_me_resp_header;

typedef struct {
  uint8_t payload_id;
  uint8_t netfn;
  uint8_t cmd;
  uint8_t target;
  uint8_t netdev;
  uint8_t channel;
  uint8_t ncsi_cmd;
} bypass_ncsi_header;

typedef struct {
  uint8_t netdev;
  uint8_t action;
} network_cmd;

typedef struct {
  uint8_t payload_id;
  uint8_t netfn;
  uint8_t cmd;
  uint8_t target;
  uint8_t netdev;
  uint8_t action;
} bypass_network_header;

typedef struct {
  uint8_t component;
  uint8_t status;
} ioc_fw_recovery_req;

enum {
  UIC_SIDEA          = 1,
  UIC_SIDEB          = 2,
};

enum {
  PLAT_INFO_SKUID_TYPE5A          = 2,
  PLAT_INFO_SKUID_TYPE5B          = 3,
  PLAT_INFO_SKUID_TYPE7_HEADNODE  = 4,
};

enum {
  DEBUG_UART_SEL_BMC = 0,
  DEBUG_UART_SEL_HOST,
  DEBUG_UART_SEL_BIC,
  DEBUG_UART_SEL_EXP_SMART,
  DEBUG_UART_SEL_EXP_SDB,
  DEBUG_UART_SEL_IOC_T5_SMART,
  DEBUG_UART_SEL_IOC_T7_SMART,
};

enum {
  CHASSIS_IN          = 0,
  CHASSIS_OUT         = 1,
};

enum IOC_RECOVERY_COMPONENT {
  IOC_RECOVERY_SCC  = 0x0,
  IOC_RECOVERY_IOCM = 0x1,
};

enum IOC_RECOVERY_STATUS {
  DISABLE_IOC_RECOVERY = 0x0,
  ENABLE_IOC_RECOVERY  = 0x1,
};

int pal_set_id_led(uint8_t slot, enum LED_HIGH_ACTIVE status);
int pal_set_status_led(uint8_t fru, status_led_color color);
int pal_set_e1s_led(uint8_t fru, e1s_led_id id, enum LED_HIGH_ACTIVE status);
int pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo);
int pal_check_gpio_prsnt(uint8_t gpio, int presnt_expect);
int pal_add_i2c_device(uint8_t bus, uint8_t addr, char *device_name);
int pal_del_i2c_device(uint8_t bus, uint8_t addr);
int pal_bind_i2c_device(uint8_t bus, uint8_t addr, char *driver_name);
int pal_unbind_i2c_device(uint8_t bus, uint8_t addr, char *driver_name);
int pal_get_sku(platformInformation *pal_sku);
int pal_get_uic_location(uint8_t *uic_id);
int pal_copy_eeprom_to_bin(const char *eeprom_file, const char *bin_file);
int pal_get_debug_card_uart_sel(uint8_t *uart_sel);
int pal_is_debug_card_present(uint8_t *status);
int pal_get_fan_latch(uint8_t *chassis_status);
void pal_specific_plat_fan_check(bool status);
int pal_get_uic_board_id(uint8_t *board_id);
int pal_post_display(uint8_t status);
int pal_get_current_led_post_code(uint8_t *post_code);
int pal_get_80port_record(uint8_t slot_id, uint8_t *res_data, size_t max_len, size_t *res_len);
int pal_get_num_slots(uint8_t *num);
int pal_write_error_code_file(unsigned char error_code_update, uint8_t error_code_status);
int pal_read_error_code_file(uint8_t *error_code_array, uint8_t error_code_array_len);
int pal_get_error_code(uint8_t *data, uint8_t* error_count);
void pal_set_error_code(unsigned char error_num, uint8_t error_code_status);
int pal_bmc_err_enable(const char *error_item);
int pal_bmc_err_disable(const char *error_item);
void pal_i2c_crash_assert_handle(int i2c_bus_num);
void pal_i2c_crash_deassert_handle(int i2c_bus_num);
int pal_get_drive_health(const char* i2c_bus_dev);
int pal_get_drive_status(const char* i2c_bus_dev);
int pal_is_crashdump_ongoing(uint8_t fru);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
