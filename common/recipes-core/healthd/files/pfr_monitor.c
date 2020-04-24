/*
 * pfr_monitor.c
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

#include <jansson.h>
#include <syslog.h>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>

#define PLATFORM_STATE 0x03
#define LAST_RECOVERY  0x05
#define LAST_PANIC     0x07
#define MAJOR_ERROR    0x08
#define MINOR_ERROR    0x09

#define INIT_PFR_ERR(tbl, idx, str) tbl[idx] = str

typedef struct _mailbox_t {
  int fd;
  uint8_t fru;
  uint8_t bus;
  uint8_t addr;
  int (*transfer)(struct _mailbox_t *, uint8_t *,uint8_t,uint8_t *,uint8_t);
} mailbox_t;

bool pfr_monitor_enabled = false;

static int pfr_monitor_interval = 2;
static uint8_t pfr_fru_count = 0;
static mailbox_t pfr_mbox[MAX_NUM_FRUS];

static const char *plat_state[256] = {0};
static const char *last_recovery[256] = {0};
static const char *last_panic[256] = {0};
static const char *major_err[256] = {0};
static const char *minor_auth_err[256] = {0};
static const char *minor_update_err[256] = {0};

static int
transfer_bridged(mailbox_t *mbox, uint8_t *tbuf, uint8_t tcnt, uint8_t *rbuf, uint8_t rcnt) {
#ifdef CONFIG_BIC
  return bic_master_write_read(mbox->fru, mbox->bus, mbox->addr, tbuf, tcnt, rbuf, rcnt);
#else
  return -1;
#endif
}

static int
transfer_i2c(mailbox_t *mbox, uint8_t *tbuf, uint8_t tcnt, uint8_t *rbuf, uint8_t rcnt) {
  int ret = -1;
  int retry = 2;
  uint8_t buf[16];

  if (mbox->fd < 0) {
    mbox->fd = i2c_cdev_slave_open(mbox->bus, mbox->addr >> 1, I2C_SLAVE_FORCE_CLAIM);
    if (mbox->fd < 0) {
      syslog(LOG_WARNING, "%s: open i2c%d failed", __func__, mbox->bus);
      return -1;
    }
  }

  do {
    memcpy(buf, tbuf, tcnt);
    if (!(ret = i2c_rdwr_msg_transfer(mbox->fd, mbox->addr, buf, tcnt, rbuf, rcnt))) {
      break;
    }

    syslog(LOG_WARNING, "%s: i2c%d rw failed, offset: %02x", __func__, mbox->bus, buf[0]);
    if (--retry > 0)
      msleep(20);
  } while (retry > 0);

  return ret;
}

static void __attribute__((destructor))
deinit_fd(void) {
  uint8_t i;

  if (!pfr_monitor_enabled) {
    return;
  }

  for (i = 0; i < MAX_NUM_FRUS; i++) {
    if (pfr_mbox[i].fd >= 0) {
      close(pfr_mbox[i].fd);
      pfr_mbox[i].fd = -1;
    }
  }
}

void
initialize_pfr_monitor_config(json_t *conf) {
  json_t *tmp, *fru;
  uint8_t i, fru_cnt, fruid, status = 0;

  do {
    if (!conf) {
      break;
    }

    tmp = json_object_get(conf, "enabled");
    if (!tmp || !json_is_boolean(tmp)) {
      break;
    }
    if (!(pfr_monitor_enabled = json_is_true(tmp))) {
      break;
    }

    tmp = json_object_get(conf, "monitor_interval");
    if (tmp && json_is_number(tmp)) {
      if ((i = json_integer_value(tmp)) > 0) {
        pfr_monitor_interval = i;
      }
    }

    pfr_fru_count = 0;
    memset(pfr_mbox, 0xFF, sizeof(pfr_mbox));  // initialize fds to -1

    tmp = json_object_get(conf, "fru");
    if (!tmp || !json_is_array(tmp)) {
      break;
    }
    fru_cnt = json_array_size(tmp);
    if (!fru_cnt || (fru_cnt > MAX_NUM_FRUS)) {
      break;  // nothing to monitor
    }

    for (i = 0; i < fru_cnt; i++) {
      fru = json_array_get(tmp, i);
      if (!fru || !json_is_number(fru)) {
        continue;
      }

      fruid = json_integer_value(fru);
      if (!pal_is_fru_prsnt(fruid, &status) && status) {
        pfr_mbox[pfr_fru_count++].fru = fruid;
      }
    }

    return;
  } while (0);

  pfr_monitor_enabled = false;
}

void *
pfr_monitor() {
  uint8_t cmd[] = {
    PLATFORM_STATE,  // Platform State
    LAST_RECOVERY,   // Last Recovery Reason
    LAST_PANIC,      // Last Panic Reason
    MAJOR_ERROR,     // Major error code
  };
  uint8_t bus, addr;
  uint8_t i, j, tbuf[8], rbuf[8];
  uint8_t sts[MAX_NUM_FRUS][sizeof(cmd)] = {0}, sts2[MAX_NUM_FRUS] = {0};
  uint8_t log_sel, sts_code, min_code;
  char log_buf[256], minor_buf[128];
  const char **log_str[] = {
    plat_state,
    last_recovery,
    last_panic,
    major_err
  };
  const char **log_str2[] = {
    minor_auth_err,
    minor_update_err
  };
  int ret;
  bool bridged;

  if (!pal_is_pfr_active()) {
    return NULL;
  }

  for (i = 0; i < pfr_fru_count; i++) {
    if (pal_get_pfr_address(pfr_mbox[i].fru, &bus, &addr, &bridged)) {
      syslog(LOG_WARNING, "%s: get PFR address failed, fru %d", __func__, pfr_mbox[i].fru);
      continue;
    }
    pfr_mbox[i].bus = bus;
    pfr_mbox[i].addr = addr;
    pfr_mbox[i].transfer = bridged ? transfer_bridged : transfer_i2c;
  }

  INIT_PFR_ERR(plat_state, 0x01, "CPLD_NIOS_WAITING_TO_START");
  INIT_PFR_ERR(plat_state, 0x02, "CPLD_NIOS_STARTED");
  INIT_PFR_ERR(plat_state, 0x03, "ENTER_T-1");
  INIT_PFR_ERR(plat_state, 0x06, "BMC_FLASH_AUTHENTICATION");
  INIT_PFR_ERR(plat_state, 0x07, "PCH_FLASH_AUTHENTICATION");
  INIT_PFR_ERR(plat_state, 0x08, "AUTHENTICATION_FAILED_LOCKDOWN");
  INIT_PFR_ERR(plat_state, 0x09, "ENTER_T0");
  INIT_PFR_ERR(plat_state, 0x0A, "T0_BMC_BOOTED");
  INIT_PFR_ERR(plat_state, 0x0B, "T0_ME_BOOTED");
  INIT_PFR_ERR(plat_state, 0x0C, "T0_ACM_BOOTED");
  INIT_PFR_ERR(plat_state, 0x0D, "T0_BIOS_BOOTED");
  INIT_PFR_ERR(plat_state, 0x0E, "T0_BOOT_COMPLETE");
  INIT_PFR_ERR(plat_state, 0x10, "PCH_FW_UPDATE");
  INIT_PFR_ERR(plat_state, 0x11, "BMC_FW_UPDATE");
  INIT_PFR_ERR(plat_state, 0x12, "CPLD_UPDATE");
  INIT_PFR_ERR(plat_state, 0x13, "CPLD_UPDATE_IN_RECOVERY_MODE");
  INIT_PFR_ERR(plat_state, 0x40, "T-1_FW_RECOVERY");
  INIT_PFR_ERR(plat_state, 0x41, "T-1_FORCED_ACTIVE_FW_RECOVERY");
  INIT_PFR_ERR(plat_state, 0x42, "WDT_TIMEOUT_RECOVERY");
  INIT_PFR_ERR(plat_state, 0x43, "CPLD_RECOVERY_IN_RECOVERY_MODE");
  INIT_PFR_ERR(plat_state, 0x44, "PIT_L1_LOCKDOWN");
  INIT_PFR_ERR(plat_state, 0x45, "PIT_L2_FW_SEALED");
  INIT_PFR_ERR(plat_state, 0x46, "PIT_L2_PCH_HASH_MISMATCH_LOCKDOWN");
  INIT_PFR_ERR(plat_state, 0x47, "PIT_L2_BMC_HASH_MISMATCH_LOCKDOWN");
  INIT_PFR_ERR(plat_state, 0xF1, "CPLD_RECOVERY_FAILED_LOCKDOWN");
  INIT_PFR_ERR(plat_state, 0xF2, "BMC_WDT_RECOVERY_3TIME_FAILED");

  INIT_PFR_ERR(last_recovery, 0x01, "PCH_ACTIVE_FAIL");
  INIT_PFR_ERR(last_recovery, 0x02, "PCH_RECOVERY_FAIL");
  INIT_PFR_ERR(last_recovery, 0x03, "ME_LAUNCH_FAIL");
  INIT_PFR_ERR(last_recovery, 0x04, "ACM_LAUNCH_FAIL");
  INIT_PFR_ERR(last_recovery, 0x05, "IBB_LAUNCH_FAIL");
  INIT_PFR_ERR(last_recovery, 0x06, "OBB_LAUNCH_FAIL");
  INIT_PFR_ERR(last_recovery, 0x07, "BMC_ACTIVE_FAIL");
  INIT_PFR_ERR(last_recovery, 0x08, "BMC_RECOVERY_FAIL");
  INIT_PFR_ERR(last_recovery, 0x09, "BMC_LAUNCH_FAIL");
  INIT_PFR_ERR(last_recovery, 0x0A, "FORCED_ACTIVE_FW_RECOVERY");

  INIT_PFR_ERR(last_panic, 0x01, "PCH_UPDATE_INTENT");
  INIT_PFR_ERR(last_panic, 0x02, "BMC_UPDATE_INTENT");
  INIT_PFR_ERR(last_panic, 0x03, "BMC_RESET_DETECTED");
  INIT_PFR_ERR(last_panic, 0x04, "BMC_WDT_EXPIRED");
  INIT_PFR_ERR(last_panic, 0x05, "ME_WDT_EXPIRED");
  INIT_PFR_ERR(last_panic, 0x06, "ACM_WDT_EXPIRED");
  INIT_PFR_ERR(last_panic, 0x07, "IBB_WDT_EXPIRED");
  INIT_PFR_ERR(last_panic, 0x08, "OBB_WDT_EXPIRED");
  INIT_PFR_ERR(last_panic, 0x09, "ACM_IBB_OBB_AUTH_FAILED");

  INIT_PFR_ERR(major_err, 0x01, "BMC_AUTH_FAILED");
  INIT_PFR_ERR(major_err, 0x02, "PCH_AUTH_FAILED");
  INIT_PFR_ERR(major_err, 0x03, "UPDATE_FROM_PCH_FAILED");
  INIT_PFR_ERR(major_err, 0x04, "UPDATE_FROM_BMC_FAILED");

  INIT_PFR_ERR(minor_auth_err, 0x01, "AUTH_ACTIVE");
  INIT_PFR_ERR(minor_auth_err, 0x02, "AUTH_RECOVERY");
  INIT_PFR_ERR(minor_auth_err, 0x03, "AUTH_ACTIVE_AND_RECOVERY");
  INIT_PFR_ERR(minor_auth_err, 0x04, "AUTH_ALL_REGIONS");

  INIT_PFR_ERR(minor_update_err, 0x01, "INVALID_UPDATE_INTENT");
  INIT_PFR_ERR(minor_update_err, 0x02, "FW_UPDATE_INVALID_SVN");
  INIT_PFR_ERR(minor_update_err, 0x03, "FW_UPDATE_AUTH_FAILED");
  INIT_PFR_ERR(minor_update_err, 0x04, "FW_UPDATE_EXCEEDED_MAX_FAILED_ATTEMPTS");
  INIT_PFR_ERR(minor_update_err, 0x05, "ACTIVE_FW_UPDATE_NOT_ALLOWED");
  INIT_PFR_ERR(minor_update_err, 0x06, "RECOVERY_FW_UPDATE_AUTH_FAILED");
  INIT_PFR_ERR(minor_update_err, 0x10, "CPLD_UPDATE_INVALID_SVN");
  INIT_PFR_ERR(minor_update_err, 0x11, "CPLD_UPDATE_AUTH_FAILED");
  INIT_PFR_ERR(minor_update_err, 0x12, "CPLD_UPDATE_EXCEEDED_MAX_FAILED_ATTEMPTS");

  sleep(2);

  while (1) {
    for (i = 0; i < pfr_fru_count; i++) {
      if (pfr_mbox[i].bus == 0xFF) {  // failed get PFR address
        continue;
      }

      for (j = 0; j < sizeof(cmd); j++) {
        tbuf[0] = cmd[j];
        ret = pfr_mbox[i].transfer(&pfr_mbox[i], tbuf, 1, rbuf, 1);
        if (ret) {
          syslog(LOG_WARNING, "i2c%u xfer failed, offset = %x", pfr_mbox[i].bus, cmd[j]);
          continue;
        }

        log_sel = 0;
        if (sts[i][j] != rbuf[0]) {
          sts[i][j] = rbuf[0];
          if (sts[i][j]) {
            log_sel = 1;
          }
        }
        sts_code = sts[i][j];

        if ((cmd[j] == MAJOR_ERROR) && sts_code && (sts_code <= 0x04)) {  // major error code: 0x01 ~ 0x04
          tbuf[0] = MINOR_ERROR;  // minor error code
          ret = pfr_mbox[i].transfer(&pfr_mbox[i], tbuf, 1, rbuf, 1);
          if (ret) {
            syslog(LOG_WARNING, "i2c%u xfer failed, offset = %x", pfr_mbox[i].bus, cmd[j]);
            continue;
          }

          if (sts2[i] != rbuf[0]) {
            sts2[i] = rbuf[0];
            log_sel = 2;
          }
        }

        if (log_sel) {
          if (log_str[j][sts_code]) {
            snprintf(log_buf, sizeof(log_buf), "%s (0x%02X, 0x%02X)", log_str[j][sts_code], cmd[j], sts_code);

            if (cmd[j] == MAJOR_ERROR) {
              min_code = sts2[i];
              if ((sts_code <= 0x04) && (log_str2[(sts_code-1)/2][min_code])) {
                snprintf(minor_buf, sizeof(minor_buf), ", %s (0x%02X, 0x%02X)",
                                    log_str2[(sts_code-1)/2][min_code], MINOR_ERROR, min_code);
              } else {
                snprintf(minor_buf, sizeof(minor_buf), ", Unknown minor (0x%02X, 0x%02X)",
                                    MINOR_ERROR, min_code);
              }
              strcat(log_buf, minor_buf);
            }
          } else {
            snprintf(log_buf, sizeof(log_buf), "Unknown status (0x%02X, 0x%02X)", cmd[j], sts_code);
          }

          syslog(LOG_CRIT, "PFR: %s, FRU: %u", log_buf, pfr_mbox[i].fru);
        }
      }
    }

    sleep(pfr_monitor_interval);
  }

  return NULL;
}
