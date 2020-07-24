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
#include <sys/mman.h>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>

#define PAGE_SIZE 0x1000
#define BMC_REBOOT_BASE 0x1e721000
#define BOOT_MAGIC 0xFB420054
#define BMC_REBOOT_SIG(base) (*(uint32_t *)((base) + 0x208))
#define PFR_ST_OFFSET(base) (*(volatile uint32_t *)((base) + 0x20C))

#define INIT_ST_TBL(tbl, idx, a, v, s) \
          { tbl[idx].addr = a; tbl[idx].val = v; tbl[idx].desc = s; }

#define PFR_STATE_SIZE 64

typedef struct _mailbox_t {
  int fd;
  uint8_t fru;
  uint8_t bus;
  uint8_t addr;
  int (*transfer)(struct _mailbox_t *, uint8_t *,uint8_t,uint8_t *,uint8_t);
} mailbox_t;

typedef struct {
  uint8_t addr;
  uint8_t val;
  const char *desc;
} pfr_state_t;

bool pfr_monitor_enabled = false;

static int pfr_monitor_interval = 10;
static int mm_fd = -1;
static uint8_t pfr_fru_count = 0;
static uint8_t *reboot_base = NULL;
static mailbox_t pfr_mbox[MAX_NUM_FRUS];
static pfr_state_t st_table[256];
static bool is_magic_set = true;


static void
init_pfr_state_table(pfr_state_t *tbl) {
  // Platform State
  // CPLD Nios firmware T-1 flow
  INIT_ST_TBL(tbl, 0x01, 0x03, 0x01, "CPLD_NIOS_WAITING_TO_START");
  INIT_ST_TBL(tbl, 0x02, 0x03, 0x02, "CPLD_NIOS_STARTED");
  INIT_ST_TBL(tbl, 0x03, 0x03, 0x03, "ENTER_T-1");
  INIT_ST_TBL(tbl, 0x06, 0x03, 0x06, "BMC_FLASH_AUTHENTICATION");
  INIT_ST_TBL(tbl, 0x07, 0x03, 0x07, "PCH_FLASH_AUTHENTICATION");
  INIT_ST_TBL(tbl, 0x08, 0x03, 0x08, "AUTHENTICATION_FAILED_LOCKDOWN");
  INIT_ST_TBL(tbl, 0x09, 0x03, 0x09, "ENTER_T0");
  // Timed boot progress
  INIT_ST_TBL(tbl, 0x0A, 0x03, 0x0A, "T0_BMC_BOOTED");
  INIT_ST_TBL(tbl, 0x0B, 0x03, 0x0B, "T0_ME_BOOTED");
  INIT_ST_TBL(tbl, 0x0C, 0x03, 0x0C, "T0_ACM_BOOTED");
  INIT_ST_TBL(tbl, 0x0D, 0x03, 0x0D, "T0_BIOS_BOOTED");
  INIT_ST_TBL(tbl, 0x0E, 0x03, 0x0E, "T0_BOOT_COMPLETE");
  // Update event
  INIT_ST_TBL(tbl, 0x10, 0x03, 0x10, "PCH_FW_UPDATE");
  INIT_ST_TBL(tbl, 0x11, 0x03, 0x11, "BMC_FW_UPDATE");
  INIT_ST_TBL(tbl, 0x12, 0x03, 0x12, "CPLD_UPDATE");
  INIT_ST_TBL(tbl, 0x13, 0x03, 0x13, "CPLD_UPDATE_IN_RECOVERY_MODE");
  // Recovery
  INIT_ST_TBL(tbl, 0x40, 0x03, 0x40, "T-1_FW_RECOVERY");
  INIT_ST_TBL(tbl, 0x41, 0x03, 0x41, "T-1_FORCED_ACTIVE_FW_RECOVERY");
  INIT_ST_TBL(tbl, 0x42, 0x03, 0x42, "WDT_TIMEOUT_RECOVERY");
  INIT_ST_TBL(tbl, 0x43, 0x03, 0x43, "CPLD_RECOVERY_IN_RECOVERY_MODE");
  // PIT
  INIT_ST_TBL(tbl, 0x44, 0x03, 0x44, "PIT_L1_LOCKDOWN");
  INIT_ST_TBL(tbl, 0x45, 0x03, 0x45, "PIT_L2_FW_SEALED");
  INIT_ST_TBL(tbl, 0x46, 0x03, 0x46, "PIT_L2_PCH_HASH_MISMATCH_LOCKDOWN");
  INIT_ST_TBL(tbl, 0x47, 0x03, 0x47, "PIT_L2_BMC_HASH_MISMATCH_LOCKDOWN");
  // Quanta front panel LED control
  INIT_ST_TBL(tbl, 0xF1, 0x03, 0xF1, "CPLD_RECOVERY_FAILED_LOCKDOWN");
  INIT_ST_TBL(tbl, 0xF2, 0x03, 0xF2, "BMC_WDT_RECOVERY_3TIME_FAILED");

  // Last Panic
  INIT_ST_TBL(tbl, 0x51, 0x07, 0x01, "PCH_UPDATE_INTENT");
  INIT_ST_TBL(tbl, 0x52, 0x07, 0x02, "BMC_UPDATE_INTENT");
  INIT_ST_TBL(tbl, 0x53, 0x07, 0x03, "BMC_RESET_DETECTED");
  INIT_ST_TBL(tbl, 0x54, 0x07, 0x04, "BMC_WDT_EXPIRED");
  INIT_ST_TBL(tbl, 0x55, 0x07, 0x05, "ME_WDT_EXPIRED");
  INIT_ST_TBL(tbl, 0x56, 0x07, 0x06, "ACM_WDT_EXPIRED");
  INIT_ST_TBL(tbl, 0x57, 0x07, 0x07, "IBB_WDT_EXPIRED");
  INIT_ST_TBL(tbl, 0x58, 0x07, 0x08, "OBB_WDT_EXPIRED");
  INIT_ST_TBL(tbl, 0x59, 0x07, 0x09, "ACM_IBB_OBB_AUTH_FAILED");

  // Last Recovery
  INIT_ST_TBL(tbl, 0x61, 0x05, 0x01, "PCH_ACTIVE_FAIL");
  INIT_ST_TBL(tbl, 0x62, 0x05, 0x02, "PCH_RECOVERY_FAIL");
  INIT_ST_TBL(tbl, 0x63, 0x05, 0x03, "ME_LAUNCH_FAIL");
  INIT_ST_TBL(tbl, 0x64, 0x05, 0x04, "ACM_LAUNCH_FAIL");
  INIT_ST_TBL(tbl, 0x65, 0x05, 0x05, "IBB_LAUNCH_FAIL");
  INIT_ST_TBL(tbl, 0x66, 0x05, 0x06, "OBB_LAUNCH_FAIL");
  INIT_ST_TBL(tbl, 0x67, 0x05, 0x07, "BMC_ACTIVE_FAIL");
  INIT_ST_TBL(tbl, 0x68, 0x05, 0x08, "BMC_RECOVERY_FAIL");
  INIT_ST_TBL(tbl, 0x69, 0x05, 0x09, "BMC_LAUNCH_FAIL");
  INIT_ST_TBL(tbl, 0x6A, 0x05, 0x0A, "FORCED_ACTIVE_FW_RECOVERY");

  // BMC auth
  INIT_ST_TBL(tbl, 0x71, 0x09, 0x01, "BMC_AUTH_FAILED%s, AUTH_ACTIVE");
  INIT_ST_TBL(tbl, 0x72, 0x09, 0x02, "BMC_AUTH_FAILED%s, AUTH_RECOVERY");
  INIT_ST_TBL(tbl, 0x73, 0x09, 0x03, "BMC_AUTH_FAILED%s, AUTH_ACTIVE_AND_RECOVERY");
  INIT_ST_TBL(tbl, 0x74, 0x09, 0x04, "BMC_AUTH_FAILED%s, AUTH_ALL_REGIONS");

  // PCH auth
  INIT_ST_TBL(tbl, 0x81, 0x09, 0x01, "PCH_AUTH_FAILED%s, AUTH_ACTIVE");
  INIT_ST_TBL(tbl, 0x82, 0x09, 0x02, "PCH_AUTH_FAILED%s, AUTH_RECOVERY");
  INIT_ST_TBL(tbl, 0x83, 0x09, 0x03, "PCH_AUTH_FAILED%s, AUTH_ACTIVE_AND_RECOVERY");
  INIT_ST_TBL(tbl, 0x84, 0x09, 0x04, "PCH_AUTH_FAILED%s, AUTH_ALL_REGIONS");

  // Update from PCH
  INIT_ST_TBL(tbl, 0x91, 0x09, 0x01, "UPDATE_FROM_PCH_FAILED%s, INVALID_UPDATE_INTENT");
  INIT_ST_TBL(tbl, 0x92, 0x09, 0x02, "UPDATE_FROM_PCH_FAILED%s, INVALID_SVN");
  INIT_ST_TBL(tbl, 0x93, 0x09, 0x03, "UPDATE_FROM_PCH_FAILED%s, AUTHENTICATION_FAILED");
  INIT_ST_TBL(tbl, 0x94, 0x09, 0x04, "UPDATE_FROM_PCH_FAILED%s, EXCEEDED_MAX_FAILED_ATTEMPTS");
  INIT_ST_TBL(tbl, 0x95, 0x09, 0x05, "UPDATE_FROM_PCH_FAILED%s, ACTIVE_FW_UPDATE_NOT_ALLOWED");
  INIT_ST_TBL(tbl, 0x96, 0x09, 0x06, "UPDATE_FROM_PCH_FAILED%s, RECOVERY_FW_UPDATE_AUTH_FAILED");

  // Update from BMC
  INIT_ST_TBL(tbl, 0xB1, 0x09, 0x01, "UPDATE_FROM_BMC_FAILED%s, INVALID_UPDATE_INTENT");
  INIT_ST_TBL(tbl, 0xB2, 0x09, 0x02, "UPDATE_FROM_BMC_FAILED%s, FW_UPDATE_INVALID_SVN");
  INIT_ST_TBL(tbl, 0xB3, 0x09, 0x03, "UPDATE_FROM_BMC_FAILED%s, FW_UPDATE_AUTH_FAILED");
  INIT_ST_TBL(tbl, 0xB4, 0x09, 0x04, "UPDATE_FROM_BMC_FAILED%s, FW_UPDATE_EXCEEDED_MAX_FAILED_ATTEMPTS");
  INIT_ST_TBL(tbl, 0xB5, 0x09, 0x05, "UPDATE_FROM_BMC_FAILED%s, ACTIVE_FW_UPDATE_NOT_ALLOWED");
  INIT_ST_TBL(tbl, 0xB6, 0x09, 0x06, "UPDATE_FROM_BMC_FAILED%s, RECOVERY_FW_UPDATE_AUTH_FAILED");
}

static void
get_last_offset(uint8_t fru, uint8_t *start, uint8_t *end) {
  uint32_t last_offs;

  fru = (fru - 1) * 2;
  last_offs = PFR_ST_OFFSET(reboot_base + (fru/4)*4) >> (fru%4)*8;
  *start = last_offs & 0xFF;
  *end = (last_offs >> 8) & 0xFF;
}

static void
set_last_offset(uint8_t fru, uint8_t start, uint8_t end) {
  uint32_t last_offs;
  uint8_t offs, shft;

  fru = (fru - 1) * 2;
  offs = (fru/4) * 4;
  shft = (fru%4) * 8;
  last_offs = PFR_ST_OFFSET(reboot_base + offs) & (0xFFFF0000 >> shft);
  PFR_ST_OFFSET(reboot_base + offs) = (((end << 8) | start) << shft) | last_offs;
}

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

  if (reboot_base) {
    munmap(reboot_base, PAGE_SIZE);
    reboot_base = NULL;
  }

  if (mm_fd >= 0) {
    close(mm_fd);
    mm_fd = -1;
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

    if ((mm_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
      syslog(LOG_ERR, "%s: devmem open failed", __func__);
      break;
    }

    if (!(reboot_base = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mm_fd, BMC_REBOOT_BASE))) {
      syslog(LOG_ERR, "%s: mmap failed", __func__);
      close(mm_fd);
      break;
    }

    if (BMC_REBOOT_SIG(reboot_base) != BOOT_MAGIC) {
      is_magic_set = false;
      for (i = 0; i < pfr_fru_count; i++) {
        set_last_offset(pfr_mbox[i].fru, 0x00, 0x01);
      }
    }

    munmap(reboot_base, PAGE_SIZE);
    reboot_base = NULL;
    close(mm_fd);
    mm_fd = -1;

    return;
  } while (0);

  pfr_monitor_enabled = false;
}

void *
pfr_monitor() {
  uint8_t bus, addr;
  uint8_t i, j, idx;
  uint8_t tbuf[8], rbuf[80];
  uint8_t start[MAX_NUM_FRUS], end[MAX_NUM_FRUS], wrapped[MAX_NUM_FRUS], last;
  char log_buf[256];
  const char *log_ptr;
  bool bridged;
  int is_por;

  if (!pal_is_pfr_active()) {
    return NULL;
  }

  if (!reboot_base) {
    if ((mm_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
      syslog(LOG_ERR, "%s: devmem open failed", __func__);
      return NULL;
    }

    if (!(reboot_base = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mm_fd, BMC_REBOOT_BASE))) {
      syslog(LOG_ERR, "%s: mmap failed", __func__);
      close(mm_fd);
      return NULL;
    }
  }

  is_por = pal_is_bmc_por();
  for (i = 0; i < pfr_fru_count; i++) {
    get_last_offset(pfr_mbox[i].fru, &start[i], &end[i]);
    if (is_por || !is_magic_set ||
        (start[i] <= 1) || (end[i] <= 1) ||
        (start[i] >= PFR_STATE_SIZE) || (end[i] >= PFR_STATE_SIZE)) {
      start[i] = 0x00;
      end[i] = 0x01;
    }
    wrapped[i] = (start[i] > end[i]) ? 1 : 0;

    if (pal_get_pfr_address(pfr_mbox[i].fru, &bus, &addr, &bridged)) {
      syslog(LOG_WARNING, "%s: get PFR address failed, fru %d", __func__, pfr_mbox[i].fru);
      continue;
    }
    pfr_mbox[i].bus = bus;
    pfr_mbox[i].addr = addr;
    pfr_mbox[i].transfer = bridged ? transfer_bridged : transfer_i2c;
  }

  memset(st_table, 0x00, sizeof(st_table));
  init_pfr_state_table(st_table);

  while (1) {
    for (i = 0; i < pfr_fru_count; i++) {
      if (pfr_mbox[i].bus == 0xFF) {  // failed get PFR address
        continue;
      }

      tbuf[0] = 0xC0;  // get start/end offset of state-history
      if (pfr_mbox[i].transfer(&pfr_mbox[i], tbuf, 1, &rbuf[0], 2)) {
        syslog(LOG_WARNING, "%s: read state-history index failed", __func__);
        continue;
      }

      if ((rbuf[0] == start[i]) && (rbuf[1] == end[i])) {
        continue;
      }

      if ((wrapped[i] || (end[i] > rbuf[1])) && (rbuf[1] > rbuf[0])) {
        start[i] = 0x00;
        end[i] = 0x01;
      }

      for (j = 0; j < PFR_STATE_SIZE; j += 16) {  // get whole state-history
        tbuf[0] = 0xC0 + j;
        if (pfr_mbox[i].transfer(&pfr_mbox[i], tbuf, 1, &rbuf[j], 16)) {
          syslog(LOG_WARNING, "%s: read state-history failed", __func__);
          break;
        }
      }
      if (j < PFR_STATE_SIZE)
        continue;

      last = end[i];
      start[i] = rbuf[0];
      end[i] = rbuf[1];

      if (last > rbuf[1]) {
        rbuf[1] += PFR_STATE_SIZE;
        wrapped[i] = 1;
      }
      for (j = last+1; j <= rbuf[1]; j++) {
        idx = j % PFR_STATE_SIZE;
        if ((idx > 1) && rbuf[idx] && st_table[rbuf[idx]].desc) {
          switch (rbuf[idx] & 0xF0) {
            case 0x70:
              sprintf(log_buf, st_table[rbuf[idx]].desc, " (0x08, 0x01)");
              log_ptr = log_buf;
              break;
            case 0x80:
              sprintf(log_buf, st_table[rbuf[idx]].desc, " (0x08, 0x02)");
              log_ptr = log_buf;
              break;
            case 0x90:
              sprintf(log_buf, st_table[rbuf[idx]].desc, " (0x08, 0x03)");
              log_ptr = log_buf;
              break;
            case 0xB0:
              sprintf(log_buf, st_table[rbuf[idx]].desc, " (0x08, 0x04)");
              log_ptr = log_buf;
              break;
            default:
              log_ptr = st_table[rbuf[idx]].desc;
              break;
          }

          syslog(LOG_CRIT, "PFR: %s (0x%02X, 0x%02X), FRU: %u", log_ptr,
                 st_table[rbuf[idx]].addr, st_table[rbuf[idx]].val, pfr_mbox[i].fru);
        }
      }

      set_last_offset(pfr_mbox[i].fru, start[i], end[i]);
    }

    sleep(pfr_monitor_interval);
  }

  return NULL;
}
