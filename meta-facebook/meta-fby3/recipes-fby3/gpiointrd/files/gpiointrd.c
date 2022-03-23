/*
 * sensord
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <sys/un.h>
#include <sys/file.h>
#include <openbmc/libgpio.h>
#include <openbmc/ipmi.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/pal.h>
#include <facebook/fby3_gpio.h>
#include <openbmc/ras.h>

#define POLL_TIMEOUT -1 /* Forever */

struct delayed_log {
  useconds_t usec;
  char msg[1024];
};

err_t platform_state[] = {
  /* Value of the PFR state. */
  // CPLD Nios firmware T0 flow
  {0x01, "PLATFORM_STATE_CPLD_NIOS_WAITING_TO_START"},
  {0x02, "PLATFORM_STATE_CPLD_NIOS_STARTED"},
  {0x03, "PLATFORM_STATE_ENTER_TMIN1"},
  {0x04, "PLATFORM_STATE_TMIN1_RESERVED1"},
  {0x05, "PLATFORM_STATE_TMIN1_RESERVED2"},
  {0x06, "PLATFORM_STATE_BMC_FLASH_AUTHENTICATION"},
  {0x07, "PLATFORM_STATE_PCH_FLASH_AUTHENTICATION"},
  {0x08, "PLATFORM_STATE_AUTHENTICATION_FAILED_LOCKDOWN"},
  {0x09, "PLATFORM_STATE_ENTER_T0"},
  // Timed Boot Progress
  {0x0A, "PLATFORM_STATE_T0_BMC_BOOTED"},
  {0x0B, "PLATFORM_STATE_T0_ME_BOOTED"},
  {0x0C, "PLATFORM_STATE_T0_ACM_BOOTED"},
  {0x0D, "PLATFORM_STATE_T0_BIOS_BOOTED"},
  {0x0E, "PLATFORM_STATE_T0_BOOT_COMPLETE"},
  // Update event
  {0x10, "PLATFORM_STATE_PCH_FW_UPDATE"},
  {0x11, "PLATFORM_STATE_BMC_FW_UPDATE"},
  {0x12, "PLATFORM_STATE_CPLD_UPDATE"},
  {0x13, "PLATFORM_STATE_CPLD_UPDATE_IN_RECOVERY_MODE"},
  {0x14, "PLATFORM_STATE_COPY_STAGING_TO_RECOVERY"},
  // Recovery
  {0x40, "PLATFORM_STATE_TMIN1_FW_RECOVERY"},
  {0x41, "PLATFORM_STATE_TMIN1_FORCED_ACTIVE_FW_RECOVERY"},
  {0x42, "PLATFORM_STATE_WDT_TIMEOUT_RECOVERY"},
  {0x43, "PLATFORM_STATE_CPLD_RECOVERY_IN_RECOVERY_MODE"},
  // PIT
  {0x44, "PLATFORM_STATE_PIT_L1_LOCKDOWN"},
  {0x45, "PLATFORM_STATE_PIT_L2_FW_SEALED"},
  {0x46, "PLATFORM_STATE_PIT_L2_PCH_HASH_MISMATCH_LOCKDOWN"},
  {0x47, "PLATFORM_STATE_PIT_L2_BMC_HASH_MISMATCH_LOCKDOWN"},
};

static void
log_slot_present(uint8_t slot_id, gpio_value_t value)
{
  // no need to consider Config C, because the HW "PRSNT_MB_BMC_SLOTX_BB_N" is not connected to Config C BMC.

  if ( value == GPIO_VALUE_LOW ) {
    syslog(LOG_CRIT, "slot%d present", slot_id);
  } else if ( value == GPIO_VALUE_HIGH ) {
    syslog(LOG_CRIT, "Abnormal - slot%d not detected", slot_id);
    return;
  }
}

static void
slot_present(gpiopoll_pin_t *gpdesc, gpio_value_t value) {
  uint32_t slot_id;
  int config_status = 0;
  uint8_t board_type = 0;
  const struct gpiopoll_config *cfg = gpio_poll_get_config(gpdesc);
  assert(cfg);
  sscanf(cfg->shadow, "PRSNT_MB_BMC_SLOT%u_BB_N", &slot_id);

  config_status = bic_is_m2_exp_prsnt(1);
  if (config_status < 0) config_status = 0;
  if ((config_status & PRESENT_2OU) == PRESENT_2OU) {
    fby3_common_get_2ou_board_type(1, &board_type);
    if (board_type == GPV3_MCHP_BOARD || board_type == GPV3_BRCM_BOARD) {
      // GPV3 config only have slot1 & 3
      if (slot_id == 2 || slot_id == 4) return;
    } else if (board_type == DP_RISER_BOARD) {
      // DP config only have slot1
      if (slot_id == 2 || slot_id == 3 || slot_id == 4) return;
    }
  }

  config_status = bic_is_m2_exp_prsnt(3);
  if (config_status < 0) config_status = 0;
  if ((config_status & PRESENT_2OU) == PRESENT_2OU) {
    fby3_common_get_2ou_board_type(1, &board_type);
    if (board_type == GPV3_MCHP_BOARD || board_type == GPV3_BRCM_BOARD) {
      // GPV3 config only have slot1 & 3
      if (slot_id == 4) return;
    }
  }

  log_gpio_change(gpdesc, value, 0, true);

  log_slot_present(slot_id, value);
  if ( value == GPIO_VALUE_LOW ) pal_check_sled_mgmt_cbl_id(slot_id, NULL, true, DVT_BB_BMC);
  pal_check_slot_cpu_present(slot_id);
}

static void 
slot_hotplug_setup(uint8_t slot_id) {
#define SCRIPT_PATH "sh /etc/init.d/server-init.sh %d &"
  char cmd[128] = {0};
  int cmd_len = sizeof(cmd);

  snprintf(cmd, cmd_len, SCRIPT_PATH, slot_id);
  if ( system(cmd) != 0 ) {
    syslog(LOG_CRIT, "Failed to run: %s", cmd);
  }
}

static void 
slot_hotplug_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  uint32_t slot_id;
  const struct gpiopoll_config *cfg = gpio_poll_get_config(gp);
  assert(cfg);
  sscanf(cfg->shadow, "PRSNT_MB_BMC_SLOT%u_BB_N", &slot_id);
  slot_hotplug_setup(slot_id);
  log_gpio_change(gp, curr, 0, true);
  log_slot_present(slot_id, curr);
  if ( curr == GPIO_VALUE_LOW ) {
    pal_check_sled_mgmt_cbl_id(slot_id, NULL, true, DVT_BB_BMC);
    // Wait for IPMB ready
    sleep(6);
    pal_check_slot_cpu_present(slot_id);
  }
}

static void
issue_slot_ocp_fault_sel(uint8_t slot_id) {
  uint8_t tbuf[32] = {0x00};
  uint8_t rbuf[32] = {0x00};
  uint8_t tlen = 0;
  uint16_t rlen = 0;

  tbuf[0]  = slot_id;
  tbuf[1]  = NETFN_STORAGE_REQ << 2;
  tbuf[2]  = CMD_STORAGE_ADD_SEL;
  tbuf[5]  = 0x02; //record ID
  tbuf[10] = 0x20; //addr 
  tbuf[11] = 0x00;
  tbuf[12] = 0x04;
  tbuf[13] = 0xC9;
  tbuf[14] = 0x46;
  //When the OCP fault is asserted, the slot is shutdown.
  //So, only the assertion event is issued.
  tbuf[15] = 0x6F;
  tbuf[16] = 0x08; //fault event
  tbuf[17] = 0xff;
  tbuf[18] = 0xff; 
  tlen = 19;
  lib_ipmi_handle(tbuf, tlen, rbuf, &rlen);
}

#if 0
static void
issue_slot_plt_state_sel(uint8_t slot_id) {
  char path[128];
  int ret = 0, i2cfd = 0, retry=0, index = 0;
  uint8_t tbuf[1] = {0}, rbuf[1] = {0};
  uint8_t tlen = 1, rlen = 1;
  uint8_t o_plat_state = -1, n_plat_state = 0;
  char *plat_str = "NA";

  snprintf(path, sizeof(path), "/dev/i2c-%d", (slot_id+SLOT_BUS_BASE));
  i2cfd = open(path, O_RDWR);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %s", __func__, path);
  }

  tbuf[0] = PLATFORM_STATE_OFFSET;
  retry = 0;
  while (retry < MAX_READ_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_INTENT_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      n_plat_state = rbuf[0];
      break;
    }
  }
  if (retry == MAX_READ_RETRY) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
  }

  if ( i2cfd > 0 ) close(i2cfd);

  if ( n_plat_state != o_plat_state && n_plat_state != 0x00) {
    for (index = 0; index < (sizeof(platform_state)/sizeof(err_t)); index++) {
      if (n_plat_state == platform_state[index].err_id) {
        plat_str = platform_state[index].err_des;
        break;
      }
    }
    if ( (n_plat_state != 0x01) && (n_plat_state != 0x02) ) {
      syslog(LOG_CRIT, "FRU: %d, PFR - Platform state: %s (0x%02X)", slot_id, plat_str, n_plat_state);
    }
    o_plat_state = n_plat_state;
  }
}
#endif

static pthread_t tid_hsc[4] = {-1, -1, -1, -1};
static void *hsc_log_handler(void *arg) {
  uint8_t tbuf[16] = {0x00};
  uint8_t tlen = 0;
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret = 0;
  int slot_id = (int)arg;

  syslog(LOG_INFO, "%s slot%d : wait BIC ready 3s", __FUNCTION__, slot_id);
  sleep(3);

  do {
    tbuf[0] = 0x9C;
    tbuf[1] = 0x9C;
    tbuf[2] = 0x00;
    tlen = 3;
    ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_HSC_STATUS, tbuf, tlen, rbuf, &rlen);
    if (ret == 0) {
      uint8_t oem_status = rbuf[4];
      if ( oem_status & (1 << 2) ) {
        syslog(LOG_CRIT, "FRU: %d, HSC TEMP FAULT (oem_status:0x%02X)", slot_id, oem_status);
      }
      if ( oem_status & (1 << 3) ) {
        syslog(LOG_CRIT, "FRU: %d, HSC VIN UV FAULT (oem_status:0x%02X)", slot_id, oem_status);
      }
      if ( oem_status & (1 << 4) ) {
        syslog(LOG_CRIT, "FRU: %d, HSC IOUT OC FAULT (oem_status:0x%02X)", slot_id, oem_status);
      }
      if ( oem_status & (1 << 5) ) {
        syslog(LOG_CRIT, "FRU: %d, HSC VIN OV FAULT (oem_status:0x%02X)", slot_id, oem_status);
      }

    } else {
      tbuf[0] = 0xB8;
      tbuf[1] = 0xD9;
      tbuf[2] = 0x57;
      tbuf[3] = 0x01;
      tbuf[4] = 0x00;
      tbuf[5] = 0x06;
      tbuf[6] = 0x80;
      tbuf[7] = 0x00;
      tbuf[8] = 0x00;
      tbuf[9] = 0x01;
      tbuf[10] = 0x02;
      tbuf[11] = 0x79;
      tlen = 12;
      ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
      if (ret < 0) {
        syslog(LOG_INFO, "slot%d : get status word fail !\n", slot_id);
        break;
      }
      uint16_t status_word = rbuf[4] | (rbuf[5] << 8);
      if ( status_word & (1 << 2) ) {
        syslog(LOG_CRIT, "FRU: %d, HSC TEMP FAULT (status_word:0x%04X)", slot_id, status_word);
      }
      if ( status_word & (1 << 3) ) {
        syslog(LOG_CRIT, "FRU: %d, HSC VIN UV FAULT (status_word:0x%04X)", slot_id, status_word);
      }
      if ( status_word & (1 << 4) ) {
        syslog(LOG_CRIT, "FRU: %d, HSC IOUT OC FAULT (status_word:0x%04X)", slot_id, status_word);
      }
      tbuf[0] = 0xB8;
      tbuf[1] = 0xD9;
      tbuf[2] = 0x57;
      tbuf[3] = 0x01;
      tbuf[4] = 0x00;
      tbuf[5] = 0x00;
      tbuf[6] = 0x80;
      tbuf[7] = 0x00;
      tbuf[8] = 0x00;
      tbuf[9] = 0x01;
      tbuf[10] = 0x00;
      tbuf[11] = 0x03;
      tlen = 12;
      ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
      if (ret < 0) {
        syslog(LOG_INFO, "slot%d : sent clear fault fail!\n", slot_id);
        break;
      }
    }
  } while (0);

  tid_hsc[slot_id - 1] = -1;
  pthread_exit(NULL);
}

static void
slot_hsc_fault_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  int ret = 0;
  uint32_t slot_id;
  static uint8_t init[4] = {0};
  const struct gpiopoll_config *cfg = gpio_poll_get_config(gp);
  assert(cfg);
  // because we don't have consistent shadow name here, we use description instead
  sscanf(cfg->description, "GPIOM%u", &slot_id);
  slot_id += 1;

  if (tid_hsc[slot_id - 1] != -1) {
    ret = pthread_cancel(tid_hsc[slot_id - 1]);
    if (ret == ESRCH) {
      syslog(LOG_INFO, "%s: No hsc_log_handler thread exists", __FUNCTION__);
    } else {
      syslog(LOG_INFO, "%s: hsc_log_handler thread is cancelled", __FUNCTION__);
    }
    tid_hsc[slot_id - 1] = -1;
  }

  if ( curr == GPIO_VALUE_LOW ) {
    log_gpio_change(gp, curr, 0, true);
    issue_slot_ocp_fault_sel(slot_id);
  } else {
    if (init[slot_id - 1] == 0 ) {
      init[slot_id - 1]++;
    } else {
      log_gpio_change(gp, curr, 0, true);
    }

    if (pthread_create(&tid_hsc[slot_id - 1], NULL, hsc_log_handler, (void *)slot_id) == 0) {
      pthread_detach(tid_hsc[slot_id - 1]);
    } else{
      tid_hsc[slot_id - 1] = -1;
      syslog(LOG_WARNING, "%s: create hsc_log_handler thread fail", __FUNCTION__);
    }
  }
}

static void
slot_hsc_fault_init(gpiopoll_pin_t *gp, gpio_value_t curr) {
  slot_hsc_fault_hndlr(gp, (gpio_value_t)NULL, curr);
}

#if 0
static void
slot_pfr_plt_state_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  uint32_t slot_id;
  uint8_t bmc_location;

  if (fby3_common_get_bmc_location(&bmc_location) < 0 ) {
    return;
  }
  if (bmc_location == NIC_BMC) {
    slot_id = 1;
  } else {
    const struct gpiopoll_config *cfg = gpio_poll_get_config(gp);
    assert(cfg);
    sscanf(cfg->shadow, "SMB_BMC_SLOT%u_ALT_N", &slot_id);
  }
  issue_slot_plt_state_sel(slot_id);
}
#endif

static void
slot_power_control(char *opt) {
  char cmd[64] = "/usr/local/bin/power-util ";
  strcat(cmd, opt);
  if ( system(cmd) != 0 ) {
    syslog(LOG_WARNING, "Failed to do %s", cmd);
  }
}

static void
slot_rst_hndler(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  uint32_t slot_id;
  char pwr_cmd[32];
  const struct gpiopoll_config *cfg = gpio_poll_get_config(gp);
  assert(cfg);
  sscanf(cfg->shadow, "FM_RESBTN_SLOT%u_BMC_N", &slot_id);
  sprintf(pwr_cmd, "slot%u reset", slot_id);
  if ( curr == GPIO_VALUE_LOW ) slot_power_control(pwr_cmd);
  log_gpio_change(gp, curr, 0, true);
}

static void
ocp_nic_hotplug_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  if (curr == GPIO_VALUE_LOW)
    syslog(LOG_CRIT, "OCP NIC Inserted");
  else
    syslog(LOG_CRIT, "OCP NIC Removed");
}

static void
ocp_nic_init(gpiopoll_pin_t *gp, gpio_value_t value) {
  if (value == GPIO_VALUE_HIGH)
    syslog(LOG_CRIT, "OCP NIC Removed");
}

// GPIO table of the class 1
static struct gpiopoll_config g_class1_gpios[] = {
  // shadow, description, edge, handler, oneshot
  {"PRSNT_MB_BMC_SLOT1_BB_N", "GPIOB4",   GPIO_EDGE_BOTH,     slot_hotplug_hndlr,       slot_present},
  {"PRSNT_MB_BMC_SLOT2_BB_N", "GPIOB5",   GPIO_EDGE_BOTH,     slot_hotplug_hndlr,       slot_present},
  {"PRSNT_MB_BMC_SLOT3_BB_N", "GPIOB6",   GPIO_EDGE_BOTH,     slot_hotplug_hndlr,       slot_present},
  {"PRSNT_MB_BMC_SLOT4_BB_N", "GPIOB7",   GPIO_EDGE_BOTH,     slot_hotplug_hndlr,       slot_present},
  {"FM_RESBTN_SLOT1_BMC_N",   "GPIOAC2",  GPIO_EDGE_BOTH,     slot_rst_hndler,          NULL},
  {"FM_RESBTN_SLOT2_BMC_N",   "GPIOAC3",  GPIO_EDGE_BOTH,     slot_rst_hndler,          NULL},
  {"FM_RESBTN_SLOT3_BMC_N",   "GPIOI4",   GPIO_EDGE_BOTH,     slot_rst_hndler,          NULL},
  {"FM_RESBTN_SLOT4_BMC_N",   "GPIOI6",   GPIO_EDGE_BOTH,     slot_rst_hndler,          NULL},
  {"HSC_FAULT_SLOT1_N",       "GPIOM0",   GPIO_EDGE_BOTH,     slot_hsc_fault_hndlr,     slot_hsc_fault_init},
  {"HSC_FAULT_BMC_SLOT2_N_R", "GPIOM1",   GPIO_EDGE_BOTH,     slot_hsc_fault_hndlr,     slot_hsc_fault_init},
  {"HSC_FAULT_SLOT3_N",       "GPIOM2",   GPIO_EDGE_BOTH,     slot_hsc_fault_hndlr,     slot_hsc_fault_init},
  {"HSC_FAULT_BMC_SLOT4_N_R", "GPIOM3",   GPIO_EDGE_BOTH,     slot_hsc_fault_hndlr,     slot_hsc_fault_init},
  {"OCP_NIC_PRSNT_BMC_N",     "GPIOM5",   GPIO_EDGE_BOTH,     ocp_nic_hotplug_hndlr,    ocp_nic_init},
  {"FAST_PROCHOT_BMC_N_R",    "GPIOL4",   GPIO_EDGE_BOTH,     fast_prochot_hndlr,       fast_prochot_init},
};

// GPIO table of the class 2
static struct gpiopoll_config g_class2_gpios[] = {
  // shadow, description, edge, handler, oneshot
};

static void
*ac_button_event() {
  unsigned int gpio_vals = 0x0;
  /*
  Baseboard AC button:
  - Press it more than 4s or will do nothing.

  Server board AC on/ off/ cycle button:
  - Pwr cycle: press it more than 4s
  - Pwr off: press it more than 8s
  - Pwr on: press it more than 1s and <= 4s
  */
  enum {
    BTN_BMC = 0x0,
    BTN_SLOT1,
    BTN_SLOT2,
    BTN_SLOT3,
    BTN_SLOT4,
    BTN_ARRAY_SIZE,

    /*Get cmd_str*/
    PWR_12V_OFF = 0x0,
    PWR_12V_ON,
    PWR_12V_CYCLE,
    SLED_CYCLE,
    UNKNOWN_PWR_ACTION,
  };
  const char *cmd_list[] = {
    "slot%c 12V-off",
    "slot%c 12V-on",
    "slot%c 12V-cycle",
    "sled%ccycle"
  };
  const char *shadows[] = {
    "ADAPTER_BUTTON_BMC_CO_N_R",
    "AC_ON_OFF_BTN_SLOT1_N",
    "AC_ON_OFF_BTN_SLOT2_N",
    "AC_ON_OFF_BTN_SLOT3_N",
    "AC_ON_OFF_BTN_SLOT4_N"
  };
  uint8_t action[BTN_ARRAY_SIZE] = {0};
  bool is_asserted[BTN_ARRAY_SIZE] = {0};
  int time_elapsed[BTN_ARRAY_SIZE] = {0};
  int i = 0;
  uint8_t pwr_sts = 0;

  pthread_detach(pthread_self());

  memset(action, UNKNOWN_PWR_ACTION, BTN_ARRAY_SIZE);
  memset(is_asserted, false, BTN_ARRAY_SIZE);

  //setup the default status
  if ( gpio_get_value_by_shadow_list(shadows, ARRAY_SIZE(shadows), &gpio_vals) < 0 ) {
    syslog(LOG_CRIT, "AC_ON_OFF buttons are not functional");
    pthread_exit(0);
  }

  //start to poll each pin
  while (1) {
    if ( gpio_get_value_by_shadow_list(shadows, ARRAY_SIZE(shadows), &gpio_vals) < 0 ) {
      syslog(LOG_WARNING, "Could not get the current values of all AC_ON_OFF buttons");
      sleep(1);
      continue;
    }

    for ( i = BTN_BMC; i < BTN_ARRAY_SIZE; i++ ) {
      if ( GETBIT(gpio_vals, i) == GPIO_VALUE_LOW ) {
        if ( is_asserted[i] == false ) {
          syslog(LOG_CRIT, "ASSERT: %s", shadows[i]);
          is_asserted[i] = true;
        }
        time_elapsed[i]++;
      } else if ( GETBIT(gpio_vals, i) == GPIO_VALUE_HIGH && time_elapsed[i] > 0 ) {
        if ( is_asserted[i] == true ) {
           syslog(LOG_CRIT, "DEASSERT: %s", shadows[i]);
           is_asserted[i] = false;
        }

        if ( i == BTN_BMC ) {
          if ( time_elapsed[i] > 4 ) {
            action[i] = SLED_CYCLE;
          }
        } else {
          pal_get_server_12v_power(i, &pwr_sts);
          if (pwr_sts == SERVER_12V_OFF && time_elapsed[i] >= 1) {
             //pwr 12V on since time_elpased >=1
             action[i] = PWR_12V_ON;
          } else if ( time_elapsed[i] > 4 ) {
            if ( time_elapsed[i] <= 8 ) {
              //pwr 12V cycle since 4 < time_elapsed <= 8
              action[i] = PWR_12V_CYCLE;
            } else {
              //pwr 12V off since time_elapsed > 8
              action[i] = PWR_12V_OFF;
            }
          }
        }

        if ( action[i] != UNKNOWN_PWR_ACTION ) {
          char opt[16] = {0};
          char c = 0;
          if ( action[i] == SLED_CYCLE ) {
            c = 0x2D; //2Dh is `-` in ASCII
          } else {
            c = 0x30; //30h is `1` in ASCII
          }
          snprintf(opt, sizeof(opt), cmd_list[action[i]], (c == 0x2D)?c:(c+i));
          slot_power_control(opt);
          action[i] = UNKNOWN_PWR_ACTION;
        }
        time_elapsed[i] = 0;
      }
    }

    //sleep periodically.
    sleep(1);
  }

  return NULL;
}

static void
init_class1_threads() {
  pthread_t tid_ac_button_event;

  //Create a thread to monitor the button
  if ( pthread_create(&tid_ac_button_event, NULL, ac_button_event, NULL) < 0 ) {
    syslog(LOG_WARNING, "Failed to create pthread_create for ac_button_event");
    exit(-1);
  }
}

static void
init_class2_threads() {
  //do nothing
}

int
main(int argc, char **argv) {
  int rc = 0;
  int pid_file = 0;
  uint8_t bmc_location = 0;
  struct gpiopoll_config *gpiop = NULL;
  size_t gpio_cnt = 0;
  gpiopoll_desc_t *polldesc = NULL;

  pid_file = open("/var/run/gpiointrd.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if (EWOULDBLOCK == errno) {
      syslog(LOG_ERR, "Another gpiointrd instance is running...\n");
      exit(-1);
    }
  } else {

    if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
      syslog(LOG_WARNING, "Failed to get the location of BMC!");
      exit(-1);
    }

    if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
      gpiop = g_class1_gpios;
      gpio_cnt = sizeof(g_class1_gpios)/sizeof(g_class1_gpios[0]);
      init_class1_threads();
    } else {
      gpiop = g_class2_gpios;
      gpio_cnt = sizeof(g_class2_gpios)/sizeof(g_class2_gpios[0]);
      init_class2_threads();
    }

    if ( gpio_cnt > 0 ) {
      openlog("gpiointrd", LOG_CONS, LOG_DAEMON);
      syslog(LOG_INFO, "gpiointrd: daemon started");

      polldesc = gpio_poll_open(gpiop, gpio_cnt);
      if ( polldesc == NULL ) {
        syslog(LOG_CRIT, "Cannot start poll operation on GPIOs");
      } else {
        if (gpio_poll(polldesc, POLL_TIMEOUT)) {
          syslog(LOG_CRIT, "Poll returned error");
        }
        gpio_poll_close(polldesc);
      }
    }
  }

  return 0;
}
