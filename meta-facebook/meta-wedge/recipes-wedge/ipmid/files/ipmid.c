/*
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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

#include "platform/sdr.h"
#include "platform/sel.h"
#include "platform/fruid.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/reboot.h>

#define SOCK_PATH "/tmp/ipmi_socket"

#define MAX_NUM_DIMMS 4
#define IPMI_SEL_VERSION  0x51
#define IPMI_SDR_VERSION  0x51

#define SIZE_AUTH_ENABLES 5
#define SIZE_IP_ADDR  4
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
#define SIZE_IPMI_RES_HDR 3

#define MAX_IPMI_MSG_SIZE 100

// IPMI request Structure (IPMI/Section 9.2)
typedef struct
{
  unsigned char netfn_lun;
  unsigned char cmd;
  unsigned char data[];
} ipmi_req_t;

// IPMI response Structure (IPMI/Section 9.3)
typedef struct
{
  unsigned char netfn_lun;
  unsigned char cmd;
  unsigned char cc;
  unsigned char data[];
} ipmi_res_t;

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
  NETFN_OEM_REQ = 0x30,
  NETFN_OEM_RES = 0x31,
};

// Chassis Command Codes (IPMI/Table H-1)
enum
{
  CMD_CHASSIS_GET_STATUS = 0x01,
  CMD_CHASSIS_GET_BOOT_OPTIONS = 0x09,
};

// Application Command Codes (IPMI/Table H-1)
enum
{
  CMD_APP_GET_DEVICE_ID = 0x01,
  CMD_APP_COLD_RESET = 0x02,
  CMD_APP_WARM_RESET = 0x03,
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

// Transport Command Codes (IPMI/Table H-1)
enum
{
  CMD_TRANSPORT_SET_LAN_CONFIG = 0x01,
  CMD_TRANSPORT_GET_LAN_CONFIG = 0x02,
};

// OEM Command Codes (Quanta/FB defined commands)
enum
{
  CMD_OEM_SET_PROC_INFO = 0x1A,
  CMD_OEM_SET_DIMM_INFO = 0x1C,
  CMD_OEM_SET_POST_START = 0x73,
  CMD_OEM_SET_POST_END = 0x74,
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

// TODO: Once data storage is finalized, the following structure needs
// to be retrieved/updated from persistent backend storage
static lan_config_t g_lan_config = { 0 };
static proc_info_t g_proc_info = { 0 };
static dimm_info_t g_dimm_info[MAX_NUM_DIMMS] = { 0 };

// TODO: Need to store this info after identifying proper storage
static sys_info_param_t g_sys_info_params;

// TODO: Based on performance testing results, might need fine grained locks
// Since the global data is specific to a NetFunction, adding locs at NetFn level
static pthread_mutex_t m_chassis;
static pthread_mutex_t m_app;
static pthread_mutex_t m_storage;
static pthread_mutex_t m_transport;
static pthread_mutex_t m_oem;

/*
 * Function(s) to handle IPMI messages with NetFn: Chassis
 */
// Get Chassis Status (IPMI/Section 28.2)
static void
chassis_get_status (unsigned char *response, unsigned char *res_len)
{
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  res->cc = CC_SUCCESS;

  // TODO: Need to obtain current power state and last power event
  // from platform and return
  *data++ = 0x01;		// Current Power State
  *data++ = 0x00;		// Last Power Event
  *data++ = 0x40;		// Misc. Chassis Status
  *data++ = 0x00;		// Front Panel Button Disable

  res_len = data - &res->data[0];
}

// Get System Boot Options (IPMI/Section 28.12)
static void
chassis_get_boot_options (unsigned char *request, unsigned char *response,
			  unsigned char *res_len)
{
  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res= (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  unsigned char param = req->data[0];

  // Fill response with default values
  res->cc = CC_SUCCESS;
  *data++ = 0x01;		// Parameter Version
  *data++ = req->data[0];	// Parameter

  // TODO: Need to store user settings and return
  switch (param)
  {
    case PARAM_SET_IN_PROG:
      *data++ = 0x00;	// Set In Progress
      break;
    case PARAM_SVC_PART_SELECT:
      *data++ = 0x00;	// Service Partition Selector
      break;
    case PARAM_SVC_PART_SCAN:
      *data++ = 0x00;	// Service Partition Scan
      break;
    case PARAM_BOOT_FLAG_CLR:
      *data++ = 0x00;	// BMC Boot Flag Valid Bit Clear
      break;
    case PARAM_BOOT_INFO_ACK:
      *data++ = 0x00;	// Write Mask
      *data++ = 0x00;	// Boot Initiator Ack Data
      break;
    case PARAM_BOOT_FLAGS:
      *data++ = 0x00;	// Boot Flags
      *data++ = 0x00;	// Boot Device Selector
      *data++ = 0x00;	// Firmwaer Verbosity
      *data++ = 0x00;	// BIOS Override
      *data++ = 0x00;	// Device Instance Selector
      break;
    case PARAM_BOOT_INIT_INFO:
      *data++ = 0x00;	// Chanel Number
      *data++ = 0x00;	// Session ID (4 bytes)
      *data++ = 0x00;
      *data++ = 0x00;
      *data++ = 0x00;
      *data++ = 0x00;	// Boot Info Timestamp (4 bytes)
      *data++ = 0x00;
      *data++ = 0x00;
      *data++ = 0x00;
      break;
    deault:
      res->cc = CC_PARAM_OUT_OF_RANGE;
      break;
  }

  if (res->cc == CC_SUCCESS) {
    *res_len = data - &res->data[0];
  }
}

// Handle Chassis Commands (IPMI/Section 28)
static void
ipmi_handle_chassis (unsigned char *request, unsigned char req_len,
		     unsigned char *response, unsigned char *res_len)
{
  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char cmd = req->cmd;

  pthread_mutex_lock(&m_chassis);
  switch (cmd)
  {
    case CMD_CHASSIS_GET_STATUS:
      chassis_get_status (response, res_len);
      break;
    case CMD_CHASSIS_GET_BOOT_OPTIONS:
      chassis_get_boot_options (request, response, res_len);
      break;
    default:
      res->cc = CC_INVALID_CMD;
      break;
  }
  pthread_mutex_unlock(&m_chassis);
}

/*
 * Function(s) to handle IPMI messages with NetFn: Application
 */
// Get Device ID (IPMI/Section 20.1)
static void
app_get_device_id (unsigned char *response, unsigned char *res_len)
{

  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  res->cc = CC_SUCCESS;

  //TODO: Following data needs to be updated based on platform
  *data++ = 0x20;		// Device ID
  *data++ = 0x81;		// Device Revision
  *data++ = 0x00;		// Firmware Revision Major
  *data++ = 0x09;		// Firmware Revision Minor
  *data++ = 0x02;		// IPMI Version
  *data++ = 0xBF;		// Additional Device Support
  *data++ = 0x15;		// Manufacturer ID1
  *data++ = 0xA0;		// Manufacturer ID2
  *data++ = 0x00;		// Manufacturer ID3
  *data++ = 0x46;		// Product ID1
  *data++ = 0x31;		// Product ID2
  *data++ = 0x00;		// Aux. Firmware Version1
  *data++ = 0x00;		// Aux. Firmware Version2
  *data++ = 0x00;		// Aux. Firmware Version3
  *data++ = 0x00;		// Aux. Firmware Version4

  *res_len = data - &res->data[0];
}

// Cold Reset (IPMI/Section 20.2)
static void
app_cold_reset (unsigned char *response, unsigned char *res_len)
{
  ipmi_res_t *res = (ipmi_res_t *) response;
  //TODO: Shall be changed later, to support platform related self cold reset
  //Reboot Open BMC FW
  reboot (RB_AUTOBOOT);

  res->cc = CC_SUCCESS;
  *res_len = 0;
}

// Warm Reset (IPMI/Section 20.3)
static void
app_warm_reset (unsigned char *response, unsigned char *res_len)
{
  ipmi_res_t *res = (ipmi_res_t *) response;

  //Reboot Open BMC FW
  reboot (RB_AUTOBOOT);

  res->cc = CC_SUCCESS;
  *res_len = 0;
}

// Get Self Test Results (IPMI/Section 20.4)
static void
app_get_selftest_results (unsigned char *response, unsigned char *res_len)
{

  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  res->cc = CC_SUCCESS;

  //TODO: Following data needs to be updated based on self-test results
  *data++ = 0x55;		// Self-Test result
  *data++ = 0x00;		// Extra error info in case of failure

  *res_len = data - &res->data[0];
}

// Get Device GUID (IPMI/Section 20.8)
static void
app_get_device_guid (unsigned char *response, unsigned char *res_len)
{
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  res->cc = 0x00;

  // TODO: Following data is Globaly Unique ID i.e. MAC Address..
  *data++ = 0x0;
  *data++ = 0x1;
  *data++ = 0x2;
  *data++ = 0x3;
  *data++ = 0x4;
  *data++ = 0x5;
  *data++ = 0x6;
  *data++ = 0x7;
  *data++ = 0x8;
  *data++ = 0x9;
  *data++ = 0xa;
  *data++ = 0xb;
  *data++ = 0xc;
  *data++ = 0xd;
  *data++ = 0xe;
  *data++ = 0xf;

  *res_len = data - &res->data[0];
}

// Get BMC Global Enables (IPMI/Section 22.2)
static void
app_get_global_enables (unsigned char *response, unsigned char *res_len)
{

  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  res->cc = CC_SUCCESS;

  *data++ = 0x09;		// Global Enable

  *res_len = data - &res->data[0];
}

// Set System Info Params (IPMI/Section 22.14a)
static void
app_set_sys_info_params (unsigned char *request, unsigned char *response,
			 unsigned char *res_len)
{

  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char param = req->data[0];

  res->cc = CC_SUCCESS;

  switch (param)
  {
    case SYS_INFO_PARAM_SET_IN_PROG:
      g_sys_info_params.set_in_prog = req->data[1];
      break;
    case SYS_INFO_PARAM_SYSFW_VER:
      memcpy(g_sys_info_params.sysfw_ver, &req->data[1], SIZE_SYSFW_VER);
      break;
    case SYS_INFO_PARAM_SYS_NAME:
      memcpy(g_sys_info_params.sys_name, &req->data[1], SIZE_SYS_NAME);
      break;
    case SYS_INFO_PARAM_PRI_OS_NAME:
      memcpy(g_sys_info_params.pri_os_name, &req->data[1], SIZE_OS_NAME);
      break;
    case SYS_INFO_PARAM_PRESENT_OS_NAME:
      memcpy(g_sys_info_params.present_os_name, &req->data[1], SIZE_OS_NAME);
      break;
    case SYS_INFO_PARAM_PRESENT_OS_VER:
      memcpy(g_sys_info_params.present_os_ver, &req->data[1], SIZE_OS_VER);
      break;
    case SYS_INFO_PARAM_BMC_URL:
      memcpy(g_sys_info_params.bmc_url, &req->data[1], SIZE_BMC_URL);
      break;
    case SYS_INFO_PARAM_OS_HV_URL:
      memcpy(g_sys_info_params.os_hv_url, &req->data[1], SIZE_OS_HV_URL);
      break;
    default:
      res->cc = CC_INVALID_PARAM;
      break;
  }

  return;
}

// Get System Info Params (IPMI/Section 22.14b)
static void
app_get_sys_info_params (unsigned char *request, unsigned char *response,
			 unsigned char *res_len)
{

  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  unsigned char param = req->data[1];

  // Fill default return values
  res->cc = CC_SUCCESS;
  *data++ = 1;		// Parameter revision

  switch (param)
  {
    case SYS_INFO_PARAM_SET_IN_PROG:
      *data++ = g_sys_info_params.set_in_prog;
      break;
    case SYS_INFO_PARAM_SYSFW_VER:
      memcpy(data, g_sys_info_params.sysfw_ver, SIZE_SYSFW_VER);
      data += SIZE_SYSFW_VER;
      break;
    case SYS_INFO_PARAM_SYS_NAME:
      memcpy(data, g_sys_info_params.sys_name, SIZE_SYS_NAME);
      data += SIZE_SYS_NAME;
      break;
    case SYS_INFO_PARAM_PRI_OS_NAME:
      memcpy(data, g_sys_info_params.pri_os_name, SIZE_OS_NAME);
      data += SIZE_OS_NAME;
      break;
    case SYS_INFO_PARAM_PRESENT_OS_NAME:
      memcpy(data, g_sys_info_params.present_os_name, SIZE_OS_NAME);
      data += SIZE_OS_NAME;
      break;
    case SYS_INFO_PARAM_PRESENT_OS_VER:
      memcpy(data, g_sys_info_params.present_os_ver, SIZE_OS_VER);
      data += SIZE_OS_VER;
      break;
    case SYS_INFO_PARAM_BMC_URL:
      memcpy(data, g_sys_info_params.bmc_url, SIZE_BMC_URL);
      data += SIZE_BMC_URL;
      break;
    case SYS_INFO_PARAM_OS_HV_URL:
      memcpy(data, g_sys_info_params.os_hv_url, SIZE_OS_HV_URL);
      data += SIZE_OS_HV_URL;
      break;
    default:
      res->cc = CC_INVALID_PARAM;
      break;
  }

  if (res->cc == CC_SUCCESS) {
    *res_len = data - &res->data[0];
  }

  return;
}

// Handle Appliction Commands (IPMI/Section 20)
static void
ipmi_handle_app (unsigned char *request, unsigned char req_len,
		 unsigned char *response, unsigned char *res_len)
{
  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char cmd = req->cmd;

  pthread_mutex_lock(&m_app);
  switch (cmd)
  {
    case CMD_APP_GET_DEVICE_ID:
      app_get_device_id (response, res_len);
      break;
    case CMD_APP_COLD_RESET:
      app_cold_reset (response, res_len);
      break;
    case CMD_APP_WARM_RESET:
      app_warm_reset (response, res_len);
      break;
    case CMD_APP_GET_SELFTEST_RESULTS:
      app_get_selftest_results (response, res_len);
      break;
    case CMD_APP_GET_DEVICE_GUID:
    case CMD_APP_GET_SYSTEM_GUID:
      // Get Device GUID and Get System GUID returns same data
      // from IPMI stack. FYI, Get System GUID will have to be
      // sent with in an IPMI session that includes session info
      app_get_device_guid (response, res_len);
      break;
    case CMD_APP_GET_GLOBAL_ENABLES:
      app_get_global_enables (response, res_len);
      break;
    case CMD_APP_SET_SYS_INFO_PARAMS:
      app_set_sys_info_params (request, response, res_len);
      break;
    case CMD_APP_GET_SYS_INFO_PARAMS:
      app_get_sys_info_params (request, response, res_len);
      break;
    default:
      res->cc = CC_INVALID_CMD;
      break;
  }
  pthread_mutex_unlock(&m_app);
}

/*
 * Function(s) to handle IPMI messages with NetFn: Storage
 */

static void
storage_get_fruid_info(unsigned char *response, unsigned char *res_len)
{
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  int size = plat_fruid_size();

  res->cc = CC_SUCCESS;

  *data++ = size & 0xFF; // FRUID size LSB
  *data++ = (size >> 8) & 0xFF; // FRUID size MSB
  *data++ = 0x00; // Device accessed by bytes

  *res_len = data - &res->data[0];

  return;
}

static void
storage_get_fruid_data(unsigned char *request, unsigned char *response,
    unsigned char *res_len)
{
  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  int offset = req->data[1] + (req->data[2] << 8);
  int count = req->data[3];

  int ret = plat_fruid_data(offset, count, &(res->data[1]));
  if (ret) {
    res->cc = CC_UNSPECIFIED_ERROR;
  } else {
    res->cc = CC_SUCCESS;
    *data++ = count;
    data += count;
  }

  if (res->cc == CC_SUCCESS) {
    *res_len = data - &res->data[0];
  }
  return;
}

static void
storage_get_sdr_info (unsigned char *response, unsigned char *res_len)
{
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  int num_entries;		// number of sdr records
  int free_space;		// free space in SDR device in bytes
  time_stamp_t ts_recent_add;	// Recent Addition Timestamp
  time_stamp_t ts_recent_erase;	// Recent Erasure Timestamp

  // Use platform APIs to get SDR information
  num_entries = plat_sdr_num_entries ();
  free_space = plat_sdr_free_space ();
  plat_sdr_ts_recent_add (&ts_recent_add);
  plat_sdr_ts_recent_erase (&ts_recent_erase);

  res->cc = CC_SUCCESS;

  *data++ = IPMI_SDR_VERSION;	// SDR version
  *data++ = num_entries & 0xFF;	// number of sdr entries
  *data++ = (num_entries >> 8) & 0xFF;
  *data++ = free_space & 0xFF;	// Free SDR Space
  *data++ = (free_space >> 8) & 0xFF;

  memcpy(data, ts_recent_add.ts, SIZE_TIME_STAMP);
  data += SIZE_TIME_STAMP;

  memcpy(data, ts_recent_erase.ts, SIZE_TIME_STAMP);
  data += SIZE_TIME_STAMP;

  *data++ = 0x02;		// Operations supported

  *res_len = data - &res->data[0];

  return;
}

static void
storage_rsv_sdr (unsigned char *response, unsigned char *res_len)
{
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  int rsv_id;			// SDR reservation ID

  // Use platform APIs to get a SDR reservation ID
  rsv_id = plat_sdr_rsv_id ();
  if (rsv_id < 0)
  {
      res->cc = CC_UNSPECIFIED_ERROR;
      return;
  }

  res->cc = CC_SUCCESS;
  *data++ = rsv_id & 0xFF;	// Reservation ID
  *data++ = (rsv_id >> 8) & 0XFF;

  *res_len = data - &res->data[0];

  return;
}

static void
storage_get_sdr (unsigned char *request, unsigned char *response,
		 unsigned char *res_len)
{
  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  int read_rec_id;		//record ID to be read
  int next_rec_id;		//record ID for the next entry
  int rsv_id;			// Reservation ID for the request
  int rec_offset;		// Read offset into the record
  int rec_bytes;		// Number of bytes to be read
  sdr_rec_t entry;		// SDR record entry
  int ret;

  rsv_id = (req->data[1] >> 8) | req->data[0];
  read_rec_id = (req->data[3] >> 8) | req->data[2];
  rec_offset = req->data[4];
  rec_bytes = req->data[5];

  // Use platform API to read the record Id and get next ID
  ret = plat_sdr_get_entry (rsv_id, read_rec_id, &entry, &next_rec_id);
  if (ret)
  {
      res->cc = CC_UNSPECIFIED_ERROR;
      return;
  }

  res->cc = CC_SUCCESS;
  *data++ = next_rec_id & 0xFF;	// next record ID
  *data++ = (next_rec_id >> 8) & 0xFF;

  memcpy (data, &entry.rec[rec_offset], rec_bytes);
  data += rec_bytes;

  *res_len = data - &res->data[0];

  return;
}

static void
storage_get_sel_info (unsigned char *response, unsigned char *res_len)
{
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  int num_entries;		// number of log entries
  int free_space;		// free space in SEL device in bytes
  time_stamp_t ts_recent_add;	// Recent Addition Timestamp
  time_stamp_t ts_recent_erase;	// Recent Erasure Timestamp

  // Use platform APIs to get SEL information
  num_entries = plat_sel_num_entries ();
  free_space = plat_sel_free_space ();
  plat_sel_ts_recent_add (&ts_recent_add);
  plat_sel_ts_recent_erase (&ts_recent_erase);

  res->cc = CC_SUCCESS;

  *data++ = IPMI_SEL_VERSION;	// SEL version
  *data++ = num_entries & 0xFF;	// number of log entries
  *data++ = (num_entries >> 8) & 0xFF;
  *data++ = free_space & 0xFF;	// Free SEL Space
  *data++ = (free_space >> 8) & 0xFF;

  memcpy(data, ts_recent_add.ts, SIZE_TIME_STAMP);
  data += SIZE_TIME_STAMP;

  memcpy(data, ts_recent_erase.ts, SIZE_TIME_STAMP);
  data += SIZE_TIME_STAMP;

  *data++ = 0x02;		// Operations supported

  *res_len = data - &res->data[0];

  return;
}

static void
storage_rsv_sel (unsigned char *response, unsigned char *res_len)
{
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  int rsv_id;			// SEL reservation ID

  // Use platform APIs to get a SEL reservation ID
  rsv_id = plat_sel_rsv_id ();
  if (rsv_id < 0)
  {
      res->cc = CC_SEL_ERASE_PROG;
      return;
  }

  res->cc = CC_SUCCESS;
  *data++ = rsv_id & 0xFF;	// Reservation ID
  *data++ = (rsv_id >> 8) & 0XFF;

  *res_len = data - &res->data[0];

  return;
}

static void
storage_get_sel (unsigned char *request, unsigned char *response,
		 unsigned char *res_len)
{
  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  int read_rec_id;		//record ID to be read
  int next_rec_id;		//record ID for the next msg
  sel_msg_t entry;		// SEL log entry
  int ret;

  read_rec_id = (req->data[3] >> 8) | req->data[2];

  // Use platform API to read the record Id and get next ID
  ret = plat_sel_get_entry (read_rec_id, &entry, &next_rec_id);
  if (ret)
  {
    res->cc = CC_UNSPECIFIED_ERROR;
    return;
  }

  res->cc = CC_SUCCESS;
  *data++ = next_rec_id & 0xFF;	// next record ID
  *data++ = (next_rec_id >> 8) & 0xFF;

  memcpy(data, entry.msg, SIZE_SEL_REC);
  data += SIZE_SEL_REC;

  *res_len = data - &res->data[0];

  return;
}

static void
storage_add_sel (unsigned char *request, unsigned char *response,
		 unsigned char *res_len)
{
  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];


  int record_id;		// Record ID for added entry
  int ret;

  sel_msg_t entry;

  memcpy(entry.msg, req->data, SIZE_SEL_REC);

  // Use platform APIs to add the new SEL entry
  ret = plat_sel_add_entry (&entry, &record_id);
  if (ret)
  {
    res->cc = CC_UNSPECIFIED_ERROR;
    return;
  }

  res->cc = CC_SUCCESS;
  *data++ = record_id & 0xFF;
  *data++ = (record_id >> 8) & 0xFF;

  *res_len = data - &res->data[0];

  return;
}

static void
storage_clr_sel (unsigned char *request, unsigned char *response,
		 unsigned char *res_len)
{
  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  sel_erase_stat_t status;
  int ret;
  int rsv_id;

  // Verify the request to contain 'CLR' characters
  if ((req->data[2] != 'C') || (req->data[3] != 'L') || (req->data[4] != 'R'))
  {
    res->cc = CC_INVALID_PARAM;
    return;
  }

  // Populate reservation ID given in request
  rsv_id = (req->data[1] << 8) | req->data[0];

  // Use platform APIs to clear or get status
  if (req->data[5] == IPMI_SEL_INIT_ERASE)
  {
    ret = plat_sel_erase (rsv_id);
  }
  else if (req->data[5] == IPMI_SEL_ERASE_STAT)
  {
    ret = plat_sel_erase_status (rsv_id, &status);
  }
  else
  {
    res->cc = CC_INVALID_PARAM;
    return;
  }

  // Handle platform error and return
  if (ret)
    {
      res->cc = CC_UNSPECIFIED_ERROR;
      return;
    }

  res->cc = CC_SUCCESS;
  *data++ = status;

  *res_len = data - &res->data[0];

  return;
}

static void
ipmi_handle_storage (unsigned char *request, unsigned char req_len,
		     unsigned char *response, unsigned char *res_len)
{
  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char cmd = req->cmd;

  res->cc = CC_SUCCESS;
  *res_len = 0;

  pthread_mutex_lock(&m_storage);
  switch (cmd)
  {
    case CMD_STORAGE_GET_FRUID_INFO:
      storage_get_fruid_info (response, res_len);
      break;
    case CMD_STORAGE_READ_FRUID_DATA:
      storage_get_fruid_data (request, response, res_len);
      break;
    case CMD_STORAGE_GET_SEL_INFO:
      storage_get_sel_info (response, res_len);
      break;
    case CMD_STORAGE_RSV_SEL:
      storage_rsv_sel (response, res_len);
      break;
    case CMD_STORAGE_ADD_SEL:
      storage_add_sel (request, response, res_len);
      break;
    case CMD_STORAGE_GET_SEL:
      storage_get_sel (request, response, res_len);
      break;
    case CMD_STORAGE_CLR_SEL:
      storage_clr_sel (request, response, res_len);
      break;
    case CMD_STORAGE_GET_SDR_INFO:
      storage_get_sdr_info (response, res_len);
      break;
    case CMD_STORAGE_RSV_SDR:
      storage_rsv_sdr (response, res_len);
      break;
    case CMD_STORAGE_GET_SDR:
      storage_get_sdr (request, response, res_len);
      break;
    default:
      res->cc = CC_INVALID_CMD;
      break;
  }

  pthread_mutex_unlock(&m_storage);
  return;
}

/*
 * Function(s) to handle IPMI messages with NetFn: Transport
 */

// Set LAN Configuration (IPMI/Section 23.1)
static void
transport_set_lan_config (unsigned char *request, unsigned char *response,
			  unsigned char *res_len)
{
  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char param = req->data[1];

  // Fill the response with default values
  res->cc = CC_SUCCESS;

  switch (param)
  {
    case LAN_PARAM_SET_IN_PROG:
      g_lan_config.set_in_prog = req->data[2];
      break;
    case LAN_PARAM_AUTH_SUPPORT:
      g_lan_config.auth_support = req->data[2];
      break;
    case LAN_PARAM_AUTH_ENABLES:
      memcpy(g_lan_config.auth_enables, &req->data[2], SIZE_AUTH_ENABLES);
      break;
    case LAN_PARAM_IP_ADDR:
      memcpy(g_lan_config.ip_addr, &req->data[2], SIZE_IP_ADDR);
      break;
    case LAN_PARAM_IP_SRC:
      g_lan_config.ip_src = req->data[2];
      break;
    case LAN_PARAM_MAC_ADDR:
      memcpy(g_lan_config.mac_addr, &req->data[2], SIZE_MAC_ADDR);
      break;
    case LAN_PARAM_NET_MASK:
      memcpy(g_lan_config.net_mask, &req->data[2], SIZE_NET_MASK);
      break;
    case LAN_PARAM_IP_HDR:
      memcpy(g_lan_config.ip_hdr, &req->data[2], SIZE_IP_HDR);
      break;
    case LAN_PARAM_PRI_RMCP_PORT:
      g_lan_config.pri_rmcp_port[0] = req->data[2];
      g_lan_config.pri_rmcp_port[1] = req->data[3];
      break;
    case LAN_PARAM_SEC_RMCP_PORT:
      g_lan_config.sec_rmcp_port[0] = req->data[2];
      g_lan_config.sec_rmcp_port[1] = req->data[3];
      break;
    case LAN_PARAM_ARP_CTRL:
      g_lan_config.arp_ctrl = req->data[2];
      break;
    case LAN_PARAM_GARP_INTERVAL:
      g_lan_config.garp_interval = req->data[2];
      break;
    case LAN_PARAM_DF_GW_IP_ADDR:
      memcpy(g_lan_config.df_gw_ip_addr, &req->data[2], SIZE_IP_ADDR);
      break;
    case LAN_PARAM_DF_GW_MAC_ADDR:
      memcpy(g_lan_config.df_gw_mac_addr, &req->data[2], SIZE_MAC_ADDR);
      break;
    case LAN_PARAM_BACK_GW_IP_ADDR:
      memcpy(g_lan_config.back_gw_ip_addr, &req->data[2], SIZE_IP_ADDR);
      break;
    case LAN_PARAM_BACK_GW_MAC_ADDR:
      memcpy(g_lan_config.back_gw_mac_addr, &req->data[2], SIZE_MAC_ADDR);
      break;
    case LAN_PARAM_COMMUNITY_STR:
      memcpy(g_lan_config.community_str, &req->data[2], SIZE_COMMUNITY_STR);
      break;
    case LAN_PARAM_NO_OF_DEST:
      g_lan_config.no_of_dest = req->data[2];
      break;
    case LAN_PARAM_DEST_TYPE:
      memcpy(g_lan_config.dest_type, &req->data[2], SIZE_DEST_TYPE);
      break;
    case LAN_PARAM_DEST_ADDR:
      memcpy(g_lan_config.dest_addr, &req->data[2], SIZE_DEST_ADDR);
      break;
    default:
      res->cc = CC_INVALID_PARAM;
      break;
  }
}

// Get LAN Configuration (IPMI/Section 23.2)
static void
transport_get_lan_config (unsigned char *request, unsigned char *response,
			  unsigned char *res_len)
{

  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  unsigned char param = req->data[1];

  // Fill the response with default values
  res->cc = CC_SUCCESS;
  *data++ = 0x01;		// Parameter revision

  switch (param)
  {
    case LAN_PARAM_SET_IN_PROG:
      *data++ = g_lan_config.set_in_prog;
      break;
    case LAN_PARAM_AUTH_SUPPORT:
      *data++ = g_lan_config.auth_support;
      break;
    case LAN_PARAM_AUTH_ENABLES:
      memcpy(data, g_lan_config.auth_enables, SIZE_AUTH_ENABLES);
      data += SIZE_AUTH_ENABLES;
      break;
    case LAN_PARAM_IP_ADDR:
      memcpy(data, g_lan_config.ip_addr, SIZE_IP_ADDR);
      data += SIZE_IP_ADDR;
      break;
    case LAN_PARAM_IP_SRC:
      *data++ = g_lan_config.ip_src;
      break;
    case LAN_PARAM_MAC_ADDR:
      memcpy(data, g_lan_config.mac_addr, SIZE_MAC_ADDR);
      data += SIZE_MAC_ADDR;
      break;
    case LAN_PARAM_NET_MASK:
      memcpy(data, g_lan_config.net_mask, SIZE_NET_MASK);
      data += SIZE_NET_MASK;
      break;
    case LAN_PARAM_IP_HDR:
      memcpy(data, g_lan_config.ip_hdr, SIZE_IP_HDR);
      data += SIZE_IP_HDR;
      break;
    case LAN_PARAM_PRI_RMCP_PORT:
      *data++ = g_lan_config.pri_rmcp_port[0];
      *data++ = g_lan_config.pri_rmcp_port[1];
      break;
    case LAN_PARAM_SEC_RMCP_PORT:
      *data++ = g_lan_config.sec_rmcp_port[0];
      *data++ = g_lan_config.sec_rmcp_port[1];
      break;
    case LAN_PARAM_ARP_CTRL:
      *data++ = g_lan_config.arp_ctrl;
      break;
    case LAN_PARAM_GARP_INTERVAL:
      *data++ = g_lan_config.garp_interval;
      break;
    case LAN_PARAM_DF_GW_IP_ADDR:
      memcpy(data, g_lan_config.df_gw_ip_addr, SIZE_IP_ADDR);
      data += SIZE_IP_ADDR;
      break;
    case LAN_PARAM_DF_GW_MAC_ADDR:
      memcpy(data, g_lan_config.df_gw_mac_addr, SIZE_MAC_ADDR);
      data += SIZE_MAC_ADDR;
      break;
    case LAN_PARAM_BACK_GW_IP_ADDR:
      memcpy(data, g_lan_config.back_gw_ip_addr, SIZE_IP_ADDR);
      data += SIZE_IP_ADDR;
      break;
    case LAN_PARAM_BACK_GW_MAC_ADDR:
      memcpy(data, g_lan_config.back_gw_mac_addr, SIZE_MAC_ADDR);
      data += SIZE_MAC_ADDR;
      break;
    case LAN_PARAM_COMMUNITY_STR:
      memcpy(data, g_lan_config.community_str, SIZE_COMMUNITY_STR);
      data += SIZE_COMMUNITY_STR;
      break;
    case LAN_PARAM_NO_OF_DEST:
      *data++ = g_lan_config.no_of_dest;
      break;
    case LAN_PARAM_DEST_TYPE:
      memcpy(data, g_lan_config.dest_type, SIZE_DEST_TYPE);
      data += SIZE_DEST_TYPE;
      break;
    case LAN_PARAM_DEST_ADDR:
      memcpy(data, g_lan_config.dest_addr, SIZE_DEST_ADDR);
      data += SIZE_DEST_ADDR;
      break;
    default:
      res->cc = CC_INVALID_PARAM;
      break;
  }

  if (res->cc == CC_SUCCESS) {
    *res_len = data - &res->data[0];
  }
}

// Handle Transport Commands (IPMI/Section 23)
static void
ipmi_handle_transport (unsigned char *request, unsigned char req_len,
		       unsigned char *response, unsigned char *res_len)
{
  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char cmd = req->cmd;

  pthread_mutex_lock(&m_transport);
  switch (cmd)
  {
    case CMD_TRANSPORT_SET_LAN_CONFIG:
      transport_set_lan_config (request, response, res_len);
      break;
    case CMD_TRANSPORT_GET_LAN_CONFIG:
      transport_get_lan_config (request, response, res_len);
      break;
    default:
      res->cc = CC_INVALID_CMD;
      break;
  }
  pthread_mutex_unlock(&m_transport);
}

/*
 * Function(s) to handle IPMI messages with NetFn: OEM
 */

static void
oem_set_proc_info (unsigned char *request, unsigned char *response,
		   unsigned char *res_len)
{
  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  g_proc_info.type = req->data[1];
  g_proc_info.freq[0] = req->data[2];
  g_proc_info.freq[1] = req->data[3];

  res->cc = CC_SUCCESS;
  *res_len = 0;
}

static void
oem_set_dimm_info (unsigned char *request, unsigned char *response,
		   unsigned char *res_len)
{
  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  unsigned char index = req->data[0];

  g_dimm_info[index].type = req->data[1];
  g_dimm_info[index].speed[0] = req->data[2];
  g_dimm_info[index].speed[1] = req->data[3];
  g_dimm_info[index].size[0] = req->data[4];
  g_dimm_info[index].size[1] = req->data[5];

  res->cc = CC_SUCCESS;
  *res_len = 0;
}

static void
oem_set_post_start (unsigned char *response, unsigned char *res_len)
{

  ipmi_res_t *res = (ipmi_res_t *) response;

  // TODO: For now logging the event, need to find usage for this info
  syslog (LOG_INFO, "POST Start Event\n");

  res->cc = CC_SUCCESS;
  *res_len = 0;
}

static void
oem_set_post_end (unsigned char *response, unsigned char *res_len)
{

  ipmi_res_t *res = (ipmi_res_t *) response;

  // TODO: For now logging the event, need to find usage for this info
  syslog (LOG_INFO, "POST End Event\n");

  res->cc = CC_SUCCESS;
  *res_len = 0;
}

static void
ipmi_handle_oem (unsigned char *request, unsigned char req_len,
		 unsigned char *response, unsigned char *res_len)
{

  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  unsigned char cmd = req->cmd;

  pthread_mutex_lock(&m_oem);
  switch (cmd)
  {
    case CMD_OEM_SET_PROC_INFO:
      oem_set_proc_info (request, response, res_len);
      break;
    case CMD_OEM_SET_DIMM_INFO:
      oem_set_dimm_info (request, response, res_len);
      break;
    case CMD_OEM_SET_POST_START:
      oem_set_post_start (response, res_len);
      break;
    case CMD_OEM_SET_POST_END:
      oem_set_post_end (response, res_len);
      break;
    default:
      res->cc = CC_INVALID_CMD;
      break;
  }
  pthread_mutex_unlock(&m_oem);
}

/*
 * Function to handle all IPMI messages
 */
static void
ipmi_handle (unsigned char *request, unsigned char req_len,
	     unsigned char *response, unsigned char *res_len)
{

  ipmi_req_t *req = (ipmi_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char netfn;

  netfn = req->netfn_lun >> 2;

  // Provide default values in the response message
  res->cmd = req->cmd;
  res->cc = 0xFF;		// Unspecified completion code
  *res_len = 0;

  switch (netfn)
  {
    case NETFN_CHASSIS_REQ:
      res->netfn_lun = NETFN_CHASSIS_RES << 2;
      ipmi_handle_chassis (request, req_len, response, res_len);
      break;
    case NETFN_APP_REQ:
      res->netfn_lun = NETFN_APP_RES << 2;
      ipmi_handle_app (request, req_len, response, res_len);
      break;
    case NETFN_STORAGE_REQ:
      res->netfn_lun = NETFN_STORAGE_RES << 2;
      ipmi_handle_storage (request, req_len, response, res_len);
      break;
    case NETFN_TRANSPORT_REQ:
      res->netfn_lun = NETFN_TRANSPORT_RES << 2;
      ipmi_handle_transport (request, req_len, response, res_len);
      break;
    case NETFN_OEM_REQ:
      res->netfn_lun = NETFN_OEM_RES << 2;
      ipmi_handle_oem (request, req_len, response, res_len);
      break;
    default:
      res->netfn_lun = (netfn + 1) << 2;
      break;
  }

  // This header includes NetFunction, Command, and Completion Code
  *res_len += SIZE_IPMI_RES_HDR;

  return;
}

void
*conn_handler(void *socket_desc) {
  int sock = *(int*)socket_desc;
  int n;
  unsigned char req_buf[MAX_IPMI_MSG_SIZE];
  unsigned char res_buf[MAX_IPMI_MSG_SIZE];
  unsigned char res_len = 0;

  n = recv (sock, req_buf, sizeof(req_buf), 0);
  if (n <= 0) {
      syslog(LOG_ALERT, "ipmid: recv() failed with %d\n", n);
      goto conn_cleanup;
  }

  ipmi_handle(req_buf, n, res_buf, &res_len);

  if (send (sock, res_buf, res_len, 0) < 0) {
    syslog(LOG_ALERT, "ipmid: send() failed\n");
  }

conn_cleanup:
  close(sock);

  pthread_exit(NULL);
  return 0;
}


int
main (void)
{
  int s, s2, t, len;
  struct sockaddr_un local, remote;
  pthread_t tid;

  daemon(1, 0);
  openlog("ipmid", LOG_CONS, LOG_DAEMON);

  plat_sel_init();
  plat_sensor_init();
  plat_sdr_init();
  plat_fruid_init();

  pthread_mutex_init(&m_chassis, NULL);
  pthread_mutex_init(&m_app, NULL);
  pthread_mutex_init(&m_storage, NULL);
  pthread_mutex_init(&m_transport, NULL);
  pthread_mutex_init(&m_oem, NULL);

  if ((s = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
  {
    syslog(LOG_ALERT, "ipmid: socket() failed\n");
    exit (1);
  }

  local.sun_family = AF_UNIX;
  strcpy (local.sun_path, SOCK_PATH);
  unlink (local.sun_path);
  len = strlen (local.sun_path) + sizeof (local.sun_family);
  if (bind (s, (struct sockaddr *) &local, len) == -1)
  {
    syslog(LOG_ALERT, "ipmid: bind() failed\n");
    exit (1);
  }

  if (listen (s, 5) == -1)
  {
    syslog(LOG_ALERT, "ipmid: listen() failed\n");
    exit (1);
  }

  while(1) {
    int n;
    t = sizeof (remote);
    if ((s2 = accept (s, (struct sockaddr *) &remote, &t)) < 0) {
      syslog(LOG_ALERT, "ipmid: accept() failed\n");
      break;
    }

    // Creating a worker thread to handle the request
    // TODO: Need to monitor the server performance with higher load and
    // see if we need to create pre-defined number of workers and schedule
    // the requests among them.
    if (pthread_create(&tid, NULL, conn_handler, (void*) &s2) < 0) {
        syslog(LOG_ALERT, "ipmid: pthread_create failed\n");
        close(s2);
        continue;
    }

    pthread_detach(tid);
  }

  close(s);

  pthread_mutex_destroy(&m_chassis);
  pthread_mutex_destroy(&m_app);
  pthread_mutex_destroy(&m_storage);
  pthread_mutex_destroy(&m_transport);
  pthread_mutex_destroy(&m_oem);

  return 0;
}
