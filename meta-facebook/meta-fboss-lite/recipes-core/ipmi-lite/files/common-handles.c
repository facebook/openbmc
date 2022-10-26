/* Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This file defines all common helper functions for IPMIv2 and the
 * configuration parsing.
 * for those platform-specific helper functions, to be defined in the
 * plat_support_api.c file.
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
#include "common-handles.h"
#include <cjson/cJSON.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <openbmc/kv.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include "cfg-parse.h"
#include "plat.h"

struct threadinfo {
  uint8_t is_running;
  uint8_t fru;
  pthread_t pt;
};

static struct threadinfo t_dump[MAX_NUM_FRUS] = {
    0,
};

int run_command(const char* cmd) {
  int status = system(cmd);
  if (status == -1) { // system error or environment error
    return 127;
  }

  // WIFEXITED(status): cmd is executed complete or not. return true for
  // success. WEXITSTATUS(status): cmd exit code
  if (WIFEXITED(status) && (WEXITSTATUS(status) == 0))
    return 0;
  else
    return -1;
}

// Use part of the function for OEM Command "CMD_OEM_GET_POSS_PCIE_CONFIG" 0xF4
int ipmi_get_poss_pcie_config(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t req_len,
    uint8_t* res_data,
    uint8_t* res_len) {
  uint8_t pcie_conf = 0x02; // Elbert
  uint8_t* data = res_data;
  bool bv = false;

  if (!get_cfg_item_bool("poss_pcie_config", &bv) && bv) {
    *data++ = pcie_conf;
    *res_len = data - res_data;
  }
  return CC_SUCCESS;
}

uint8_t ipmi_set_power_restore_policy(
    uint8_t slot,
    uint8_t* pwr_policy,
    uint8_t* res_data) {
  uint8_t completion_code = CC_SUCCESS; /* Fill response with default values */
  const char* key = "server_por_cfg";
  unsigned char policy = *pwr_policy & 0x07; /* Power restore policy */
  bool bv = false;

  if (!get_cfg_item_bool("set_power_restore_policy", &bv) && bv) {
    switch (policy) {
      case 0:
        if (kv_set(key, "off", 0, KV_FPERSIST) != 0)
          completion_code = CC_UNSPECIFIED_ERROR;
        break;
      case 1:
        if (kv_set(key, "lps", 0, KV_FPERSIST) != 0)
          completion_code = CC_UNSPECIFIED_ERROR;
        break;
      case 2:
        if (kv_set(key, "on", 0, KV_FPERSIST) != 0)
          completion_code = CC_UNSPECIFIED_ERROR;
        break;
      case 3:
        /* no change (just get present policy support) */
        break;
      default:
        completion_code = CC_PARAM_OUT_OF_RANGE;
        break;
    }
  }
  return completion_code;
}

int ipmi_get_sysfw_ver(uint8_t slot, uint8_t* ver) {
  int i = 0, j = 0;
  int ret;
  int msb, lsb;
  const char* key = "sysfw_ver_server";
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4] = {0};

  if (!get_cfg_item_kv(key)) {
    if (ver == NULL) {
      syslog(LOG_ERR, "%s() Pointer \"ver\" is NULL.\n", __func__);
      return -1;
    }

    ret = kv_get(key, str, NULL, KV_FPERSIST);
    if (ret) {
      return ret;
    }

    for (i = 0; i < 2 * SIZE_SYSFW_VER; i += 2) {
      snprintf(tstr, sizeof(tstr), "%c\n", str[i]);
      msb = strtol(tstr, NULL, 16);

      snprintf(tstr, sizeof(tstr), "%c\n", str[i + 1]);
      lsb = strtol(tstr, NULL, 16);
      ver[j++] = (msb << 4) | lsb;
    }
  }
  return 0;
}

uint8_t ipmi_set_slot_power_policy(uint8_t* pwr_policy, uint8_t* res_data) {
  return 0;
}

int ipmi_get_boot_order(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t* boot,
    uint8_t* res_len) {
  int ret, msb, lsb, i, j = 0;
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4];
  const char* key = "server_boot_order";

  if (get_cfg_item_kv(key)) {
    return CC_INVALID_CMD;
  }

  ret = kv_get(key, str, NULL, KV_FPERSIST);
  if (ret) {
    *res_len = 0;
    return ret;
  }

  for (i = 0; i < 2 * SIZE_BOOT_ORDER; i += 2) {
    snprintf(tstr, sizeof(tstr), "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    snprintf(tstr, sizeof(tstr), "%c\n", str[i + 1]);
    lsb = strtol(tstr, NULL, 16);
    boot[j++] = (msb << 4) | lsb;
  }
  *res_len = SIZE_BOOT_ORDER;

  return 0;
}

int ipmi_set_boot_order(
    uint8_t slot,
    uint8_t* boot,
    uint8_t* res_data,
    uint8_t* res_len) {
  int i, j, offset, network_dev = 0;
  char str[MAX_VALUE_LEN] = {0};
  enum {
    BOOT_DEVICE_IPV4 = 0x1,
    BOOT_DEVICE_IPV6 = 0x9,
  };
  const char* key = "server_boot_order";

  *res_len = 0;
  if (get_cfg_item_kv(key)) {
    return CC_INVALID_CMD;
  }

  for (i = offset = 0; i < SIZE_BOOT_ORDER && offset < sizeof(str); i++) {
    if (i > 0) { // byte[0] is boot mode, byte[1:5] are boot order
      for (j = i + 1; j < SIZE_BOOT_ORDER; j++) {
        if (boot[i] == boot[j])
          return CC_INVALID_PARAM;
      }

      // If bit[2:0] is 001b (Network), bit[3] is IPv4/IPv6 order
      // bit[3]=0b: IPv4 first
      // bit[3]=1b: IPv6 first
      if ((boot[i] == BOOT_DEVICE_IPV4) || (boot[i] == BOOT_DEVICE_IPV6))
        network_dev++;
    }

    offset += snprintf(str + offset, sizeof(str) - offset, "%02x", boot[i]);
  }

  // not allow having more than 1 network boot device in the boot order
  if (network_dev > 1)
    return CC_INVALID_PARAM;

  return kv_set(key, str, 0, KV_FPERSIST);
}

// By default, add_cri_sel will do the best effort to
// log critical messages to default log file.
// Some platform has already overriden this function with
// empty function (do not log anything)
void add_cri_sel(char* str) {
  syslog(LOG_LOCAL0 | LOG_ERR, "%s", str);
}

void ipmi_update_ts_sled(void) {
  const char* key = "timestamp_sled";
  char tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts;
  bool bv = false;

  if (!get_cfg_item_bool("update_timestamp_sled", &bv) && bv) {
    clock_gettime(CLOCK_REALTIME, &ts);
    sprintf(tstr, "%ld", ts.tv_sec);
    kv_set(key, tstr, 0, KV_FPERSIST);
  }
}

int get_restart_cause(uint8_t slot, uint8_t* restart_cause) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  unsigned int cause;

  sprintf(key, "fru%d_restart_cause", slot);
  if (kv_get(key, value, NULL, KV_FPERSIST)) {
    return -1;
  }
  if (sscanf(value, "%u", &cause) != 1) {
    return -1;
  }
  *restart_cause = cause;
  return 0;
}

int set_restart_cause(uint8_t slot, uint8_t restart_cause) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};

  sprintf(key, "fru%d_restart_cause", slot);
  sprintf(value, "%d", restart_cause);

  if (kv_set(key, value, 0, KV_FPERSIST)) {
    return -1;
  }
  return 0;
}

// Read output from sysfs into buffer
static int read_sysfs(char* path, char* data, int data_size) {
  int fd;
  int ret;
  int bytes_read;

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    ret = errno;
    close(fd);
    return ret;
  }

  bytes_read = read(fd, data, data_size);
  if (bytes_read == data_size) {
    ret = 0;
  } else {
    ret = -1;
  }

  close(fd);
  return ret;
}

void msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while (nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

/*
 *  Default implementation if is_fru_x86 is to always return true
 *  on all FRU, on all platform. This is because all uServers so far are X86.
 *  This logic can be "OVERRIDDEN" as needed, on any newer platform,
 *  in order to return dynamic value, based on the information which
 *  BMC collected so far.
 *  (therefore mix-and-match supported; not sure if needed.)
 */
static bool is_fru_x86(uint8_t fru) {
  return true;
}

static int get_x86_event_sensor_name(uint8_t fru, uint8_t snr_num, char* name) {
  if (is_fru_x86(fru)) {
    switch (snr_num) {
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

bool ipmi_parse_sel_helper(uint8_t fru, uint8_t* sel, char* error_log) {
  bool parsed = true;
  uint8_t snr_type = sel[10];
  uint8_t snr_num = sel[11];
  uint8_t* event_data = &sel[10];
  uint8_t* ed = &event_data[3];
  char temp_log[512] = {0};
  uint8_t temp;
  uint8_t sen_type = event_data[0];
  uint8_t chn_num, dimm_num;
  uint8_t idx;

  /*Used by decoding ME event*/
  char* nm_capability_status[2] = {"Not Available", "Available"};
  char* nm_domain_name[6] = {
      "Entire Platform",
      "CPU Subsystem",
      "Memory Subsystem",
      "HW Protection",
      "High Power I/O subsystem",
      "Unknown"};
  char* nm_err_type[17] = {
      "Unknown",
      "Unknown",
      "Unknown",
      "Unknown",
      "Unknown",
      "Unknown",
      "Unknown",
      "Extended Telemetry Device Reading Failure",
      "Outlet Temperature Reading Failure",
      "Volumetric Airflow Reading Failure",
      "Policy Misconfiguration",
      "Power Sensor Reading Failure",
      "Inlet Temperature Reading Failure",
      "Host Communication Error",
      "Real-time Clock Synchronization Failure",
      "Platform Shutdown Initiated by Intel NM Policy",
      "Unknown"};
  char* nm_health_type[4] = {"Unknown", "Unknown", "SensorIntelNM", "Unknown"};
  const char* thres_event_name[16] = {
      [0] = "Lower Non-critical",
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

  switch (snr_num) {
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
        else if (ed[1] == 0x80)
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

      sprintf(temp_log, "CRITICAL_IRQ, %s,FRU:%u", error_log, fru);
      add_cri_sel(temp_log);

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
        sprintf(
            temp_log,
            "MACHINE_CHK_ERR, %s bank Number %d,FRU:%u",
            error_log,
            ed[1],
            fru);
        add_cri_sel(temp_log);
      } else if ((ed[0] & 0x0F) == 0x0C) {
        strcat(error_log, "Correctable");
        sprintf(
            temp_log,
            "MACHINE_CHK_ERR, %s bank Number %d,FRU:%u",
            error_log,
            ed[1],
            fru);
        add_cri_sel(temp_log);
      } else {
        strcat(error_log, "Unknown");
        sprintf(
            temp_log,
            "MACHINE_CHK_ERR, %s bank Number %d,FRU:%u",
            error_log,
            ed[1],
            fru);
        add_cri_sel(temp_log);
      }

      sprintf(temp_log, ", Machine Check bank Number %d ", ed[1]);
      strcat(error_log, temp_log);
      sprintf(temp_log, ", CPU %d, Core %d ", ed[2] >> 5, ed[2] & 0x1F);
      strcat(error_log, temp_log);

      break;

    case PCIE_ERR:

      if ((ed[0] & 0xF) == 0x4) {
        sprintf(
            error_log,
            "PCI PERR (Bus %02X / Dev %02X / Fun %02X)",
            ed[2],
            ed[1] >> 3,
            ed[1] & 0x7);
      } else if ((ed[0] & 0xF) == 0x5) {
        sprintf(
            error_log,
            "PCI SERR (Bus %02X / Dev %02X / Fun %02X)",
            ed[2],
            ed[1] >> 3,
            ed[1] & 0x7);
      } else if ((ed[0] & 0xF) == 0x7) {
        sprintf(
            error_log,
            "Correctable (Bus %02X / Dev %02X / Fun %02X)",
            ed[2],
            ed[1] >> 3,
            ed[1] & 0x7);
      } else if ((ed[0] & 0xF) == 0x8) {
        sprintf(
            error_log,
            "Uncorrectable (Bus %02X / Dev %02X / Fun %02X)",
            ed[2],
            ed[1] >> 3,
            ed[1] & 0x7);
      } else if ((ed[0] & 0xF) == 0xA) {
        sprintf(
            error_log,
            "Bus Fatal (Bus %02X / Dev %02X / Fun %02X)",
            ed[2],
            ed[1] >> 3,
            ed[1] & 0x7);
      } else if ((ed[0] & 0xF) == 0xD) {
        unsigned int vendor_id = (unsigned int)ed[1] << 8 | (unsigned int)ed[2];
        sprintf(error_log, "Vendor ID: 0x%4x", vendor_id);
      } else if ((ed[0] & 0xF) == 0xE) {
        unsigned int device_id = (unsigned int)ed[1] << 8 | (unsigned int)ed[2];
        sprintf(error_log, "Device ID: 0x%4x", device_id);
      } else if ((ed[0] & 0xF) == 0xF) {
        sprintf(
            error_log, "Error ID from downstream: 0x%2x 0x%2x", ed[1], ed[2]);
      } else {
        strcat(error_log, "Unknown");
      }

      sprintf(temp_log, "PCI_ERR %s,FRU:%u", error_log, fru);
      add_cri_sel(temp_log);

      break;

    case IIO_ERR:
      if ((ed[0] & 0xF) == 0) {
        sprintf(temp_log, "CPU %d, Error ID 0x%X", (ed[2] & 0xE0) >> 5, ed[1]);
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
      add_cri_sel(temp_log);
      break;

    case MEMORY_ECC_ERR:
    case MEMORY_ERR_LOG_DIS:
      if (snr_num == MEMORY_ECC_ERR) {
        // SEL from MEMORY_ECC_ERR Sensor
        if ((ed[0] & 0x0F) == 0x0) {
          if (sen_type == 0x0C) {
            strcat(error_log, "Correctable");
            sprintf(temp_log, "DIMM%02X ECC err,FRU:%u", ed[2], fru);
            add_cri_sel(temp_log);
          } else if (sen_type == 0x10)
            strcat(error_log, "Correctable ECC error Logging Disabled");
        } else if ((ed[0] & 0x0F) == 0x1) {
          strcat(error_log, "Uncorrectable");
          sprintf(temp_log, "DIMM%02X UECC err,FRU:%u", ed[2], fru);
          add_cri_sel(temp_log);
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
            sprintf(
                temp_log,
                "DIMM%c%d ECC err,FRU:%u",
                'A' + chn_num,
                dimm_num,
                fru);
            add_cri_sel(temp_log);
          } else if ((ed[0] & 0x0F) == 0x1) {
            sprintf(
                temp_log,
                "DIMM%c%d UECC err,FRU:%u",
                'A' + chn_num,
                dimm_num,
                fru);
            add_cri_sel(temp_log);
          }
        }
        /* Then continue parse the error into a string. */
        /* All Info Valid                               */
        sprintf(
            temp_log,
            " (CPU# %d, CHN# %d, DIMM# %d)",
            (ed[2] & 0xE0) >> 5,
            (ed[2] & 0x18) >> 3,
            ed[2] & 0x7);
      } else if (((ed[1] & 0xC) >> 2) == 0x1) {
        /* DIMM info not valid */
        sprintf(
            temp_log,
            " (CPU# %d, CHN# %d)",
            (ed[2] & 0xE0) >> 5,
            (ed[2] & 0x18) >> 3);
      } else if (((ed[1] & 0xC) >> 2) == 0x2) {
        /* CHN info not valid */
        sprintf(
            temp_log, " (CPU# %d, DIMM# %d)", (ed[2] & 0xE0) >> 5, ed[2] & 0x7);
      } else if (((ed[1] & 0xC) >> 2) == 0x3) {
        /* CPU info not valid */
        sprintf(
            temp_log, " (CHN# %d, DIMM# %d)", (ed[2] & 0x18) >> 3, ed[2] & 0x7);
      }
      strcat(error_log, temp_log);

      break;

    case PWR_ERR:
      if (ed[0] == 0x1) {
        strcat(error_log, "SYS_PWROK failure");
        /* Also try logging to Critial log file, if available */
        sprintf(temp_log, "SYS_PWROK failure,FRU:%u", fru);
        add_cri_sel(temp_log);
      } else if (ed[0] == 0x2) {
        strcat(error_log, "PCH_PWROK failure");
        /* Also try logging to Critial log file, if available */
        sprintf(temp_log, "PCH_PWROK failure,FRU:%u", fru);
        add_cri_sel(temp_log);
      } else
        strcat(error_log, "Unknown");
      break;

    case CATERR_A:
    case CATERR_B:
      if (ed[0] == 0x0) {
        strcat(error_log, "IERR/CATERR");
        /* Also try logging to Critial log file, if available */
        sprintf(temp_log, "IERR,FRU:%u", fru);
        add_cri_sel(temp_log);
      } else if (ed[0] == 0xB) {
        strcat(error_log, "MCERR/CATERR");
        /* Also try logging to Critial log file, if available */
        sprintf(temp_log, "MCERR,FRU:%u", fru);
        add_cri_sel(temp_log);
      } else if (ed[0] == 0xD) {
        strcat(error_log, "MCERR/RMCA");
        /* Also try logging to Critial log file, if available */
        sprintf(temp_log, "RMCA,FRU:%u", fru);
        add_cri_sel(temp_log);
      } else
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
      add_cri_sel(temp_log);
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
          // 0x14 and 0x15 are reserved
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
      sprintf(
          temp_log,
          "%s, curr_val: %d C, thresh_val: %d C",
          thres_event_name[idx] == NULL ? "Unknown" : thres_event_name[idx],
          ed[1],
          ed[2]);
      strcat(error_log, temp_log);
      break;
    case NM_HEALTH: {
      uint8_t health_type_index = (ed[0] & 0xf);
      uint8_t domain_index = (ed[1] & 0xf);
      uint8_t err_type_index = ((ed[1] >> 4) & 0xf);

      sprintf(
          error_log,
          "%s,Domain:%s,ErrType:%s,Err:0x%x",
          nm_health_type[health_type_index],
          nm_domain_name[domain_index],
          nm_err_type[err_type_index],
          ed[2]);
    }
      return 1;
      break;

    case NM_CAPABILITIES:
      if (ed[0] & 0x7) // BIT1=policy, BIT2=monitoring, BIT3=pwr limit and the
                       // others are reserved
      {
        int capability_index = 0;
        char* policy_capability = NULL;
        char* monitoring_capability = NULL;
        char* pwr_limit_capability = NULL;

        capability_index = BIT(ed[0], 0);
        policy_capability = nm_capability_status[capability_index];

        capability_index = BIT(ed[0], 1);
        monitoring_capability = nm_capability_status[capability_index];

        capability_index = BIT(ed[0], 2);
        pwr_limit_capability = nm_capability_status[capability_index];

        sprintf(
            error_log,
            "PolicyInterface:%s,Monitoring:%s,PowerLimit:%s",
            policy_capability,
            monitoring_capability,
            pwr_limit_capability);
      } else {
        strcat(error_log, "Unknown");
      }

      return 1;
      break;

    case NM_THRESHOLD: {
      uint8_t threshold_number = (ed[0] & 0x3);
      uint8_t domain_index = (ed[1] & 0xf);
      uint8_t policy_id = ed[2];
      uint8_t policy_event_index = BIT(ed[0], 3);
      char* policy_event[2] = {
          "Threshold Exceeded", "Policy Correction Time Exceeded"};

      sprintf(
          error_log,
          "Threshold Number:%d-%s,Domain:%s,PolicyID:0x%x",
          threshold_number,
          policy_event[policy_event_index],
          nm_domain_name[domain_index],
          policy_id);
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
      if (ed[0] == 0x00)
        strcat(error_log, "Limit Not Exceeded");
      else if (ed[0] == 0x01)
        strcat(error_log, "Limit Exceeded");
      else
        strcat(error_log, "Unknown");
      break;

    case HPR_WARNING:
      if (ed[2] == 0x01) {
        if (ed[1] == 0xFF)
          strcat(temp_log, "Infinite Time");
        else
          sprintf(temp_log, "%d minutes", ed[1]);
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

int ipmi_chassis_control(uint8_t slot, uint8_t* req_data, uint8_t req_len) {
  uint8_t comp_code = CC_SUCCESS, cmd = 0, status = 0;
  bool bv = false;

  if (get_cfg_item_bool("chassis_control", &bv) || !bv) {
    return CC_INVALID_PARAM;
  }

  if (slot > FRU_SMB) {
    return CC_UNSPECIFIED_ERROR;
  }

  if (req_len != 1) {
    return CC_INVALID_LENGTH;
  }

  if (ipmi_get_server_power(FRU_SCM, &status))
    return CC_UNSPECIFIED_ERROR;

  switch (req_data[0]) {
    case 0x00: // power off
      cmd = SERVER_POWER_OFF;
      break;
    case 0x01: // power on
      cmd = SERVER_POWER_ON;
      break;
    case 0x02: // power cycle
      cmd = SERVER_POWER_CYCLE;
      break;
    case 0x03: // power reset
      cmd = SERVER_POWER_RESET;
      break;
    case 0x05: // graceful-shutdown
      cmd = SERVER_GRACEFUL_SHUTDOWN;
      break;
    default:
      return CC_INVALID_DATA_FIELD;
      break;
  }

  if (status != SERVER_POWER_ON && req_data[0] != 0x01)
    return CC_NOT_SUPP_IN_CURR_STATE;
  else if (status == SERVER_POWER_ON && req_data[0] == 0x01)
    return CC_NOT_SUPP_IN_CURR_STATE;

  if (g_fv->set_server_power(FRU_SCM, cmd))
    comp_code = CC_UNSPECIFIED_ERROR;

  return comp_code;
}

int ipmi_bmc_reboot(int cmd) {
  // sync filesystem caches
  sync();

  if (cmd == 0) {
    return run_command("/sbin/reboot");
  }

  // flag for healthd logging reboot cause
  run_command("/etc/init.d/setup-reboot.sh > /dev/null 2>&1");
  return reboot(cmd);
}

static void get_common_dimm_location_str(
    _dimm_info dimm_info,
    char* dimm_location_str,
    char* dimm_str) {
  // Check Channel and Slot
  if (dimm_info.channel == 0xFF && dimm_info.slot == 0xFF) {
    sprintf(dimm_str, "unknown");
    sprintf(
        dimm_location_str,
        "DIMM Slot Location: Sled %02d/Socket %02d, Channel unknown, Slot "
        "unknown, DIMM unknown",
        dimm_info.sled,
        dimm_info.socket);
  } else {
    dimm_info.channel &= 0x07; // Channel Bit[3:0]
    dimm_info.slot &= 0x03; // Slot [0-2]
    sprintf(dimm_str, "%c%d", 'A' + dimm_info.channel, dimm_info.slot);
    sprintf(
        dimm_location_str,
        "DIMM Slot Location: Sled %02d/Socket %02d, Channel %02d, Slot "
        "%02d, DIMM %s",
        dimm_info.sled,
        dimm_info.socket,
        dimm_info.channel,
        dimm_info.slot,
        dimm_str);
  }
}

int ipmi_parse_oem_sel(uint8_t fru, uint8_t* sel, char* error_log) {
  char crisel[128];
  uint8_t mp_mfg_id[] = {0x4c, 0x1c, 0x00};
  bool bv = false;

  if (get_cfg_item_bool("parse_oem_sel", &bv) || !bv) {
    error_log[0] = '\0';
    return 0;
  }

  if ((sel[2] == 0xC0) && !memcmp(&sel[7], mp_mfg_id, sizeof(mp_mfg_id))) {
    snprintf(crisel, sizeof(crisel), "Slot %u PCIe err,FRU:%u", sel[14], fru);
    add_cri_sel(crisel);
  }
  return 0;
}

int ipmi_parse_oem_unified_sel(uint8_t fru, uint8_t* sel, char* error_log) {
  char* mem_err[] = {
      "Memory training failure",
      "Memory correctable error",
      "Memory uncorrectable error",
      "Memory correctable error (Patrol scrub)",
      "Memory uncorrectable error (Patrol scrub)",
      "Memory Parity Error event",
      "Reserved"};
  char* upi_err[] = {"UPI Init error", "Reserved"};
  char* post_err[] = {
      "System PXE boot fail",
      "CMOS/NVRAM configuration cleared",
      "TPM Self-Test Fail",
      "Reserved"};
  char* pcie_event[] = {
      "PCIe DPC Event",
      "PCIe LER Event",
      "PCIe Link Retraining and Recovery",
      "PCIe Link CRC Error Check and Retry",
      "PCIe Corrupt Data Containment",
      "PCIe Express ECRC",
      "Reserved"};
  char* mem_event[] = {
      "Memory PPR event",
      "Memory Correctable Error logging limit reached",
      "Memory disable/map-out for FRB",
      "Memory SDDC",
      "Memory Address range/Partial mirroring",
      "Memory ADDDC",
      "Memory SMBus hang recovery",
      "No DIMM in System",
      "Reserved"};
  char* upi_event[] = {
      "Successful LLR without Phy Reinit",
      "Successful LLR with Phy Reinit",
      "COR Phy Lane failure, recovery in x8 width",
      "Reserved"};
  uint8_t general_info = sel[3];
  uint8_t error_type = general_info & 0xF;
  uint8_t plat, event_type, estr_idx;
  _dimm_info dimm_info = {
      (sel[8] >> 4) & 0x03, // Sled
      sel[8] & 0x0F, // Socket
      sel[9], // Channel
      sel[10] // Slot
  };
  char dimm_str[8] = {0};
  char dimm_location_str[128] = {0};
  char temp_log[128] = {0};
  error_log[0] = '\0';

  switch (error_type) {
    case UNIFIED_PCIE_ERR:
      plat = (general_info & 0x10) >> 4;
      if (plat == 0) { // x86
        sprintf(
            error_log,
            "GeneralInfo: x86/PCIeErr(0x%02X), Bus %02X/Dev %02X/Fun %02X, "
            "TotalErrID1Cnt: 0x%04X, ErrID2: 0x%02X, ErrID1: 0x%02X",
            general_info,
            sel[11],
            sel[10] >> 3,
            sel[10] & 0x7,
            ((sel[13] << 8) | sel[12]),
            sel[14],
            sel[15]);
      } else {
        sprintf(
            error_log,
            "GeneralInfo: ARM/PCIeErr(0x%02X), Aux. Info: 0x%04X, Bus "
            "%02X/Dev %02X/Fun %02X, TotalErrID1Cnt: 0x%04X, ErrID2: 0x%02X, "
            "ErrID1: 0x%02X",
            general_info,
            ((sel[9] << 8) | sel[8]),
            sel[11],
            sel[10] >> 3,
            sel[10] & 0x7,
            ((sel[13] << 8) | sel[12]),
            sel[14],
            sel[15]);
      }
      sprintf(
          temp_log,
          "B %02X D %02X F %02X PCIe err,FRU:%u",
          sel[11],
          sel[10] >> 3,
          sel[10] & 0x7,
          fru);
      add_cri_sel(temp_log);
      break;

    case UNIFIED_MEM_ERR:
      // get dimm location data string.
      get_common_dimm_location_str(dimm_info, dimm_location_str, dimm_str);
      plat = (sel[12] & 0x80) >> 7;
      event_type = sel[12] & 0xF;
      switch (event_type) {
        case MEMORY_TRAINING_ERR:
          if (plat == 0) { // Intel
            sprintf(
                error_log,
                "GeneralInfo: MEMORY_ECC_ERR(0x%02X), %s, DIMM Failure Event: "
                "%s, Major Code: 0x%02X, Minor Code: 0x%02X",
                general_info,
                dimm_location_str,
                mem_err[event_type],
                sel[13],
                sel[14]);
          } else { // AMD
            sprintf(
                error_log,
                "GeneralInfo: MEMORY_ECC_ERR(0x%02X), %s, DIMM Failure Event: "
                "%s, Major Code: 0x%02X, Minor Code: 0x%04X",
                general_info,
                dimm_location_str,
                mem_err[event_type],
                sel[13],
                (sel[15] << 8) | sel[14]);
          }
          sprintf(temp_log, "DIMM %s initialization fails", dimm_str);
          add_cri_sel(temp_log);
          break;

        default:
          sprintf(dimm_str, "%c%d", 'A' + dimm_info.channel, dimm_info.slot);
          estr_idx = (event_type < ARRAY_SIZE(mem_err))
              ? event_type
              : (ARRAY_SIZE(mem_err) - 1);
          sprintf(
              error_log,
              "GeneralInfo: MEMORY_ECC_ERR(0x%02X), %s, DIMM Failure Event: %s",
              general_info,
              dimm_location_str,
              mem_err[estr_idx]);

          if ((event_type == MEMORY_CORRECTABLE_ERR) ||
              (event_type == MEMORY_CORR_ERR_PTRL_SCR)) {
            sprintf(temp_log, "DIMM%s ECC err,FRU:%u", dimm_str, fru);
            add_cri_sel(temp_log);
          } else if (
              (event_type == MEMORY_UNCORRECTABLE_ERR) ||
              (event_type == MEMORY_UNCORR_ERR_PTRL_SCR)) {
            sprintf(temp_log, "DIMM%s UECC err,FRU:%u", dimm_str, fru);
            add_cri_sel(temp_log);
          }
          break;
      }
      break;

    case UNIFIED_UPI_ERR:
      event_type = sel[12] & 0xF;
      estr_idx = (event_type < ARRAY_SIZE(upi_err)) ? event_type
                                                    : (ARRAY_SIZE(upi_err) - 1);

      switch (event_type) {
        case UPI_INIT_ERR:
          sprintf(
              error_log,
              "GeneralInfo: UPIErr(0x%02X), UPI Port Location: Sled "
              "%02d/Socket %02d, Port %02d, UPI Failure Event: %s, Major Code: "
              "0x%02X, Minor Code: 0x%02X",
              general_info,
              dimm_info.sled,
              dimm_info.socket,
              sel[9] & 0xF,
              upi_err[estr_idx],
              sel[13],
              sel[14]);
          break;
        default:
          sprintf(
              error_log,
              "GeneralInfo: UPIErr(0x%02X), UPI Port Location: Sled "
              "%02d/Socket %02d, Port %02d, UPI Failure Event: %s",
              general_info,
              dimm_info.sled,
              dimm_info.socket,
              sel[9] & 0xF,
              upi_err[estr_idx]);
          break;
      }
      break;

    case UNIFIED_IIO_ERR: {
      uint8_t stack = sel[9];
      uint8_t sel_error_type = sel[13];
      uint8_t sel_error_severity = sel[14];
      uint8_t sel_error_id = sel[15];

      sprintf(
          error_log,
          "GeneralInfo: IIOErr(0x%02X), IIO Port Location: Sled %02d/Socket "
          "%02d, Stack 0x%02X, Error Type: 0x%02X, Error Severity: 0x%02X, "
          "Error ID: 0x%02X",
          general_info,
          dimm_info.sled,
          dimm_info.socket,
          stack,
          sel_error_type,
          sel_error_severity,
          sel_error_id);
      sprintf(
          temp_log,
          "IIO_ERR CPU%d. Error ID(%02X)",
          dimm_info.socket,
          sel_error_id);
      add_cri_sel(temp_log);
      break;
    }

    case UNIFIED_POST_ERR:
      event_type = sel[8] & 0xF;
      estr_idx = (event_type < ARRAY_SIZE(post_err))
          ? event_type
          : (ARRAY_SIZE(post_err) - 1);
      sprintf(
          error_log,
          "GeneralInfo: POST(0x%02X), POST Failure Event: %s",
          general_info,
          post_err[estr_idx]);
      break;

    case UNIFIED_PCIE_EVENT:
      event_type = sel[8] & 0xF;
      estr_idx = (event_type < ARRAY_SIZE(pcie_event))
          ? event_type
          : (ARRAY_SIZE(pcie_event) - 1);

      switch (event_type) {
        case PCIE_DPC:
          sprintf(
              error_log,
              "GeneralInfo: PCIeEvent(0x%02X), PCIe Failure Event: %s, Status: "
              "0x%04X, Source ID: 0x%04X",
              general_info,
              pcie_event[estr_idx],
              (sel[11] << 8) | sel[10],
              (sel[13] << 8) | sel[12]);
          break;
        default:
          sprintf(
              error_log,
              "GeneralInfo: PCIeEvent(0x%02X), PCIe Failure Event: %s",
              general_info,
              pcie_event[estr_idx]);
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
          sprintf(
              error_log,
              "GeneralInfo: MemEvent(0x%02X), %s, DIMM Failure Event: %s",
              general_info,
              dimm_location_str,
              (sel[13] & 0x01) ? "PPR fail" : "PPR success");
          break;
        case MEM_NO_DIMM:
          sprintf(
              error_log,
              "GeneralInfo: MemEvent(0x%02X), DIMM Failure Event: %s",
              general_info,
              mem_event[event_type]);
          break;
        default:
          estr_idx = (event_type < ARRAY_SIZE(mem_event))
              ? event_type
              : (ARRAY_SIZE(mem_event) - 1);
          sprintf(
              error_log,
              "GeneralInfo: MemEvent(0x%02X), %s, DIMM Failure Event: %s",
              general_info,
              dimm_location_str,
              mem_event[estr_idx]);
          break;
      }
      break;

    case UNIFIED_UPI_EVENT:
      event_type = sel[12] & 0xF;
      estr_idx = (event_type < ARRAY_SIZE(upi_event))
          ? event_type
          : (ARRAY_SIZE(upi_event) - 1);
      sprintf(
          error_log,
          "GeneralInfo: UPIEvent(0x%02X), UPI Port Location: Sled "
          "%02d/Socket %02d, Port %02d, UPI Failure Event: %s",
          general_info,
          dimm_info.sled,
          dimm_info.socket,
          sel[9] & 0xF,
          upi_event[estr_idx]);
      break;

    case UNIFIED_BOOT_GUARD:
      sprintf(
          error_log,
          "GeneralInfo: Boot Guard ACM Failure Events(0x%02X), Error "
          "Class(0x%02X), Error Code(0x%02X)",
          general_info,
          sel[8],
          sel[9]);
      break;

    default:
      sprintf(
          error_log,
          "Undefined Error Type(0x%02X), Raw: "
          "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
          error_type,
          sel[3],
          sel[4],
          sel[5],
          sel[6],
          sel[7],
          sel[8],
          sel[9],
          sel[10],
          sel[11],
          sel[12],
          sel[13],
          sel[14],
          sel[15]);
      break;
  }

  return 0;
}

void* generate_dump(void* arg) {
  uint8_t fru = *(uint8_t*)arg;
  char cmd[128];
  char fname[128];
  char fruname[16];

  // Usually the pthread cancel state are enable by default but
  // here we explicitly would like to enable them
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  g_fv->get_fru_name(fru, fruname); // scm

  snprintf(fname, sizeof(fname), "/var/run/autodump%d.pid", fru);
  if (access(fname, F_OK) == 0) {
    if (unlink(fname) != 0) {
      OBMC_ERROR(errno, "failed to delete %s", fname);
    }
  }

  // Execute automatic crashdump
  snprintf(cmd, sizeof(cmd), "%s %s", CRASHDUMP_BIN, fruname);
  RUN_SHELL_CMD(cmd);

  syslog(LOG_CRIT, "Crashdump for FRU: %d is generated.", fru);
  return 0;
}

static int ipmi_store_crashdump(uint8_t fru) {
  int ret;
  char cmd[100];

  // Check if the crashdump script exist
  if (access(CRASHDUMP_BIN, F_OK) == -1) {
    syslog(
        LOG_CRIT,
        "Crashdump for FRU: %d failed : "
        "auto crashdump binary is not preset",
        fru);
    return 0;
  }

  // Check if a crashdump for that fru is already running.
  // If yes, kill that thread and start a new one.
  if (t_dump[fru - 1].is_running) {
    ret = pthread_cancel(t_dump[fru - 1].pt);
    if (ret == ESRCH) {
      syslog(LOG_INFO, "pal_store_crashdump: No Crashdump pthread exists");
    } else {
      pthread_join(t_dump[fru - 1].pt, NULL);
      snprintf(
          cmd,
          sizeof(cmd),
          "ps | grep '{dump.sh}' | grep 'scm' "
          "| awk '{print $1}'| xargs kill");
      RUN_SHELL_CMD(cmd);
      snprintf(
          cmd,
          sizeof(cmd),
          "ps | grep 'bic-util' | grep 'scm' "
          "| awk '{print $1}'| xargs kill");
      RUN_SHELL_CMD(cmd);
#ifdef DEBUG
      syslog(
          LOG_INFO,
          "pal_store_crashdump:"
          " Previous crashdump thread is cancelled");
#endif
    }
  }

  // Start a thread to generate the crashdump
  t_dump[fru - 1].fru = fru;
  if (pthread_create(
          &(t_dump[fru - 1].pt),
          NULL,
          generate_dump,
          (void*)&t_dump[fru - 1].fru) < 0) {
    syslog(
        LOG_WARNING,
        "pal_store_crashdump: pthread_create for"
        " FRU %d failed\n",
        fru);
    return -1;
  }
  t_dump[fru - 1].is_running = 1;
  syslog(LOG_INFO, "Crashdump for FRU: %d is being generated.", fru);
  return 0;
}

int ipmi_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t* event_data) {
  char* key = "server_sel_error";
  char cvalue[MAX_VALUE_LEN] = {0};
  static int assert_cnt[MAX_NUM_SLOTS] = {0};
  bool bv = false;

  if (get_cfg_item_bool(key, &bv) || !bv) {
    return CC_SUCCESS;
  }

  switch (fru) {
    case FRU_SCM:
      switch (snr_num) {
        case CATERR_B:
          ipmi_store_crashdump(fru);
          break;

        case 0x00: // don't care sensor number 00h
          return 0;
      }

      if ((event_data[2] & 0x80) == 0) { // 0: Assertion,  1: Deassertion
        assert_cnt[fru - 1]++;
      } else {
        if (--assert_cnt[fru - 1] < 0)
          assert_cnt[fru - 1] = 0;
      }
      sprintf(cvalue, "%s", (assert_cnt[fru - 1] > 0) ? "0" : "1");
      break;

    default:
      return -1;
  }
  /* Write the value "0" which means FRU_STATUS_BAD */
  return kv_set(key, cvalue, 0, KV_FPERSIST);
}

int slotid_to_fruid(int slotid) {
  // This function is for mapping to fruid from slotid
  // If the platform's slotid is different with fruid, need to rewrite
  // this function in project layer.
  return slotid;
}

static bool is_fw_update_ongoing(uint8_t fruid) {
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

int ipmi_get_fruid_path(uint8_t fru, char* path) {
  return CC_INVALID_PARAM;
}

// GUID based on RFC4122 format @ https://tools.ietf.org/html/rfc4122
void populate_guid(char* guid, char* str) {
  unsigned int secs;
  unsigned int usecs;
  struct timeval tv;
  uint8_t count;
  uint8_t lsb, msb;
  int i, r;

  // Populate time
  gettimeofday(&tv, NULL);

  secs = tv.tv_sec;
  usecs = tv.tv_usec;
  guid[0] = usecs & 0xFF;
  guid[1] = (usecs >> 8) & 0xFF;
  guid[2] = (usecs >> 16) & 0xFF;
  guid[3] = (usecs >> 24) & 0xFF;
  guid[4] = secs & 0xFF;
  guid[5] = (secs >> 8) & 0xFF;
  guid[6] = (secs >> 16) & 0xFF;
  guid[7] = (secs >> 24) & 0x0F;

  // Populate version
  guid[7] |= 0x10;

  // Populate clock seq with randmom number
  // getrandom(&guid[8], 2, 0);
  srand(time(NULL));
  // memcpy(&guid[8], rand(), 2);
  r = rand();
  guid[8] = r & 0xFF;
  guid[9] = (r >> 8) & 0xFF;

  // Use string to populate 6 bytes unique
  // e.g. LSP62100035 => 'S' 'P' 0x62 0x10 0x00 0x35
  count = 0;
  for (i = strlen(str) - 1; i >= 0; i--) {
    if (count == 6) {
      break;
    }

    // If alphabet use the character as is
    if (isalpha(str[i])) {
      guid[15 - count] = str[i];
      count++;
      continue;
    }

    // If it is 0-9, use two numbers as BCD
    lsb = str[i] - '0';
    if (i > 0) {
      i--;
      if (isalpha(str[i])) {
        i++;
        msb = 0;
      } else {
        msb = str[i] - '0';
      }
    } else {
      msb = 0;
    }
    guid[15 - count] = (msb << 4) | lsb;
    count++;
  }

  // zero the remaining bytes, if any
  if (count != 6) {
    memset(&guid[10], 0, 6 - count);
  }
}

int ipmi_is_crashdump_ongoing(uint8_t fru) {
  char fname[128];
  char value[MAX_VALUE_LEN] = {0};
  struct timespec ts;
  int ret;

  // if pid file not exist, return false
  sprintf(fname, "/var/run/autodump%d.pid", fru);
  if (access(fname, F_OK) != 0) {
    return 0;
  }

  // check the crashdump file in /tmp/cache_store/fru$1_crashdump
  sprintf(fname, "fru%d_crashdump", fru);
  ret = kv_get(fname, value, NULL, 0);
  if (ret < 0) {
    return 0;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (strtoul(value, NULL, 10) > ts.tv_sec) {
    return 1;
  }

  // over the threshold time, return false
  return 0; /* false */
}

int ipmi_is_cplddump_ongoing(uint8_t fru) {
  return CC_SUCCESS;
}

// GUID for System and Device
int set_guid(char* guid) {
  int ret;
  char key[MAX_KEY_LEN];

  sprintf(key, "%s", "bmc_guid");

  ret = kv_set(key, guid, GUID_SIZE, KV_FPERSIST);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "ipmi_set_guid: kv_set failed");
#endif
  }
  return ret;
}

int ipmi_get_plat_guid(uint8_t fru, char* guid) {
  return CC_SUCCESS;
}

int ipmi_set_plat_guid(uint8_t fru, char* str) {
  return CC_SUCCESS;
}

int ipmi_set_ppin_info(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t req_len,
    uint8_t* res_data,
    uint8_t* res_len) {
  const char* key = "server_cpu_ppin";
  char str[MAX_VALUE_LEN] = {0};
  int i;
  int completion_code = CC_UNSPECIFIED_ERROR;
  bool bv = false;
  *res_len = 0;

  if (get_cfg_item_bool("server_cpu_ppin", &bv) || !bv) {
    return CC_SUCCESS;
  }

  if (req_len > SIZE_CPU_PPIN * 2)
    req_len = SIZE_CPU_PPIN * 2;

  for (i = 0; i < req_len; i++) {
    sprintf(str + (i * 2), "%02x", req_data[i]);
  }

  if (kv_set(key, str, 0, KV_FPERSIST) != 0)
    return completion_code;

  completion_code = CC_SUCCESS;
  return completion_code;
}

int ipmi_handle_fan_fru_checksum_sel(char* log, uint8_t log_len) {
  return CC_INVALID_PARAM;
}

int ipmi_handle_string_sel(char* log, uint8_t log_len) {
  return CC_SUCCESS;
}

int ipmi_set_bios_cap_fw_ver(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t req_len,
    uint8_t* res_data,
    uint8_t* res_len) {
  return CC_INVALID_PARAM;
}

int ipmi_set_ioc_fw_recovery(
    uint8_t* ioc_recovery_setting,
    uint8_t req_len,
    uint8_t* res_data,
    uint8_t* res_len) {
  return CC_INVALID_PARAM;
}

int ipmi_get_ioc_fw_recovery(
    uint8_t ioc_recovery_component,
    uint8_t* res_data,
    uint8_t* res_len) {
  return CC_INVALID_PARAM;
}

int ipmi_setup_exp_uart_bridging(void) {
  return CC_INVALID_PARAM;
}

int ipmi_teardown_exp_uart_bridging(void) {
  return CC_INVALID_PARAM;
}

int ipmi_get_ioc_wwid(
    uint8_t ioc_component,
    uint8_t* res_data,
    uint8_t* res_len) {
  return CC_INVALID_PARAM;
}

int ipmi_oem_bios_extra_setup(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t req_len,
    uint8_t* res_data,
    uint8_t* res_len) {
  return CC_INVALID_CMD;
}

int ipmi_handle_oem_1s_asd_msg_in(
    uint8_t slot,
    uint8_t* data,
    uint8_t data_len) {
  return CC_INVALID_PARAM;
}

int ipmi_handle_oem_1s_ras_dump_in(
    uint8_t slot,
    uint8_t* data,
    uint8_t data_len) {
  return CC_INVALID_PARAM;
}

int ipmi_display_4byte_post_code(uint8_t slot, uint32_t postcode_dw) {
  return 0;
}

int ipmi_get_fw_ver(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t* res_data,
    uint8_t* res_len) {
  return CC_INVALID_PARAM;
}

int ipmi_get_host_system_mode(uint8_t* mode) {
  return CC_INVALID_PARAM;
}

int ipmi_set_host_system_mode(uint8_t mode) {
  return CC_INVALID_PARAM;
}

int ipmi_set_usb_path(uint8_t slot, uint8_t endpoint) {
  return CC_INVALID_PARAM;
}

uint8_t ipmi_ipmb_get_sensor_val(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t req_len,
    uint8_t* res_data,
    uint8_t* res_len) {
  *res_len = 0;
  return 0;
}

int ipmi_correct_sensor_reading_from_cache(
    uint8_t fru,
    uint8_t sensor_id,
    float* value) {
  return CC_SUCCESS;
}

int ipmi_is_slot_server(uint8_t fru) {
  return CC_SUCCESS;
}

void ipmi_get_chassis_status(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t* res_data,
    uint8_t* res_len) {
  char key[MAX_KEY_LEN];
  char buff[MAX_VALUE_LEN];
  int policy = 3;
  uint8_t status, ret;
  unsigned char* data = res_data;
  bool bv = false;

  if (get_cfg_item_bool("chassis_control", &bv) || !bv) {
    *res_len = 0;
    return;
  }

  /* Platform Power Policy */
  sprintf(key, "%s", "server_por_cfg");
  if (kv_get(key, buff, 0, KV_FPERSIST) == 0) {
    if (!memcmp(buff, "off", strlen("off")))
      policy = 0;
    else if (!memcmp(buff, "lps", strlen("lps")))
      policy = 1;
    else if (!memcmp(buff, "on", strlen("on")))
      policy = 2;
    else
      policy = 3;
  }

  /* Current Power State */
  ret = ipmi_get_server_power(FRU_SCM, &status);
  if (ret == 0) {
    *data++ = status | (policy << 5);
  } else {
    /* load default */
    OBMC_WARN("ipmid: ipmi_get_server_power failed for server\n");
    *data++ = 0x00 | (policy << 5);
  }
  *data++ = 0x00; /* Last Power Event */
  *data++ = 0x40; /* Misc. Chassis Status */
  *data++ = 0x00; /* Front Panel Button Disable */
  *res_len = data - res_data;
}

static int get_last_pwr_state(uint8_t fru, char* state) {
  int ret;
  char key[MAX_KEY_LEN];

  sprintf(key, "%s", "pwr_server_last_state");

  ret = kv_get(key, state, 0, KV_FPERSIST);
  if (ret < 0) {
#ifdef DEBUG
    syslog(
        LOG_WARNING,
        "get_last_pwr_state: kv_get failed for "
        "fru %u",
        fru);
#endif
  }
  return ret;
}

int ipmi_get_server_power(uint8_t slot_id, uint8_t* status) {
  int ret;
  uint8_t retry = MAX_READ_RETRY;
  char value[MAX_VALUE_LEN];
  char sysfs_value[4] = {0};
  char path[LARGEST_DEVICE_NAME + 1];

  if (get_cfg_item_str("server_power_fs_path", path, LARGEST_DEVICE_NAME)) {
    *status = 0;
    return CC_SUCCESS;
  }

  /* Check if the SCM is turned on or not */
  while (retry) {
    ret = read_sysfs(path, sysfs_value, sizeof(sysfs_value) - 1);
    if (!ret)
      break;
    msleep(50);
    retry--;
  }

  if (ret) {
    // Check previous power state
    syslog(
        LOG_INFO,
        "get_server_power: read_sysfs returned error hence"
        " reading the kv_store for last power state for fru %d",
        slot_id);
    get_last_pwr_state(slot_id, value);
    if (!(strncmp(value, "off", strlen("off")))) {
      *status = SERVER_POWER_OFF;
    } else if (!(strncmp(value, "on", strlen("on")))) {
      *status = SERVER_POWER_ON;
    } else {
      return ret;
    }

    return 0;
  }

  if (!strcmp(sysfs_value, ELBERT_SCM_PWR_ON)) {
    *status = SERVER_POWER_ON;
  } else {
    *status = SERVER_POWER_OFF;
  }

  return 0;
}

int plat_sensor_name(uint8_t fru, uint8_t sensor_num, char* name) {
  bool bv = false;

  if (get_cfg_item_bool("platform_sensor_name", &bv) || !bv) {
    return -1;
  }

  switch (fru) {
    case FRU_SCM:
      switch (sensor_num) {
        case BIC_SENSOR_SYSTEM_STATUS:
          sprintf(name, "SYSTEM_STATUS");
          break;
        case BIC_SENSOR_SYS_BOOT_STAT:
          sprintf(name, "SYS_BOOT_STAT");
          break;
        case BIC_SENSOR_CPU_DIMM_HOT:
          sprintf(name, "CPU_DIMM_HOT");
          break;
        case BIC_SENSOR_PROC_FAIL:
          sprintf(name, "PROC_FAIL");
          break;
        case BIC_SENSOR_VR_HOT:
          sprintf(name, "VR_HOT");
          break;
        default:
          return -1;
      }
      break;
  }
  return 0;
}

int ipmi_get_event_sensor_name(uint8_t fru, uint8_t* sel, char* name) {
  uint8_t snr_type = sel[10];
  uint8_t snr_num = sel[11];

  // If SNR_TYPE is OS_BOOT, sensor name is OS
  switch (snr_type) {
    case OS_BOOT:
      // OS_BOOT used by OS
      sprintf(name, "OS");
      return 0;
    default:
      if (plat_sensor_name(fru, snr_num, name) != 0) {
        break;
      }
      return 0;
  }
  // Otherwise, translate it based on snr_num
  return get_x86_event_sensor_name(fru, snr_num, name);
}

int ipmi_is_fru_prsnt(uint8_t fru, uint8_t* status) {
  *status = 1;
  return CC_SUCCESS;
}

static int write_device(const char* device, const char* value) {
  FILE* fp;
  int rc;

  fp = fopen(device, "w");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device for write %s", device);
#endif
    return err;
  }

  rc = fputs(value, fp);
  fclose(fp);

  if (rc < 0) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to write device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

/* Update the Reset button input to the server at given slot */
int ipmi_set_rst_btn(uint8_t slot, uint8_t status) {
  char* val;
  char path[LARGEST_DEVICE_NAME];

  if (get_cfg_item_str("set_reset_button", path, LARGEST_DEVICE_NAME - 1)) {
    return CC_SUCCESS;
  }

  if (slot != FRU_SCM) {
    return -1;
  }

  if (status) {
    val = "1";
  } else {
    val = "0";
  }

  if (write_device(path, val)) {
    return -1;
  }
  return CC_SUCCESS;
}

int ipmi_get_board_id(
    uint8_t slot,
    uint8_t* req_data,
    uint8_t req_len,
    uint8_t* res_data,
    uint8_t* res_len) {
  return CC_SUCCESS;
}

static int lpc_snoop_read_legacy(uint8_t* buf, size_t max_len, size_t* len) {
  FILE* fp = fopen("/sys/devices/platform/ast-snoop-dma.0/data_history", "r");
  uint8_t postcode;
  size_t i;

  if (fp == NULL) {
    syslog(LOG_WARNING, "get_80port_record: Cannot open device");
    return CC_NOT_SUPP_IN_CURR_STATE;
  }
  for (i = 0; i < max_len; i++) {
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

static int lpc_snoop_read(uint8_t* buf, size_t max_len, size_t* rlen) {
  const char* dev_path = "/dev/aspeed-lpc-snoop0";
  const char* cache_key = "postcode";
  uint8_t cache[MAX_VALUE_LEN * 2];
  size_t len = 0, cache_len = 0;
  int fd;
  uint8_t postcode;

  if (kv_get(cache_key, (char*)cache, &len, 0)) {
    len = cache_len = 0;
  } else {
    cache_len = len;
  }

  /* Open and read as much as we can store. Our in-mem
   * cache is twice as the file-backed path. */
  fd = open(dev_path, O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    return CC_NOT_SUPP_IN_CURR_STATE;
  }
  while (len < sizeof(cache) && read(fd, &postcode, 1) == 1) {
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
    if (kv_set(cache_key, (char*)cache, len, 0)) {
      syslog(LOG_CRIT, "%zu postcodes dropped\n", len - cache_len);
    }
  }
  len = len > max_len ? max_len : len;
  memcpy(buf, cache, len);
  *rlen = len;
  return CC_SUCCESS;
}

int ipmi_get_80port_record(
    uint8_t slot,
    uint8_t* buf,
    size_t max_len,
    size_t* len) {
  static bool legacy = false, legacy_checked = false;

  if (buf == NULL || len == NULL || max_len == 0)
    return -1;

  if (!ipmi_is_slot_server(slot)) {
    syslog(
        LOG_WARNING, "ipmi_get_80port_record: slot %d is not supported", slot);
    return CC_INVALID_PARAM;
  }

  if (legacy_checked == false) {
    if (access("/sys/devices/platform/ast-snoop-dma.0/data_history", F_OK) ==
        0) {
      legacy = true;
      legacy_checked = true;
    } else if (access("/dev/aspeed-lpc-snoop0", F_OK) == 0) {
      legacy_checked = true;
    } else {
      return CC_INVALID_PARAM;
    }
  }

  // Support for snoop-dma on 4.1 kernel.
  if (legacy) {
    return lpc_snoop_read_legacy(buf, max_len, len);
  }
  return lpc_snoop_read(buf, max_len, len);
}

int ipmi_get_fru_name(uint8_t fru, char* name) {
  sprintf(name, "fru%u", fru);
  return CC_SUCCESS;
}

int ipmi_post_handle(uint8_t slot, uint8_t status) {
  return CC_SUCCESS;
}

int ipmi_set_server_power(uint8_t slot_id, uint8_t cmd) {
  return CC_SUCCESS;
}

bool ipmi_is_fw_update_ongoing_system(void) {
  // Base on fru number to sum up if fw update is onging.
  int slot_num = 0;
  int i;

  get_cfg_item_num("max_slot_num", &slot_num);
  for (i = 0; i <= slot_num; i++) { // 0 is reserved for BMC update
    int fruid = slotid_to_fruid(i);
    if (is_fw_update_ongoing(fruid) ==
        true) // if any slot is true, then we can return true
      return true;
  }
  return false;
}

int ipmi_is_test_board(void) {
  // Non Test Board:0
  return 0;
}

int ipmi_set_sysfw_ver(uint8_t slot, uint8_t* ver) {
  int i;
  const char* key = "sysfw_ver_server";
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10];

  if (!get_cfg_item_kv(key)) {
    for (i = 0; i < SIZE_SYSFW_VER; i++) {
      sprintf(tstr, "%02x", ver[i]);
      strcat(str, tstr);
    }
    return kv_set(key, str, 0, KV_FPERSIST);
  }
  return 0;
}

void init_func_vector() {
  g_fv->get_80port_record = ipmi_get_80port_record;
  g_fv->post_handle = ipmi_post_handle;
  g_fv->is_test_board = ipmi_is_test_board;

  g_fv->get_plat_guid = ipmi_get_plat_guid;
  g_fv->set_plat_guid = ipmi_set_plat_guid;
  g_fv->is_fru_prsnt = ipmi_is_fru_prsnt;
  g_fv->get_board_id = ipmi_get_board_id;
  g_fv->get_fru_name = ipmi_get_fru_name;
  g_fv->set_server_power = ipmi_set_server_power;
  g_fv->is_fw_update_ongoing_system = ipmi_is_fw_update_ongoing_system;
}
