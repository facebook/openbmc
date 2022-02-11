/*
 *
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

#include "led-controld.h"

int yamp_write_sysfs(char *str, unsigned int val)
{
    char buf[YAMP_BUFFER_SZ];
    snprintf(buf, YAMP_BUFFER_SZ, "echo %d > %s\n", val, str);
    return system(buf);
}

int yamp_read_sysfs(char *str, int *val)
{
    FILE *fp;
    char value[64];
    char *endPtr;
    int read_val = 0;;
    int rc = 0;
    long int long_val = 0;

    fp = fopen(str, "r");
    if (!fp) {
      return YAMP_FS_ERR;
    }
    rc = fscanf(fp, "%s", value);
    fclose(fp);
    if (rc != 1) {
      return YAMP_READ_ERR;
    }

    long_val = strtol(value, &endPtr, 0);

    if (endPtr >= (value + strlen(value)))
    {
       // Practically, we read 8bit registers only, whose maximum value is
       // at most 0xFF, so the following is safe.
       read_val = (int) long_val;
       *val = read_val;
    } else {
         printf("Invalid string parsing. path:[%s] text_val:[%s]\
             errno:%d str_ptr:0x%08x end_ptr:0x%08x\n", str, value,
             errno,
             (unsigned int)value, (unsigned int)endPtr);
         return YAMP_PARSE_ERR;
    }
    return 0;
}

int yamp_write_cpld_reg(char *prefix, char *reg, int val)
{
    char buf[YAMP_BUFFER_SZ];
    snprintf(buf, YAMP_BUFFER_SZ, "%s%s", prefix, reg);
    return yamp_write_sysfs(buf, val);
}

int yamp_read_cpld_reg(char *prefix, char *reg, int *val)
{
    char buf[YAMP_BUFFER_SZ];
    snprintf(buf, YAMP_BUFFER_SZ, "%s%s", prefix, reg);
    return yamp_read_sysfs(buf, val);
}

void yamp_set_blink_freq() {
  int msec = BLINK_FREQ_MSEC;
  int lower = msec % 256;
  int upper = msec / 256;
  yamp_write_cpld_reg(SUP_PREFIX, SUP_LED_FREQ_H, upper);
  yamp_write_cpld_reg(SUP_PREFIX, SUP_LED_FREQ_L, lower);
}

void yamp_set_sysled(char *name, int green, int red, int blink)
{
    char led_name[YAMP_BUFFER_SZ];

    snprintf(led_name, YAMP_BUFFER_SZ, "%s%s_led", SUP_PREFIX, name);
    yamp_write_cpld_reg(led_name, COLOR_GREEN, green);
    yamp_write_cpld_reg(led_name, COLOR_RED, red);
    yamp_write_cpld_reg(led_name, COLOR_BLINK, blink);

    return;
}

bool yamp_update_fan_led(bool prev_fan_state, bool *prev_fan_ok)
{
    bool all_fan_ok = true;
    bool this_fan_ok = true;
    char buf[YAMP_BUFFER_SZ];
    int fan;
    int read_val = 0, rc = 0;
    for (fan = 1; fan <= 5; fan++) {
        this_fan_ok = true;
        snprintf(buf, YAMP_BUFFER_SZ, "%sfan%d%s", FAN_PREFIX, fan, SCD_FAN_PRESENT);
        // Check if FAN present
        rc = yamp_read_sysfs(buf, &read_val);
        if (rc != 0) {
            this_fan_ok = false;
            syslog(LOG_WARNING, "Fan %d is not present", fan);
        } else {
            if (read_val != 1) {
                this_fan_ok = false;
                syslog(LOG_WARNING, "Fan %d is not present", fan);
            } else {
                // If fan is present, check rpm is over a minimum threshold
                snprintf(buf, YAMP_BUFFER_SZ, "%sfan%d%s",
                         FAN_PREFIX, fan, FAN_RPM);
                rc = yamp_read_sysfs(buf, &read_val);
                if ((rc != 0) || (read_val < YAMP_FAN_RPM_MIN)) {
                    this_fan_ok = false;
                    syslog(LOG_WARNING, "Fan %d running at %d is less than %d, the minimum threshold",
                        fan, read_val, YAMP_FAN_RPM_MIN);
                }
            }
        }

        if (!this_fan_ok)
          all_fan_ok = false;

        if ((this_fan_ok != prev_fan_ok[fan])) {
          // If current fan is not good, do the best effort to set fan led to red
          snprintf(buf, YAMP_BUFFER_SZ, "%sfan%d%s", FAN_PREFIX, fan, SCD_LED_GREEN);
          yamp_write_sysfs(buf, this_fan_ok ? 1 : 0);
          snprintf(buf, YAMP_BUFFER_SZ, "%sfan%d%s", FAN_PREFIX, fan, SCD_LED_RED);
          yamp_write_sysfs(buf, this_fan_ok ? 0 : 1);
        }
        prev_fan_ok[fan] = this_fan_ok;
    }
    if (prev_fan_state != all_fan_ok) {
        if (all_fan_ok) {
            yamp_set_sysled("fan", 1, 0, 0);
        } else {
            yamp_set_sysled("fan", 0, 1, 1);
        }
        syslog(LOG_WARNING, "FAN Led color change occured previous:%s now:%s\n",
            prev_fan_state? "Good" : "BAD",
            all_fan_ok? "Good" : "BAD");
    }
    return all_fan_ok;
}

bool yamp_update_psu_led(bool prev_psu_state)
{
    bool all_psu_ok = true;
    bool this_psu_ok = true;
    int psu = 0;
    int iter = 0;
    int expected_psu[2] = {1, 4};
    int psu_status_reg = 0;
    int psu_present = 0;
    int psu_input_ok = 0;
    yamp_read_cpld_reg(SCD_PREFIX, SCD_PSU_STATUS, &psu_status_reg);
    for (iter = 0; iter < sizeof(expected_psu)/sizeof(int); iter++) {
        psu = expected_psu[iter] - 1; // 0-based in register definition
        this_psu_ok = true;
        psu_present = (psu_status_reg >> (SCD_PSU_PRESENT_BIT + psu)) & 0x1;
        psu_input_ok = (psu_status_reg >> (SCD_PSU_OK_BIT + psu)) & 0x1;
        if ((psu_present != 1) || (psu_input_ok != 1)){
           this_psu_ok = false;
           syslog(LOG_WARNING, "PSU %d is %s and the input is %s\n",expected_psu[iter],
               psu_present ? "present" : "not present",
               psu_input_ok ? "ok" : "not ok");
        }if (!this_psu_ok) {
          all_psu_ok = false;
        }

    }
    if (prev_psu_state != all_psu_ok) {
        if (all_psu_ok) {
            yamp_set_sysled("psu", 1, 0, 0);
        } else {
            yamp_set_sysled("psu", 0, 1, 1);
        }
        syslog(LOG_WARNING, "PSU Led color change occured previous:%s now:%s\n",
            prev_psu_state? "Good" : "BAD",
            all_psu_ok? "Good" : "BAD");
    }
    return all_psu_ok;
}

bool yamp_update_beacon_led(bool prev_beacon_on)
{
    bool beacon_on;
    if(access(BEACON_FS_STUB, F_OK) != -1) {
        beacon_on = true;
    } else {
        beacon_on = false;
    }

    if (prev_beacon_on != beacon_on) {
        if (beacon_on) {
            yamp_write_cpld_reg(SUP_PREFIX, BEACON_LED, 1);
        } else {
            yamp_write_cpld_reg(SUP_PREFIX, BEACON_LED, 0);
        }
    }
    return beacon_on;
}

bool yamp_check_upgrade()
{
    bool is_upgrading = false;
    // Each line of ps output is much smaller than 256B, so it's safe
    char buf[YAMP_BUFFER_SZ];
    FILE *fp;
    // This is best effort service. Check upgrade process only if ps
    // command was able to execute
    fp = popen("ps", "r");
    if (fp) {
      while(fgets(buf, YAMP_BUFFER_SZ, fp) != NULL) {
          if (strstr(buf, YAMP_BIOS_UTIL) ||
              strstr(buf, YAMP_FPGA_UTIL) ||
              strstr(buf, YAMP_BMC_FLASH)) {
              is_upgrading = true;
              break;
          }
      }
      pclose(fp);
    }
    return is_upgrading;
}

bool yamp_update_sys_led(bool prev_sys_ok, bool psu_ok, bool fan_ok)
{
    bool sys_ok = true;
    bool is_upgrading = false;
    int blink_on = 0;
    int lc = 0;
    int rc = 0;
    int read_val = 0;
    char buf[YAMP_BUFFER_SZ];

    // Check if any upgrade running
    is_upgrading = yamp_check_upgrade();

    if (!psu_ok || !fan_ok) {
      sys_ok = false;
      if (!psu_ok)
        syslog(LOG_WARNING, "psu error for updating sys led. See last psu details in this log for root cause");
      else if (!fan_ok)
        syslog(LOG_WARNING, "fan error for updating sys led. See last fan details in this log for root cause");
    } else {
      // If PSU and FAN are present, check if all LCs are present too
      for (lc = 1; lc <= YAMP_MAX_LC; lc++) {
          snprintf(buf, YAMP_BUFFER_SZ, "%s%s%d%s",
              SCD_PREFIX, YAMP_LC, lc, YAMP_LC_PRSNT);
          rc = yamp_read_sysfs(buf, &read_val);
          if ((rc != 0) || (read_val == 0)) {
              sys_ok = false;
              break;
          }
      }
    }
    if (is_upgrading || (prev_sys_ok != sys_ok)) {
        if (is_upgrading) {
          blink_on = 1;
          syslog(LOG_INFO, "Yamp is upgrading\n");
        } else {
          blink_on = 0;
        }
        if (sys_ok) {
            yamp_set_sysled("system", 1, 0, blink_on);
        } else {
            yamp_set_sysled("system", 0, 1, blink_on);
        }
        syslog(LOG_WARNING, "SYS Led color change occured previous:%s now:%s\n",
            prev_sys_ok? "Good" : "BAD",
            sys_ok? "Good" : "BAD");
    }
    return sys_ok;
}

void yamp_init_all_led() {
  int fan = 0;
  char buf[YAMP_BUFFER_SZ];
  yamp_set_sysled("psu", 1, 0, 0);
  yamp_set_sysled("fan", 1, 0, 0);
  yamp_set_sysled("system", 1, 0, 0);
  yamp_write_cpld_reg(SUP_PREFIX, BEACON_LED, 0);
  for (fan = 1; fan <= 5 ; fan++) {
      snprintf(buf, YAMP_BUFFER_SZ, "%sfan%d%s", FAN_PREFIX, fan, SCD_LED_GREEN);
      yamp_write_sysfs(buf, 1);
      snprintf(buf, YAMP_BUFFER_SZ, "%sfan%d%s", FAN_PREFIX, fan, SCD_LED_RED);
      yamp_write_sysfs(buf, 0);
  }
}

// Lightweight LED control daemon
int main (int argc, char * const argv[]) {
    // Remember current LED status, and update it only when the
    // state changes
    bool psu_ok = true;
    bool fan_ok = true;
    bool sys_ok = true;
    bool beacon_on = false;
    bool prev_fan_ok[5] = { true, true, true, true, true};
    // Initializ LED frequency
    yamp_set_blink_freq();
    // Since all status start as "good", turn on all green LEDS
    // This will be updated in no time, in the following loop,
    // if needed.
    yamp_init_all_led();
    while(true) {
        // Update FAN LED and LEDs on each fan
        fan_ok = yamp_update_fan_led(fan_ok, prev_fan_ok);

        // Update PSU LED and LEDs on each psu
        psu_ok = yamp_update_psu_led(psu_ok);

        // Update Beacon LED
        beacon_on = yamp_update_beacon_led(beacon_on);

        // Update SYS LED
        sys_ok = yamp_update_sys_led(sys_ok, psu_ok, fan_ok);

        // Sleep until next sweep
        sleep(UPDATE_FREQ_SEC);
    }

    // Execution is not supposed to be here. End with error code (-1)
    return -1;
}
