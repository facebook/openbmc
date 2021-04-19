/*
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#define _XOPEN_SOURCE
#define _GNU_SOURCE
#include "obmc-pal.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/reboot.h>
#include <sys/wait.h>
#include <openbmc/kv.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <math.h>

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"

//#define _STRINGIFY(bw) #bw
//#define STRINGIFY(bw) _STRINGIFY(bw)
#define MACHINE __MACHINE__

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof((_a)[0]))
#endif

// PAL Variable
size_t pal_pwm_cnt __attribute__((weak)) = 0;
size_t pal_tach_cnt __attribute__((weak)) = 0;
size_t pal_fan_opt_cnt __attribute__((weak)) = 0;
char pal_pwm_list[] __attribute__((weak)) = "";
char pal_tach_list[] __attribute__((weak)) = "";
char pal_fan_opt_list[] __attribute__((weak)) = "";

// PAL functions
int __attribute__((weak))
pal_is_bmc_por(void)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_bmc_reboot(int cmd) {
  if (cmd == 0) {
    return run_command("/sbin/reboot");
  }

  // flag for healthd logging reboot cause
  run_command("/etc/init.d/setup-reboot.sh > /dev/null 2>&1");
  return reboot(cmd);
}

int __attribute__((weak))
pal_set_last_boot_time(uint8_t slot, uint8_t *last_boot_time)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_get_last_boot_time(uint8_t slot, uint8_t *last_boot_time)
{
  return PAL_ENOTSUP;
}

void __attribute__((weak))
pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len)
{
  *res_len = 0;
  return;
}

int __attribute__((weak))
pal_chassis_control(uint8_t slot, uint8_t *req_data, uint8_t req_len)
{
  return PAL_ENOTSUP;
}

void __attribute__((weak))
pal_get_sys_intf_caps(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len)
{
  return;
}

static int
pal_lpc_snoop_read_legacy(uint8_t *buf, size_t max_len, size_t *len)
{
  FILE *fp = fopen("/sys/devices/platform/ast-snoop-dma.0/data_history", "r");
  uint8_t postcode;
  size_t i;

  if (fp == NULL) {
    syslog(LOG_WARNING, "pal_get_80port_record: Cannot open device");
    return PAL_ENOTREADY;
  }
  for (i=0; i < max_len; i++) {
    // %hhx: unsigned char*
    if (fscanf(fp, "%hhx%*s", &postcode) == 1) {
      buf[i] = postcode;
    } else {
      break;
    }
  }
  if (len)
    *len = i;
  fclose(fp);
  return 0;
}

static int pal_lpc_snoop_read(uint8_t *buf, size_t max_len, size_t *rlen)
{
  const char *dev_path = "/dev/aspeed-lpc-snoop0";
  const char *cache_key = "postcode";
  uint8_t cache[MAX_VALUE_LEN * 2];
  size_t len = 0, cache_len = 0;
  int fd;
  uint8_t postcode;

  if (kv_get(cache_key, (char *)cache, &len, 0)) {
    len = cache_len = 0;
  } else {
    cache_len = len;
  }

  /* Open and read as much as we can store. Our in-mem
   * cache is twice as the file-backed path. */
  fd = open(dev_path, O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    return PAL_ENOTREADY;
  }
  while (len < sizeof(cache) &&
      read(fd, &postcode, 1) == 1) {
    cache[len++] = postcode;
  }
  close(fd);
  /* Update the file-backed cache only if something
   * changed since we read it */
  if (len > cache_len) {
    /* Since our in-mem cache can be twice of our file-backed
     * cache, ensure that we are storing the latest but also
     * limit the number to MAX_VALUE_LEN */
    if (len > MAX_VALUE_LEN) {
      memmove(cache, &cache[len - MAX_VALUE_LEN], MAX_VALUE_LEN);
      len = MAX_VALUE_LEN;
    }
    if (kv_set(cache_key, (char *)cache, len, 0)) {
      syslog(LOG_CRIT, "%zu postcodes dropped\n", len - cache_len);
    }
  }
  len = len > max_len ? max_len : len;
  memcpy(buf, cache, len);
  *rlen = len;
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_80port_record(uint8_t slot, uint8_t *buf, size_t max_len, size_t *len)
{
  static bool legacy = false, legacy_checked = false;

  if (buf == NULL || len == NULL || max_len == 0)
    return -1;

  if (!pal_is_slot_server(slot)) {
    syslog(LOG_WARNING, "pal_get_80port_record: slot %d is not supported", slot);
    return PAL_ENOTSUP;
  }

  if (legacy_checked == false) {
    if (access("/sys/devices/platform/ast-snoop-dma.0/data_history", F_OK) == 0) {
      legacy = true;
      legacy_checked = true;
    } else if (access("/dev/aspeed-lpc-snoop0", F_OK) == 0) {
      legacy_checked = true;
    } else {
      return PAL_ENOTSUP;
    }
  }

  // Support for snoop-dma on 4.1 kernel.
  if (legacy) {
    return pal_lpc_snoop_read_legacy(buf, max_len, len);
  }
  return pal_lpc_snoop_read(buf, max_len, len);

}

int __attribute__((weak))
pal_set_boot_order(uint8_t slot, uint8_t *boot, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_get_boot_order(uint8_t slot, uint8_t *req_data, uint8_t *boot, uint8_t *res_len)
{
  return PAL_ENOTSUP;
}

void __attribute__((weak))
pal_set_boot_option(unsigned char para,unsigned char* pbuff)
{
  return;
}

int __attribute__((weak))
pal_get_boot_option(unsigned char para,unsigned char* pbuff)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_set_bios_current_boot_list(uint8_t slot, uint8_t *boot_list, uint8_t list_length, uint8_t *cc) {
  return 0;
}

int __attribute__((weak))
pal_get_bios_current_boot_list(uint8_t slot, uint8_t *boot_list, uint8_t *list_length) {
  return 0;
}

int __attribute__((weak))
pal_set_bios_fixed_boot_device(uint8_t slot, uint8_t *fixed_boot_device) {
  return 0;
}

int __attribute__((weak))
pal_get_bios_fixed_boot_device(uint8_t slot, uint8_t *fixed_boot_device) {
  return 0;
}

int __attribute__((weak))
pal_set_bios_restores_default_setting(uint8_t slot, uint8_t *default_setting) {
  return 0;
}

int __attribute__((weak))
pal_get_bios_restores_default_setting(uint8_t slot, uint8_t *default_setting) {
  return 0;
}

uint8_t __attribute__((weak))
pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy, uint8_t *res_data) {
  return 0;
}

uint8_t __attribute__((weak))
pal_set_slot_power_policy(uint8_t *pwr_policy, uint8_t *res_data) {
  return 0;
}

int __attribute__((weak))
pal_bmc_err_disable(const char *error_item) {
  return 0;
}

int __attribute__((weak))
pal_bmc_err_enable(const char *error_item) {
  return 0;
}

void __attribute__((weak))
pal_set_post_start(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len)
{
// TODO: For now logging the event, need to find usage for this info
  syslog (LOG_INFO, "POST Start Event for Payload#%d\n", slot);
  *res_len = 0;
  return;
}

void __attribute__((weak))
pal_set_post_end(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len)
{
// TODO: For now logging the event, need to find usage for this info
  syslog (LOG_INFO, "POST End Event for Payload#%d\n", slot);
  *res_len = 0;
}

int __attribute__((weak))
pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_parse_oem_unified_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  //
  // If a platform needs to process a specific type of SEL message
  // differently from what the common code does,
  // this function can be overriden to do such unique handling
  // instead of calling the common SEL parsing logic (pal_parse_oem_unified_sel_common.)
  //
  // This default handler will not perform any special handling, and will
  // just call the default handler (pal_parse_oem_unified_sel_common) for every type of
  // SEL msessages.
  //

  pal_parse_oem_unified_sel_common(fru, sel, error_log);

  return 0;
}

static void
get_common_dimm_location_str(_dimm_info dimm_info, char* dimm_location_str, char* dimm_str)
{
  // Check Channel and Slot
  if (dimm_info.channel == 0xFF && dimm_info.slot == 0xFF) {
    sprintf(dimm_str, "unknown");
    sprintf(dimm_location_str, "DIMM Slot Location: Sled %02d/Socket %02d, Channel unknown, Slot unknown, DIMM unknown",
            dimm_info.sled, dimm_info.socket);
  } else {
    dimm_info.channel &= 0x07; // Channel Bit[3:0]
    dimm_info.slot &= 0x03;    // Slot [0-2]
    pal_convert_to_dimm_str(dimm_info.socket, dimm_info.channel, dimm_info.slot, dimm_str);
    sprintf(dimm_location_str, "DIMM Slot Location: Sled %02d/Socket %02d, Channel %02d, Slot %02d, DIMM %s",
            dimm_info.sled, dimm_info.socket, dimm_info.channel, dimm_info.slot, dimm_str);
  }
}

int __attribute__((weak))
pal_parse_oem_unified_sel_common(uint8_t fru, uint8_t *sel, char *error_log)
{
  char *mem_err[] = {
    "Memory training failure",
    "Memory correctable error",
    "Memory uncorrectable error",
    "Memory correctable error (Patrol scrub)",
    "Memory uncorrectable error (Patrol scrub)",
    "Memory Parity Error event",
    "Reserved"
  };
  char *upi_err[] = {
    "UPI Init error",
    "Reserved"
  };
  char *post_err[] = {
    "System PXE boot fail",
    "CMOS/NVRAM configuration cleared",
    "TPM Self-Test Fail",
    "Reserved"
  };
  char *pcie_event[] = {
    "PCIe DPC Event",
    "PCIe LER Event",
    "PCIe Link Retraining and Recovery",
    "PCIe Link CRC Error Check and Retry",
    "PCIe Corrupt Data Containment",
    "PCIe Express ECRC",
    "Reserved"
  };
  char *mem_event[] = {
    "Memory PPR event",
    "Memory Correctable Error logging limit reached",
    "Memory disable/map-out for FRB",
    "Memory SDDC",
    "Memory Address range/Partial mirroring",
    "Memory ADDDC",
    "Memory SMBus hang recovery",
    "No DIMM in System",
    "Reserved"
  };
  char *upi_event[] = {
    "Successful LLR without Phy Reinit",
    "Successful LLR with Phy Reinit",
    "COR Phy Lane failure, recovery in x8 width",
    "Reserved"
  };
  uint8_t general_info = sel[3];
  uint8_t error_type = general_info & 0xF;
  uint8_t plat, event_type, estr_idx;
  _dimm_info dimm_info = {
    (sel[8]>>4) & 0x03, // Sled
    sel[8] & 0x0F,      // Socket
    sel[9],             // Channel
    sel[10]             // Slot
  };
  char dimm_str[8] = {0};
  char dimm_location_str[128] = {0};
  char temp_log[128] = {0};
  error_log[0] = '\0';

  switch (error_type) {
    case UNIFIED_PCIE_ERR:
      plat = (general_info & 0x10) >> 4;
      if (plat == 0) {  //x86
        sprintf(error_log, "GeneralInfo: x86/PCIeErr(0x%02X), Bus %02X/Dev %02X/Fun %02X, TotalErrID1Cnt: 0x%04X, ErrID2: 0x%02X, ErrID1: 0x%02X",
                general_info, sel[11], sel[10]>>3, sel[10]&0x7, ((sel[13]<<8)|sel[12]), sel[14], sel[15]);
      } else {
        sprintf(error_log, "GeneralInfo: ARM/PCIeErr(0x%02X), Aux. Info: 0x%04X, Bus %02X/Dev %02X/Fun %02X, TotalErrID1Cnt: 0x%04X, ErrID2: 0x%02X, ErrID1: 0x%02X",
                general_info, ((sel[9]<<8)|sel[8]), sel[11], sel[10]>>3,
                sel[10]&0x7, ((sel[13]<<8)|sel[12]),sel[14], sel[15]);
      }
      sprintf(temp_log, "B %02X D %02X F %02X PCIe err,FRU:%u", sel[11], sel[10]>>3, sel[10]&0x7, fru);
      pal_add_cri_sel(temp_log);
      break;

    case UNIFIED_MEM_ERR:
      // get dimm location data string.
      get_common_dimm_location_str(dimm_info, dimm_location_str, dimm_str);
      plat = (sel[12] & 0x80) >> 7;
      event_type = sel[12] & 0xF;
      switch (event_type) {
        case MEMORY_TRAINING_ERR:
          if (plat == 0) { //Intel
            sprintf(error_log, "GeneralInfo: MEMORY_ECC_ERR(0x%02X), %s, DIMM Failure Event: %s, Major Code: 0x%02X, Minor Code: 0x%02X",
                    general_info, dimm_location_str,
                    mem_err[event_type], sel[13], sel[14]);
          } else { //AMD
            sprintf(error_log, "GeneralInfo: MEMORY_ECC_ERR(0x%02X), %s, DIMM Failure Event: %s, Major Code: 0x%02X, Minor Code: 0x%04X",
                    general_info, dimm_location_str,
                    mem_err[event_type], sel[13], (sel[15]<<8)|sel[14]);
          }
          break;
        default:
          pal_convert_to_dimm_str(dimm_info.socket, dimm_info.channel, dimm_info.slot, dimm_str);
          estr_idx = (event_type < ARRAY_SIZE(mem_err)) ? event_type : (ARRAY_SIZE(mem_err) - 1);
          sprintf(error_log, "GeneralInfo: MEMORY_ECC_ERR(0x%02X), %s, DIMM Failure Event: %s",
                  general_info, dimm_location_str, mem_err[estr_idx]);

          if ((event_type == MEMORY_CORRECTABLE_ERR) || (event_type == MEMORY_CORR_ERR_PTRL_SCR)) {
            sprintf(temp_log, "DIMM%s ECC err,FRU:%u", dimm_str, fru);
            pal_add_cri_sel(temp_log);
          } else if ((event_type == MEMORY_UNCORRECTABLE_ERR) || (event_type == MEMORY_UNCORR_ERR_PTRL_SCR)) {
            sprintf(temp_log, "DIMM%s UECC err,FRU:%u", dimm_str, fru);
            pal_add_cri_sel(temp_log);
          }
          break;
      }
      break;

    case UNIFIED_UPI_ERR:
      event_type = sel[12] & 0xF;
      estr_idx = (event_type < ARRAY_SIZE(upi_err)) ? event_type : (ARRAY_SIZE(upi_err) - 1);

      switch (event_type) {
        case UPI_INIT_ERR:
          sprintf(error_log, "GeneralInfo: UPIErr(0x%02X), UPI Port Location: Sled %02d/Socket %02d, Port %02d, UPI Failure Event: %s, Major Code: 0x%02X, Minor Code: 0x%02X",
                  general_info, dimm_info.sled, dimm_info.socket, sel[9]&0xF, upi_err[estr_idx], sel[13], sel[14]);
          break;
        default:
          sprintf(error_log, "GeneralInfo: UPIErr(0x%02X), UPI Port Location: Sled %02d/Socket %02d, Port %02d, UPI Failure Event: %s",
                  general_info, dimm_info.sled, dimm_info.socket, sel[9]&0xF, upi_err[estr_idx]);
          break;
      }
      break;

    case UNIFIED_IIO_ERR:
      sprintf(error_log, "GeneralInfo: IIOErr(0x%02X), IIO Port Location: Sled %02d/Socket %02d, Stack 0x%02X, Error ID: 0x%02X",
              general_info, dimm_info.sled, dimm_info.socket, sel[9], sel[12]);
      sprintf(temp_log, "IIO_ERR CPU%d. Error ID(%02X)",dimm_info.socket, sel[12]);
      pal_add_cri_sel(temp_log);
      break;

    case UNIFIED_POST_ERR:
      event_type = sel[8] & 0xF;
      estr_idx = (event_type < ARRAY_SIZE(post_err)) ? event_type : (ARRAY_SIZE(post_err) - 1);
      sprintf(error_log, "GeneralInfo: POST(0x%02X), POST Failure Event: %s", general_info, post_err[estr_idx]);
      break;

    case UNIFIED_PCIE_EVENT:
      event_type = sel[8] & 0xF;
      estr_idx = (event_type < ARRAY_SIZE(pcie_event)) ? event_type : (ARRAY_SIZE(pcie_event) - 1);

      switch (event_type) {
        case PCIE_DPC:
          sprintf(error_log, "GeneralInfo: PCIeEvent(0x%02X), PCIe Failure Event: %s, Status: 0x%04X, Source ID: 0x%04X",
                  general_info, pcie_event[estr_idx], (sel[11]<<8)|sel[10], (sel[13]<<8)|sel[12]);
          break;
        default:
          sprintf(error_log, "GeneralInfo: PCIeEvent(0x%02X), PCIe Failure Event: %s",
                  general_info, pcie_event[estr_idx]);
          break;
      }
      break;

    case UNIFIED_MEM_EVENT:
      // get dimm location data string.
      get_common_dimm_location_str(dimm_info, dimm_location_str, dimm_str);

      // Event-Type Bit[3:0]
      event_type = sel[12] & 0x0F;
      switch (event_type) {
        case MEM_PPR:
          sprintf(error_log, "GeneralInfo: MemEvent(0x%02X), %s, DIMM Failure Event: %s",
                  general_info, dimm_location_str, (sel[13]&0x01)?"PPR fail":"PPR success");
          break;
        case MEM_NO_DIMM:
          sprintf(error_log, "GeneralInfo: MemEvent(0x%02X), DIMM Failure Event: %s", 
                  general_info, mem_event[event_type]);
          break;
        default:
          estr_idx = (event_type < ARRAY_SIZE(mem_event)) ? event_type : (ARRAY_SIZE(mem_event) - 1);
          sprintf(error_log, "GeneralInfo: MemEvent(0x%02X), %s, DIMM Failure Event: %s",
                  general_info, dimm_location_str, mem_event[estr_idx]);
          break;
      }
      break;

    case UNIFIED_UPI_EVENT:
      event_type = sel[12] & 0xF;
      estr_idx = (event_type < ARRAY_SIZE(upi_event)) ? event_type : (ARRAY_SIZE(upi_event) - 1);
      sprintf(error_log, "GeneralInfo: UPIEvent(0x%02X), UPI Port Location: Sled %02d/Socket %02d, Port %02d, UPI Failure Event: %s",
              general_info, dimm_info.sled, dimm_info.socket, sel[9]&0xF, upi_event[estr_idx]);
      break;

    case UNIFIED_BOOT_GUARD:
      sprintf(error_log, "GeneralInfo: Boot Guard ACM Failure Events(0x%02X), Error Class(0x%02X), Error Code(0x%02X)",
              general_info, sel[8], sel[9]);
      break;

    default:
      sprintf(error_log, "Undefined Error Type(0x%02X), Raw: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
              error_type, sel[3], sel[4], sel[5], sel[6], sel[7], sel[8], sel[9], sel[10], sel[11], sel[12], sel[13],
              sel[14], sel[15]);
      break;
  }

  return 0;
}

int __attribute__((weak))
pal_parse_mem_mapping_string(uint8_t channel, bool *support_mem_mapping, char *error_log)
{
  if ( support_mem_mapping ) {
    *support_mem_mapping = false;
  }
  if (error_log) {
    error_log[0] = '\0';
  }
  return PAL_EOK;
}

int __attribute__((weak))
pal_convert_to_dimm_str(uint8_t cpu, uint8_t channel, uint8_t slot, char *str)
{
  sprintf(str, "%c%d", 'A'+channel, slot);
  return PAL_EOK;
}

int __attribute__((weak))
pal_parse_oem_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  error_log[0] = '\0';
  return 0;
}

int __attribute__((weak))
pal_set_ppin_info(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_sled_ac_cycle(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_slot_led(uint8_t fru, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_pcie_port_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_set_pcie_port_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_set_imc_version(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_EOK;
}

uint8_t __attribute__((weak))
pal_add_cper_log(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_EOK;
}

uint8_t __attribute__((weak))
pal_set_psb_info(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_ENOTSUP;
}

uint8_t __attribute__((weak))
pal_add_imc_log(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_EOK;
}

uint8_t __attribute__((weak))
pal_parse_ras_sel(uint8_t slot, uint8_t *sel, char *error_log)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_fru_post(uint8_t fru, uint8_t value)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_cpu_mem_threshold(const char* threshold_path)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_platform_name(char *name)
{
  strcpy(name, MACHINE);
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_num_slots(uint8_t *num)
{
  *num = 0;
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_num_devs(uint8_t slot, uint8_t *num)
{
  *num = 0;
  return PAL_EOK;
}

int __attribute__((weak))
pal_is_fru_prsnt(uint8_t fru, uint8_t *status)
{
  *status = 1;
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_slot_index(unsigned char payload_id)
{
  return payload_id;
}

int __attribute__((weak))
pal_get_server_power(uint8_t slot_id, uint8_t *status)
{
  *status = 0;
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_server_power(uint8_t slot_id, uint8_t cmd)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_device_power(uint8_t slot_id, uint8_t dev_id, uint8_t *status, uint8_t *type)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_device_power(uint8_t slot_id, uint8_t dev_id, uint8_t cmd)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_power_button_override(uint8_t slot_id)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_sled_cycle(void)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_post_handle(uint8_t slot, uint8_t status)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_rst_btn(uint8_t slot, uint8_t status)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_led(uint8_t led, uint8_t status)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_hb_led(uint8_t status)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_fru_list(char *list)
{
  *list = '\0';
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_dev_list(uint8_t fru, char *list)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_get_fru_id(char *str, uint8_t *fru)
{
  unsigned int _fru;
  if (sscanf(str, "fru%u", &_fru) == 1) {
    *fru = (uint8_t)_fru;
    return PAL_EOK;
  }
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_get_dev_id(char *str, uint8_t *fru)
{
  unsigned int _fru;
  if (sscanf(str, "fru%u", &_fru) == 1) {
    *fru = (uint8_t)_fru;
    return PAL_EOK;
  }
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_get_fru_name(uint8_t fru, char *name)
{
  sprintf(name, "fru%u", fru);
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_fru_capability(uint8_t fru, unsigned int *caps)
{
  *caps = FRU_CAPABILITY_ALL;
  return 0;
}

int __attribute__((weak))
pal_get_dev_capability(uint8_t fru, uint8_t dev, unsigned int *caps)
{
  *caps = FRU_CAPABILITY_ALL;
  return 0;
}

int __attribute__((weak))
pal_get_dev_fruid_name(uint8_t fru, uint8_t dev, char *name)
{
  char fruname[64];
  int ret;
  if (fru == 0)
    return -1;
  ret = pal_get_fruid_name(fru, fruname);
  if (ret < 0)
    return ret;
  sprintf(name, "%s Device %u", fruname, (unsigned int)dev - 1);
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_dev_name(uint8_t fru, uint8_t dev, char *name)
{
  if (dev == 0)
    return -1;
  sprintf(name, "device%u", (unsigned int)dev - 1);
  return 0;
}

int __attribute__((weak))
pal_get_fruid_path(uint8_t fru, char *path)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_get_dev_fruid_path(uint8_t fru, uint8_t dev_id, char *path)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_get_fruid_eeprom_path(uint8_t fru, char *path)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_get_fruid_name(uint8_t fru, char *name)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_fruid_write(uint8_t slot, char *path)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_dev_fruid_write(uint8_t fru, uint8_t dev_id, char *path)
{
  return PAL_EOK;
}


int __attribute__((weak))
pal_get_fru_devtty(uint8_t fru, char *devtty)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_cfg_key_check(char *key)
{
  /* TODO: Leverage the code from
   * meta-facebook/meta-fbttn/recipes-fbttn/fblibs/files/pal/pal.c;
   * once all platforms using cfg-util uses cfg_support_key_list[]
   */
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_key_value(char *key, char *value)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_key_value(char *key, char *value)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_def_key_value(void)
{
  return PAL_EOK;
}

void __attribute__((weak))
pal_dump_key_value(void)
{
  return;
}

int __attribute__((weak))
pal_get_last_pwr_state(uint8_t fru, char *state)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_last_pwr_state(uint8_t fru, char *state)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_sys_guid(uint8_t slot, char *guid)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_sys_guid(uint8_t fru, char *guid)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_sysfw_ver(uint8_t slot, uint8_t *ver)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_sysfw_ver(uint8_t slot, uint8_t *ver)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_is_cmd_valid(uint8_t *data)
{
  return PAL_EOK;
}

/*
 *  Default implementation if pal_is_fru_x86 is to always return true
 *  on all FRU, on all platform. This is because all uServers so far are X86.
 *  This logic can be "OVERRIDDEN" as needed, on any newer platform,
 *  in order to return dynamic value, based on the information which
 *  BMC collected so far.
 *  (therefore mix-and-match supported; not sure if needed.)
 */
bool __attribute__((weak))
pal_is_fru_x86(uint8_t fru)
{
  return true;
}

int __attribute__((weak))
pal_get_x86_event_sensor_name(uint8_t fru, uint8_t snr_num,
                                     char *name)
{
  if (pal_is_fru_x86(fru))
  {
    switch(snr_num) {
      case SYSTEM_EVENT:
        sprintf(name, "SYSTEM_EVENT");
        break;
      case THERM_THRESH_EVT:
        sprintf(name, "THERM_THRESH_EVT");
        break;
      case CRITICAL_IRQ:
        sprintf(name, "CRITICAL_IRQ");
        break;
      case POST_ERROR:
        sprintf(name, "POST_ERROR");
        break;
      case MACHINE_CHK_ERR:
        sprintf(name, "MACHINE_CHK_ERR");
        break;
      case PCIE_ERR:
        sprintf(name, "PCIE_ERR");
        break;
      case IIO_ERR:
        sprintf(name, "IIO_ERR");
        break;
      case SMN_ERR:
        sprintf(name, "SMN_ERR");
        break;
      case USB_ERR:
        sprintf(name, "USB_ERR");
        break;
      case PSB_ERR:
        sprintf(name, "PSB_STS");
        break;
      case MEMORY_ECC_ERR:
        sprintf(name, "MEMORY_ECC_ERR");
        break;
      case MEMORY_ERR_LOG_DIS:
        sprintf(name, "MEMORY_ERR_LOG_DIS");
        break;
      case PROCHOT_EXT:
        sprintf(name, "PROCHOT_EXT");
        break;
      case PWR_ERR:
        sprintf(name, "PWR_ERR");
        break;
      case CATERR_A:
      case CATERR_B:
        sprintf(name, "CATERR");
        break;
      case CPU_DIMM_HOT:
        sprintf(name, "CPU_DIMM_HOT");
        break;
      case SOFTWARE_NMI:
        sprintf(name, "SOFTWARE_NMI");
        break;
      case CPU0_THERM_STATUS:
        sprintf(name, "CPU0_THERM_STATUS");
        break;
      case CPU1_THERM_STATUS:
        sprintf(name, "CPU1_THERM_STATUS");
        break;
      case CPU2_THERM_STATUS:
        sprintf(name, "CPU2_THERM_STATUS");
        break;
      case CPU3_THERM_STATUS:
        sprintf(name, "CPU3_THERM_STATUS");
        break;
      case ME_POWER_STATE:
        sprintf(name, "ME_POWER_STATE");
        break;
      case SPS_FW_HEALTH:
        sprintf(name, "SPS_FW_HEALTH");
        break;
      case NM_EXCEPTION_A:
        sprintf(name, "NM_EXCEPTION");
        break;
      case PCH_THERM_THRESHOLD:
        sprintf(name, "PCH_THERM_THRESHOLD");
        break;
      case NM_HEALTH:
        sprintf(name, "NM_HEALTH");
        break;
      case NM_CAPABILITIES:
        sprintf(name, "NM_CAPABILITIES");
        break;
      case NM_THRESHOLD:
        sprintf(name, "NM_THRESHOLD");
        break;
      case PWR_THRESH_EVT:
        sprintf(name, "PWR_THRESH_EVT");
        break;
      case HPR_WARNING:
        sprintf(name, "HPR_WARNING");
        break;
      default:
        sprintf(name, "Unknown");
        break;
    }
    return 0;
  } else {
    sprintf(name, "UNDEFINED:NOT_X86");
  }
  return 0;
}

int __attribute__((weak))
pal_get_event_sensor_name(uint8_t fru, uint8_t *sel, char *name) {
  uint8_t snr_type = sel[10];
  uint8_t snr_num = sel[11];

  // If SNR_TYPE is OS_BOOT, sensor name is OS
  switch (snr_type) {
    case OS_BOOT:
      // OS_BOOT used by OS
      sprintf(name, "OS");
      return 0;
  }
  // Otherwise, translate it based on snr_num
  return pal_get_x86_event_sensor_name(fru, snr_num, name);
}

int __attribute__((weak))
pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_oem_unified_sel_handler(uint8_t fru, uint8_t general_info, uint8_t *sel)
{
  return PAL_EOK;
}

/*
 * A Function to parse common SEL message, a helper funciton
 * for pal_parse_sel.
 *
 * Note that this function __CANNOT__ be overriden.
 * To add board specific routine, please override pal_parse_sel.
 */

bool
pal_parse_sel_helper(uint8_t fru, uint8_t *sel, char *error_log)
{

  bool parsed = true;
  uint8_t snr_type = sel[10];
  uint8_t snr_num = sel[11];
  uint8_t *event_data = &sel[10];
  uint8_t *ed = &event_data[3];
  char temp_log[512] = {0};
  uint8_t temp;
  uint8_t sen_type = event_data[0];
  uint8_t chn_num, dimm_num;
  uint8_t idx;

  /*Used by decoding ME event*/
  char *nm_capability_status[2] = {"Not Available", "Available"};
  char *nm_domain_name[6] = {"Entire Platform", "CPU Subsystem", "Memory Subsystem", "HW Protection", "High Power I/O subsystem", "Unknown"};
  char *nm_err_type[17] =
                    {"Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown",
                     "Extended Telemetry Device Reading Failure", "Outlet Temperature Reading Failure",
                     "Volumetric Airflow Reading Failure", "Policy Misconfiguration",
                     "Power Sensor Reading Failure", "Inlet Temperature Reading Failure",
                     "Host Communication Error", "Real-time Clock Synchronization Failure",
                     "Platform Shutdown Initiated by Intel NM Policy", "Unknown"};
  char *nm_health_type[4] = {"Unknown", "Unknown", "SensorIntelNM", "Unknown"};
  const char *thres_event_name[16] = {[0] = "Lower Non-critical",
                                      [2] = "Lower Critical",
                                      [4] = "Lower Non-recoverable",
                                      [7] = "Upper Non-critical",
                                      [9] = "Upper Critical",
                                      [11] = "Upper Non-recoverable"};


  strcpy(error_log, "");

  switch (snr_type) {
    case OS_BOOT:
      // OS_BOOT used by OS
      switch (ed[0] & 0xF) {
        case 0x07:
          strcat(error_log, "Base OS/Hypervisor Installation started");
          break;
        case 0x08:
          strcat(error_log, "Base OS/Hypervisor Installation completed");
          break;
        case 0x09:
          strcat(error_log, "Base OS/Hypervisor Installation aborted");
          break;
        case 0x0A:
          strcat(error_log, "Base OS/Hypervisor Installation failed");
          break;
        default:
          strcat(error_log, "Unknown");
          parsed = false;
          break;
      }
      return parsed;
  }

  switch(snr_num) {
    case SYSTEM_EVENT:
      if (ed[0] == 0xE5) {
        strcat(error_log, "Cause of Time change - ");

        if (ed[2] == 0x00)
          strcat(error_log, "NTP");
        else if (ed[2] == 0x01)
          strcat(error_log, "Host RTL");
        else if (ed[2] == 0x02)
          strcat(error_log, "Set SEL time cmd ");
        else if (ed[2] == 0x03)
          strcat(error_log, "Set SEL time UTC offset cmd");
        else
          strcat(error_log, "Unknown");

        if (ed[1] == 0x00)
          strcat(error_log, " - First Time");
        else if(ed[1] == 0x80)
          strcat(error_log, " - Second Time");

      }
      break;

    case THERM_THRESH_EVT:
      if (ed[0] == 0x1)
        strcat(error_log, "Limit Exceeded");
      else
        strcat(error_log, "Unknown");
      break;

    case CRITICAL_IRQ:

      if (ed[0] == 0x0)
        strcat(error_log, "NMI / Diagnostic Interrupt");
      else if (ed[0] == 0x03)
        strcat(error_log, "Software NMI");
      else
        strcat(error_log, "Unknown");

      sprintf(temp_log,  "CRITICAL_IRQ, %s,FRU:%u", error_log, fru);
      pal_add_cri_sel(temp_log);

      break;

    case POST_ERROR:
      if ((ed[0] & 0x0F) == 0x0)
        strcat(error_log, "System Firmware Error");
      else
        strcat(error_log, "Unknown");
      if (((ed[0] >> 6) & 0x03) == 0x3) {
        // TODO: Need to implement IPMI spec based Post Code
        strcat(error_log, ", IPMI Post Code");
      } else if (((ed[0] >> 6) & 0x03) == 0x2) {
        sprintf(temp_log, ", OEM Post Code 0x%02X%02X", ed[2], ed[1]);
        strcat(error_log, temp_log);
        switch ((ed[2] << 8) | ed[1]) {
          case 0xA105:
            sprintf(temp_log, ", BMC Failed (No Response)");
            strcat(error_log, temp_log);
            break;
          case 0xA106:
            sprintf(temp_log, ", BMC Failed (Self Test Fail)");
            strcat(error_log, temp_log);
            break;
          case 0xA10A:
            sprintf(temp_log, ", System Firmware Corruption Detected");
            strcat(error_log, temp_log);
            break;
          case 0xA10B:
            sprintf(temp_log, ", TPM Self-Test FAIL Detected");
            strcat(error_log, temp_log);
            break;
          default:
            break;
        }
      }
      break;

    case MACHINE_CHK_ERR:
      if ((ed[0] & 0x0F) == 0x0B) {
        strcat(error_log, "Uncorrectable");
        sprintf(temp_log, "MACHINE_CHK_ERR, %s bank Number %d,FRU:%u", error_log, ed[1], fru);
        pal_add_cri_sel(temp_log);
      } else if ((ed[0] & 0x0F) == 0x0C) {
        strcat(error_log, "Correctable");
        sprintf(temp_log, "MACHINE_CHK_ERR, %s bank Number %d,FRU:%u", error_log, ed[1], fru);
        pal_add_cri_sel(temp_log);
      } else {
        strcat(error_log, "Unknown");
        sprintf(temp_log, "MACHINE_CHK_ERR, %s bank Number %d,FRU:%u", error_log, ed[1], fru);
        pal_add_cri_sel(temp_log);
      }

      sprintf(temp_log, ", Machine Check bank Number %d ", ed[1]);
      strcat(error_log, temp_log);
      sprintf(temp_log, ", CPU %d, Core %d ", ed[2] >> 5, ed[2] & 0x1F);
      strcat(error_log, temp_log);

      break;

    case PCIE_ERR:

      if ((ed[0] & 0xF) == 0x4) {
        sprintf(error_log, "PCI PERR (Bus %02X / Dev %02X / Fun %02X)", ed[2], ed[1] >> 3, ed[1] & 0x7);
      } else if ((ed[0] & 0xF) == 0x5) {
        sprintf(error_log, "PCI SERR (Bus %02X / Dev %02X / Fun %02X)", ed[2], ed[1] >> 3, ed[1] & 0x7);
      } else if ((ed[0] & 0xF) == 0x7) {
        sprintf(error_log, "Correctable (Bus %02X / Dev %02X / Fun %02X)", ed[2], ed[1] >> 3, ed[1] & 0x7);
      } else if ((ed[0] & 0xF) == 0x8) {
        sprintf(error_log, "Uncorrectable (Bus %02X / Dev %02X / Fun %02X)", ed[2], ed[1] >> 3, ed[1] & 0x7);
      } else if ((ed[0] & 0xF) == 0xA) {
        sprintf(error_log, "Bus Fatal (Bus %02X / Dev %02X / Fun %02X)", ed[2], ed[1] >> 3, ed[1] & 0x7);
      } else if ((ed[0] & 0xF) == 0xD) {
        unsigned int vendor_id = (unsigned int)ed[1] << 8 | (unsigned int)ed[2];
        sprintf(error_log, "Vendor ID: 0x%4x", vendor_id);
      } else if ((ed[0] & 0xF) == 0xE) {
        unsigned int device_id = (unsigned int)ed[1] << 8 | (unsigned int)ed[2];
        sprintf(error_log, "Device ID: 0x%4x", device_id);
      } else if ((ed[0] & 0xF) == 0xF) {
        sprintf(error_log, "Error ID from downstream: 0x%2x 0x%2x", ed[1], ed[2]);
      } else {
        strcat(error_log, "Unknown");
      }

      sprintf(temp_log, "PCI_ERR %s,FRU:%u", error_log, fru);
      pal_add_cri_sel(temp_log);

      break;

    case IIO_ERR:
      if ((ed[0] & 0xF) == 0) {

        sprintf(temp_log, "CPU %d, Error ID 0x%X", (ed[2] & 0xE0) >> 5,
            ed[1]);
        strcat(error_log, temp_log);

        temp = ed[2] & 0xF;
        if (temp == 0x0)
          strcat(error_log, " - IRP0");
        else if (temp == 0x1)
          strcat(error_log, " - IRP1");
        else if (temp == 0x2)
          strcat(error_log, " - IIO-Core");
        else if (temp == 0x3)
          strcat(error_log, " - VT-d");
        else if (temp == 0x4)
          strcat(error_log, " - Intel Quick Data");
        else if (temp == 0x5)
          strcat(error_log, " - Misc");
        else if (temp == 0x6)
          strcat(error_log, " - DMA");
        else if (temp == 0x7)
          strcat(error_log, " - ITC");
        else if (temp == 0x8)
          strcat(error_log, " - OTC");
        else if (temp == 0x9)
          strcat(error_log, " - CI");
        else
          strcat(error_log, " - Reserved");
      } else
        strcat(error_log, "Unknown");
      sprintf(temp_log, "IIO_ERR %s,FRU:%u", error_log, fru);
      pal_add_cri_sel(temp_log);
      break;

    case MEMORY_ECC_ERR:
    case MEMORY_ERR_LOG_DIS:
      if (snr_num == MEMORY_ECC_ERR) {
        // SEL from MEMORY_ECC_ERR Sensor
        if ((ed[0] & 0x0F) == 0x0) {
          if (sen_type == 0x0C) {
            strcat(error_log, "Correctable");
            sprintf(temp_log, "DIMM%02X ECC err,FRU:%u", ed[2], fru);
            pal_add_cri_sel(temp_log);
          } else if (sen_type == 0x10)
            strcat(error_log, "Correctable ECC error Logging Disabled");
        } else if ((ed[0] & 0x0F) == 0x1) {
          strcat(error_log, "Uncorrectable");
          sprintf(temp_log, "DIMM%02X UECC err,FRU:%u", ed[2], fru);
          pal_add_cri_sel(temp_log);
        } else if ((ed[0] & 0x0F) == 0x5)
          strcat(error_log, "Correctable ECC error Logging Limit Reached");
        else
          strcat(error_log, "Unknown");
      } else if (snr_num == MEMORY_ERR_LOG_DIS) {
        // SEL from MEMORY_ERR_LOG_DIS Sensor
        if ((ed[0] & 0x0F) == 0x0)
          strcat(error_log, "Correctable Memory Error Logging Disabled");
        else
          strcat(error_log, "Unknown");
      }

      // Common routine for both MEM_ECC_ERR and MEMORY_ERR_LOG_DIS
      sprintf(temp_log, " (DIMM %02X)", ed[2]);
      strcat(error_log, temp_log);

      sprintf(temp_log, " Logical Rank %d", ed[1] & 0x03);
      strcat(error_log, temp_log);

      // DIMM number (ed[2]):
      // Bit[7:5]: Socket number  (Range: 0-7)
      // Bit[4:2]: Channel number (Range: 0-7)
      // Bit[1:0]: DIMM number    (Range: 0-3)
      if (((ed[1] & 0xC) >> 2) == 0x0) {
        /* All Info Valid */
        chn_num = (ed[2] & 0x1C) >> 2;
        dimm_num = ed[2] & 0x3;

        /* If critical SEL logging is available, do it */
        if (sen_type == 0x0C) {
          if ((ed[0] & 0x0F) == 0x0) {
            sprintf(temp_log, "DIMM%c%d ECC err,FRU:%u", 'A'+chn_num,
                    dimm_num, fru);
            pal_add_cri_sel(temp_log);
          } else if ((ed[0] & 0x0F) == 0x1) {
            sprintf(temp_log, "DIMM%c%d UECC err,FRU:%u", 'A'+chn_num,
                    dimm_num, fru);
            pal_add_cri_sel(temp_log);
          }
        }
        /* Then continue parse the error into a string. */
        /* All Info Valid                               */
        sprintf(temp_log, " (CPU# %d, CHN# %d, DIMM# %d)",
            (ed[2] & 0xE0) >> 5, (ed[2] & 0x18) >> 3, ed[2] & 0x7);
      } else if (((ed[1] & 0xC) >> 2) == 0x1) {
        /* DIMM info not valid */
        sprintf(temp_log, " (CPU# %d, CHN# %d)",
            (ed[2] & 0xE0) >> 5, (ed[2] & 0x18) >> 3);
      } else if (((ed[1] & 0xC) >> 2) == 0x2) {
        /* CHN info not valid */
        sprintf(temp_log, " (CPU# %d, DIMM# %d)",
            (ed[2] & 0xE0) >> 5, ed[2] & 0x7);
      } else if (((ed[1] & 0xC) >> 2) == 0x3) {
        /* CPU info not valid */
        sprintf(temp_log, " (CHN# %d, DIMM# %d)",
            (ed[2] & 0x18) >> 3, ed[2] & 0x7);
      }
      strcat(error_log, temp_log);

      break;


    case PWR_ERR:
      if (ed[0] == 0x1) {
        strcat(error_log, "SYS_PWROK failure");
        /* Also try logging to Critial log file, if available */
        sprintf(temp_log, "SYS_PWROK failure,FRU:%u", fru);
        pal_add_cri_sel(temp_log);
      } else if (ed[0] == 0x2) {
        strcat(error_log, "PCH_PWROK failure");
        /* Also try logging to Critial log file, if available */
        sprintf(temp_log, "PCH_PWROK failure,FRU:%u", fru);
        pal_add_cri_sel(temp_log);
      }
      else
        strcat(error_log, "Unknown");
      break;

    case CATERR_A:
    case CATERR_B:
      if (ed[0] == 0x0) {
        strcat(error_log, "IERR/CATERR");
        /* Also try logging to Critial log file, if available */
        sprintf(temp_log, "IERR,FRU:%u", fru);
        pal_add_cri_sel(temp_log);
      } else if (ed[0] == 0xB) {
        strcat(error_log, "MCERR/CATERR");
        /* Also try logging to Critial log file, if available */
        sprintf(temp_log, "MCERR,FRU:%u", fru);
        pal_add_cri_sel(temp_log);
      }
      else
        strcat(error_log, "Unknown");
      break;

    case MSMI:
      if (ed[0] == 0x0)
        strcat(error_log, "IERR/MSMI");
      else if (ed[0] == 0xB)
        strcat(error_log, "MCERR/MSMI");
      else
        strcat(error_log, "Unknown");
      break;

    case CPU_DIMM_HOT:
      if ((ed[0] << 16 | ed[1] << 8 | ed[2]) == 0x01FFFF)
        strcat(error_log, "SOC MEMHOT");
      else
        strcat(error_log, "Unknown");
      sprintf(temp_log, "CPU_DIMM_HOT %s,FRU:%u", error_log, fru);
      pal_add_cri_sel(temp_log);
      break;

    case SOFTWARE_NMI:
      if ((ed[0] << 16 | ed[1] << 8 | ed[2]) == 0x03FFFF)
        strcat(error_log, "Software NMI");
      else
        strcat(error_log, "Unknown SW NMI");
      break;

    case ME_POWER_STATE:
      switch (ed[0]) {
        case 0:
          sprintf(error_log, "RUNNING");
          break;
        case 2:
          sprintf(error_log, "POWER_OFF");
          break;
        default:
          sprintf(error_log, "Unknown[%d]", ed[0]);
          break;
      }
      break;
    case SPS_FW_HEALTH:
      if ((ed[0] & 0x0F) == 0x00) {
        switch (ed[1]) {
          case 0x00:
            strcat(error_log, "Recovery GPIO forced");
            return 1;
          case 0x01:
            strcat(error_log, "Image execution failed");
            return 1;
          case 0x02:
            strcat(error_log, "Flash erase error");
            return 1;
          case 0x03:
            strcat(error_log, "Flash state information");
            return 1;
          case 0x04:
            strcat(error_log, "Internal error");
            return 1;
          case 0x05:
            strcat(error_log, "BMC did not respond");
            return 1;
          case 0x06:
            strcat(error_log, "Direct Flash update");
            return 1;
          case 0x07:
            strcat(error_log, "Manufacturing error");
            return 1;
          case 0x08:
            strcat(error_log, "Automatic Restore to Factory Presets");
            return 1;
          case 0x09:
            strcat(error_log, "Firmware Exception");
            return 1;
          case 0x0A:
            strcat(error_log, "Flash Wear-Out Protection Warning");
            return 1;
          case 0x0D:
            strcat(error_log, "DMI interface error");
            return 1;
          case 0x0E:
            strcat(error_log, "MCTP interface error");
            return 1;
          case 0x0F:
            strcat(error_log, "Auto-configuration finished");
            return 1;
          case 0x10:
            strcat(error_log, "Unsupported Segment Defined Feature");
            return 1;
          case 0x12:
            strcat(error_log, "CPU Debug Capability Disabled");
            return 1;
          case 0x13:
            strcat(error_log, "UMA operation error");
            return 1;
          //0x14 and 0x15 are reserved
          case 0x16:
            strcat(error_log, "Intel PTT Health");
            return 1;
          case 0x17:
            strcat(error_log, "Intel Boot Guard Health");
            return 1;
          case 0x18:
            strcat(error_log, "Restricted mode information");
            return 1;
          case 0x19:
            strcat(error_log, "MultiPCH mode misconfiguration");
            return 1;
          case 0x1A:
            strcat(error_log, "Flash Descriptor Region Verification Error");
            return 1;
          default:
            strcat(error_log, "Unknown");
            break;
        }
      } else if ((ed[0] & 0x0F) == 0x01) {
        strcat(error_log, "SMBus link failure");
        return 1;
      } else {
        strcat(error_log, "Unknown");
      }
      break;

    /*NM4.0 #550710, Revision 1.95, and turn to p.155*/
    case NM_EXCEPTION_A:
      if (ed[0] == 0xA8) {
        strcat(error_log, "Policy Correction Time Exceeded");
        return 1;
      } else
         strcat(error_log, "Unknown");
      break;
    case PCH_THERM_THRESHOLD:
      idx = ed[0] & 0x0f;
      sprintf(temp_log, "%s, curr_val: %d C, thresh_val: %d C", thres_event_name[idx] == NULL ? "Unknown" : thres_event_name[idx],ed[1],ed[2]);
      strcat(error_log, temp_log);
      break;
    case NM_HEALTH:
      {
        uint8_t health_type_index = (ed[0] & 0xf);
        uint8_t domain_index = (ed[1] & 0xf);
        uint8_t err_type_index = ((ed[1] >> 4) & 0xf);

        sprintf(error_log, "%s,Domain:%s,ErrType:%s,Err:0x%x", nm_health_type[health_type_index], nm_domain_name[domain_index], nm_err_type[err_type_index], ed[2]);
      }
      return 1;
      break;

    case NM_CAPABILITIES:
      if (ed[0] & 0x7)//BIT1=policy, BIT2=monitoring, BIT3=pwr limit and the others are reserved
      {
        int capability_index = 0;
        char *policy_capability = NULL;
        char *monitoring_capability = NULL;
        char *pwr_limit_capability = NULL;

        capability_index = BIT(ed[0], 0);
        policy_capability = nm_capability_status[ capability_index ];

        capability_index = BIT(ed[0], 1);
        monitoring_capability = nm_capability_status[ capability_index ];

        capability_index = BIT(ed[0], 2);
        pwr_limit_capability = nm_capability_status[ capability_index ];

        sprintf(error_log, "PolicyInterface:%s,Monitoring:%s,PowerLimit:%s",
          policy_capability, monitoring_capability, pwr_limit_capability);
      }
      else
      {
        strcat(error_log, "Unknown");
      }

      return 1;
      break;

    case NM_THRESHOLD:
      {
        uint8_t threshold_number = (ed[0] & 0x3);
        uint8_t domain_index = (ed[1] & 0xf);
        uint8_t policy_id = ed[2];
        uint8_t policy_event_index = BIT(ed[0], 3);
        char *policy_event[2] = {"Threshold Exceeded", "Policy Correction Time Exceeded"};

        sprintf(error_log, "Threshold Number:%d-%s,Domain:%s,PolicyID:0x%x",
          threshold_number, policy_event[policy_event_index], nm_domain_name[domain_index], policy_id);
      }
      return 1;
      break;

    case CPU0_THERM_STATUS:
    case CPU1_THERM_STATUS:
    case CPU2_THERM_STATUS:
    case CPU3_THERM_STATUS:
      if (ed[0] == 0x00)
        strcat(error_log, "CPU Critical Temperature");
      else if (ed[0] == 0x01)
        strcat(error_log, "PROCHOT#");
      else if (ed[0] == 0x02)
        strcat(error_log, "TCC Activation");
      else
        strcat(error_log, "Unknown");
      break;

    case PWR_THRESH_EVT:
      if (ed[0]  == 0x00)
        strcat(error_log, "Limit Not Exceeded");
      else if (ed[0]  == 0x01)
        strcat(error_log, "Limit Exceeded");
      else
        strcat(error_log, "Unknown");
      break;

    case HPR_WARNING:
      if (ed[2]  == 0x01) {
        if (ed[1] == 0xFF)
          strcat(temp_log, "Infinite Time");
        else
          sprintf(temp_log, "%d minutes",ed[1]);
        strcat(error_log, temp_log);
      } else {
        strcat(error_log, "Unknown");
      }
      break;

    default:
      sprintf(error_log, "Unknown");
      parsed = false;
      break;
  }
  if (((event_data[2] & 0x80) >> 7) == 0) {
    sprintf(temp_log, " Assertion");
    strcat(error_log, temp_log);
  } else {
    sprintf(temp_log, " Deassertion");
    strcat(error_log, temp_log);
  }
  return parsed;
}


int __attribute__((weak))
pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  //
  // If a platform needs to process a specific type of SEL message
  // differently from what the common code does,
  // this function can be overriden to do such unique handling
  // instead of calling the common SEL parsing logic (pal_parse_sel_helper.)
  //
  // This default handler will not perform any special handling, and will
  // just call the default handler (pal_parse_sel_helper) for every type of
  // SEL msessages.
  //

  pal_parse_sel_helper(fru, sel, error_log);

  return 0;
}

// By default, pal_add_cri_sel will do the best effort to
// log critical messages to default log file.
// Some platform has already overriden this function with
// empty function (do not log anything)
void __attribute__((weak))
pal_add_cri_sel(char *str)
{
  syslog(LOG_LOCAL0 | LOG_ERR, "%s", str);
}

int __attribute__((weak))
pal_set_sensor_health(uint8_t fru, uint8_t value)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_fru_health(uint8_t fru, uint8_t *value)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_fan_name(uint8_t num, char *name)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_fan_speed(uint8_t fan, int *rpm)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_fan_speed(uint8_t fan, uint8_t pwm)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_pwm_value(uint8_t fan_num, uint8_t *value)
{
  return PAL_EOK;
}

bool __attribute__((weak))
pal_is_fan_prsnt(uint8_t fan)
{
  return true;
}

int __attribute__((weak))
pal_fan_dead_handle(int fan_num)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_fan_recovered_handle(int fan_num)
{
  return PAL_EOK;
}

void __attribute__((weak))
pal_inform_bic_mode(uint8_t fru, uint8_t mode)
{
  return;
}

void __attribute__((weak))
pal_update_ts_sled(void)
{
  return;
}

int __attribute__((weak))
pal_handle_dcmi(uint8_t fru, uint8_t *tbuf, uint8_t tlen, uint8_t *rbuf, uint8_t *rlen)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_is_fru_ready(uint8_t fru, uint8_t *status)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_is_slot_server(uint8_t fru)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_is_slot_support_update(uint8_t fru)
{
  return pal_is_slot_server(fru);
}

int __attribute__((weak))
pal_self_tray_location(uint8_t *value)
{
  return PAL_EOK;
}

void __attribute__((weak))
pal_log_clear(char *fru)
{
  return;
}

int __attribute__((weak))
pal_get_dev_guid(uint8_t fru, char *guid)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_dev_guid(uint8_t fru, char *guid)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_get_plat_sku_id(void)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_is_test_board(void)
{
  //Non Test Board:0
  return 0;
}

int __attribute__((weak))
pal_get_fw_info(uint8_t fru, unsigned char target, unsigned char* res, unsigned char* res_len)
{
  return PAL_EOK;
}

void __attribute__((weak))
pal_i2c_crash_assert_handle(int i2c_bus_num)
{
  return;
}

void __attribute__((weak))
pal_i2c_crash_deassert_handle(int i2c_bus_num)
{
  return;
}

int __attribute__((weak))
pal_is_cplddump_ongoing(uint8_t fru)
{
  return PAL_EOK;
}

bool __attribute__((weak))
pal_is_cplddump_ongoing_system(void)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_is_crashdump_ongoing(uint8_t fru)
{
  char fname[128];
  char value[MAX_VALUE_LEN] = {0};
  struct timespec ts;
  int ret;

  //if pid file not exist, return false
  sprintf(fname, "/var/run/autodump%d.pid", fru);
  if ( access(fname, F_OK) != 0 )
  {
    return 0;
  }

  //check the crashdump file in /tmp/cache_store/fru$1_crashdump
  sprintf(fname, "fru%d_crashdump", fru);
  ret = kv_get(fname, value, NULL, 0);
  if (ret < 0)
  {
     return 0;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (strtoul(value, NULL, 10) > ts.tv_sec)
  {
     return 1;
  }

  //over the threshold time, return false
  return 0;                     /* false */
}

bool __attribute__((weak))
pal_is_crashdump_ongoing_system(void)
{
  //Base on fru number to check if autodump is onging.
  uint8_t max_slot_num = 0;
  int i;

  pal_get_num_slots(&max_slot_num);

  for(i = 1; i <= max_slot_num; i++) //fru start from 1
  {
    int fruid = pal_slotid_to_fruid(i);
    if ( 1 == pal_is_crashdump_ongoing(fruid) )
    {
      return true;
    }
  }

  return false;
}

int __attribute__((weak))
pal_open_fw_update_flag(void) {
  return -1;
}

int __attribute__((weak))
pal_remove_fw_update_flag(void) {
  return -1;
}

int __attribute__((weak))
pal_get_fw_update_flag(void)
{
  return 0;
}

int __attribute__((weak))
pal_set_machine_configuration(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_handle_string_sel(char *log, uint8_t log_len)
{
  return PAL_EOK;
}

int __attribute__((weak))
pal_set_adr_trigger(uint8_t slot, bool trigger)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_flock_flag_retry(int fd, unsigned int flag)
{
  int ret = 0;
  int retry_count = 0;

  ret = flock(fd, flag);
  while (ret && (retry_count < 3)) {
    retry_count++;
    msleep(100);
    ret = flock(fd, flag);
  }
  if (ret) {
    return -1;
  }

  return 0;
}

int __attribute__((weak))
pal_flock_retry(int fd)
{
  int ret = 0;
  int retry_count = 0;

  ret = flock(fd, LOCK_EX | LOCK_NB);
  while (ret && (retry_count < 3)) {
    retry_count++;
    msleep(100);
    ret = flock(fd, LOCK_EX | LOCK_NB);
  }
  if (ret) {
    return -1;
  }

  return 0;
}

int __attribute__((weak))
pal_unflock_retry(int fd)
{
  int ret = 0;
  int retry_count = 0;

  ret = flock(fd, LOCK_UN);
  while (ret && (retry_count < 3)) {
    retry_count++;
    msleep(100);
    ret = flock(fd, LOCK_UN);
  }
  if (ret) {
    return -1;
  }

  return 0;
}

int __attribute__((weak))
pal_slotid_to_fruid(int slotid)
{
  // This function is for mapping to fruid from slotid
  // If the platform's slotid is different with fruid, need to rewrite
  // this function in project layer.
  return slotid;
}

int __attribute__((weak))
pal_devnum_to_fruid(int devnum)
{
  // This function is for mapping to fruid from devnum
  // If the platform's devnum is different with fruid, need to rewrite
  // this function in project layer.
  return devnum;
}

int __attribute__((weak))
pal_channel_to_bus(int channel)
{
  // This function is for mapping to bus from channel
  // If the platform's channel is different with bus, need to rewrite
  // this function in project layer.
  return channel;
}

int __attribute__((weak))
pal_set_fw_update_ongoing(uint8_t fruid, uint16_t tmout) {
  char key[64] = {0};
  char value[64] = {0};
  struct timespec ts;

  sprintf(key, "fru%d_fwupd", fruid);

  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += tmout;
  sprintf(value, "%ld", ts.tv_sec);

  if (kv_set(key, value, 0, 0) < 0) {
     return -1;
  }

  return 0;
}

bool __attribute__((weak))
pal_is_fw_update_ongoing(uint8_t fruid) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  int ret;
  struct timespec ts;

  sprintf(key, "fru%d_fwupd", fruid);
  ret = kv_get(key, value, NULL, 0);
  if (ret < 0) {
     return false;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (strtoul(value, NULL, 10) > ts.tv_sec)
     return true;

  return false;
}

bool __attribute__((weak))
pal_is_fw_update_ongoing_system(void) {
  //Base on fru number to sum up if fw update is onging.
  uint8_t max_slot_num = 0;
  int i;

  pal_get_num_slots(&max_slot_num);

  for(i = 0; i <= max_slot_num; i++) { // 0 is reserved for BMC update
    int fruid = pal_slotid_to_fruid(i);
    if (pal_is_fw_update_ongoing(fruid) == true) //if any slot is true, then we can return true
      return true;
  }

  return false;
}

bool __attribute__((weak))
pal_can_change_power(uint8_t fru)
{
  char fruname[32];
  if (pal_get_fru_name(fru, fruname)) {
    sprintf(fruname, "fru%d", fru);
  }
  if (pal_is_fw_update_ongoing(fru)) {
    printf("FW update for %s is ongoing, block the power controlling.\n", fruname);
    return false;
  }
  if (pal_is_crashdump_ongoing(fru)) {
    printf("Crashdump for %s is ongoing, block the power controlling.\n", fruname);
    return false;
  }
  if (pal_is_cplddump_ongoing(fru)) {
    printf("CPLD dump for %s is ongoing, block the power controlling.\n", fruname);
    return false;
  }
  return true;
}

int __attribute__((weak))
pal_set_fw_update_state(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_EOK;
}

int __attribute__((weak))
run_command(const char* cmd) {
  int status = system(cmd);
  if (status == -1) { // system error or environment error
    return 127;
  }

  // WIFEXITED(status): cmd is executed complete or not. return true for success.
  // WEXITSTATUS(status): cmd exit code
  if (WIFEXITED(status) && (WEXITSTATUS(status) == 0))
    return 0;
  else
    return -1;
}

int __attribute__((weak))
pal_get_restart_cause(uint8_t slot, uint8_t *restart_cause) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  unsigned int cause;

  sprintf(key, "fru%d_restart_cause", slot);

  if (kv_get(key, value, NULL, KV_FPERSIST)) {
    return -1;
  }
  if(sscanf(value, "%u", &cause) != 1) {
    return -1;
  }
  *restart_cause = cause;
  return 0;
}

int __attribute__((weak))
pal_set_restart_cause(uint8_t slot, uint8_t restart_cause) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};

  sprintf(key, "fru%d_restart_cause", slot);
  sprintf(value, "%d", restart_cause);

  if (kv_set(key, value, 0, KV_FPERSIST)) {
    return -1;
  }
  return 0;
}

int __attribute__((weak))
pal_get_nm_selftest_result(uint8_t fruid, uint8_t *data)
{
  return PAL_ENOTSUP;
}


int __attribute__((weak))
pal_handle_oem_1s_intr(uint8_t slot, uint8_t *data)
{
  return 0;
}

int __attribute__((weak))
pal_handle_oem_1s_asd_msg_in(uint8_t slot, uint8_t *data, uint8_t data_len)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_handle_oem_1s_ras_dump_in(uint8_t slot, uint8_t *data, uint8_t data_len)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_set_gpio_value(int gpio_num, uint8_t value) {
  char vpath[64] = {0};
  char *val;
  FILE *fp = NULL;
  int rc = 0;
  int ret = 0;
  int retry_cnt = 5;
  int i = 0;

  sprintf(vpath, GPIO_VAL, gpio_num);
  val = (value == 0) ? "0": "1";

  for (i = 0; i < retry_cnt; i++) {
    fp = fopen(vpath, "w");
    if (fp == NULL) {
      syslog(LOG_ERR, "%s(): failed to open device %s (%s)",
                       __func__, vpath, strerror(errno));
      if (i == (retry_cnt - 1)) {
        return errno;
      }
    } else {
      break;
    }
    msleep(100);
  }
  for (i = 0; i < retry_cnt; i++) {
    ret = 0;
    rc = fputs(val, fp);
    if (rc < 0) {
      syslog(LOG_ERR, "failed to write device %s (%s)", vpath, strerror(errno));
      if (i == (retry_cnt - 1)) {
        ret = errno;
      }
    } else {
      break;
    }
    msleep(100);
  }

  fclose(fp);

  return ret;
}

int __attribute__((weak))
pal_get_gpio_value(int gpio_num, uint8_t *value) {
  char vpath[64] = {0};
  FILE *fp = NULL;
  int rc = 0;
  int ret = 0;
  int retry_cnt = 5;
  int i = 0;

  sprintf(vpath, GPIO_VAL, gpio_num);

  for (i = 0; i < retry_cnt; i++) {
    fp = fopen(vpath, "r");
    if (fp == NULL) {
      syslog(LOG_ERR, "%s(): failed to open device %s (%s)",
                       __func__, vpath, strerror(errno));
      if (i == (retry_cnt - 1)) {
        return errno;
      }
    } else {
      break;
    }
    msleep(100);
  }
  for (i = 0; i < retry_cnt; i++) {
    int ival;
    ret = 0;
    rc = fscanf(fp, "%d", &ival);
    if (rc != 1) {
      syslog(LOG_ERR, "failed to read device %s (%s)", vpath, strerror(errno));
      if (i == (retry_cnt - 1)) {
        ret = errno;
      }
    } else {
      *value = (uint8_t)ival;
      break;
    }
    msleep(100);
  }

  fclose(fp);

  return ret;
}

int __attribute__((weak))
pal_ipmb_processing(int bus, void *buf, uint16_t size)
{
  return 0;
}

int __attribute__((weak))
pal_ipmb_finished(int bus, void *buf, uint16_t size)
{
  return 0;
}

// Helper functions for some of PAL routines
void __attribute__((weak))
 msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

int __attribute__((weak))
pal_bypass_cmd(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  // Completion Code Invalid Command
  return 0xC1;
}

int __attribute__((weak))
pal_bypass_dev_card(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  // Completion Code Invalid Command
  return 0xC1;
}

void __attribute__((weak))
pal_set_def_restart_cause(uint8_t slot) {
  uint8_t cause;
  if (pal_get_restart_cause(slot, &cause)) {
    // If no restart cause is set, set the default to be
    // PWR_ON_PUSH_BUTTON since that is the most obvious cause
    // since BMC has just booted up and started the ipmid.
    pal_set_restart_cause(slot, RESTART_CAUSE_PWR_ON_PUSH_BUTTON);
  }
}

int __attribute__((weak))
pal_compare_fru_data(char *fru_out, char *fru_in, int cmp_size)
{
  FILE *fru_in_fd=NULL;
  FILE *fru_out_fd=NULL;
  uint8_t *fru_in_arr=NULL;
  uint8_t *fru_out_arr=NULL;
  int ret=PAL_EOK;
  int size=0;
  int i;

  //get the size
  size=cmp_size * sizeof(uint8_t);

  //open the fru_in file
  fru_in_fd = fopen(fru_in, "rb");
  if ( NULL == fru_in_fd ) {
    syslog(LOG_WARNING, "[%s] unable to open the file: %s", __func__, fru_in);
    ret=PAL_ENOTSUP;
    goto error_exit;
  }

  //open the fru_out file
  fru_out_fd = fopen(fru_out, "rb");
  if ( NULL == fru_out_fd ) {
    syslog(LOG_WARNING, "[%s] unable to open the file: %s", __func__, fru_out);
    ret=PAL_ENOTSUP;
    goto error_exit;
  }

  //get the fru_in data
  fru_in_arr = (uint8_t*) malloc(size);
  ret = fread(fru_in_arr, sizeof(uint8_t), size, fru_in_fd);
  if ( ret != size )
  {
    syslog(LOG_WARNING, "[%s] Get fru_in data fail", __func__);
    ret=PAL_ENOTSUP;
    goto error_exit;
  }

#ifdef FRU_DEBUG
  syslog(LOG_WARNING,"[%s] Print Read_in", __func__);
  for ( i=0; i<size; i++ )
  {
    syslog(LOG_WARNING, "[%s]ReadIn[%d]=%x", __func__, i, fru_in_arr[i]);
  }
#endif

  //get the fru_out data
  fru_out_arr = (uint8_t*) malloc(size);
  ret=fread(fru_out_arr, sizeof(uint8_t), size, fru_out_fd);
  if ( ret != size )
  {
    syslog(LOG_WARNING, "[%s] Get fru_out data fail", __func__);
    ret=PAL_ENOTSUP;
    goto error_exit;
  }

#ifdef FRU_DEBUG
  syslog(LOG_WARNING,"[%s] Print Read_out", __func__);
  for ( i=0; i<size; i++ )
  {
    syslog(LOG_WARNING, "[%s]ReadOut[%d]=%x", __func__, i, fru_out_arr[i]);
  }

#endif

  for ( i=0; i<size; i++ )
  {
    if ( fru_in_arr[i] != fru_out_arr[i] )
    {
      printf("[%s]FRU Comparison Fail. Data Mismatch\n", __func__);
      syslog(LOG_WARNING, "[%s]FRU Comparison Fail. Data Mismatch", __func__);
      syslog(LOG_WARNING, "[%s]Index:%d In:%x Out:%x", __func__, i, fru_in_arr[i], fru_out_arr[i]);
      ret=PAL_ENOTSUP;
      goto error_exit;
    }
  }

  ret=PAL_EOK;

error_exit:

  if ( NULL != fru_in_fd )
  {
     fclose(fru_in_fd);
  }

  if ( NULL != fru_out_fd )
  {
     fclose(fru_out_fd);
  }

  if ( NULL != fru_in_arr )
  {
    free(fru_in_arr);
  }

  if ( NULL != fru_out_arr )
  {
    free(fru_out_arr);
  }

  return ret;
}

void __attribute__((weak))
pal_get_me_name(uint8_t fru, char *target_name) {
  strcpy(target_name, "ME");
  return;
}

int __attribute__((weak))
pal_set_tpm_physical_presence(uint8_t slot, uint8_t presence) {
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_get_tpm_physical_presence(uint8_t slot) {
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_create_TPMTimer(int fru) {
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_force_update_bic_fw(uint8_t slot_id, uint8_t comp, char *path) {
  return -2;  //means not support
}

void __attribute__((weak))
pal_specific_plat_fan_check(bool status)
{
  return;
}

bool __attribute__((weak))
pal_get_pair_fru(uint8_t slot_id, uint8_t *pair_fru)
{
  return false;
}

char * __attribute__((weak))
pal_get_pwn_list(void)
{
  return pal_pwm_list;
}

char * __attribute__((weak))
pal_get_tach_list(void)
{
  return pal_tach_list;
}

char * __attribute__((weak))
pal_get_fan_opt_list(void)
{
  return pal_fan_opt_list;
}

int __attribute__((weak))
pal_get_pwm_cnt(void)
{
  return pal_pwm_cnt;
}

int __attribute__((weak))
pal_get_tach_cnt(void)
{
  return pal_tach_cnt;
}

int __attribute__((weak))
pal_get_fan_opt_cnt(void)
{
  return pal_fan_opt_cnt;
}

int __attribute__((weak))
pal_set_fan_ctrl(char *ctrl_opt)
{
  return -1;
}

int __attribute__((weak))
pal_set_time_sync(uint8_t *req_data, uint8_t req_len)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_get_nic_fru_id(void)
{
  return -1;
}

int __attribute__((weak))
pal_get_bmc_ipmb_slave_addr(uint16_t *slave_addr, uint8_t bus_id)
{
  *slave_addr = BMC_SLAVE_ADDR;
  return 0;
}

int __attribute__((weak))
pal_is_mcu_ready(uint8_t bus)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_wait_mcu_ready2update(uint8_t bus)
{
  sleep(2);
  return 0;
}

int __attribute__((weak))
pal_set_sdr_update_flag(uint8_t slot, uint8_t update) {
  return 0;
}

int __attribute__((weak))
pal_get_sdr_update_flag(uint8_t slot) {
  return 0;
}

int __attribute__((weak))
pal_fw_update_prepare(uint8_t fru, const char *comp) {
  return 0;
}

int __attribute__((weak))
pal_fw_update_finished(uint8_t fru, const char *comp, int status) {
  return status;
}

bool __attribute__((weak))
pal_is_modify_sel_time(uint8_t *sel, int size) {

  return false;
}

int __attribute__((weak))
pal_update_sensor_reading_sdr (uint8_t fru) {
  return 0;
}

int __attribute__((weak))
pal_get_80port_page_record(uint8_t slot, uint8_t page_num, uint8_t *buf, size_t max_len, size_t *len) {
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_set_usb_path (uint8_t slot, uint8_t endpoint) {
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_display_4byte_post_code(uint8_t slot, uint32_t postcode_dw)
{
  return 0;
}

void __attribute__((weak))
pal_get_eth_intf_name(char* intf_name) {
  snprintf(intf_name, 8, "eth0");
}

int __attribute__((weak))
pal_get_host_system_mode(uint8_t *mode) {
  return PAL_ENOTSUP;
}

uint8_t __attribute__((weak))
pal_ipmb_get_sensor_val(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  *res_len = 0;
  return 0;
}

int __attribute__((weak))
pal_sensor_monitor_initial(void) {
  return 0;
}

int __attribute__((weak))
pal_sensor_thresh_init(void)
{
  return 0;
}

int __attribute__((weak))
pal_set_host_system_mode(uint8_t mode) {
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_is_pfr_active(void) {
  return PFR_NONE;
}

int __attribute__((weak))
pal_get_pfr_address(uint8_t fru, uint8_t *bus, uint8_t *addr, bool *bridged)
{
  return -1;
}

int __attribute__((weak))
pal_get_pfr_update_address(uint8_t fru, uint8_t *bus, uint8_t *addr, bool *bridged)
{
  return -1;
}

int __attribute__((weak))
pal_get_dev_card_sensor(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_set_bios_cap_fw_ver(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  return PAL_ENOTSUP;
}

int __attribute__((weak))
pal_is_sensor_valid(uint8_t fru, uint8_t snr_num) {
  return 0;
}

int __attribute__((weak))
pal_get_fw_ver(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len) {
  return PAL_ENOTSUP;
}

bool __attribute__((weak))
pal_is_aggregate_snr_valid(uint8_t snr_num) {
  return true;
}

int __attribute__((weak))
pal_set_ioc_fw_recovery(uint8_t *ioc_recovery_setting, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  return CC_NOT_SUPP_IN_CURR_STATE;
}

int __attribute__((weak))
pal_get_ioc_fw_recovery(uint8_t ioc_recovery_component, uint8_t *res_data, uint8_t *res_len) {
  return CC_NOT_SUPP_IN_CURR_STATE;
}

int __attribute__((weak))
pal_setup_exp_uart_bridging(void) {
  return CC_NOT_SUPP_IN_CURR_STATE;
}

int __attribute__((weak))
pal_teardown_exp_uart_bridging(void) {
  return CC_NOT_SUPP_IN_CURR_STATE;
}

int __attribute__((weak))
pal_convert_sensor_reading(sdr_full_t *sdr, int in_value, float *out_value) {
  uint8_t m_lsb = 0, m_msb = 0;
  uint16_t m = 0;
  uint8_t b_lsb = 0, b_msb = 0;
  uint16_t b = 0;
  int8_t b_exp = 0, r_exp = 0;

  if (sdr == NULL || out_value == NULL) {
    syslog(LOG_ERR, "%s() Failed to convert sensor reading: input parameter is NULL", __func__);
    return -1;
  }

  m_lsb = sdr->m_val;
  m_msb = sdr->m_tolerance >> 6;
  m = (m_msb << 8) | m_lsb;

  b_lsb = sdr->b_val;
  b_msb = sdr->b_accuracy >> 6;
  b = (b_msb << 8) | b_lsb;

  // exponents are 2's complement 4-bit number
  b_exp = sdr->rb_exp & 0xF;
  if (b_exp > 7) {
    b_exp = (~b_exp + 1) & 0xF;
    b_exp = -b_exp;
  }

  r_exp = (sdr->rb_exp >> 4) & 0xF;
  if (r_exp > 7) {
    r_exp = (~r_exp + 1) & 0xF;
    r_exp = -r_exp;
  }

  *out_value = (float)(((m * in_value) + (b * pow(10, b_exp))) * (pow(10, r_exp)));

  return 0;
}
