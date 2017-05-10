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

#include "sdr.h"
#include "sel.h"
#include "fruid.h"
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
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>
#include <sys/reboot.h>

#define SIZE_IANA_ID 3
#define SIZE_GUID 16

//declare for clearing BIOS flag
#define BIOS_Timeout 600
// Boot valid flag
#define BIOS_BOOT_VALID_FLAG (1U << 7)
#define CMOS_VALID_FLAG      (1U << 1)

static unsigned char IsTimerStart[MAX_NODES] = {0};


extern void plat_lan_init(lan_config_t *lan);

// TODO: Once data storage is finalized, the following structure needs
// to be retrieved/updated from persistant backend storage
static lan_config_t g_lan_config = { 0 };
static proc_info_t g_proc_info = { 0 };
static dimm_info_t g_dimm_info[MAX_NUM_DIMMS] = { 0 };

// TODO: Need to store this info after identifying proper storage
static sys_info_param_t g_sys_info_params;

// IPMI Watchdog Timer Structure
struct watchdog_data {
  pthread_mutex_t mutex;
  pthread_t tid;
  uint8_t slot;
  uint8_t valid;
  uint8_t run;
  uint8_t no_log;
  uint8_t use;
  uint8_t pre_action;
  uint8_t action;
  uint8_t pre_interval;
  uint8_t expiration;
  uint16_t init_count_down;
  uint16_t present_count_down;
};

static struct watchdog_data *g_wdt[MAX_NUM_FRUS];

static char* wdt_use_name[8] = {
  "reserved",
  "BIOS FRB2",
  "BIOS/POST",
  "OS Load",
  "SMS/OS",
  "OEM",
  "reserved",
  "reserved",
};

static char* wdt_action_name[8] = {
  "Timer expired",
  "Hard Reset",
  "Power Down",
  "Power Cycle",
  "reserved",
  "reserved",
  "reserved",
  "reserved",
};

// TODO: Based on performance testing results, might need fine grained locks
// Since the global data is specific to a NetFunction, adding locs at NetFn level
static pthread_mutex_t m_chassis;
static pthread_mutex_t m_sensor;
static pthread_mutex_t m_app;
static pthread_mutex_t m_storage;
static pthread_mutex_t m_transport;
static pthread_mutex_t m_oem;
static pthread_mutex_t m_oem_1s;
static pthread_mutex_t m_oem_usb_dbg;
static pthread_mutex_t m_oem_q;

static void ipmi_handle(unsigned char *request, unsigned char req_len,
       unsigned char *response, unsigned char *res_len);

static struct watchdog_data *get_watchdog(int slot_id)
{
  if (slot_id > MAX_NUM_FRUS)
    return NULL;
  return g_wdt[slot_id - 1];
}

static int length_check(unsigned char cmd_len, unsigned char req_len, unsigned char *response, unsigned char *res_len)
{
  ipmi_res_t *res = (ipmi_res_t *) response;
  // req_len = cmd_len + 3 (payload_id, cmd and netfn)
  if( req_len != (cmd_len + 3) ){
    res->cc = CC_INVALID_LENGTH;
    *res_len = 1;
    return 1;
  }
  return 0;
}

/*
 **Function to handle with clearing BIOS flag
 */
void
*clear_bios_data_timer(void *ptr)
{
  int timer = 0;
  unsigned char boot[SIZE_BOOT_ORDER]={0};
  unsigned char slot_id = *(uint8_t *)ptr;
  unsigned char res_len;
  pthread_detach(pthread_self());

  while( timer <= BIOS_Timeout )
  {
#ifdef DEBUG
    syslog(LOG_WARNING, "[%s][%lu] Timer: %d\n", __func__, pthread_self(), timer);
#endif
    sleep(1);

    timer++;
  }

  //get boot order setting
  pal_get_boot_order(slot_id, NULL, boot, &res_len);

#ifdef DEBUG
  syslog(LOG_WARNING, "[%s][%lu] Get: %x %x %x %x %x %x\n", __func__, pthread_self() ,boot[0], boot[1], boot[2], boot[3], boot[4], boot[5]);
#endif

  //clear boot-valid and cmos bits due to timeout:
  boot[0] &= ~(BIOS_BOOT_VALID_FLAG | CMOS_VALID_FLAG);

#ifdef DEBUG
  syslog(LOG_WARNING, "[%s][%lu] Set: %x %x %x %x %x %x\n", __func__, pthread_self() , boot[0], boot[1], boot[2], boot[3], boot[4], boot[5]);
#endif

  //set data
  pal_set_boot_order(slot_id, boot, NULL, &res_len);

  IsTimerStart[slot_id - 1] = false;

  pthread_exit(0);
}

/*
 * Function(s) to handle IPMI messages with NetFn: Chassis
 */
// Get Chassis Status (IPMI/Section 28.2)
static void
chassis_get_status (unsigned char *request, unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  res->cc = CC_SUCCESS;

  pal_get_chassis_status(req->payload_id, req->data, res->data, res_len);
}

// Set Power Restore Policy (IPMI/Section 28.8)
static void
chassis_set_power_restore_policy(unsigned char *request, unsigned char req_len,
                                 unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res= (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  *data++ = 0x07;  // Power restore policy support(bitfield)

  res->cc = pal_set_power_restore_policy(req->payload_id, req->data, res->data);
  if (res->cc == CC_SUCCESS) {
    *res_len = data - &res->data[0];
  }
}

// Get System Boot Options (IPMI/Section 28.12)
static void
chassis_get_boot_options (unsigned char *request, unsigned char req_len,
                          unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res= (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  unsigned char param = req->data[0];

  if(param >= 8) {
    res->cc = CC_PARAM_OUT_OF_RANGE;
    return;
  }

  // Fill response with default values
  res->cc = CC_SUCCESS;
  *data++ = 0x01;   // Parameter Version
  *data++ = req->data[0]; // Parameter

  *res_len = pal_get_boot_option(param, data) + 2;
}

// Set System Boot Options (IPMI/Section 28)
static void
chassis_set_boot_options (unsigned char *request, unsigned char req_len,
                          unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res= (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  unsigned char param = req->data[0];

  // Fill response with default values
  if(param >= 8) {
    res->cc = CC_PARAM_OUT_OF_RANGE;
    return;
  }

  res->cc = CC_SUCCESS;

  pal_set_boot_option(param,req->data+1);

  *res_len = data - &res->data[0];
}

// Handle Chassis Commands (IPMI/Section 28)
static void
ipmi_handle_chassis (unsigned char *request, unsigned char req_len,
         unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char cmd = req->cmd;

  pthread_mutex_lock(&m_chassis);
  switch (cmd)
  {
    case CMD_CHASSIS_GET_STATUS:
      chassis_get_status (request, response, res_len);
      break;
    case CMD_CHASSIS_SET_POWER_RESTORE_POLICY:
      chassis_set_power_restore_policy(request, req_len, response, res_len);
      break;
    case CMD_CHASSIS_SET_BOOT_OPTIONS:
      chassis_set_boot_options(request, req_len, response, res_len);
      break;
    case CMD_CHASSIS_GET_BOOT_OPTIONS:
      chassis_get_boot_options(request, req_len, response, res_len);
      break;
    default:
      res->cc = CC_INVALID_CMD;
      break;
  }
  pthread_mutex_unlock(&m_chassis);
}

/*
 * Function(s) to handle IPMI messages with NetFn: Sensor
 */
// Platform Event Message (IPMI/Section 29.3)
static void
sensor_plat_event_msg(unsigned char *request, unsigned char req_len,
                      unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  int record_id;    // Record ID for added entry
  int ret;

  sel_msg_t entry;

  // Platform event provides only last 7 bytes of SEL's 16-byte entry
  entry.msg[2] = 0x02;  /* Set Record Type to be system event record.*/
  memcpy(&entry.msg[9], req->data, 7);

  // Use platform APIs to add the new SEL entry
  ret = sel_add_entry (req->payload_id, &entry, &record_id);
  if (ret)
  {
    res->cc = CC_UNSPECIFIED_ERROR;
    return;
  }

  res->cc = CC_SUCCESS;
}

// Handle Sensor/Event Commands (IPMI/Section 29)
static void
ipmi_handle_sensor(unsigned char *request, unsigned char req_len,
                   unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char cmd = req->cmd;

  pthread_mutex_lock(&m_sensor);
  switch (cmd)
  {
    case CMD_SENSOR_PLAT_EVENT_MSG:
      sensor_plat_event_msg(request, req_len, response, res_len);
      break;
    default:
      res->cc = CC_INVALID_CMD;
      break;
  }
  pthread_mutex_unlock(&m_sensor);
}

/*
 * Function(s) to handle IPMI messages with NetFn: Application
 */
// Get Device ID (IPMI/Section 20.1)
static void
app_get_device_id (unsigned char *request, unsigned char req_len,
                   unsigned char *response, unsigned char *res_len)
{

  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  FILE *fp=NULL;
  int fv_major = 0x01, fv_minor = 0x03;
  char buffer[32];

  if(length_check(0, req_len, response, res_len))
  {
    return;
  }

  fp = fopen("/etc/issue","r");
  if (fp != NULL)
  {
     if (fgets(buffer, sizeof(buffer), fp))
         sscanf(buffer, "%*[^v]v%d.%d", &fv_major, &fv_minor);
     fclose(fp);
  }

  res->cc = CC_SUCCESS;

  //TODO: Following data needs to be updated based on platform
  *data++ = 0x20;   // Device ID
  *data++ = 0x81;   // Device Revision
  *data++ = fv_major & 0x7f;      // Firmware Revision Major
  *data++ = ((fv_minor / 10) << 4) | (fv_minor % 10);      // Firmware Revision Minor
  *data++ = 0x02;   // IPMI Version
  *data++ = 0xBF;   // Additional Device Support
  *data++ = 0x15;   // Manufacturer ID1
  *data++ = 0xA0;   // Manufacturer ID2
  *data++ = 0x00;   // Manufacturer ID3
  *data++ = 0x46;   // Product ID1
  *data++ = 0x31;   // Product ID2
  *data++ = 0x00;   // Aux. Firmware Version1
  *data++ = 0x00;   // Aux. Firmware Version2
  *data++ = 0x00;   // Aux. Firmware Version3
  *data++ = 0x00;   // Aux. Firmware Version4

  *res_len = data - &res->data[0];
}

// Cold Reset (IPMI/Section 20.2)
static void
app_cold_reset(unsigned char *request, unsigned char req_len,
              unsigned char *response, unsigned char *res_len)
{
  reboot(RB_AUTOBOOT);
}


// Get Self Test Results (IPMI/Section 20.4)
static void
app_get_selftest_results (unsigned char *request, unsigned char req_len,
              unsigned char *response, unsigned char *res_len)
{

  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  res->cc = CC_SUCCESS;

  //TODO: Following data needs to be updated based on self-test results
  *data++ = 0x55;   // Self-Test result
  *data++ = 0x00;   // Extra error info in case of failure

  *res_len = data - &res->data[0];
}

// Manufacturing Test On (IPMI/Section 20.5)
static void
app_manufacturing_test_on (unsigned char *request, unsigned char req_len,
                           unsigned char *response, unsigned char *res_len)
{

  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  res->cc = CC_SUCCESS;

  if ((!memcmp(req->data, "sled-cycle", strlen("sled-cycle"))) &&
      (req_len - ((void*)req->data - (void*)req)) == strlen("sled-cycle")) {
    system("/usr/local/bin/power-util sled-cycle");
  } else {
    res->cc = CC_INVALID_PARAM;
  }

  *res_len = data - &res->data[0];
}

// Get Device GUID (IPMI/Section 20.8)
static void
app_get_device_guid (unsigned char *request, unsigned char req_len,
                    unsigned char *response, unsigned char *res_len)
{
  int ret;

  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  // Get the 16 bytes of Device GUID from PAL library
  ret = pal_get_dev_guid(req->payload_id, res->data);
  if (ret) {
      res->cc = CC_UNSPECIFIED_ERROR;
      *res_len = 0x00;
  } else {
      res->cc = CC_SUCCESS;
      *res_len = SIZE_GUID;
  }
}

static void
app_get_device_sys_guid (unsigned char *request, unsigned char req_len,
                        unsigned char *response, unsigned char *res_len)
{
  int ret;

  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  // Get the 16 bytes of System GUID from PAL library
  ret = pal_get_sys_guid(req->payload_id, res->data);
  if (ret) {
      res->cc = CC_UNSPECIFIED_ERROR;
      *res_len = 0x00;
  } else {
      res->cc = CC_SUCCESS;
      *res_len = SIZE_GUID;
  }
}

// Reset Watchdog Timer (IPMI/Section 27.5)
static void
app_reset_watchdog_timer (unsigned char *request, unsigned char req_len,
                        unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  struct watchdog_data *wdt = get_watchdog(req->payload_id);

  if (!wdt) {
    res->cc = CC_NOT_SUPP_IN_CURR_STATE;
    *res_len = 0;
    return;
  }

  pthread_mutex_lock(&wdt->mutex);
  if (wdt->valid) {
    res->cc = CC_SUCCESS;
    wdt->present_count_down = wdt->init_count_down;
    wdt->run = 1;
  }
  else
    res->cc = CC_INVALID_PARAM; // un-initialized watchdog
  pthread_mutex_unlock(&wdt->mutex);
  *res_len = data - &res->data[0];
}

// Set Watchdog Timer (IPMI/Section 27.6)
static void
app_set_watchdog_timer (unsigned char *request, unsigned char req_len,
                        unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t*) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  struct watchdog_data *wdt = get_watchdog(req->payload_id);

  if (!wdt) {
    res->cc = CC_NOT_SUPP_IN_CURR_STATE;
    *res_len = 0;
    return;
  }

  // no support pre-itmeout interrupt
  if (req->data[1] & 0xF0) {
    res->cc = CC_PARAM_OUT_OF_RANGE;
    *res_len = 0;
    return;
  }

  pthread_mutex_lock(&wdt->mutex);
  wdt->no_log = req->data[0] >> 7;
  wdt->use = req->data[0] & 0x7;
  wdt->pre_action = 0; // no support
  wdt->action = req->data[1] & 0x7;
  wdt->pre_interval = req->data[2];
  wdt->expiration &= ~(req->data[3]);
  wdt->init_count_down = (req->data[5]<<8 | req->data[4]);
  wdt->present_count_down = wdt->init_count_down;
  if (!(req->data[0] & 0x40)) // 'do not stop timer' bit
    wdt->run = 0;
  wdt->valid = 1;
  pthread_mutex_unlock(&wdt->mutex);
  res->cc = CC_SUCCESS;

  *res_len = data - &res->data[0];
}

// Get Watchdog Timer (IPMI/Section 27.7)
static void
app_get_watchdog_timer (unsigned char *request, unsigned char req_len,
                        unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  unsigned char byte;
  struct watchdog_data *wdt = get_watchdog(req->payload_id);

  if (!wdt) {
    res->cc = CC_NOT_SUPP_IN_CURR_STATE;
    *res_len = 0;
    return;
  }

  pthread_mutex_lock(&wdt->mutex);
  byte = wdt->use;
  if (wdt->no_log)
    byte |= 0x80;
  if (wdt->run)
    byte |= 0x40;
  *data++ = byte;
  *data++ = (wdt->pre_action << 4 | wdt->action);
  *data++ = wdt->pre_interval;
  *data++ = wdt->expiration;
  *data++ = wdt->init_count_down & 0xFF;
  *data++ = (wdt->init_count_down >> 8) & 0xFF;
  *data++ = wdt->present_count_down & 0xFF;
  *data++ = (wdt->present_count_down >> 8) & 0xFF;
  pthread_mutex_unlock(&wdt->mutex);
  res->cc = CC_SUCCESS;

  *res_len = data - &res->data[0];
}

// Set BMC Global Enables (IPMI/Section 22.1)
static void
app_set_global_enables (unsigned char *request, unsigned char req_len,
                        unsigned char *response, unsigned char *res_len)
{

  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  res->cc = CC_SUCCESS;

  // Do nothing

  *res_len = data - &res->data[0];
}

// Get BMC Global Enables (IPMI/Section 22.2)
static void
app_get_global_enables (unsigned char *request, unsigned char req_len,
                        unsigned char *response, unsigned char *res_len)
{

  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  res->cc = CC_SUCCESS;

  *data++ = 0x0C;   // Global Enable

  *res_len = data - &res->data[0];
}

// Clear Message flags Command (IPMI/Section 22.3)
static void
app_clear_message_flags (unsigned char *request, unsigned char req_len,
                         unsigned char *response, unsigned char *res_len)
{

  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  res->cc = CC_SUCCESS;

  // Do nothing

  *res_len = data - &res->data[0];
}

// Set System Info Params (IPMI/Section 22.14a)
static void
app_set_sys_info_params (unsigned char *request, unsigned char req_len,
                         unsigned char *response, unsigned char *res_len)
{

  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
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
      pal_set_sysfw_ver(req->payload_id, g_sys_info_params.sysfw_ver);
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
    case SYS_INFO_PARAM_BIOS_CURRENT_BOOT_LIST:
      memcpy(g_sys_info_params.bios_current_boot_list, &req->data[1], req_len-4); // boot list length = req_len-4 (payload_id, cmd, netfn, param)
      pal_set_bios_current_boot_list(req->payload_id, g_sys_info_params.bios_current_boot_list, req_len-4, &res->cc);
      break;
    case SYS_INFO_PARAM_BIOS_FIXED_BOOT_DEVICE:
      if(length_check(SIZE_BIOS_FIXED_BOOT_DEVICE+1, req_len, response, res_len))
        break;
      memcpy(g_sys_info_params.bios_fixed_boot_device, &req->data[1], SIZE_BIOS_FIXED_BOOT_DEVICE);
      pal_set_bios_fixed_boot_device(req->payload_id, g_sys_info_params.bios_fixed_boot_device);
      break;
    case SYS_INFO_PARAM_BIOS_RESTORES_DEFAULT_SETTING:
      if(length_check(SIZE_BIOS_RESTORES_DEFAULT_SETTING+1, req_len, response, res_len))
        break;
      memcpy(g_sys_info_params.bios_restores_default_setting, &req->data[1], SIZE_BIOS_RESTORES_DEFAULT_SETTING);
      pal_set_bios_restores_default_setting(req->payload_id, g_sys_info_params.bios_restores_default_setting);
      break;
    case SYS_INFO_PARAM_LAST_BOOT_TIME:
      if(length_check(SIZE_LAST_BOOT_TIME+1, req_len, response, res_len))
        break;
      memcpy(g_sys_info_params.last_boot_time, &req->data[1], SIZE_LAST_BOOT_TIME);
      pal_set_last_boot_time(req->payload_id, g_sys_info_params.last_boot_time);
      break;
    default:
      res->cc = CC_INVALID_PARAM;
      break;
  }

  return;
}

// Get System Info Params (IPMI/Section 22.14b)
static void
app_get_sys_info_params (unsigned char *request, unsigned char req_len,
                         unsigned char *response, unsigned char *res_len)
{

  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  unsigned char param = req->data[1];

  // Fill default return values
  res->cc = CC_SUCCESS;
  *data++ = 1;		// Parameter revision
  if(!length_check(4, req_len, response, res_len))
  {
    switch (param)
    {
      case SYS_INFO_PARAM_SET_IN_PROG:
        *data++ = g_sys_info_params.set_in_prog;
        break;
      case SYS_INFO_PARAM_SYSFW_VER:
        pal_get_sysfw_ver(req->payload_id, g_sys_info_params.sysfw_ver);
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
      case SYS_INFO_PARAM_BIOS_CURRENT_BOOT_LIST:
        if(pal_get_bios_current_boot_list(req->payload_id, g_sys_info_params.bios_current_boot_list, res_len))
        {
          res->cc = CC_UNSPECIFIED_ERROR;
          break;
        }
        memcpy(data, g_sys_info_params.bios_current_boot_list, *res_len);
        data += *res_len;
        break;
      case SYS_INFO_PARAM_BIOS_FIXED_BOOT_DEVICE:
        if(pal_get_bios_fixed_boot_device(req->payload_id, g_sys_info_params.bios_fixed_boot_device))
        {
          res->cc = CC_UNSPECIFIED_ERROR;
          break;
        }
        memcpy(data, g_sys_info_params.bios_fixed_boot_device, SIZE_BIOS_FIXED_BOOT_DEVICE);
        data += SIZE_BIOS_FIXED_BOOT_DEVICE;
        break;
      case SYS_INFO_PARAM_BIOS_RESTORES_DEFAULT_SETTING:
        if(pal_get_bios_restores_default_setting(req->payload_id, g_sys_info_params.bios_restores_default_setting))
        {
          res->cc = CC_UNSPECIFIED_ERROR;
          break;
        }
        memcpy(data, g_sys_info_params.bios_restores_default_setting, SIZE_BIOS_RESTORES_DEFAULT_SETTING);
        data += SIZE_BIOS_RESTORES_DEFAULT_SETTING;
        break;
      case SYS_INFO_PARAM_LAST_BOOT_TIME:
        if(pal_get_last_boot_time(req->payload_id, g_sys_info_params.last_boot_time))
        {
          res->cc = CC_UNSPECIFIED_ERROR;
          break;
        }
        memcpy(data, g_sys_info_params.last_boot_time, SIZE_LAST_BOOT_TIME);
        data += SIZE_LAST_BOOT_TIME;
        break;
      default:
        res->cc = CC_INVALID_PARAM;
        break;
    }
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
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char cmd = req->cmd;

  pthread_mutex_lock(&m_app);
  switch (cmd)
  {
    case CMD_APP_GET_DEVICE_ID:
      app_get_device_id (request, req_len, response, res_len);
      break;
    case CMD_APP_COLD_RESET:
      app_cold_reset (request, req_len, response, res_len);
      break;
    case CMD_APP_GET_SELFTEST_RESULTS:
      app_get_selftest_results (request, req_len, response, res_len);
      break;
    case CMD_APP_MANUFACTURING_TEST_ON:
      app_manufacturing_test_on (request, req_len, response, res_len);
      break;
    case CMD_APP_GET_DEVICE_GUID:
      app_get_device_guid (request, req_len, response, res_len);
      break;
    case CMD_APP_GET_SYSTEM_GUID:
      app_get_device_sys_guid (request, req_len, response, res_len);
      break;
    case CMD_APP_RESET_WDT:
      app_reset_watchdog_timer (request, req_len, response, res_len);
      break;
    case CMD_APP_SET_WDT:
      app_set_watchdog_timer (request, req_len, response, res_len);
      break;
    case CMD_APP_GET_WDT:
      app_get_watchdog_timer (request, req_len, response, res_len);
      break;
    case CMD_APP_SET_GLOBAL_ENABLES:
      app_set_global_enables (request, req_len, response, res_len);
      break;
    case CMD_APP_GET_GLOBAL_ENABLES:
      app_get_global_enables (request, req_len, response, res_len);
      break;
    case CMD_APP_SET_SYS_INFO_PARAMS:
      app_set_sys_info_params (request, req_len, response, res_len);
      break;
    case CMD_APP_CLEAR_MESSAGE_FLAGS:
      app_clear_message_flags (request, req_len, response, res_len);
      break;
    case CMD_APP_GET_SYS_INFO_PARAMS:
      app_get_sys_info_params (request,  req_len, response, res_len);
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
storage_get_fruid_info(unsigned char *request, unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  int size = plat_fruid_size(req->payload_id);

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
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  int offset = req->data[1] + (req->data[2] << 8);
  int count = req->data[3];

  int ret = plat_fruid_data(req->payload_id, offset, count, &(res->data[1]));
  if (ret) {
    res->cc = CC_UNSPECIFIED_ERROR;
  } else {
    res->cc = CC_SUCCESS;
    *data++ = count;
    data += count;
  }

  if (res->cc == CC_SUCCESS) {
    *(unsigned short*)res_len = data - &res->data[0];
  }
  return;
}

static void
storage_get_sdr_info (unsigned char *response, unsigned char *res_len)
{
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  int num_entries;    // number of sdr records
  int free_space;   // free space in SDR device in bytes
  time_stamp_t ts_recent_add; // Recent Addition Timestamp
  time_stamp_t ts_recent_erase; // Recent Erasure Timestamp

  // Use platform APIs to get SDR information
  num_entries = sdr_num_entries ();
  free_space = sdr_free_space ();
  sdr_ts_recent_add (&ts_recent_add);
  sdr_ts_recent_erase (&ts_recent_erase);

  res->cc = CC_SUCCESS;

  *data++ = IPMI_SDR_VERSION; // SDR version
  *data++ = num_entries & 0xFF; // number of sdr entries
  *data++ = (num_entries >> 8) & 0xFF;
  *data++ = free_space & 0xFF;  // Free SDR Space
  *data++ = (free_space >> 8) & 0xFF;

  memcpy(data, ts_recent_add.ts, SIZE_TIME_STAMP);
  data += SIZE_TIME_STAMP;

  memcpy(data, ts_recent_erase.ts, SIZE_TIME_STAMP);
  data += SIZE_TIME_STAMP;

  *data++ = 0x02;   // Operations supported

  *res_len = data - &res->data[0];

  return;
}

static void
storage_rsv_sdr (unsigned char *request, unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  int rsv_id;     // SDR reservation ID

  // Use platform APIs to get a SDR reservation ID
  rsv_id = sdr_rsv_id(req->payload_id);
  if (rsv_id < 0)
  {
      res->cc = CC_UNSPECIFIED_ERROR;
      return;
  }

  res->cc = CC_SUCCESS;
  *data++ = rsv_id & 0xFF;  // Reservation ID
  *data++ = (rsv_id >> 8) & 0XFF;

  *res_len = data - &res->data[0];

  return;
}

static void
storage_get_sdr (unsigned char *request, unsigned char *response,
     unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  int read_rec_id;    //record ID to be read
  int next_rec_id;    //record ID for the next entry
  int rsv_id;     // Reservation ID for the request
  int rec_offset;   // Read offset into the record
  int rec_bytes;    // Number of bytes to be read
  sdr_rec_t entry;    // SDR record entry
  int ret;

  rsv_id = (req->data[1] << 8) | req->data[0];
  read_rec_id = (req->data[3] << 8) | req->data[2];
  rec_offset = req->data[4];
  rec_bytes = req->data[5];

  // Use platform API to read the record Id and get next ID
  ret = sdr_get_entry (req->payload_id, rsv_id, read_rec_id, &entry, &next_rec_id);
  if (ret)
  {
      res->cc = CC_UNSPECIFIED_ERROR;
      return;
  }

  res->cc = CC_SUCCESS;
  *data++ = next_rec_id & 0xFF; // next record ID
  *data++ = (next_rec_id >> 8) & 0xFF;

  memcpy (data, &entry.rec[rec_offset], rec_bytes);
  data += rec_bytes;

  *res_len = data - &res->data[0];

  return;
}

static void
storage_get_sel_info (unsigned char *request, unsigned char *response,
                      unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  int num_entries;    // number of log entries
  int free_space;   // free space in SEL device in bytes
  time_stamp_t ts_recent_add; // Recent Addition Timestamp
  time_stamp_t ts_recent_erase; // Recent Erasure Timestamp

  // Use platform APIs to get SEL information
  num_entries = sel_num_entries (req->payload_id);
  free_space = sel_free_space (req->payload_id);
  sel_ts_recent_add (req->payload_id, &ts_recent_add);
  sel_ts_recent_erase (req->payload_id, &ts_recent_erase);

  res->cc = CC_SUCCESS;

  *data++ = IPMI_SEL_VERSION; // SEL version
  *data++ = num_entries & 0xFF; // number of log entries
  *data++ = (num_entries >> 8) & 0xFF;
  *data++ = free_space & 0xFF;  // Free SEL Space
  *data++ = (free_space >> 8) & 0xFF;

  memcpy(data, ts_recent_add.ts, SIZE_TIME_STAMP);
  data += SIZE_TIME_STAMP;

  memcpy(data, ts_recent_erase.ts, SIZE_TIME_STAMP);
  data += SIZE_TIME_STAMP;

  *data++ = 0x02;   // Operations supported

  *res_len = data - &res->data[0];

  return;
}

static void
storage_rsv_sel (unsigned char * request, unsigned char *response,
                  unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  int rsv_id;     // SEL reservation ID

  // Use platform APIs to get a SEL reservation ID
  rsv_id = sel_rsv_id (req->payload_id);
  if (rsv_id < 0)
  {
      res->cc = CC_SEL_ERASE_PROG;
      return;
  }

  res->cc = CC_SUCCESS;
  *data++ = rsv_id & 0xFF;  // Reservation ID
  *data++ = (rsv_id >> 8) & 0XFF;

  *res_len = data - &res->data[0];

  return;
}

static void
storage_get_sel (unsigned char *request, unsigned char *response,
     unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  int read_rec_id;    //record ID to be read
  int next_rec_id;    //record ID for the next msg
  sel_msg_t entry;    // SEL log entry
  int ret;
  unsigned char offset = req->data[4];
  unsigned char len = req->data[5];

  if (len == 0xFF) {  // FFh means read entire record
    offset = 0;
    len = SIZE_SEL_REC;
  } else if ((offset >= SIZE_SEL_REC) || (len > SIZE_SEL_REC) || ((offset+len) > SIZE_SEL_REC)) {
    res->cc = CC_PARAM_OUT_OF_RANGE;
    return;
  }

  read_rec_id = (req->data[3] << 8) | req->data[2];

  // Use platform API to read the record Id and get next ID
  ret = sel_get_entry (req->payload_id, read_rec_id, &entry, &next_rec_id);
  if (ret)
  {
    res->cc = CC_UNSPECIFIED_ERROR;
    return;
  }

  res->cc = CC_SUCCESS;
  *data++ = next_rec_id & 0xFF; // next record ID
  *data++ = (next_rec_id >> 8) & 0xFF;

  memcpy(data, &entry.msg[offset], len);
  data += len;

  *res_len = data - &res->data[0];

  return;
}

static void
storage_add_sel (unsigned char *request, unsigned char *response,
     unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];


  int record_id;    // Record ID for added entry
  int ret;

  sel_msg_t entry;

  memcpy(entry.msg, req->data, SIZE_SEL_REC);

  // Use platform APIs to add the new SEL entry
  ret = sel_add_entry (req->payload_id, &entry, &record_id);
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
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
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
    ret = sel_erase (req->payload_id, rsv_id);
  }
  else if (req->data[5] == IPMI_SEL_ERASE_STAT)
  {
    ret = sel_erase_status (req->payload_id, rsv_id, &status);
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
storage_get_sel_time (unsigned char *response, unsigned char *res_len)
{
  ipmi_res_t *res = (ipmi_res_t *) response;

  res->cc = CC_SUCCESS;

  time_stamp_fill(res->data);

  *res_len = SIZE_TIME_STAMP;

  return;
}

static void
storage_get_sel_utc (unsigned char *response, unsigned char *res_len)
{
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];

  res->cc = CC_SUCCESS;

  // TODO: For now, the SEL time stamp is based on UTC time,
  // so return 0x0000 as offset. Might need to change once
  // supporting zones in SEL time stamps
  *data++ = 0x00;
  *data++ = 0x00;

  *res_len = data - &res->data[0];
}

static void
ipmi_handle_storage (unsigned char *request, unsigned char req_len,
         unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char cmd = req->cmd;

  res->cc = CC_SUCCESS;
  *res_len = 0;

  pthread_mutex_lock(&m_storage);
  switch (cmd)
  {
    case CMD_STORAGE_GET_FRUID_INFO:
      storage_get_fruid_info (request, response, res_len);
      break;
    case CMD_STORAGE_READ_FRUID_DATA:
      storage_get_fruid_data (request, response, res_len);
      break;
    case CMD_STORAGE_GET_SEL_INFO:
      storage_get_sel_info (request, response, res_len);
      break;
    case CMD_STORAGE_RSV_SEL:
      storage_rsv_sel (request, response, res_len);
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
#if 0 // To avoid BIOS using this command to update RTC
      // TBD: Respond only if BMC's time has synced with NTP
    case CMD_STORAGE_GET_SEL_TIME:
      storage_get_sel_time (response, res_len);
      break;
#endif
    case CMD_STORAGE_GET_SEL_UTC:
      storage_get_sel_utc (response, res_len);
      break;
    case CMD_STORAGE_GET_SDR_INFO:
      storage_get_sdr_info (response, res_len);
      break;
    case CMD_STORAGE_RSV_SDR:
      storage_rsv_sdr (request, response, res_len);
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
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
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
    case LAN_PARAM_IP6_ADDR:
      memcpy(g_lan_config.ip6_addr, &req->data[2], SIZE_IP6_ADDR);
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

  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  unsigned char param = req->data[1];

  // Fill the response with default values
  res->cc = CC_SUCCESS;
  *data++ = 0x11;   // Parameter revision

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
      plat_lan_init(&g_lan_config);
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
    case LAN_PARAM_IP6_ADDR:
      plat_lan_init(&g_lan_config);
      memcpy(data, g_lan_config.ip6_addr, SIZE_IP6_ADDR);
      data += SIZE_IP6_ADDR;
      break;
   case LAN_PARAM_IP6_DYNAMIC_ADDR:
      plat_lan_init(&g_lan_config);
      data[0] = 0; // Selector
      data[1] = 0x02; // DHCPv6
      memcpy(&data[2], g_lan_config.ip6_addr, SIZE_IP6_ADDR);
      data[18] = g_lan_config.ip6_prefix;
      data[19] = 0x00; // Active
      data += SIZE_IP6_ADDR + 4;
      break;
    case LAN_PARAM_ADDR_ENABLES:
      data[0] = 0x02; // Enable both IPv4 and IPv6
      data ++;
      break;
    default:
      res->cc = CC_INVALID_PARAM;
      break;
  }

  if (res->cc == CC_SUCCESS) {
    *res_len = data - &res->data[0];
  }
}

// Get SoL Configuration (IPMI/Section 26.3)
static void
transport_get_sol_config (unsigned char *request, unsigned char *response,
        unsigned char *res_len)
{

  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char *data = &res->data[0];
  unsigned char param = req->data[1];

  // Fill the response with default values
  res->cc = CC_SUCCESS;
  *data++ = 0x01;   // Parameter revision

  switch (param)
  {
    case SOL_PARAM_SET_IN_PROG:
      *data++ = 0x00;
      break;
    case SOL_PARAM_SOL_ENABLE:
      *data++ = 0x01;
      break;
    case SOL_PARAM_SOL_AUTH:
      *data++ = 0x02;
      break;
    case SOL_PARAM_SOL_THRESHOLD:
      *data++ = 0x00;
      *data++ = 0x01;
      break;
    case SOL_PARAM_SOL_RETRY:
      *data++ = 0x00;
      *data++ = 0x00;
      break;
    case SOL_PARAM_SOL_BITRATE:
    case SOL_PARAM_SOL_NV_BITRATE:
      *data++ = 0x09;
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
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
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
    case CMD_TRANSPORT_GET_SOL_CONFIG:
      transport_get_sol_config (request, response, res_len);
      break;
    default:
      res->cc = CC_INVALID_CMD;
      break;
  }
  pthread_mutex_unlock(&m_transport);
}

/*
 * Function(s) to handle IPMI messages with NetFn: DCMI
 */
static void
ipmi_handle_dcmi(unsigned char *request, unsigned char req_len,
     unsigned char *response, unsigned char *res_len)
{
  int ret;
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  // If there is no command to process return
  if (req->cmd == 0x0) {
    res->cc = CC_UNSPECIFIED_ERROR;
    *res_len = 0;
    return;
  }

  // Since DCMI handling is specific to platform, call PAL to process
  ret = pal_handle_dcmi(req->payload_id, &request[1], req_len-1, res->data, res_len);
  if (ret < 0) {
    res->cc = CC_UNSPECIFIED_ERROR;
    *res_len = 0;
    return;
  }

  res->cc = CC_SUCCESS;
}

/*
 * Function(s) to handle IPMI messages with NetFn: OEM
 */
static void
oem_set_proc_info (unsigned char *request, unsigned char *response,
       unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
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
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
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

/*
 * Function(s) to handle IPMI messages with NetFn: OEM 0x36
 */
static void
oem_q_set_proc_info (unsigned char *request, unsigned char req_len, unsigned char *response,
       unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;


  res->cc = CC_SUCCESS;
  *res_len = 0;
}

static void
oem_q_get_proc_info (unsigned char *request, unsigned char req_len, unsigned char *response,
       unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  *res_len = 0 ;
  res->cc = CC_SUCCESS;
}

static void
oem_q_set_dimm_info (unsigned char *request, unsigned char req_len, unsigned char *response,
       unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  res->cc = CC_SUCCESS;
  *res_len = 0;
}

static void
oem_q_get_dimm_info (unsigned char *request, unsigned char req_len, unsigned char *response,
       unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  *res_len = 0;
  res->cc = CC_SUCCESS;
}

static void
oem_q_set_drive_info(unsigned char *request, unsigned char req_len, unsigned char *response,
       unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  res->cc = CC_SUCCESS;
  *res_len = 0;
}

static void
oem_q_get_drive_info(unsigned char *request, unsigned char req_len, unsigned char *response,
       unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  *res_len = 0 ;
  res->cc = CC_SUCCESS;
}

static void
oem_set_boot_order(unsigned char *request, unsigned char req_len,
                   unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  int RetVal;
  int ret;
  static pthread_t bios_timer_tid[MAX_NODES];

  if ( IsTimerStart[req->payload_id - 1] )
  {
#ifdef DEBUG
    syslog(LOG_WARNING, "[%s] Close the previous thread\n", __func__);
#endif
    pthread_cancel(bios_timer_tid[req->payload_id - 1]);
  }

  /*Create timer thread*/
  RetVal = pthread_create( &bios_timer_tid[req->payload_id - 1], NULL, clear_bios_data_timer, &req->payload_id );

  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Create BIOS timer thread failed!\n", __func__);

    res->cc = CC_NODE_BUSY;
    *res_len = 0;
    return;
  }

  IsTimerStart[req->payload_id - 1] = true;

  ret = pal_set_boot_order(req->payload_id, req->data, res->data, res_len);

  if(ret == 0)
  {
	res->cc = CC_SUCCESS;
  }
  else
  {
	res->cc = CC_INVALID_PARAM;
  }
}

static void
oem_get_boot_order(unsigned char *request, unsigned char req_len,
                   unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  int RetVal;

  RetVal = pal_get_boot_order(req->payload_id, req->data, res->data, res_len);

#ifdef DEBUG
  syslog(LOG_WARNING, "[%s] Get: %x %x %x %x %x %x\n", __func__, res->data[0], res->data[1], res->data[2], res->data[3], res->data[4], res->data[5]);
#endif
  if(RetVal == 0)
  {
	res->cc = CC_SUCCESS;
  }
  else
  {
	res->cc = CC_INVALID_PARAM;
  }
}

static void
oem_set_ppr (unsigned char *request, unsigned char req_len,
                   unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  int selector = req->data[0],  i =1 , para, size = 0, offset;
  char temp[20]= {0}, file_path[45];
  unsigned char data[350];
  FILE *fp;

  if (access("/mnt/data/ppr/", F_OK) == -1)
     mkdir("/mnt/data/ppr/", 0777);

  *res_len = 0;
  res->cc = CC_SUCCESS;
  switch(selector) {
    case 1:
      para =  req->data[1] & 0x7f;
      if( para != soft_ppr && para != hard_ppr  && para != test_mode && para != enable_ppr) {
        res->cc = CC_INVALID_DATA_FIELD;
        return;
      }
      fp = fopen("/mnt/data/ppr/ppr_row_count", "r");
      if (!fp) {
        res->cc = CC_NOT_SUPP_IN_CURR_STATE;
        return;
      }
      fread(res->data, 1, 1, fp);
      if(res->data[0] == 0) {
        fclose(fp);
        res->cc = CC_NOT_SUPP_IN_CURR_STATE;
        return;
      }
      fclose(fp);
      sprintf(temp, "%c", req->data[1]);
      fp = fopen("/mnt/data/ppr/ppr_action", "w");
      if(fp != NULL)
        fwrite(temp, sizeof(char), 1, fp);
      break;
    case 2:
      if(req->data[1]>100) {
        res->cc = CC_PARAM_OUT_OF_RANGE;
        return;
      }
      else
      {
        sprintf(temp, "%c", req->data[1]);
        fp = fopen("/mnt/data/ppr/ppr_row_count", "w");
        if(fp != NULL)
          fwrite(temp, sizeof(char), 1, fp);
      }
      break;
    case 3:
      if(req->data[1] < 0 || req->data[1] >100) {
        res->cc = CC_PARAM_OUT_OF_RANGE;
        return;
      }
      if(req_len != 12) {
        res->cc = CC_INVALID_LENGTH;
        return;
      }
      fp = fopen("/mnt/data/ppr/ppr_row_addr", "a+");
      fseek(fp, 0, SEEK_END);
      size = ftell(fp);
      fseek(fp, 0, SEEK_SET);
      if(size >= 50 * 8) {
        res->cc = CC_PARAM_OUT_OF_RANGE;
        fclose(fp);
        return;
      }
      offset = 0;
      if(size != 0) {
        while(fread(temp, 1, 8, fp)) {
          offset = ftell(fp);
           if(temp[0] == req->data[1]) {
           fclose(fp);
           fp = fopen("/mnt/data/ppr/ppr_row_addr", "w+");
             offset =  ftell(fp) - 8;
             if (offset < 0)
               offset = 0;
             break;
           }
        }
      }
      fseek(fp, offset , SEEK_SET);
      i = 1;
      while( i <= req_len - 4) {
        sprintf(temp, "%c", req->data[i]);
        fwrite(temp, sizeof(char), 1, fp);
        i++;
      }
    break;
    case 4:
      if(req->data[1] < 0 || req->data[1] >100) {
        res->cc = CC_PARAM_OUT_OF_RANGE;
        return;
      }
      if(req_len != 21) {
        res->cc = CC_INVALID_LENGTH;
        return;
      }
      fp = fopen("/mnt/data/ppr/ppr_history_data", "a+");
      fseek(fp, 0, SEEK_END);
      size = ftell(fp);
      fseek(fp, 0, SEEK_SET);
      if(size >= 50 * 17) {
        res->cc = CC_PARAM_OUT_OF_RANGE;
        fclose(fp);
        return;
      }
      offset = 0;
      if(size != 0) {
        while(fread(temp, 1, 17, fp)) {
          offset = ftell(fp);
          if(temp[0] == req->data[1]) {
            fclose(fp);
            fp = fopen("/mnt/data/ppr/ppr_history_data", "w+");
            offset =  ftell(fp) - 17;
            if (offset < 0)
              offset = 0;
            break;
          }
        }
      }
      fseek(fp, offset , SEEK_SET);
      i=1;
      while( i <= req_len - 4) {
        sprintf(temp, "%c", req->data[i]);
        fwrite(temp, sizeof(char), 1, fp);
        i++;
      }
      break;
    default:
      res->cc = CC_INVALID_PARAM;
      break;
    }
   fclose(fp);

}

static void
oem_get_ppr (unsigned char *request, unsigned char req_len,
                   unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  int selector = req->data[0], i = 0, para=0;;
  FILE *fp;
  char temp[20], kpath[20], file_path[45];
  int temp_value;

  sprintf(kpath, "%s", "/mnt/data/ppr");
  if (access(kpath, F_OK) == -1)
    mkdir(kpath, 0777);

  res->cc = CC_SUCCESS;
  switch(selector) {
    case 1:
      fp = fopen("/mnt/data/ppr/ppr_row_count", "r");
      if (!fp) {
        fp = fopen("/mnt/data/ppr/ppr_row_count", "w");
        sprintf(temp, "%c",0);
        fwrite(temp, sizeof(char), 1, fp);
      }
      else
        fread(res->data, 1, 1, fp);
      fclose(fp);
      fp = fopen("/mnt/data/ppr/ppr_action", "r");
      if (!fp) {
        fp = fopen("/mnt/data/ppr/ppr_action", "w");
        sprintf(temp, "%c",0);
        fwrite(temp, sizeof(char), 1, fp);
        fclose(fp);
        res->data[0] = 0;
        *res_len = 1;
        return;
      }
      if (res->data[0] != 0 ) {
        fread(res->data, 1, 1, fp);
        if((res->data[0] & 0x80) == 0 )
          res->data[0] = 0;
      }
      *res_len = 1;
      break;
    case 2:
      fp = fopen("/mnt/data/ppr/ppr_row_count", "r");
      if (!fp) {
        fp = fopen("/mnt/data/ppr/ppr_row_count", "w");
        sprintf(temp, "%c",0);
        fwrite(temp, sizeof(char), 1, fp);
        res->data[0] = 0;
      }
      else
        fread(res->data, 1, 1, fp);
      *res_len = 1;
      break;
    case 3:
      if(req->data[1] < 0 || req->data[1] >100) {
        res->cc = CC_PARAM_OUT_OF_RANGE;
        return;
      }

      fp = fopen("/mnt/data/ppr/ppr_row_addr", "r+");
      if (!fp) {
        res->cc = CC_PARAM_OUT_OF_RANGE;
        return;
      }
      while(fread(res->data, 1, 8, fp)) {
        if(res->data[0] == req->data[1]) {
          i = 1;
          break;
        }
      }
      if(i!=1)
        *res_len = 0;
      else
        *res_len = 8;
      break;
    case 4:
      if(req->data[1] < 0 || req->data[1] >100) {
        res->cc = CC_PARAM_OUT_OF_RANGE;
        return;
      }
      fp = fopen("/mnt/data/ppr/ppr_history_data", "r+");
      if (!fp) {
        res->cc = CC_PARAM_OUT_OF_RANGE;
        return;
      }
      while(fread(res->data, 1, 17, fp)) {
        if(res->data[0] == req->data[1]) {
          i = 1;
          break;
        }
      }
      if(i!=1)
        *res_len = 0;
      else
        *res_len = 17;
      break;
    default:
      res->cc = CC_INVALID_PARAM;
      break;
  }
  fclose(fp);
}

static void
oem_set_post_start (unsigned char *request, unsigned char *response,
                  unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  pal_set_post_start(req->payload_id, req->data, res->data, res_len);

  res->cc = CC_SUCCESS;
}

static void
oem_set_post_end (unsigned char *request, unsigned char *response,
                unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  pal_update_ts_sled();
  pal_set_post_end(req->payload_id, req->data, res->data, res_len);

  res->cc = CC_SUCCESS;
}

static void
oem_set_ppin_info(unsigned char *request, unsigned char req_len, 
		   unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  res->cc = pal_set_ppin_info(req->payload_id, req->data, req_len, res->data, res_len);

  return;
}

static void
oem_get_plat_info(unsigned char *request, unsigned char *response,
                  unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  int ret;
  uint8_t pres  = 0x00;
  uint8_t pinfo = 0x00;

  // Platform info:
  // Bit[7]: Not Present:0/Present:1  (from pal)
  // Bit[6]: Test Board:1, Non Test Board:0 (TODO from pal)
  // Bit[5-3] : SKU ID:000:Yosemite, 010:Triton
  // Bit[2:0] : Slot Index, 1 based
  ret = pal_is_fru_prsnt(req->payload_id, &pres);
  if (ret) {
    res->cc = CC_UNSPECIFIED_ERROR;
    *res_len = 0x00;
    return;
  }

  // Populate the presence bit[7]
  if (pres) {
    pinfo = 0x80;
  }

  // Get the SKU ID bit[5-3]
  pinfo |= (pal_get_plat_sku_id() << 3);

  // Populate the slot Index bit[2:0]
  pinfo |= req->payload_id;

  // Prepare response buffer
  res->cc = CC_SUCCESS;
  res->data[0] = pinfo;
  *res_len = 0x01;
}

static void
oem_get_poss_pcie_config(unsigned char *request, unsigned char *response,
                  unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  int ret;
  uint8_t pcie_conf = 0x00;

  ret = pal_get_poss_pcie_config(&pcie_conf);
  if (ret) {
    res->cc = CC_UNSPECIFIED_ERROR;
    *res_len = 0x00;
    return;
  }

  // Prepare response buffer
  res->cc = CC_SUCCESS;
  res->data[0] = pcie_conf;
  *res_len = 0x01;
}

static void
oem_get_board_id(unsigned char *request, unsigned char req_len, 
					unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  res->cc = pal_get_board_id(req->payload_id, req->data, req_len, res->data, res_len);

  return;
}

static void
oem_get_80port_record ( unsigned char *request, unsigned char req_len,
                        unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  int ret;

  res->cc = pal_get_80port_record (req->payload_id, req->data, req_len, res->data, res_len);

  return;
}

static void
oem_get_fw_info ( unsigned char *request, unsigned char req_len,
                  unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  int ret;

  ret = pal_get_fw_info(req->data[0], res->data, res_len);

  if (ret != 0) {
    res->cc = CC_UNSPECIFIED_ERROR;
    *res_len = 0x00;
    return;
  }

  // Prepare response buffer
  res->cc = CC_SUCCESS;
}

static void
ipmi_handle_oem (unsigned char *request, unsigned char req_len,
     unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
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
    case CMD_OEM_SET_BOOT_ORDER:
      oem_set_boot_order(request, req_len, response, res_len);
      break;
    case CMD_OEM_GET_BOOT_ORDER:
      oem_get_boot_order(request, req_len, response, res_len);
      break;
    case CMD_OEM_SET_PPR:
      oem_set_ppr (request, req_len, response, res_len);
      break;
    case CMD_OEM_GET_PPR:
      oem_get_ppr (request, req_len, response, res_len);
      break;
    case CMD_OEM_SET_POST_START:
      oem_set_post_start (request, response, res_len);
      break;
    case CMD_OEM_SET_POST_END:
      oem_set_post_end (request, response, res_len);
      break;
    case CMD_OEM_SET_PPIN_INFO:
      oem_set_ppin_info (request, req_len, response, res_len);
      break;  
    case CMD_OEM_GET_PLAT_INFO:
      oem_get_plat_info (request, response, res_len);
      break;
    case CMD_OEM_GET_PCIE_CONFIG:
      oem_get_poss_pcie_config (request, response, res_len);
      break;
    case CMD_OEM_GET_BOARD_ID:
      oem_get_board_id (request, req_len, response, res_len);
      break;
    case CMD_OEM_GET_80PORT_RECORD:
      oem_get_80port_record (request, req_len, response, res_len);
      break;
    case CMD_OEM_GET_FW_INFO:
      oem_get_fw_info (request, req_len, response, res_len);
      break;
    default:
      res->cc = CC_INVALID_CMD;
      break;
  }
  pthread_mutex_unlock(&m_oem);
}

static void
ipmi_handle_oem_q (unsigned char *request, unsigned char req_len,
     unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  unsigned char cmd = req->cmd;
  pthread_mutex_lock(&m_oem_q);
  switch (cmd)
  {
    case CMD_OEM_Q_SET_PROC_INFO:
      oem_q_set_proc_info (request, req_len, response, res_len);
      break;
    case CMD_OEM_Q_GET_PROC_INFO:
      oem_q_get_proc_info (request, req_len, response, res_len);
      break;
    case CMD_OEM_Q_SET_DIMM_INFO:
      oem_q_set_dimm_info (request, req_len, response, res_len);
      break;
    case CMD_OEM_Q_GET_DIMM_INFO:
      oem_q_get_dimm_info (request, req_len, response, res_len);
      break;
    case CMD_OEM_Q_SET_DRIVE_INFO:
      oem_q_set_drive_info (request, req_len, response, res_len);
      break;
    case CMD_OEM_Q_GET_DRIVE_INFO:
      oem_q_get_drive_info (request, req_len, response, res_len);
      break;
    default:
      res->cc = CC_INVALID_CMD;
      break;
  }
  pthread_mutex_unlock(&m_oem_q);
}

static void
oem_1s_handle_ipmb_kcs(unsigned char *request, unsigned char req_len,
       unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  // Need to extract bridged IPMI command and handle
  unsigned char req_buf[MAX_IPMI_MSG_SIZE] = {0};
  unsigned char res_buf[MAX_IPMI_MSG_SIZE] = {0};

  // Add the payload id from the bridged command
  req_buf[0] = req->payload_id;

  // Remove OEM IPMI Header (including 1 byte for interface type, 3 bytes for IANA ID)
  // The offset moves by one due to the payload ID
  memcpy(&req_buf[1], &request[BIC_INTF_HDR_SIZE], req_len - BIC_INTF_HDR_SIZE + 1);

  // Send the bridged KCS command along with the payload ID
  // The offset moves by one due to the payload ID
  ipmi_handle(req_buf, req_len - BIC_INTF_HDR_SIZE + 1, res_buf, res_len);

  // Copy the response back (1 byte interface type, 3 bytes for IANA ID)
  memcpy(&res->data[4], res_buf, *res_len);

  // Add the OEM command's response
  res->cc = CC_SUCCESS;
  memcpy(res->data, &req->data, SIZE_IANA_ID); // IANA ID
  res->data[3] = req->data[3]; // Bridge-IC interface
  *res_len += 4; // Interface type + IANA ID
}

static void
oem_1s_handle_ipmb_req(unsigned char *request, unsigned char req_len,
       unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  // handle based on Bridge-IC interface
  switch(req->data[3]) {
    case BIC_INTF_ME:
#ifdef DEBUG
      syslog(LOG_INFO, "oem_1s_handle_ipmb_req: Command received from ME for "
                  "payload#%d\n", req->payload_id);
#endif
      oem_1s_handle_ipmb_kcs(request, req_len, response, res_len);

      break;
    case BIC_INTF_SOL:
      // TODO: Need to call Optional SoL message handler
#ifdef DEBUG
      syslog(LOG_INFO, "oem_1s_handle_ipmb_req: Command received from SOL for "
                  "payload#%d\n", req->payload_id);
#endif
      memcpy(res->data, req->data, 4); //IANA ID + Interface type
      res->cc = CC_SUCCESS;
      *res_len = 4;
      break;
    case BIC_INTF_KCS:
    case BIC_INTF_KCS_SMM:
      oem_1s_handle_ipmb_kcs(request, req_len, response, res_len);
      break;
    default:
      // TODO: Need to add additonal interface handler, if supported
      syslog(LOG_WARNING, "oem_1s_handle_ipmb_req: Command received on intf#%d "
                 "for payload#%d", req->data[3], req->payload_id);
      memcpy(res->data, req->data, 4); //IANA ID + Interface type
      res->cc = CC_INVALID_PARAM;
      *res_len = 4;
      break;
  }
}

static void
ipmi_handle_oem_1s(unsigned char *request, unsigned char req_len,
     unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  int i;

  unsigned char cmd = req->cmd;

  pthread_mutex_lock(&m_oem_1s);
  switch (cmd)
  {
    case CMD_OEM_1S_MSG_IN:
      // Add unlock-lock mechanism whenever BIC send second NetFn 0x38 to BMC.
      // Avoid deadlock of the same NetFn.
      if (req->data[4] == (NETFN_OEM_1S_REQ << 2)) {
        pthread_mutex_unlock(&m_oem_1s);
      }
      oem_1s_handle_ipmb_req(request, req_len, response, res_len);
      if (req->data[4] == (NETFN_OEM_1S_REQ << 2)) {
        pthread_mutex_lock(&m_oem_1s);
      }
      break;
    case CMD_OEM_1S_INTR:
      syslog(LOG_INFO, "ipmi_handle_oem_1s: 1S server interrupt#%d received "
                "for payload#%d\n", req->data[3], req->payload_id);

      res->cc = CC_SUCCESS;
      memcpy(res->data, req->data, SIZE_IANA_ID); //IANA ID
      *res_len = 3;
      break;
    case CMD_OEM_1S_POST_BUF:
      // Skip the first 3 bytes of IANA ID and one byte of length field
      for (i = SIZE_IANA_ID+1; i <= req->data[SIZE_IANA_ID]+SIZE_IANA_ID; i++) {
        pal_post_handle(req->payload_id, req->data[i]);
      }

      res->cc = CC_SUCCESS;
      memcpy(res->data, req->data, SIZE_IANA_ID); //IANA ID
      *res_len = 3;
      break;
    case CMD_OEM_1S_PLAT_DISC:
      syslog(LOG_INFO, "ipmi_handle_oem_1s: Platform Discovery received for "
                "payload#%d\n", req->payload_id);
      res->cc = CC_SUCCESS;
      memcpy(res->data, req->data, SIZE_IANA_ID); //IANA ID
      *res_len = 3;
      break;
    case CMD_OEM_1S_BIC_RESET:
      syslog(LOG_INFO, "ipmi_handle_oem_1s: BIC Reset received "
                "for payload#%d\n", req->payload_id);

      if (req->data[3] == 0x0) {
         syslog(LOG_WARNING, "Cold Reset by Firmware Update\n");
         res->cc = CC_SUCCESS;
      } else if (req->data[3] == 0x01) {
         syslog(LOG_WARNING, "WDT Reset\n");
         res->cc = CC_SUCCESS;
      } else {
         syslog(LOG_WARNING, "Error\n");
         res->cc = CC_INVALID_PARAM;
      }

      memcpy(res->data, req->data, SIZE_IANA_ID); //IANA ID
      *res_len = 3;
      break;
    case CMD_OEM_1S_BIC_UPDATE_MODE:
#ifdef DEBUG
      syslog(LOG_INFO, "ipmi_handle_oem_1s: BIC Update Mode received "
                "for payload#%d\n", req->payload_id);
#endif
      if (req->data[3] == 0x1) {
         syslog(LOG_INFO, "BIC Mode: Normal\n");
         res->cc = CC_SUCCESS;
      } else if (req->data[3] == 0x0F) {
         syslog(LOG_INFO, "BIC Mode: Update\n");
         res->cc = CC_SUCCESS;
      } else {
         syslog(LOG_WARNING, "Error\n");
         res->cc = CC_INVALID_PARAM;
      }

      pal_inform_bic_mode(req->payload_id, req->data[3]);

      memcpy(res->data, req->data, SIZE_IANA_ID); //IANA ID
      *res_len = 3;
      break;
    default:
      res->cc = CC_INVALID_CMD;
      memcpy(res->data, req->data, SIZE_IANA_ID); //IANA ID
      *res_len = 3;
      break;
  }
  pthread_mutex_unlock(&m_oem_1s);
}

static void
oem_usb_dbg_get_frame_info(unsigned char *request, unsigned char req_len,
       unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  uint8_t num_frames;
  int ret;

  ret = plat_udbg_get_frame_info(&num_frames);
  if (ret) {
    memcpy(res->data, req->data, 3); // IANA ID
    res->cc = CC_UNSPECIFIED_ERROR;
    *res_len = SIZE_IANA_ID;
    return;
  }

  memcpy(res->data, req->data, SIZE_IANA_ID); // IANA ID
  res->data[3] = num_frames;
  res->cc = CC_SUCCESS;
  *res_len = SIZE_IANA_ID + 1;
}

static void
oem_usb_dbg_get_updated_frames(unsigned char *request, unsigned char req_len,
       unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  uint8_t num_updates;
  uint8_t updated_frames[256];
  int ret;

  ret = plat_udbg_get_updated_frames(&num_updates, updated_frames);
  if (ret) {
    memcpy(res->data, req->data, SIZE_IANA_ID); // IANA ID
    res->cc = CC_UNSPECIFIED_ERROR;
    *res_len = SIZE_IANA_ID;
    return;
  }

  memcpy(res->data, req->data, SIZE_IANA_ID); // IANA ID
  res->data[3] = num_updates;
  memcpy(&res->data[4], updated_frames, num_updates);
  res->cc = CC_SUCCESS;
  *res_len = SIZE_IANA_ID + 1 + num_updates;
}

static void
oem_usb_dbg_get_post_desc(unsigned char *request, unsigned char req_len,
       unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  uint8_t index;
  uint8_t next;
  uint8_t end;
  uint8_t phase;
  uint8_t count;
  uint8_t desc[256] = {0};
  int ret;

  index = req->data[3];
  phase = req->data[4];

  ret = plat_udbg_get_post_desc(index, &next, phase, &end, &count, desc);
  if (ret) {
    memcpy(res->data, req->data, SIZE_IANA_ID); // IANA ID
    res->cc = CC_UNSPECIFIED_ERROR;
    *res_len = SIZE_IANA_ID;
    return;
  }

  memcpy(res->data, req->data, SIZE_IANA_ID); // IANA ID
  res->data[3] = index;
  res->data[4] = next;
  res->data[5] = phase;
  res->data[6] = end;
  res->data[7] = count;
  memcpy(&res->data[8], desc, count);
  res->cc = CC_SUCCESS;
  *res_len = SIZE_IANA_ID + 5 + count;
}

static void
oem_usb_dbg_get_gpio_desc(unsigned char *request, unsigned char req_len,
       unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  uint8_t index;
  uint8_t next;
  uint8_t level;
  uint8_t in_out;
  uint8_t count;
  uint8_t desc[256];
  int ret;

  index = req->data[3];

  ret = plat_udbg_get_gpio_desc(index, &next, &level, &in_out, &count, desc);
  if (ret) {
    memcpy(res->data, req->data, SIZE_IANA_ID); // IANA ID
    res->cc = CC_UNSPECIFIED_ERROR;
    *res_len = SIZE_IANA_ID;
    return;
  }

  memcpy(res->data, req->data, SIZE_IANA_ID); // IANA ID
  res->data[3] = index;
  res->data[4] = next;
  res->data[5] = level;
  res->data[6] = in_out;
  res->data[7] = count;
  memcpy(&res->data[8], desc, count);
  res->cc = CC_SUCCESS;
  *res_len = SIZE_IANA_ID + 5 + count;
}

static void
oem_usb_dbg_get_frame_data(unsigned char *request, unsigned char req_len,
       unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;

  uint8_t frame;
  uint8_t page;
  uint8_t next;
  uint8_t count;
  uint8_t data[256] = {0};
  int ret;

  frame = req->data[3];
  page = req->data[4];

  ret = plat_udbg_get_frame_data(frame, page, &next, &count, data);
  if (ret) {
    memcpy(res->data, req->data, SIZE_IANA_ID); // IANA ID
    res->cc = CC_UNSPECIFIED_ERROR;
    *res_len = SIZE_IANA_ID;
    return;
  }

  memcpy(res->data, req->data, SIZE_IANA_ID); // IANA ID
  res->data[3] = frame;
  res->data[4] = page;
  res->data[5] = next;
  res->data[6] = count;
  memcpy(&res->data[7], data, count);
  res->cc = CC_SUCCESS;
  *res_len = SIZE_IANA_ID + 4 + count;
}

static void
ipmi_handle_oem_usb_dbg(unsigned char *request, unsigned char req_len,
     unsigned char *response, unsigned char *res_len)
{
  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  int i;

  unsigned char cmd = req->cmd;

  pthread_mutex_lock(&m_oem_usb_dbg);
  switch (cmd)
  {
    case CMD_OEM_USB_DBG_GET_FRAME_INFO:
      oem_usb_dbg_get_frame_info(request, req_len, response, res_len);
      break;
    case CMD_OEM_USB_DBG_GET_UPDATED_FRAMES:
      oem_usb_dbg_get_updated_frames(request, req_len, response, res_len);
      break;
    case CMD_OEM_USB_DBG_GET_POST_DESC:
      oem_usb_dbg_get_post_desc(request, req_len, response, res_len);
      break;
    case CMD_OEM_USB_DBG_GET_GPIO_DESC:
      oem_usb_dbg_get_gpio_desc(request, req_len, response, res_len);
      break;
    case CMD_OEM_USB_DBG_GET_FRAME_DATA:
      oem_usb_dbg_get_frame_data(request, req_len, response, res_len);
      break;
    default:
      res->cc = CC_INVALID_CMD;
      memcpy(res->data, req->data, SIZE_IANA_ID); //IANA ID
      *res_len = 3;
      break;
  }
  pthread_mutex_unlock(&m_oem_usb_dbg);
}

/*
 * Function to handle all IPMI messages
 */
static void
ipmi_handle (unsigned char *request, unsigned char req_len,
       unsigned char *response, unsigned char *res_len)
{

  ipmi_mn_req_t *req = (ipmi_mn_req_t *) request;
  ipmi_res_t *res = (ipmi_res_t *) response;
  unsigned char netfn;
  netfn = req->netfn_lun >> 2;

  // Provide default values in the response message
  res->cmd = req->cmd;
  res->cc = 0xFF;   // Unspecified completion code
  printf("ipmi_handle netfn %x cmd %x len %d\n", netfn, req->cmd, req_len);
  *(unsigned short*)res_len = 0;

  switch (netfn)
  {
    case NETFN_CHASSIS_REQ:
      res->netfn_lun = NETFN_CHASSIS_RES << 2;
      ipmi_handle_chassis (request, req_len, response, res_len);
      break;
    case NETFN_SENSOR_REQ:
      res->netfn_lun = NETFN_SENSOR_RES << 2;
      ipmi_handle_sensor (request, req_len, response, res_len);
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
    case NETFN_DCMI_REQ:
      res->netfn_lun = NETFN_DCMI_RES << 2;
      ipmi_handle_dcmi(request, req_len, response, res_len);
      break;
    case NETFN_OEM_REQ:
      res->netfn_lun = NETFN_OEM_RES << 2;
      ipmi_handle_oem (request, req_len, response, res_len);
      break;
    case NETFN_OEM_Q_REQ:
      res->netfn_lun = NETFN_OEM_Q_RES << 2;
      ipmi_handle_oem_q (request, req_len, response, res_len);
      break;
    case NETFN_OEM_1S_REQ:
      res->netfn_lun = NETFN_OEM_1S_RES << 2;
      ipmi_handle_oem_1s(request, req_len, response, res_len);
      break;
    case NETFN_OEM_USB_DBG_REQ:
      res->netfn_lun = NETFN_OEM_USB_DBG_RES << 2;
      ipmi_handle_oem_usb_dbg(request, req_len, response, res_len);
      break;
    default:
      res->netfn_lun = (netfn + 1) << 2;
      break;
  }

  // This header includes NetFunction, Command, and Completion Code
  *(unsigned short*)res_len += IPMI_RESP_HDR_SIZE;

  return;
}

void
*conn_handler(void *socket_desc) {
  int *p_sock = (int*)socket_desc;
  int sock = *p_sock;
  int n;
  unsigned char req_buf[MAX_IPMI_MSG_SIZE];
  unsigned char res_buf[MAX_IPMI_MSG_SIZE];
  unsigned short res_len = 0;
  struct timeval tv;
  int rc = 0;

  // setup timeout for receving on socket
  tv.tv_sec = TIMEOUT_IPMI;
  tv.tv_usec = 0;

  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

  n = recv (sock, req_buf, sizeof(req_buf), 0);
  rc = errno;
  if (n <= 0) {
      syslog(LOG_WARNING, "ipmid: recv() failed with %d, errno: %d\n", n, rc);
      goto conn_cleanup;
  }

  ipmi_handle(req_buf, n, res_buf, (unsigned char*)&res_len);

  if (send (sock, res_buf, res_len, 0) < 0) {
    syslog(LOG_WARNING, "ipmid: send() failed\n");
  }

conn_cleanup:
  close(sock);
  free(p_sock);

  pthread_exit(NULL);
  return 0;
}

void *
wdt_timer (void *arg) {
  int ret;
  struct watchdog_data *wdt = (struct watchdog_data *)arg;
  uint8_t status;
  char timer_use[32];
  int action = 0;

  while (1) {
    usleep(100*1000);
    pthread_mutex_lock(&wdt->mutex);
    if (wdt->valid && wdt->run) {

      // Check if power off
      ret = pal_get_server_power(wdt->slot, &status);
      if ((ret >= 0) && (status == SERVER_POWER_OFF)) {
        wdt->run = 0;
        pthread_mutex_unlock(&wdt->mutex);
        continue;
      }

      // count down; counter 0 and run associated timer events occur immediately
      if (wdt->present_count_down)
        wdt->present_count_down--;

      // Pre-timeout no support
      /*
      if (wdt->present_count_down == wdt->pre_interval*10) {
      }
      */

      // Timeout
      if (wdt->present_count_down == 0) {
        wdt->expiration |= (1 << wdt->use);

        // Execute actin out of mutex
        action = wdt->action;

        if (wdt->no_log) {
          wdt->no_log = 0;
        }
        else {
          syslog(LOG_CRIT, "%s Watchdog %s",
            wdt_use_name[wdt->use & 0x7],
            wdt_action_name[action & 0x7]);
        }

        wdt->run = 0;
      } /* End of Timeout Action*/
    } /* End of Valid and Run */
    pthread_mutex_unlock(&wdt->mutex);

    // Execute actin out of mutex
    if (action) {
      switch (action) {
      case 1: // Hard Reset
        pal_set_server_power(wdt->slot, SERVER_POWER_RESET);
        break;
      case 2: // Power Down
        pal_set_server_power(wdt->slot, SERVER_POWER_OFF);
        break;
      case 3: // Power Cycle
        pal_set_server_power(wdt->slot, SERVER_POWER_CYCLE);
        break;
      case 0: // no action
      default:
        break;
      }
      action = 0;
    }

  } /* Forever while */

  pthread_exit(NULL);
}

int
main (void)
{
  int s, s2, t, fru, len;
  struct sockaddr_un local, remote;
  pthread_t tid;
  int *p_s2;
  int rc = 0;

  //daemon(1, 1);
  //openlog("ipmid", LOG_CONS, LOG_DAEMON);


  plat_fruid_init();
  plat_sensor_init();
  plat_lan_init(&g_lan_config);

  sdr_init();
  sel_init();

  pthread_mutex_init(&m_chassis, NULL);
  pthread_mutex_init(&m_sensor, NULL);
  pthread_mutex_init(&m_app, NULL);
  pthread_mutex_init(&m_storage, NULL);
  pthread_mutex_init(&m_transport, NULL);
  pthread_mutex_init(&m_oem, NULL);
  pthread_mutex_init(&m_oem_1s, NULL);
  pthread_mutex_init(&m_oem_usb_dbg, NULL);
  pthread_mutex_init(&m_oem_q, NULL);

  for (fru = 1; fru <= MAX_NUM_FRUS; fru++) {
    if (pal_is_slot_server(fru)) {
      struct watchdog_data *wdt_data = calloc(1, sizeof(struct watchdog_data));
      if (!wdt_data) {
        syslog(LOG_WARNING, "ipmid: allocation wdt info failed!\n");
	continue;
      }
      wdt_data->slot = fru;
      wdt_data->valid = 0;
      wdt_data->pre_interval = 1;
      pthread_mutex_init(&wdt_data->mutex, NULL);

      g_wdt[fru - 1] = wdt_data;
      pthread_create(&wdt_data->tid, NULL, wdt_timer, wdt_data);
      pthread_detach(wdt_data->tid);
    }
  }

  if ((s = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
  {
    syslog(LOG_WARNING, "ipmid: socket() failed\n");
    exit (1);
  }

  local.sun_family = AF_UNIX;
  strcpy (local.sun_path, SOCK_PATH_IPMI);
  unlink (local.sun_path);
  len = strlen (local.sun_path) + sizeof (local.sun_family);
  if (bind (s, (struct sockaddr *) &local, len) == -1)
  {
    syslog(LOG_WARNING, "ipmid: bind() failed\n");
    exit (1);
  }

  if (listen (s, 5) == -1)
  {
    syslog(LOG_WARNING, "ipmid: listen() failed\n");
    exit (1);
  }

  while(1) {
    int n;
    t = sizeof (remote);
    // TODO: seen accept() call fails and need further debug
    if ((s2 = accept (s, (struct sockaddr *) &remote, &t)) < 0) {
      rc = errno;
      syslog(LOG_WARNING, "ipmid: accept() failed with ret: %x, errno: %x\n", s2, rc);
      sleep(5);
      continue;
    }

    // Creating a worker thread to handle the request
    // TODO: Need to monitor the server performance with higher load and
    // see if we need to create pre-defined number of workers and schedule
    // the requests among them.
    p_s2 = malloc(sizeof(int));
    *p_s2 = s2;
    if (pthread_create(&tid, NULL, conn_handler, (void*) p_s2) < 0) {
        syslog(LOG_WARNING, "ipmid: pthread_create failed\n");
        close(s2);
        continue;
    }

    pthread_detach(tid);
  }

  close(s);

  pthread_mutex_destroy(&m_chassis);
  pthread_mutex_destroy(&m_sensor);
  pthread_mutex_destroy(&m_app);
  pthread_mutex_destroy(&m_storage);
  pthread_mutex_destroy(&m_transport);
  pthread_mutex_destroy(&m_oem);
  pthread_mutex_destroy(&m_oem_1s);
  pthread_mutex_destroy(&m_oem_usb_dbg);
  pthread_mutex_destroy(&m_oem_q);

  return 0;
}
