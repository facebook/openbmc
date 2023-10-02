/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#include "led-controld.h"

int elbert_write_sysfs(char* str, unsigned int val) {
  char buf[ELBERT_BUFFER_SZ];
  snprintf(buf, ELBERT_BUFFER_SZ, "echo %u > %s\n", val, str);
  return system(buf);
}

int elbert_read_sysfs(char* str, int* val) {
  FILE* fp;
  char value[64];
  char* endPtr;
  int read_val = 0;
  int rc = 0;
  long int long_val = 0;

  fp = fopen(str, "r");
  if (!fp) {
    return ELBERT_FS_ERR;
  }
  rc = fscanf(fp, "%63s", value);
  fclose(fp);
  if (rc != 1) {
    return ELBERT_READ_ERR;
  }

  long_val = strtol(value, &endPtr, 0);

  if (endPtr >= (value + strlen(value))) {
    /* Practically, we read 8-bit registers only, whose maximum value is
       at most 0xFF, so the following is safe. */
    read_val = (int)long_val;
    *val = read_val;
  } else {
    printf(
        "Invalid string parsing. path:[%s] text_val:[%s]\
            errno:%d str_ptr:0x%08x end_ptr:0x%08x\n",
        str,
        value,
        errno,
        (unsigned int)value,
        (unsigned int)endPtr);
    return ELBERT_PARSE_ERR;
  }
  return 0;
}

int elbert_write_cpld_reg(char* prefix, char* reg, int val) {
  char buf[ELBERT_BUFFER_SZ];
  snprintf(buf, ELBERT_BUFFER_SZ, "%s%s", prefix, reg);
  return elbert_write_sysfs(buf, val);
}

int elbert_read_cpld_reg(char* prefix, char* reg, int* val) {
  char buf[ELBERT_BUFFER_SZ];
  snprintf(buf, ELBERT_BUFFER_SZ, "%s%s", prefix, reg);
  return elbert_read_sysfs(buf, val);
}

void elbert_set_blink_freq(int msec) {
  int lower = msec % 256;
  int upper = msec / 256;
  elbert_write_cpld_reg(SCM_PREFIX, SCM_LED_FREQ_H, upper);
  elbert_write_cpld_reg(SCM_PREFIX, SCM_LED_FREQ_L, lower);
}

void elbert_set_sysled(char* name, int blue, int amber,
                       int green, int red, int blink) {
  char led_name[ELBERT_BUFFER_SZ];

  // Green and red are not used.
  snprintf(led_name, ELBERT_BUFFER_SZ, "%s%s_led", SCM_PREFIX, name);
  elbert_write_cpld_reg(led_name, COLOR_GREEN, green);
  elbert_write_cpld_reg(led_name, COLOR_RED, red);

  // System status LED blue is controlled through beacon LED register.
  if (strncmp(name, "system", ELBERT_BUFFER_SZ) == 0) {
    elbert_write_cpld_reg(SCM_PREFIX, BEACON_LED, blue);
    elbert_write_cpld_reg(SCM_PREFIX, BEACON_LED_BLINK, blink);
  } else {
    elbert_write_cpld_reg(led_name, COLOR_BLUE, blue);
  }
  elbert_write_cpld_reg(led_name, COLOR_AMBER, amber);
  elbert_write_cpld_reg(led_name, COLOR_BLINK, blink);

  return;
}

void elbert_update_fan_led(struct system_status* status) {
  bool all_fan_ok = true;
  bool this_fan_ok = true;
  char buf[ELBERT_BUFFER_SZ];
  int fan;
  int read_val = 0, rc = 0;

  /* FSCD sets the LEDs on each fan, so don't do that here. Only set the FAN
     status LED. */
  for (fan = 1; fan <= ELBERT_MAX_FAN; fan++) {
    this_fan_ok = true;
    snprintf(
        buf, ELBERT_BUFFER_SZ, "%sfan%d%s", FAN_PREFIX, fan, SMB_FAN_PRESENT);
    // Check if FAN present
    rc = elbert_read_sysfs(buf, &read_val);
    if (rc != 0) {
      this_fan_ok = false;
      if (this_fan_ok != status->fan_ok[fan - 1]) {
        syslog(LOG_WARNING, "Fan %d is not present", fan);
      }
    } else {
      if (read_val != 1) {
        this_fan_ok = false;
        if (this_fan_ok != status->fan_ok[fan - 1]) {
          syslog(LOG_WARNING, "Fan %d is not present", fan);
        }
      } else {
        // If fan is present, check rpm is over a minimum threshold
        snprintf(buf, ELBERT_BUFFER_SZ, "%sfan%d%s", FAN_PREFIX, fan, FAN_RPM);
        rc = elbert_read_sysfs(buf, &read_val);
        if ((rc != 0) || (read_val < ELBERT_FAN_RPM_MIN)) {
          this_fan_ok = false;
          if (this_fan_ok != status->fan_ok[fan - 1]) {
            syslog(
                LOG_WARNING,
                "Fan %d running at %d is less than %d, the minimum threshold",
                fan,
                read_val,
                ELBERT_FAN_RPM_MIN);
          }
        }
      }
    }

    if (!this_fan_ok) {
      all_fan_ok = false;
    }
    status->fan_ok[fan - 1] = this_fan_ok;
  }

  if (all_fan_ok != status->all_fan_ok) {
    if (all_fan_ok) {
      elbert_set_sysled("fan", 1, 0, 0, 0, 0);
    } else {
      elbert_set_sysled("fan", 0, 1, 0, 0, 0);
    }
    syslog(
        LOG_WARNING,
        "FAN LED color change occured previous:%s now:%s\n",
        status->all_fan_ok ? "Good" : "BAD",
        all_fan_ok ? "Good" : "BAD");
  }
  status->prev_all_fan_ok = status->all_fan_ok;
  status->all_fan_ok = all_fan_ok;
}

void elbert_update_psu_led(struct system_status* status) {
  int psu = 0;
  int psu_present_reg = 0;
  int psu_present = 0;
  int psu_status_reg = 0;
  int psu_input_ok = 0;
  int psu_output_ok = 0;
  int psu_count = 0;
  bool all_psu_ok = true;
  bool this_psu_ok = true;

  elbert_read_cpld_reg(SMB_PREFIX, SMB_PSU_STATUS, &psu_status_reg);
  elbert_read_cpld_reg(SMB_PREFIX, SMB_PSU_PRESENT, &psu_present_reg);
  for (psu = 1; psu <= ELBERT_MAX_PSU; psu++) {
    // PSU numbering is 0-based in register definition.
    this_psu_ok = true;
    psu_present = (psu_present_reg >> (psu - 1)) & 0x1;
    psu_input_ok = (psu_status_reg >> (SMB_PSU_INPUT_BIT + (psu - 1))) & 0x1;
    psu_output_ok = (psu_status_reg >> (SMB_PSU_OUTPUT_BIT + (psu - 1))) & 0x1;
    if ((psu_present == 1) && ((psu_input_ok != 1) || (psu_output_ok != 1))) {
      this_psu_ok = false;
      if (this_psu_ok != status->psu_ok[psu - 1]) {
        syslog(
            LOG_WARNING,
            "PSU %d is %s and the input is %s and the output is %s\n",
            psu,
            psu_present ? "present" : "not present",
            psu_input_ok ? "ok" : "not ok",
            psu_output_ok ? "ok" : "not ok");
      }
    }

    if (!this_psu_ok)
      all_psu_ok = false;
    else if (psu_present == 1)
      psu_count++;

    status->psu_ok[psu - 1] = this_psu_ok;
  }
  
  if (psu_count < 2){
    all_psu_ok = false;
    syslog(LOG_WARNING, "Less than 2 PSUs detected");
  }

  if (all_psu_ok != status->all_psu_ok) {
    if (all_psu_ok) {
      elbert_set_sysled("psu", 1, 0, 0, 0, 0);
    } else {
      elbert_set_sysled("psu", 0, 1, 0, 0, 0);
    }
    syslog(
        LOG_WARNING,
        "PSU LED color change occured previous:%s now:%s\n",
        status->all_psu_ok ? "Good" : "BAD",
        all_psu_ok ? "Good" : "BAD");
  }
  status->prev_all_psu_ok = status->all_psu_ok;
  status->all_psu_ok = all_psu_ok;
}

void elbert_update_smb_led(struct system_status* status) {
  bool smb_ok = true;
  int smb_pwr_good = 0;
  int smb_snr_health = 0;
  int rc = 0;

  // Check switchcard powergood.
  rc = elbert_read_cpld_reg(SCM_PREFIX, SMB_PWR_GOOD, &smb_pwr_good);
  if ((rc != 0) || (smb_pwr_good != 1)) {
    smb_ok = false;
    if ((smb_ok != status->smb_ok) || (smb_pwr_good != status->smb_pwr_good)) {
      syslog(LOG_WARNING, "SMB power is not ok\n");
    }
  } else {
    // If powergood is ok, check for temp/voltage sensor health.
    rc = pal_get_fru_health(FRU_SMB, (uint8_t*)&smb_snr_health);
    if ((rc != 0) || (smb_snr_health != 1)) {
      smb_ok = false;
      if ((smb_ok != status->smb_ok) ||
          (smb_snr_health != status->smb_snr_health)) {
        syslog(LOG_WARNING, "SMB sensor out of range\n");
      }
    }
    status->smb_snr_health = smb_snr_health;
  }
  status->smb_pwr_good = smb_pwr_good;

  if (smb_ok != status->smb_ok) {
    if (smb_ok) {
      elbert_set_sysled("sc", 1, 0, 0, 0, 0);
    } else {
      elbert_set_sysled("sc", 0, 1, 0, 0, 0);
      syslog(
          LOG_WARNING,
          "SMB LED color change occured previous:%s now:%s\n",
          status->smb_ok ? "Good" : "BAD",
          smb_ok ? "Good" : "BAD");
    }
  }

  status->prev_smb_ok = status->smb_ok;
  status->smb_ok = smb_ok;
}

bool elbert_check_beacon_led(int *beacon_mode) {
  // Check for beacon stub file. If found, read mode.
  int rc = 0;
  bool beacon_on = false;

  rc = elbert_read_sysfs(BEACON_FS_STUB, beacon_mode);
  if (rc == 0 && (*beacon_mode >= BEACON_MODE_LOCATOR &&
                  *beacon_mode <= BEACON_MODE_MAX)) {
    beacon_on = true;
  } else {
    *beacon_mode = BEACON_MODE_OFF;
  }

  return beacon_on;
}

bool elbert_check_upgrade() {
  bool is_upgrading = false;
  // Each line of ps output is much smaller than 256B, so it's safe.
  char buf[ELBERT_BUFFER_SZ];
  FILE* fp;
  /* This is best effort service. Check upgrade process only if ps
     command was able to execute. */
  fp = popen("ps", "r");
  if (fp) {
    while (fgets(buf, ELBERT_BUFFER_SZ, fp) != NULL) {
      if (strstr(buf, ELBERT_BIOS_UTIL) || strstr(buf, ELBERT_FPGA_UTIL) ||
          strstr(buf, ELBERT_BMC_FLASH) || strstr(buf, ELBERT_PSU_UTIL)) {
        is_upgrading = true;
        break;
      }
    }
    pclose(fp);
  }
  return is_upgrading;
}

void elbert_update_sys_led(struct system_status* status) {
  bool sys_ok = true;
  bool beacon_on = false;
  bool upgraded = false;
  int pim = 0;
  int rc = 0;
  int read_val = 0;
  char buf[ELBERT_BUFFER_SZ];
  int beacon_mode;

  // Check if beacon LED is wanted.
  beacon_on = elbert_check_beacon_led(&beacon_mode);

  // Check system status.
  if (!status->all_psu_ok || !status->all_fan_ok || !status->smb_ok) {
    sys_ok = false;
    if (!status->all_psu_ok &&
        (status->all_psu_ok != status->prev_all_psu_ok)) {
      syslog(
          LOG_WARNING,
          "PSU error for updating SYS LED. See last PSU details in this log for root cause");
    } else if (
        !status->all_fan_ok &&
        (status->all_fan_ok != status->prev_all_fan_ok)) {
      syslog(
          LOG_WARNING,
          "Fan error for updating SYS LED. See last FAN details in this log for root cause");
    } else if (!status->smb_ok && (status->smb_ok != status->prev_smb_ok)) {
      syslog(
          LOG_WARNING,
          "SMB error for updating SYS LED. See last SMB details in this log for root cause");
    }
  } else {
    // If PSU and FAN are present, check if all PIMs are present too.
    for (pim = 2; pim <= ELBERT_MAX_PIM_ID; pim++) {
      snprintf(
          buf, ELBERT_BUFFER_SZ, "%spim%d_present", SMB_PREFIX, pim);
      rc = elbert_read_sysfs(buf, &read_val);
      if ((rc != 0) || (read_val == 0)) {
        sys_ok = false;
        break;
      }
    }
  }

  // Beacon takes priority over other policy.
  if (beacon_on) {
    // If beacon is still on and mode hasn't changed, do nothing.
    if (beacon_mode != status->beacon_mode) {
      switch (beacon_mode) {
        case BEACON_MODE_LOCATOR:
          // Blink blue @ 1s freq.
          elbert_set_blink_freq(BEACON_BLINK_FREQ_LOCATOR);
          elbert_set_sysled("system", 1, 0, 0, 0, 1);
          syslog(LOG_INFO, "Beacon LED is ON in mode locator\n");
          break;
        case BEACON_MODE_NETSTATE:
          // Blink red @ .25s freq.
          elbert_set_blink_freq(BEACON_BLINK_FREQ_NETSTATE);
          elbert_set_sysled("system", 0, 0, 0, 1, 1);
          syslog(LOG_INFO, "Beacon LED is ON in mode netstate\n");
          break;
        case BEACON_MODE_DRAINED:
          // Blink green @ .5s freq.
          elbert_set_blink_freq(BEACON_BLINK_FREQ_DRAINED);
          elbert_set_sysled("system", 0, 0, 1, 0, 1);
          syslog(LOG_INFO, "Beacon LED is ON in mode drained\n");
          break;
        case BEACON_MODE_AUDIT:
          // Blink amber @ .75s freq.
          elbert_set_blink_freq(BEACON_BLINK_FREQ_AUDIT);
          elbert_set_sysled("system", 0, 1, 0, 0, 1);
          syslog(LOG_INFO, "Beacon LED is ON in mode audit\n");
          break;
        default:
          syslog(LOG_ERR, "Beacon LED invalid mode %d\n", beacon_mode);
          break;
      }
    }
  } else {
    if (elbert_check_upgrade()) {
      syslog(LOG_INFO, "Elbert is upgrading\n");
      do {
        // Alternating flash blue/amber.
        elbert_set_sysled("system", 1, 0, 0, 0, 0);
        usleep(UPGRADE_FLASH_FREQ_USEC);
        elbert_set_sysled("system", 0, 1, 0, 0, 0);
        usleep(UPGRADE_FLASH_FREQ_USEC);
      } while (elbert_check_upgrade());
      upgraded = true;
    }

    /* Need to force LED update in cases where status has changed or status
     * hasn't changed but beacon has been turned off or upgrade occurred. */
    if ((sys_ok != status->sys_ok) || (beacon_on != status->beacon_on) ||
        upgraded) {
      // Reset blink rate after beacon off.
      if (beacon_on != status->beacon_on) {
        elbert_set_blink_freq(DEFAULT_BLINK_FREQ_MSEC);
        syslog(LOG_INFO, "Beacon LED is OFF\n");
      }

      if (sys_ok) {
        elbert_set_sysled("system", 1, 0, 0, 0, 0);
      } else {
        elbert_set_sysled("system", 0, 1, 0, 0, 0);
      }

      if (sys_ok != status->sys_ok) {
        syslog(
            LOG_WARNING,
            "SYS LED color change occured previous:%s now:%s\n",
            status->sys_ok ? "Good" : "BAD",
            sys_ok ? "Good" : "BAD");
      }
    }
  }

  status->beacon_on = beacon_on;
  status->beacon_mode = beacon_mode;
  status->sys_ok = sys_ok;
}

void elbert_init_all_led() {
  int fan = 0;
  char buf[ELBERT_BUFFER_SZ];
  elbert_set_sysled("psu", 1, 0, 0, 0, 0);
  elbert_set_sysled("fan", 1, 0, 0, 0, 0);
  elbert_set_sysled("sc", 1, 0, 0, 0, 0);
  elbert_set_sysled("system", 1, 0, 0, 0, 0);
  for (fan = 1; fan <= ELBERT_MAX_FAN; fan++) {
    snprintf(
        buf, ELBERT_BUFFER_SZ, "%sfan%d%s", FAN_PREFIX, fan, SMB_LED_GREEN);
    elbert_write_sysfs(buf, 0);
    snprintf(buf, ELBERT_BUFFER_SZ, "%sfan%d%s", FAN_PREFIX, fan, SMB_LED_RED);
    elbert_write_sysfs(buf, 0);
    snprintf(buf, ELBERT_BUFFER_SZ, "%sfan%d%s", FAN_PREFIX, fan, SMB_LED_BLUE);
    elbert_write_sysfs(buf, 1);
    snprintf(
        buf, ELBERT_BUFFER_SZ, "%sfan%d%s", FAN_PREFIX, fan, SMB_LED_AMBER);
    elbert_write_sysfs(buf, 0);
  }
}

void init_system_status(struct system_status* status) {
  int i;

  // Initialize all fields to "good" state.
  status->prev_all_psu_ok = true;
  status->all_psu_ok = true;
  for (i = 0; i < ELBERT_MAX_PSU; i++) {
    status->psu_ok[i] = true;
  }

  status->prev_all_fan_ok = true;
  status->all_fan_ok = true;
  for (i = 0; i < ELBERT_MAX_FAN; i++) {
    status->fan_ok[i] = true;
  }

  status->smb_pwr_good = 1;
  status->smb_snr_health = 1;
  status->prev_smb_ok = true;
  status->smb_ok = true;

  status->sys_ok = true;
  status->beacon_on = false;
  status->beacon_mode = BEACON_MODE_OFF;
}

// Lightweight LED control daemon.
int main(int argc, char* const argv[]) {
  struct system_status status;

  // Set default values to "good" state.
  init_system_status(&status);

  // Initialize LED frequency.
  elbert_set_blink_freq(DEFAULT_BLINK_FREQ_MSEC);

  /* Since all status start as "good", turn on all blue LEDS
     This will be updated in no time, in the following loop,
     if needed. */
  elbert_init_all_led();

  while (true) {
    // Update FAN LED and LEDs on each fan.
    elbert_update_fan_led(&status);

    // Update PSU LED and LEDs on each psu.
    elbert_update_psu_led(&status);

    // Update SMB LED.
    elbert_update_smb_led(&status);

    // Update SYS LED.
    elbert_update_sys_led(&status);

    // Sleep until next sweep.
    sleep(UPDATE_FREQ_SEC);
  }

  // Execution is not supposed to be here. End with error code (-1).
  return -1;
}
