/*
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
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
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/time.h>
#include <openbmc/pal.h>

#define COUNT_ECC_RELOG 600
#define PAGE_SIZE  0x1000
#define AST_MCR_BASE 0x1e6e0000
#define INTR_CTRL_STS_OFFSET 0x50
#define ADDR_FIRST_UNRECOVER_ECC_OFFSET 0x58
#define ADDR_LAST_RECOVER_ECC_OFFSET 0x5c
#define THRES1_ECC_RECOVERABLE_ERROR_COUNTER (256 / 2)
#define THRES2_ECC_RECOVERABLE_ERROR_COUNTER (256 * 9 / 10)
#define THRES1_ECC_UNRECOVERABLE_ERROR_COUNTER (16 / 2)
#define THRES2_ECC_UNRECOVERABLE_ERROR_COUNTER (16 * 9 / 10)

// Thread to monitor the ECC counter
static void
ecc_mon_handler() {
  uint32_t mcr_fd;
  uint32_t mcr50;
  uint32_t mcr58;
  uint32_t mcr5c;
  uint16_t ecc_recoverable_error_counter = 0;
  uint8_t ecc_unrecoverable_error_counter = 0;
  void *mcr_reg;
  void *mcr50_reg;
  void *mcr58_reg;
  void *mcr5c_reg;
  int is_ecc_occurred[2] = {0};
  int ecc_health_last_state = 1;
  int ecc_health_kv_state = 1;
  char tmp_health[MAX_VALUE_LEN];
  int ret;
  int relog_counter = 0;
  int retry_err = 0;

  while (1) {

    mcr_fd = open("/dev/mem", O_RDWR | O_SYNC );
    if (mcr_fd < 0) {
      // In case of error opening the file, sleep for 2 sec and retry.
      // During continuous failures, log the error every 20 minutes.
      sleep(2);
      if (++retry_err >= 600) {
        syslog(LOG_ERR, "%s - cannot open /dev/mem", __func__);
        retry_err = 0;
      }
      continue;
    }

    retry_err = 0;

    mcr_reg = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mcr_fd,
        AST_MCR_BASE);
    mcr50_reg = (char*)mcr_reg + INTR_CTRL_STS_OFFSET;
    mcr50 = *(volatile uint32_t*) mcr50_reg;

    mcr58_reg = (char*)mcr_reg + ADDR_FIRST_UNRECOVER_ECC_OFFSET;
    mcr58 = *(volatile uint32_t*) mcr58_reg;

    mcr5c_reg = (char*)mcr_reg + ADDR_LAST_RECOVER_ECC_OFFSET;
    mcr5c = *(volatile uint32_t*) mcr5c_reg;

    munmap(mcr_reg, PAGE_SIZE);
    close(mcr_fd);

    // get current health status from kv_store
    memset(tmp_health, 0, MAX_VALUE_LEN);
    ret = pal_get_key_value("ecc_health", tmp_health);
    if (ret){
      syslog(LOG_ERR, " %s - kv get ecc_health status failed", __func__);
    }

    ecc_health_kv_state = atoi(tmp_health);

    ecc_recoverable_error_counter = (mcr50 >> 16) & 0xFF;
    ecc_unrecoverable_error_counter = (mcr50 >> 12) & 0xF;

    // Check ECC recoverable error counter
    if (ecc_recoverable_error_counter >= THRES1_ECC_RECOVERABLE_ERROR_COUNTER) {
      if (is_ecc_occurred[0] == 0) {
        syslog(LOG_CRIT, "ECC occurred (over 50%%): Recoverable Error "
            "Counter = %d Address of last recoverable ECC error = 0x%x",
            ecc_recoverable_error_counter, (mcr5c >> 4) & 0xFFFFFFFF);
        is_ecc_occurred[0] = 1;
      }
      if (ecc_recoverable_error_counter >=
          THRES2_ECC_RECOVERABLE_ERROR_COUNTER) {
        if (is_ecc_occurred[0] == 1) {
          syslog(LOG_CRIT, "ECC occurred (over 90%%): Recoverable Error "
              "Counter = %d Address of last recoverable ECC error = 0x%x",
              ecc_recoverable_error_counter, (mcr5c >> 4) & 0xFFFFFFFF);
          is_ecc_occurred[0] = 2;
        }
      }
      pal_set_key_value("ecc_health", "0");
    }

    // Check ECC un-recoverable error counter
    if (ecc_unrecoverable_error_counter >=
        THRES1_ECC_UNRECOVERABLE_ERROR_COUNTER) {
      if (is_ecc_occurred[1] == 0) {
        syslog(LOG_CRIT, "ECC occurred (over 50%%): Un-recoverable Error "
            "Counter = %d Address of first un-recoverable ECC error = 0x%x",
            ecc_unrecoverable_error_counter, (mcr58 >> 4) & 0xFFFFFFFF);
        is_ecc_occurred[1] = 1;
      }
      if (ecc_unrecoverable_error_counter >=
          THRES2_ECC_UNRECOVERABLE_ERROR_COUNTER) {
        if (is_ecc_occurred[1] == 1) {
          syslog(LOG_CRIT, "ECC occurred (over 90%%): Un-recoverable Error "
              "Counter = %d Address of first un-recoverable ECC error = 0x%x",
              ecc_unrecoverable_error_counter, (mcr58 >> 4) & 0xFFFFFFFF);
          is_ecc_occurred[1] = 2;
        }
      }
      pal_set_key_value("ecc_health", "0");
    }

    // If log-util clear all fru, cleaning ECC error status
    // After doing it, gpiod will regenerate assert
    // Generage a syslog every 600 loop counter (20 minutes)
    if ((relog_counter >= COUNT_ECC_RELOG) ||
        ((ecc_health_kv_state != ecc_health_last_state) &&
         (ecc_health_kv_state == 1))) {
      is_ecc_occurred[0] = 0;
      is_ecc_occurred[1] = 0;
      relog_counter = 0;
    }
    ecc_health_last_state = ecc_health_kv_state;
    relog_counter++;
    sleep(2);
  }
}

int
main (int argc, char * const argv[]) {

  ecc_mon_handler();

  return 0;
}
