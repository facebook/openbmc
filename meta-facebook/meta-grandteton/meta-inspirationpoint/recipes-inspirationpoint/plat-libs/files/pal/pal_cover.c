#include <stdio.h>
#include <stdint.h>
#include <syslog.h>
#include <pthread.h>
#include <esmi_cpuid_msr.h>
#include <esmi_rmi.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/kv.h>
#include "pal.h"

#define ERROR_LOG_MAX_LENGTH 128

int
parse_mce_error_sel(uint8_t fru, uint8_t *event_data, char *error_log) {
  uint8_t *ed = &event_data[3];
  uint8_t bank_num;
  uint8_t error_type = ((ed[1] & 0x60) >> 5);
  char temp_log[512] = {0};

  strcpy(error_log, "");
  if ((ed[0] & 0x0F) == 0x0B) { //Uncorrectable
    switch (error_type) {
      case 0x00:
        strcat(error_log, "Uncorrected Recoverable Error, ");
        break;
      case 0x01:
        strcat(error_log, "Uncorrected Thread Fatal Error, ");
        break;
      case 0x02:
        strcat(error_log, "Uncorrected System Fatal Error, ");
        break;
      default:
        strcat(error_log, "Unknown, ");
        break;
    }
  } else if((ed[0] & 0x0F) == 0x0C) { //Correctable
    switch (error_type) {
      case 0x00:
        strcat(error_log, "Correctable Error, ");
        break;
      case 0x01:
        strcat(error_log, "Deferred Error, ");
        break;
      default:
        strcat(error_log, "Unknown, ");
        break;
    }
  }
  bank_num = ed[1] & 0x1F;
  snprintf(temp_log, sizeof(temp_log), "Bank Number %d, ", bank_num);
  strcat(error_log, temp_log);

  snprintf(temp_log, sizeof(temp_log), "CPU %d, Core %d", ((ed[2] & 0xE0) >> 5), (ed[2] & 0x1F));
  strcat(error_log, temp_log);

  return 0;
}

int
pal_parse_bic_sensor_alert_event(uint8_t fru, uint8_t *event_data, char *error_log)
{
  if (pal_is_artemis() != true) {
    return -1;
  }

  enum EVENT_TYPE {
    CB_P0V8_VR_ALERT = 0x01,
    CB_POWER_BRICK_ALERT = 0x02,
    CB_P1V25_MONITOR_ALERT = 0x03,
    CB_P12V_ACCL1_MONITOR_ALERT = 0x04,
    CB_P12V_ACCL2_MONITOR_ALERT,
    CB_P12V_ACCL3_MONITOR_ALERT,
    CB_P12V_ACCL4_MONITOR_ALERT,
    CB_P12V_ACCL5_MONITOR_ALERT,
    CB_P12V_ACCL6_MONITOR_ALERT,
    CB_P12V_ACCL7_MONITOR_ALERT,
    CB_P12V_ACCL8_MONITOR_ALERT,
    CB_P12V_ACCL9_MONITOR_ALERT,
    CB_P12V_ACCL10_MONITOR_ALERT,
    CB_P12V_ACCL11_MONITOR_ALERT,
    CB_P12V_ACCL12_MONITOR_ALERT,
  };

  enum SOURCE_TYPE {
    MAIN_SOURCE,
    SECOND_SOURCE,
  };

  uint8_t event_dir = event_data[2] & 0x80;
  uint8_t *event_info = &event_data[3];
  char event_name[64] = { 0 };

  switch (event_info[0]) {
    case CB_P0V8_VR_ALERT:
      snprintf(event_name, sizeof(event_name), "P0V8 VR Alert");
      break;
    case CB_POWER_BRICK_ALERT:
      snprintf(event_name, sizeof(event_name), "Power Brick Alert");

      switch (event_info[1]) {
      case MAIN_SOURCE:
        strcat(event_name, " (Device: Q50SN120A1)");
        break;
      case SECOND_SOURCE:
        strcat(event_name, " (Device: BMR351)");
        break;
      default:
        strcat(event_name, " (Device: Unknown)");
        break;
      }
      break;
    case CB_P1V25_MONITOR_ALERT:
      snprintf(event_name, sizeof(event_name), "P1V25 Monitor Alert");

      switch (event_info[1]) {
      case MAIN_SOURCE:
        strcat(event_name, " (Device: SQ52205)");
        break;
      case SECOND_SOURCE:
        strcat(event_name, " (Device: INA233)");
        break;
      default:
        strcat(event_name, " (Device: Unknown)");
        break;
      }
      break;
    case CB_P12V_ACCL1_MONITOR_ALERT:
    case CB_P12V_ACCL2_MONITOR_ALERT:
    case CB_P12V_ACCL3_MONITOR_ALERT:
    case CB_P12V_ACCL4_MONITOR_ALERT:
    case CB_P12V_ACCL5_MONITOR_ALERT:
    case CB_P12V_ACCL6_MONITOR_ALERT:
    case CB_P12V_ACCL7_MONITOR_ALERT:
    case CB_P12V_ACCL8_MONITOR_ALERT:
    case CB_P12V_ACCL9_MONITOR_ALERT:
    case CB_P12V_ACCL10_MONITOR_ALERT:
    case CB_P12V_ACCL11_MONITOR_ALERT:
    case CB_P12V_ACCL12_MONITOR_ALERT:
      snprintf(event_name, sizeof(event_name), "P12V ACCL%d Monitor Alert", (event_info[0] - CB_P12V_ACCL1_MONITOR_ALERT + 1));

      switch (event_info[1]) {
      case MAIN_SOURCE:
        strcat(event_name, " (Device: INA233)");
        break;
      case SECOND_SOURCE:
        strcat(event_name, " (Device: SQ52205)");
        break;
      default:
        strcat(event_name, " (Device: Unknown)");
        break;
      }
      break;
    default:
      return -1;
  }

  snprintf(error_log, ERROR_LOG_MAX_LENGTH, "%s status(0x%02X%02X%02X) %s", event_name, event_info[0], event_info[1], event_info[2], 
		  (((event_dir & 0x80) == 0) ? "Assertion" : "Deassertion"));
  return 0;
}

int
pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  int ret = 0;
  uint8_t snr_num = sel[11];
  uint8_t *event_data = &sel[10];
  bool parsed = true;

  strcpy(error_log, "");
  switch(snr_num) {
    case MACHINE_CHK_ERR:
      parse_mce_error_sel(fru, event_data, error_log);
      parsed = true;
      break;
    case SENSOR_NUM_BIC_ALERT:
      ret = pal_parse_bic_sensor_alert_event(fru, event_data, error_log);
      if (ret == 0) {
        parsed = true;
      }
      break;
    default:
      parsed = false;
      break;
  }

  if (parsed == true) {
    return 0;
  }

  pal_parse_sel_helper(fru, sel, error_log);
  return 0;
}

int
pal_control_mux_to_target_ch(uint8_t bus, uint8_t mux_addr, uint8_t channel)
{
  int ret = PAL_ENOTSUP;
  int fd;
  char fn[32];
  uint8_t retry;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if(fd < 0) {
    syslog(LOG_WARNING,"[%s] Cannot open bus %d", __func__, bus);
    ret = PAL_ENOTSUP;
    goto error_exit;
  }

  ret = pal_flock_retry(fd);
  if (ret == -1) {
   syslog(LOG_WARNING, "[%s] Failed to flock: %s", __func__, strerror(errno));
   goto error_exit;
  }

  tbuf[0] = 1 << channel;
  retry = 3;

  while(retry > 0) {
    ret = i2c_rdwr_msg_transfer(fd, mux_addr, tbuf, 1, rbuf, 0);
    if (PAL_EOK == ret){
      break;
    }
    msleep(50);
    retry--;
  }

  if(retry == 0) {
    syslog(LOG_WARNING,"[%s] Cannot switch the mux on bus %d", __func__, bus);
  }

  if (pal_unflock_retry(fd) == -1) {
   syslog(LOG_WARNING, "[%s] Failed to unflock: %s", __func__, strerror(errno));
  }

error_exit:
  if (fd > 0){
    close(fd);
  }

  return ret;
}

int
pal_check_psb_error(uint8_t head, uint8_t last) {
  if(head == 0xEE) {
    switch (last) {
      case 0x03:
        syslog(LOG_CRIT, "PSB Event(EE0003) Error in BIOS Directory Table - Too many directory entries");
        break;
      case 0x04:
        syslog(LOG_CRIT, "PSB Event(EE0004) Internal error - Invalid parameter");
        break;
      case 0x05:
        syslog(LOG_CRIT, "PSB Event(EE0005) Internal error - Invalid data length");
        break;
      case 0x0B:
        syslog(LOG_CRIT, "PSB Event(EE000B) Internal error - Out of resources");
        break;
      case 0x10:
        syslog(LOG_CRIT, "PSB Event(EE0010) Error in BIOS Directory Table - Reset image destination invalid");
        break;
      case 0x13:
        syslog(LOG_CRIT, "PSB Event(EE0013) Error retrieving FW header during FW validation");
        break;
      case 0x14:
        syslog(LOG_CRIT, "PSB Event(EE0014) Invalid key size");
        break;
      case 0x18:
        syslog(LOG_CRIT, "PSB Event(EE0018) Error validating binary");
        break;
      case 0x22:
        syslog(LOG_CRIT, "PSB Event(EE0022) P0: BIOS signing key entry not found");
        break;
      case 0x3E:
        syslog(LOG_CRIT, "PSB Event(EE003E) P0: Error reading fuse info");
        break;
      case 0x62:
        syslog(LOG_CRIT, "PSB Event(EE0062) Internal error - Error mapping fuse");
        break;
      case 0x64:
        syslog(LOG_CRIT, "PSB Event(EE0064) P0: Timeout error attempting to fuse");
        break;
      case 0x69:
        syslog(LOG_CRIT, "PSB Event(EE0069) BIOS OEM key revoked");
        break;
      case 0x6C:
        syslog(LOG_CRIT, "PSB Event(EE006C) P0: Error in BIOS Directory Table - Reset image not found");
        break;
      case 0x6F:
        syslog(LOG_CRIT, "PSB Event(EE006F) P0: OEM BIOS Signing Key Usage flag violation");
        break;
      case 0x78:
        syslog(LOG_CRIT, "PSB Event(EE0078) P0: BIOS RTM Signature entry not found");
        break;
      case 0x79:
        syslog(LOG_CRIT, "PSB Event(EE0079) P0: BIOS Copy to DRAM failed");
        break;
      case 0x7A:
        syslog(LOG_CRIT, "PSB Event(EE007A) P0: BIOS RTM Signature verification failed");
        break;
      case 0x7B:
        syslog(LOG_CRIT, "PSB Event(EE007B) P0: OEM BIOS Signing Key failed signature verification");
        break;
      case 0x7C:
        syslog(LOG_CRIT, "PSB Event(EE007C) P0: Platform Vendor ID and/or Model ID binding violation");
        break;
      case 0x7D:
        syslog(LOG_CRIT, "PSB Event(EE007D) P0: BIOS Copy bit is unset for reset image");
        break;
      case 0x7E:
        syslog(LOG_CRIT, "PSB Event(EE007E) P0: Requested fuse is already blown, reblow will cause ASIC malfunction");
        break;
      case 0x7F:
        syslog(LOG_CRIT, "PSB Event(EE007F) P0: Error with actual fusing operation");
        break;
      case 0x80:
        syslog(LOG_CRIT, "PSB Event(EE0080) P1: Error reading fuse info");
        break;
      case 0x81:
        syslog(LOG_CRIT, "PSB Event(EE0081) P1: Platform Vendor ID and/or Model ID binding violation");
        break;
      case 0x82:
        syslog(LOG_CRIT, "PSB Event(EE0082) P1: Requested fuse is already blown, reblow will cause ASIC malfunction");
        break;
      case 0x83:
        syslog(LOG_CRIT, "PSB Event(EE0083) P1: Error with actual fusing operation");
        break;
      case 0x92:
        syslog(LOG_CRIT, "PSB Event(EE0092) P0: Error in BIOS Directory Table - Firmware type not found");
        break;
      default:
        break;
    }
  }

  return 0;
}

int
pal_check_abl_error(uint32_t postcode) {
  //Check ABL section
  switch (postcode) {
    case 0xEA00E321:
      syslog(LOG_CRIT, "ABL Error(EA00E321) Mixed 3DS and Non-3DS DIMM in a channel");
      break;
    case 0xEA00E322:
      syslog(LOG_CRIT, "ABL Error(EA00E322) Mixed x4 and x8 DIMM in a channel");
      break;
    case 0xEA00E328:
      syslog(LOG_CRIT, "ABL Error(EA00E328) CPU OPN Mismatch in case of Multi Socket population");
      break;
    case 0xEA00E32B:
      syslog(LOG_CRIT, "ABL Error(EA00E32B) BIST Failure");
      break;
    case 0xEA00E32D:
      syslog(LOG_CRIT, "ABL Error(EA00E32D) Mix DIMM with different ECC bit size in a channel");
      break;
    case 0xEA00E332:
      syslog(LOG_CRIT, "ABL Error(EA00E332) Incompatible OPN");
      break;
    case 0xEA00E33E:
      syslog(LOG_CRIT, "ABL Error(EA00E33E) Timeout at PMFW SwitchToMemoryTrainingState");
      break;
    case 0xEA00E345:
      syslog(LOG_CRIT, "ABL Error(EA00E345) ABL Timeout to detect CPU OPN Mismatch in case of Multi Socket population");
      break;
    case 0xEA00E2E7:
      syslog(LOG_CRIT, "ABL Error(EA00E2E7) Mix RDIMM & LRDIMM in a channel");
      break;
    case 0xEA00E2EF:
      syslog(LOG_CRIT, "ABL Error(EA00E2EF) Unsupproted DIMM Config Error");
      break;
    default:
      break;
  }

  return 0;
}

int
pal_mrc_warning_detect(uint8_t slot, uint32_t postcode) {
  static uint8_t mrc_position = 0;
  static uint8_t total_error = 0;
  static char dimm_loop_codes[MAX_VALUE_LEN] = {0};
  static const uint8_t max_error_count = sizeof(dimm_loop_codes) / (sizeof(uint16_t) * 6);
  uint16_t *record_buf = (uint16_t *)dimm_loop_codes;
  uint8_t count = 0;
  bool need_start_over = false;
  char value[MAX_VALUE_LEN];

  pal_get_mrc_warning_count(slot, &count);
  if (count > 0) {
    return PAL_EOK;
  }

  // AMD 4 Bytes postcode DIMM looping rule
  // 0. [DD EE 00 00]: fixed tag
  // 1. [DD EE 00 TT]: TT is total error count
  // 2. [DD EE 00 II]: II is error index
  // 3. [DD EE 00 XX]: XX is dimm location
  // 4. [DD EE 00 YY]: YY is major error code
  // 5. [DD EE ZZ ZZ]: ZZ is minor error code (2 bytes)

  if ((postcode & 0xFFFF0000) != 0xDDEE0000) {
    need_start_over = true;
  } else {
    if ((mrc_position % 6) == 0) {
      // DIMM loop fixed tag
      if (postcode != 0xDDEE0000) {
        need_start_over = true;
      } else {
        record_buf[mrc_position++] = postcode & 0xFFFF;
      }
    } else if ((mrc_position % 6) == 1) {
      // total error count
      if (total_error == 0) {
        total_error = (postcode & 0xFF);
        total_error = (total_error < max_error_count) ? total_error : max_error_count;
      }
      record_buf[mrc_position++] = total_error;
    } else {
      record_buf[mrc_position++] = postcode & 0xFFFF;
    }
  }

  if (need_start_over) {
    mrc_position = 0;
    total_error = 0;
  }

  if (total_error > 0 && mrc_position >= (total_error * 6)) {
    snprintf(value, sizeof(value), "%u", total_error);
    kv_set("mrc_warning", value, 0, 0);
    kv_set("dimm_loop_codes", dimm_loop_codes, mrc_position * sizeof(uint16_t), 0);
    mrc_position = 0;
    total_error = 0;
  }

  return PAL_EOK;
}

int
pal_get_mrc_warning_count(uint8_t slot, uint8_t *count) {
  char value[MAX_VALUE_LEN] = {0};

  if (kv_get("mrc_warning", value, NULL, 0)) {
    *count = 0;
    return PAL_EOK;
  }
  *count = (uint8_t)strtol(value, NULL, 10);

  return PAL_EOK;
}

int
pal_get_dimm_loop_pattern(uint8_t slot, uint8_t index, DIMM_PATTERN *dimm_loop_pattern) {
  char dimm_loop_codes[MAX_VALUE_LEN] = {0};
  uint16_t *record_buf = (uint16_t *)dimm_loop_codes;
  size_t len;

  if (!dimm_loop_pattern) {
    return -1;
  }

  if (kv_get("dimm_loop_codes", dimm_loop_codes, &len, 0)) {
    return -1;
  }
  if (index >= record_buf[1]) {  // total error count
    return -1;
  }

  dimm_loop_pattern->start_code = record_buf[(index * 6)];
  snprintf(dimm_loop_pattern->dimm_location, sizeof(dimm_loop_pattern->dimm_location),
           "%02X", record_buf[(index * 6) + 3]);
  dimm_loop_pattern->major_code = record_buf[(index * 6) + 4];
  dimm_loop_pattern->minor_code = record_buf[(index * 6) + 5];

  return 0;
}

int pal_udbg_get_frame_total_num() {
  return 5;
}

int
pal_get_80port_page_record(uint8_t slot, uint8_t page_num, uint8_t *res_data, size_t max_len, size_t *res_len) {
  char key[MAX_KEY_LEN], value[MAX_VALUE_LEN] = {0};
  size_t len = 0;

  if (res_data == NULL || res_len == NULL) {
    return -1;
  }

  if (slot != FRU_MB) {
    return PAL_ENOTSUP;
  }

  snprintf(key, sizeof(key), "pcc_postcode_%u", page_num);
  if (kv_get(key, value, &len, 0) != 0) {
    *res_len = 0;
    return 0;
  }

  if (len > 0) {
    if (len > max_len) {
      len = max_len;
    }
    memcpy(res_data, value, len);
  }
  *res_len = len;

  return 0;
}

int
pal_get_80port_record(uint8_t slot, uint8_t *buf, size_t max_len, size_t *len) {
  int page, j, index = 0;
  uint8_t post_buf[MAX_VALUE_LEN];
  size_t page_len = 0;

  if (slot != FRU_MB) {
    return -1;
  }

  max_len /= sizeof(uint32_t);
  for (page = PCC_PAGE; page > 0 && index < max_len; --page) {
    if (pal_get_80port_page_record(slot, page, post_buf, sizeof(post_buf), &page_len)) {
      continue;
    }
    page_len /= sizeof(uint32_t);
    if (page_len == 0) {
      continue;
    }

    for (j = page_len - 1; j >= 0 && index < max_len; --j) {
      ((uint32_t *)buf)[index++] = ((uint32_t *)post_buf)[j];
    }
  }
  *len = index * sizeof(uint32_t);

  return 0;
}

int
pal_clear_psb_cache(void) {
  char key[MAX_KEY_LEN] = {0};
  snprintf(key, sizeof(key), "slot%d_psb_config_raw", FRU_MB);
  return kv_del(key, KV_FPERSIST);
}

bool
pal_check_apml_ras_status(uint8_t soc_num, uint8_t *ras_sts) {
  oob_status_t ret;
  int lock = -1;

  if (!is_cpu_socket_occupy(soc_num)) {
    return false;
  }

  lock = apml_channel_lock(soc_num);
  ret = read_sbrmi_ras_status(soc_num, ras_sts);
  apml_channel_unlock(lock);
  if (ret == OOB_SUCCESS && *ras_sts) {
    return true;
  }

  return false;
}

int
pal_get_cpu_id(uint8_t soc_num) {
  oob_status_t ret;
  int lock = -1, retry = 2;
  uint32_t core_id = 0;
  uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
  char key[MAX_KEY_LEN];
  char cpuid[MAX_VALUE_LEN], value[MAX_VALUE_LEN] = {0};

  if (!is_cpu_socket_occupy(soc_num)) {
    return -1;
  }

  for (; retry >= 0; --retry) {
    lock = apml_channel_lock(soc_num);
    ret = esmi_oob_cpuid(soc_num, core_id, &eax, &ebx, &ecx, &edx);
    apml_channel_unlock(lock);
    if (ret == OOB_SUCCESS) {
      break;
    }
    if (retry > 0) {
      msleep(100);
    }
  }
  if (ret != OOB_SUCCESS) {
    return -1;
  }

  snprintf(key, sizeof(key), "cpu%u_id", soc_num);
  snprintf(cpuid, sizeof(cpuid), "%08x %08x %08x %08x", eax, ebx, ecx, edx);
  if (kv_get(key, value, NULL, KV_FPERSIST) == 0) {
    if (strncmp(cpuid, value, sizeof(cpuid)) == 0) {
      return 0;
    }
  }

  return kv_set(key, cpuid, 0, KV_FPERSIST);
}

bool
fru_presence_ext(uint8_t fru_id, uint8_t *status) {
  gpio_value_t gpio_value;

  switch (fru_id) {
    case FRU_NIC0:
      gpio_value = gpio_get_value_by_shadow(FM_OCP0_PRSNT);
      *status = (gpio_value == GPIO_VALUE_LOW) ? FRU_PRSNT : FRU_NOT_PRSNT;
      return true;
    case FRU_NIC1:
      gpio_value = gpio_get_value_by_shadow(FM_OCP1_PRSNT);
      *status = (gpio_value == GPIO_VALUE_LOW) ? FRU_PRSNT : FRU_NOT_PRSNT;
      return true;
    default:
      *status = FRU_PRSNT;
      return true;
  }
}

int
pal_set_rst_btn(uint8_t slot, uint8_t val) {
  int ret, delay;
  gpio_desc_t *gdesc = NULL;
  gpio_value_t kb_enable;

  if (slot != FRU_MB) {
    return -1;
  }

  kb_enable = gpio_get_value_by_shadow(KB_RESET_EN);
  if (kb_enable == GPIO_VALUE_INVALID) {
    return -1;
  }

  if (kb_enable) {
    // Block the KB reset command until post completed.
    if (val == GPIO_VALUE_LOW && !pal_bios_completed(slot)) {
      return -1;
    }
    gdesc = gpio_open_by_shadow(RST_KB_RESET_N);
    delay = 20;
  }
  else {
    gdesc = gpio_open_by_shadow(FP_RST_BTN_OUT_N);
    delay = 100;
  }

  ret = gpio_set_value(gdesc, val);
  msleep(delay);
  gpio_close(gdesc);
  return ret;
}

int
pal_toggle_rst_btn(uint8_t slot) {
  int ret;
  ret = pal_set_rst_btn(FRU_MB, 0);
  ret |= pal_set_rst_btn(FRU_MB, 1);
  return ret;
}

static void* hgx_pwr_limit_check (void* arg) {
  int err_type;
  static bool is_logged = false;
  gpio_value_t hmc_ready, plat_rst;

  pthread_detach(pthread_self());

  while (1) {
    hmc_ready = gpio_get_value_by_shadow(HMC_READY);
    plat_rst  = gpio_get_value_by_shadow(RST_PLTRST_N);
    if(hmc_ready == GPIO_VALUE_HIGH) {
      // Monitor HGX GPU when power on
      if (plat_rst == GPIO_VALUE_HIGH) {
        err_type = system("sh /etc/init.d/setup-fan.sh hgx &");

        if (err_type != 0 && is_logged != true) {
          syslog(LOG_WARNING, "%s: err_type = %d", __FUNCTION__, err_type);
          is_logged = true;
        }
        else {
          is_logged = false;
        }
      }
    }
    else {
      // Exit when HMC is not ready
      pthread_exit(NULL);
    }
    sleep(60);
  }
}

void hgx_pwr_limit_mon (void) {
  pthread_t tid_pwr_limit_mon;

 if (pthread_create(&tid_pwr_limit_mon, NULL, hgx_pwr_limit_check, NULL)) {
    syslog(LOG_WARNING, "pthread was created fail for hgx_pwr_limit_mon");
  }
}
