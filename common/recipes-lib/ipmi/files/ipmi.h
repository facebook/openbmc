/*
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
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

#ifndef __IPMI_H__
#define __IPMI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SOCK_PATH_IPMI "/tmp/ipmi_socket"

#define IPMI_SEL_VERSION  0x51
#define IPMI_SDR_VERSION  0x51

#define MAX_NUM_DIMMS 4

#define SIZE_AUTH_ENABLES 5
#define SIZE_IP_ADDR  4
#define SIZE_IP6_ADDR  16
#define SIZE_MAC_ADDR 6
#define SIZE_NET_MASK 4
#define SIZE_IP_HDR 3
#define SIZE_RMCP_PORT  2
#define SIZE_COMMUNITY_STR 18
#define SIZE_DEST_TYPE  4
#define SIZE_DEST_ADDR  18
#define SIZE_TIME_STAMP 4

#define SIZE_PROC_FREQ  2
#define SIZE_DIMM_SPEED 2
#define SIZE_DIMM_SIZE  2

#define SIZE_SYSFW_VER  17
#define SIZE_SYS_NAME 17
#define SIZE_OS_NAME  17
#define SIZE_OS_VER 17
#define SIZE_BMC_URL  17
#define SIZE_OS_HV_URL  17

#define SIZE_SEL_REC 16

// NetFn, Command, Checksum, IANAID(3 bytes), BIC Interface type
#define BIC_INTF_HDR_SIZE 7

#define LUN_OFFSET 2

//NetFn, Cmd
#define IPMI_REQ_HDR_SIZE 2

// NetFn, Cmd, CC
#define IPMI_RESP_HDR_SIZE 3

#define MAX_IPMI_MSG_SIZE 300

// Type Definition
#define TYPE_BINARY          0
#define TYPE_BCD_PLUS        1
#define TYPE_ASCII_6BIT      2
#define TYPE_ASCII_8BIT      3

#define TIMEOUT_IPMI  5

#define SIZE_BOOT_ORDER 6

// IPMI request Structure (IPMI/Section 9.2)
typedef struct
{
  unsigned char netfn_lun;
  unsigned char cmd;
  unsigned char data[];
} ipmi_req_t;

// IPMI Multi Node request Structure
// Supports additional member to identify the node#
typedef struct
{
  unsigned char payload_id;
  unsigned char netfn_lun;
  unsigned char cmd;
  unsigned char data[];
} ipmi_mn_req_t;

// IPMI response Structure (IPMI/Section 9.3)
typedef struct
{
  unsigned char netfn_lun;
  unsigned char cmd;
  unsigned char cc;
  unsigned char data[];
} ipmi_res_t;

// IPMI/Spec Table 20-2
typedef struct _ipmi_dev_id_t {
  uint8_t dev_id;
  uint8_t dev_rev;
  uint8_t fw_rev1;
  uint8_t fw_rev2;
  uint8_t ipmi_ver;
  uint8_t dev_support;
  uint8_t mfg_id[3];
  uint8_t prod_id[2];
  uint8_t aux_fw_rev[4];
} ipmi_dev_id_t;


typedef struct _ipmi_fruid_info_t {
  uint8_t size_lsb;
  uint8_t size_msb;
  uint8_t bytes_words;
} ipmi_fruid_info_t;

#pragma pack(push, 1)

// Full Sensor SDR record; IPMI/Section 43.1
typedef struct {
  // Sensor Record Header
  unsigned char rec_id[2];
  unsigned char ver;
  unsigned char type;
  unsigned char len;
  // Record Key Bytes
  unsigned char owner;
  unsigned char lun;
  unsigned char sensor_num;
  // Record Body Bytes
  unsigned char ent_id;
  unsigned char ent_inst;
  unsigned char sensor_init;
  unsigned char sensor_caps;
  unsigned char sensor_type;
  unsigned char evt_read_type;
  union {
    unsigned char assert_evt_mask[2];
    unsigned char lt_read_mask[2];
  };
  union {
    unsigned char deassert_evt_mask[2];
    unsigned char ut_read_mask[2];
  };
  union {
    unsigned char read_evt_mask[2];
    unsigned char set_thresh_mask[2];
  };
  unsigned char sensor_units1;
  unsigned char sensor_units2;
  unsigned char sensor_units3;
  unsigned char linear;
  unsigned char m_val;
  unsigned char m_tolerance;
  unsigned char b_val;
  unsigned char b_accuracy;
  unsigned char accuracy_dir;
  unsigned char rb_exp;
  unsigned char analog_flags;
  unsigned char nominal;
  unsigned char normal_max;
  unsigned char normal_min;
  unsigned char max_reading;
  unsigned char min_reading;
  unsigned char unr_thresh;
  unsigned char uc_thresh;
  unsigned char unc_thresh;
  unsigned char lnr_thresh;
  unsigned char lc_thresh;
  unsigned char lnc_thresh;
  unsigned char pos_hyst;
  unsigned char neg_hyst;
  unsigned char rsvd[2];
  unsigned char oem;
  unsigned char str_type_len;
  char str[16];
} sdr_full_t;

typedef struct _ipmi_sel_sdr_info_t {
  uint8_t ver;
  uint16_t rec_count;
  uint16_t free_space;
  uint8_t add_ts[4];
  uint8_t erase_ts[4];
  uint8_t oper;
} ipmi_sel_sdr_info_t;

typedef struct _ipmi_sel_sdr_req_t {
  uint16_t rsv_id;
  uint16_t rec_id;
  uint8_t offset;
  uint8_t nbytes;
} ipmi_sel_sdr_req_t;

typedef struct _ipmi_sel_sdr_res_t {
  uint16_t next_rec_id;
  uint8_t data[];
} ipmi_sel_sdr_res_t;

#pragma pack(pop)

// LAN Configuration Structure (IPMI/Table 23.4)
typedef struct
{
  unsigned char set_in_prog;
  unsigned char auth_support;
  unsigned char auth_enables[SIZE_AUTH_ENABLES];
  unsigned char ip_addr[SIZE_IP_ADDR];
  unsigned char ip_src;
  unsigned char mac_addr[SIZE_MAC_ADDR];
  unsigned char net_mask[SIZE_NET_MASK];
  unsigned char ip_hdr[SIZE_IP_HDR];
  unsigned char pri_rmcp_port[SIZE_RMCP_PORT];
  unsigned char sec_rmcp_port[SIZE_RMCP_PORT];
  unsigned char arp_ctrl;
  unsigned char garp_interval;
  unsigned char df_gw_ip_addr[SIZE_IP_ADDR];
  unsigned char df_gw_mac_addr[SIZE_MAC_ADDR];
  unsigned char back_gw_ip_addr[SIZE_IP_ADDR];
  unsigned char back_gw_mac_addr[SIZE_MAC_ADDR];
  unsigned char community_str[SIZE_COMMUNITY_STR];
  unsigned char no_of_dest;
  unsigned char dest_type[SIZE_DEST_TYPE];
  unsigned char dest_addr[SIZE_DEST_ADDR];
  unsigned char ip6_addr[SIZE_IP6_ADDR];
} lan_config_t;

// Structure to store Processor Information
typedef struct
{
  unsigned char type;
  unsigned char freq[SIZE_PROC_FREQ];
} proc_info_t;

// Structure to store DIMM Information
typedef struct
{
  unsigned char type;
  unsigned char speed[SIZE_DIMM_SPEED];
  unsigned char size[SIZE_DIMM_SIZE];
} dimm_info_t;


// Structure for System Info Params (IPMI/Section 22.14a)
typedef struct
{
  unsigned char set_in_prog;
  unsigned char sysfw_ver[SIZE_SYSFW_VER];
  unsigned char sys_name[SIZE_SYS_NAME];
  unsigned char pri_os_name[SIZE_OS_NAME];
  unsigned char present_os_name[SIZE_OS_NAME];
  unsigned char present_os_ver[SIZE_OS_VER];
  unsigned char bmc_url[SIZE_BMC_URL];
  unsigned char os_hv_url[SIZE_OS_HV_URL];
} sys_info_param_t;

// Structure for Sensor Reading (IPMI/Section 35.14)
typedef struct
{
  uint8_t value;
  uint8_t flags;
  uint8_t status;
  uint8_t ext_status;
} ipmi_sensor_reading_t;

// Network Function Codes (IPMI/Section 5.1)
enum
{
  NETFN_CHASSIS_REQ = 0x00,
  NETFN_CHASSIS_RES,
  NETFN_BRIDGE_REQ,
  NETFN_BRIDGE_RES,
  NETFN_SENSOR_REQ,
  NETFN_SENSOR_RES,
  NETFN_APP_REQ,
  NETFN_APP_RES,
  NETFN_FIRMWARE_REQ,
  NETFN_FIRMWARE_RES,
  NETFN_STORAGE_REQ,
  NETFN_STORAGE_RES,
  NETFN_TRANSPORT_REQ,
  NETFN_TRANSPORT_RES,
  NETFN_DCMI_REQ = 0x2C,
  NETFN_DCMI_RES = 0x2D,
  NETFN_OEM_REQ = 0x30,
  NETFN_OEM_RES = 0x31,
  NETFN_OEM_1S_REQ = 0x38,
  NETFN_OEM_1S_RES = 0x39,
};

// Chassis Command Codes (IPMI/Table H-1)
enum
{
  CMD_CHASSIS_GET_STATUS = 0x01,
  CMD_CHASSIS_GET_BOOT_OPTIONS = 0x09,
};

// Sensor Command Codes (IPMI/Table H-1)
enum
{
  CMD_SENSOR_PLAT_EVENT_MSG = 0x02,
};

// Application Command Codes (IPMI/Table H-1)
enum
{
  CMD_APP_GET_DEVICE_ID = 0x01,
  CMD_APP_COLD_RESET = 0x02,
  CMD_APP_GET_SELFTEST_RESULTS = 0x04,
  CMD_APP_GET_DEVICE_GUID = 0x08,
  CMD_APP_RESET_WDT = 0x22,
  CMD_APP_SET_WDT = 0x24,
  CMD_APP_GET_WDT = 0x25,
  CMD_APP_GET_GLOBAL_ENABLES = 0x2F,
  CMD_APP_GET_SYSTEM_GUID = 0x37,
  CMD_APP_SET_SYS_INFO_PARAMS = 0x58,
  CMD_APP_GET_SYS_INFO_PARAMS = 0x59,
};

// Storage Command Codes (IPMI/Table H-1)
enum
{
  CMD_STORAGE_GET_FRUID_INFO = 0x10,
  CMD_STORAGE_READ_FRUID_DATA = 0x11,
  CMD_STORAGE_WRITE_FRUID_DATA = 0x12,
  CMD_STORAGE_GET_SDR_INFO = 0x20,
  CMD_STORAGE_RSV_SDR = 0x22,
  CMD_STORAGE_GET_SDR = 0x23,
  CMD_STORAGE_GET_SEL_INFO = 0x40,
  CMD_STORAGE_RSV_SEL = 0x42,
  CMD_STORAGE_GET_SEL = 0x43,
  CMD_STORAGE_ADD_SEL = 0x44,
  CMD_STORAGE_CLR_SEL = 0x47,
  CMD_STORAGE_GET_SEL_TIME = 0x48,
  CMD_STORAGE_GET_SEL_UTC = 0x5C,
};

// Sensor Command Codes (IPMI/Table H-1)
enum
{
  CMD_SENSOR_GET_SENSOR_READING = 0x2D,
};

// Transport Command Codes (IPMI/Table H-1)
enum
{
  CMD_TRANSPORT_SET_LAN_CONFIG = 0x01,
  CMD_TRANSPORT_GET_LAN_CONFIG = 0x02,
  CMD_TRANSPORT_GET_SOL_CONFIG = 0x22,
};

// OEM Command Codes (Quanta/FB defined commands)
enum
{
  CMD_OEM_SET_PROC_INFO = 0x1A,
  CMD_OEM_SET_DIMM_INFO = 0x1C,
  CMD_OEM_GET_BOARD_ID = 0x37,
  CMD_OEM_SET_BOOT_ORDER = 0x52,
  CMD_OEM_GET_BOOT_ORDER = 0x53,
  CMD_OEM_SET_POST_START = 0x73,
  CMD_OEM_SET_POST_END = 0x74,
  CMD_OEM_GET_SLOT_INFO = 0x7E,
};

// OEM 1S Command Codes (Quanta/FB defined commands)
enum
{
  CMD_OEM_1S_MSG_IN = 0x1,
  CMD_OEM_1S_MSG_OUT = 0x2,
  CMD_OEM_1S_GET_GPIO = 0x3,
  CMD_OEM_1S_SET_GPIO = 0x4,
  CMD_OEM_1S_GET_GPIO_CONFIG = 0x5,
  CMD_OEM_1S_SET_GPIO_CONFIG = 0x6,
  CMD_OEM_1S_INTR = 0x7,
  CMD_OEM_1S_POST_BUF = 0x8,
  CMD_OEM_1S_UPDATE_FW = 0x9,
  CMD_OEM_1S_GET_FW_CKSUM = 0xA,
  CMD_OEM_1S_GET_FW_VER = 0xB,
  CMD_OEM_1S_ENABLE_BIC_UPDATE = 0xC,
  CMD_OEM_1S_GET_CONFIG = 0xE,
  CMD_OEM_1S_PLAT_DISC = 0xF,
  CMD_OEM_1S_SET_CONFIG = 0x10,
  CMD_OEM_1S_BIC_RESET = 0x11,
  CMD_OEM_1S_GET_POST_BUF = 0x12,
  CMD_OEM_1S_BIC_UPDATE_MODE = 0x13,
};


// IPMI command Completion Codes (IPMI/Section 5.2)
enum
{
  CC_SUCCESS = 0x00,
  CC_INVALID_PARAM = 0x80,
  CC_SEL_ERASE_PROG = 0x81,
  CC_INVALID_CMD = 0xC1,
  CC_PARAM_OUT_OF_RANGE = 0xC9,
  CC_UNSPECIFIED_ERROR = 0xFF,
};

// LAN Configuration parameters (IPMI/Table 23-4)
enum
{
  LAN_PARAM_SET_IN_PROG,
  LAN_PARAM_AUTH_SUPPORT,
  LAN_PARAM_AUTH_ENABLES,
  LAN_PARAM_IP_ADDR,
  LAN_PARAM_IP_SRC,
  LAN_PARAM_MAC_ADDR,
  LAN_PARAM_NET_MASK,
  LAN_PARAM_IP_HDR,
  LAN_PARAM_PRI_RMCP_PORT,
  LAN_PARAM_SEC_RMCP_PORT,
  LAN_PARAM_ARP_CTRL,
  LAN_PARAM_GARP_INTERVAL,
  LAN_PARAM_DF_GW_IP_ADDR,
  LAN_PARAM_DF_GW_MAC_ADDR,
  LAN_PARAM_BACK_GW_IP_ADDR,
  LAN_PARAM_BACK_GW_MAC_ADDR,
  LAN_PARAM_COMMUNITY_STR,
  LAN_PARAM_NO_OF_DEST,
  LAN_PARAM_DEST_TYPE,
  LAN_PARAM_DEST_ADDR,
  LAN_PARAM_IP6_ADDR = 197, /* OEM parameter for IPv6 */
};

// SOL Configuration parameters (IPMI/Table 26-5)
enum
{
  SOL_PARAM_SET_IN_PROG,
  SOL_PARAM_SOL_ENABLE,
  SOL_PARAM_SOL_AUTH,
  SOL_PARAM_SOL_THRESHOLD,
  SOL_PARAM_SOL_RETRY,
  SOL_PARAM_SOL_BITRATE,
  SOL_PARAM_SOL_NV_BITRATE,
};

// Boot Option Parameters (IPMI/Table 28-14)
  enum
{
  PARAM_SET_IN_PROG = 0x00,
  PARAM_SVC_PART_SELECT,
  PARAM_SVC_PART_SCAN,
  PARAM_BOOT_FLAG_CLR,
  PARAM_BOOT_INFO_ACK,
  PARAM_BOOT_FLAGS,
  PARAM_BOOT_INIT_INFO,
};

//System Info Parameters (IPMI/Table 22-16c)
enum
{
  SYS_INFO_PARAM_SET_IN_PROG,
  SYS_INFO_PARAM_SYSFW_VER,
  SYS_INFO_PARAM_SYS_NAME,
  SYS_INFO_PARAM_PRI_OS_NAME,
  SYS_INFO_PARAM_PRESENT_OS_NAME,
  SYS_INFO_PARAM_PRESENT_OS_VER,
  SYS_INFO_PARAM_BMC_URL,
  SYS_INFO_PARAM_OS_HV_URL,
};

// Bridge-IC interface on which this command initiated
enum
{
  BIC_INTF_ME = 0x01,
  BIC_INTF_SOL = 0x02,
  BIC_INTF_KCS = 0x03,
  BIC_INTF_KCS_SMM = 0x04,
};

void lib_ipmi_handle(unsigned char *request, unsigned char req_len,
                 unsigned char *response, unsigned char *res_len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __IPMI_H__ */
