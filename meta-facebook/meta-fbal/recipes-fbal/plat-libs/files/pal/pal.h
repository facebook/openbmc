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

#include <openbmc/obmc-pal.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "pal_sensors.h"
#include "pal_health.h"
#include "pal_sbmc.h"
#include "pal_power.h"
#include "pal_cm.h"
#include "pal_ep.h"
#include "pal_cc.h"

#define PWR_OPTION_LIST "status, graceful-shutdown, off, on, reset, cycle"
#define FRU_EEPROM_MB_T  "/sys/class/i2c-dev/i2c-4/device/4-00%d/eeprom"
#define FRU_EEPROM_NIC0  "/sys/class/i2c-dev/i2c-17/device/17-0050/eeprom"
#define FRU_EEPROM_NIC1  "/sys/class/i2c-dev/i2c-18/device/18-0052/eeprom"
#define FRU_EEPROM_BMC  "/sys/class/i2c-dev/i2c-13/device/13-0056/eeprom"
#define LARGEST_DEVICE_NAME (120)
#define UNIT_DIV            (1000)
#define ERR_NOT_READY       (-2)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define PAGE_SIZE  0x1000
#define AST_GPIO_BASE 0x1e780000
#define UARTSW_OFFSET 0x68

#define PFR_MAILBOX_BUS  (4)
#define PFR_MAILBOX_ADDR (0xB0)

#define CMD_GET_MAIN_CPLD_VER (0x01)
#define CMD_GET_MOD_CPLD_VER  (0x02)
#define CMD_GET_GLB_CPLD_VER  (0x03)

#define PREPARE_FW_UPDATE_CLEAR (0x00)
#define PREPARE_FW_UPDATE_SET   (0x01)


enum {
  FRU_ALL  = 0,
  FRU_TRAY0_MB,
  FRU_PDB,
  FRU_TRAY0_NIC0,
  FRU_TRAY0_NIC1,
  FRU_OCPDBG,
  FRU_TRAY0_BMC,
  FRU_TRAY1_MB,
  FRU_TRAY1_NIC0,
  FRU_TRAY1_NIC1,
  FRU_TRAY1_BMC,
  FRU_CNT,
};

enum {
  REV_PO = 0,
  REV_EVT,
  REV_DVT,
  REV_PVT,
  REV_MP,
};

extern uint8_t FRU_MB;
extern uint8_t FRU_NIC0;
extern uint8_t FRU_NIC1;
extern uint8_t FRU_BMC;


#define MAX_NUM_FRUS    (FRU_CNT-1)
#define MAX_NODES       FRU_PDB
#define READING_SKIP    (1)
#define READING_NA      (-2)


// Sensors Under Side Plane
enum {
  MB_SENSOR_TBD,
};

enum{
  MEZZ_SENSOR_TBD,
};

enum {
  BOOT_DEVICE_USB      = 0x0,
  BOOT_DEVICE_IPV4     = 0x1,
  BOOT_DEVICE_HDD      = 0x2,
  BOOT_DEVICE_CDROM    = 0x3,
  BOOT_DEVICE_OTHERS   = 0x4,
  BOOT_DEVICE_IPV6     = 0x9,
  BOOT_DEVICE_RESERVED = 0xff,
};

enum {
  IPMI_CHANNEL_0 = 0,
  IPMI_CHANNEL_1,
  IPMI_CHANNEL_2,
  IPMI_CHANNEL_3,
  IPMI_CHANNEL_4,
  IPMI_CHANNEL_5,
  IPMI_CHANNEL_6,
  IPMI_CHANNEL_7,
  IPMI_CHANNEL_8,
  IPMI_CHANNEL_9,
  IPMI_CHANNEL_A,
  IPMI_CHANNEL_B,
  IPMI_CHANNEL_C,
  IPMI_CHANNEL_D,
  IPMI_CHANNEL_E,
  IPMI_CHANNEL_F,
};

enum {
  I2C_BUS_0 = 0,
  I2C_BUS_1,
  I2C_BUS_2,
  I2C_BUS_3,
  I2C_BUS_4,
  I2C_BUS_5,
  I2C_BUS_6,
  I2C_BUS_7,
  I2C_BUS_8,
  I2C_BUS_9,
  I2C_BUS_10,
  I2C_BUS_11,
  I2C_BUS_12,
  I2C_BUS_13,
  I2C_BUS_14,
  I2C_BUS_15,
  I2C_BUS_16,
  I2C_BUS_17,
  I2C_BUS_18,
  I2C_BUS_19,
  I2C_BUS_20,
  I2C_BUS_21,
  I2C_BUS_22,
  I2C_BUS_23,
};

//Other BMC Device INfo
#define BMC_IPMB_BUS_ID            (I2C_BUS_2)
#define BMC0_SLAVE_DEF_ADDR        (0x20)
#define BMC1_SLAVE_DEF_ADDR        (0x22)
#define BMC2_SLAVE_DEF_ADDR        (0x24)
#define BMC3_SLAVE_DEF_ADDR        (0x26)

//Main CPLD Device Info
#define MAIN_CPLD_BUS_ID           (I2C_BUS_4)
#define MAIN_CPLD_PWR_STATE_ADDR   (0x32)
#define MAIN_CPLD_PWR_STATE_CMD    (0x0003)

#define CPLD_PWR_CPU_FAULT            (1)
#define CPLD_PWR_CPU_OFF              (2)
#define CPLD_PWR_CPU_PVCCIO           (3)
#define CPLD_PWR_P1V8_PCIE_P1V1       (4)
#define CPLD_PWR_PVCCIN               (6)
#define CPLD_PWR_PVCCSA               (7)
#define CPLD_PWR_CPU_DONE             (8)
#define CPLD_PWR_PVCCSA_OFF           (9)
#define CPLD_PWR_PVCCIN_OFF          (10)
#define CPLD_PWR_P1V8_PCIE_P1V1_OFF  (12)
#define CPLD_PWR_PVCCIO_OFF          (13)

//NM Device Info
#define NM_IPMB_BUS_ID             (I2C_BUS_5)
#define NM_SLAVE_ADDR              (0x2C)

//FBEP Device Info
#define ASIC_IPMB_BUS_ID           (I2C_BUS_6)
#define ASIC_BMC_SLAVE_ADDR        (0x2C)


#define NM_IPMB_BUS_ID          (I2C_BUS_5)
#define NM_SLAVE_ADDR           (0x2C)
#define BMC0_SLAVE_DEF_ADDR     (0x20)
#define BMC1_SLAVE_DEF_ADDR     (0x22)
#define BMC2_SLAVE_DEF_ADDR     (0x24)
#define BMC3_SLAVE_DEF_ADDR     (0x26)
#define BMC_IPMB_BUS_ID         (I2C_BUS_2)
#define ASIC_BMC_SLAVE_ADDR     (0x2C)
#define ASIC_IPMB_BUS_ID        (I2C_BUS_6)
#define IOX_BMC_SLAVE_ADDR      (0x2E)
#define IOX_IPMB_BUS_ID         (I2C_BUS_6)

#define MAIN_CPLD_SLV_ADDR      (0xB4)
#define MOD_CPLD_SLV_ADDR       (0xAA)
#define GLB_CPLD_SLV_ADDR       (0xAA)
#define MAIN_CPLD_BUS_NUM       (4)
#define MOD_CPLD_BUS_NUM        (4)
#define GLB_CPLD_BUS_NUM        (23)
#define CPLD_VER_REG            (0x00100028)

enum {
  MB_ID0 = 0,
  MB_ID1
};

enum {
  MB_4S_EP_MODE = 0, //SKT_ID[2:1] 00
  MB_4S_EX_MODE,     //SKT_ID[2:1] 01
  MB_2S_MODE,        //SKT_ID[2:1] 10
};

int pal_set_id_led(uint8_t fru, uint8_t status);
int pal_set_fault_led(uint8_t fru, uint8_t status);
int pal_get_rst_btn(uint8_t *status);
int pal_postcode_select(int option);
int pal_uart_select_led_set(void);
int pal_get_me_fw_ver(uint8_t bus, uint8_t addr, uint8_t *ver);
int pal_get_platform_id(uint8_t *id);
int pal_get_config_is_master(void);
int pal_get_mb_position(uint8_t* pos);
int pal_get_board_rev_id(uint8_t *id);
void fru_eeprom_mb_check(char* mb_path);
bool is_cpu_socket_occupy(uint8_t cpu_idx);
int pal_get_syscfg_text(char *text);
int pal_peer_tray_get_lan_config(uint8_t sel, uint8_t *buf, uint8_t *rlen);
int pal_get_target_bmc_addr(uint8_t *tar_bmc_addr);
bool pal_skip_access_me(void);
int pal_i2c_write_read (uint8_t bus, uint8_t addr,
                        uint8_t *txbuf, uint8_t txlen,
                        uint8_t *rxbuf, uint8_t rxlen);

enum {
  POSTCODE_BY_BMC,
  POSTCODE_BY_HOST,
};

enum {
  BRIDGE_2_CM = BYPASS_CNT,
  BRIDGE_2_MB_BMC0,
  BRIDGE_2_MB_BMC1,
  BRIDGE_2_MB_BMC2,
  BRIDGE_2_MB_BMC3,
  BRIDGE_2_ASIC_BMC,
  BRIDGE_2_IOX_BMC,
};

enum {
  CC_OEM_DEVICE_NOT_PRESENT = 0x30,
  CC_OEM_DEVICE_INFO_ERR = 0x31,
  CC_OEM_DEVICE_DESTINATION_ERR = 0x32,
  CC_OEM_DEVICE_SEND_SLAVE_RESTORE_POWER_POLICY_FAIL =0x33,
  CC_OEM_GET_SELF_ADDRESS_ERR = 0x34,
  CC_OEM_ONLY_SUPPORT_MASTER = 0x35,
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
