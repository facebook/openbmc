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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <openbmc/kv.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>

#define BTN_MAX_SAMPLES   200
#define BTN_POWER_OFF     40

#define ADM1278_ADDR 0x10
#define LM75_1_ADDR 0x48
#define LM75_2_ADDR 0x4b
#define MAX34461_ADDR 0x74
#define FPGA_16Q_ADDR 0x60
#define FPGA_4DD_ADDR 0x61

static int
i2c_open(uint8_t bus, uint8_t addr) {
  int fd = -1;
  int rc = -1;
  char fn[32];

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);

  if (fd == -1) {
    syslog(LOG_WARNING,
            "Failed to open i2c device %s, errno=%d", fn, errno);
    return -1;
  }

  rc = ioctl(fd, I2C_SLAVE_FORCE, addr);
  if (rc < 0) {
    syslog(LOG_WARNING,
            "Failed to open slave @ address 0x%x, errno=%d", addr, errno);
    close(fd);
    return -1;
  }

  return fd;
}

static int
pim_driver_add(uint8_t num) {
  int ret = 0;
  uint8_t bus = ((num - 1) * 8) + 80;

  sleep(5); /* Sleep to avoid mount max34461 fail. */
  ret += pal_add_i2c_device((bus + 2), LM75_1_ADDR, "tmp75");
  ret += pal_add_i2c_device((bus + 3), LM75_2_ADDR, "tmp75");
  ret += pal_add_i2c_device((bus + 4), ADM1278_ADDR, "adm1278");
  ret += pal_add_i2c_device((bus + 6), MAX34461_ADDR, "max34461");

  return ret;
}

static int
pim_driver_del(uint8_t num) {
  int ret = 0;
  uint8_t bus = ((num - 1) * 8) + 80;

  ret += pal_del_i2c_device((bus + 2), LM75_1_ADDR);
  ret += pal_del_i2c_device((bus + 3), LM75_2_ADDR);
  ret += pal_del_i2c_device((bus + 4), ADM1278_ADDR);
  ret += pal_del_i2c_device((bus + 6), MAX34461_ADDR);

  return ret;
}

static int
set_pim_slot_id(uint8_t num) {
  int ret = -1, fd = -1;
  uint8_t bus = ((num - 1) * 8) + 80;
  uint8_t fru = num + 2;

  if (pal_get_pim_type(fru) == 0) {
    fd = i2c_open(bus, FPGA_16Q_ADDR);
  } else {
    fd = i2c_open(bus, FPGA_4DD_ADDR);
  }
  if (fd < 0) {
    return -1;
  }

  ret = i2c_smbus_write_byte_data(fd, 0x03, num);
  close(fd);

  return ret;
}

// Thread for monitoring scm plug
static void *
scm_monitor_handler(void *unused){
  int curr = -1;
  int prev = -1;
  int ret;
  uint8_t prsnt = 0;
  uint8_t power;

  while (1) {
    ret = pal_is_fru_prsnt(FRU_SCM, &prsnt);
    if (ret) {
      goto scm_mon_out;
    }
    curr = prsnt;
    if (curr != prev) {
      if (curr) {
        // SCM was inserted
        syslog(LOG_WARNING, "SCM Insertion\n");

        /* Setup TH3 PCI-e repeater */
        run_command("/usr/local/bin/setup_pcie_repeater.sh th3 write");
        ret = pal_get_server_power(FRU_SCM, &power);
        if (ret) {
          goto scm_mon_out;
        }
        if (power == SERVER_POWER_OFF) {
          sleep(3);
          syslog(LOG_WARNING, "SCM power on\n");
          pal_set_server_power(FRU_SCM, SERVER_POWER_ON);
          /* Setup management port LED */
          run_command("/usr/local/bin/setup_mgmt.sh led");
          goto scm_mon_out;
        }
      } else {
        // SCM was removed
        syslog(LOG_WARNING, "SCM Extraction\n");
      }
    }
scm_mon_out:
    prev = curr;
      sleep(1);
  }
  return 0;
}

// Thread for monitoring pim plug
static void *
pim_monitor_handler(void *unused){
  uint8_t fru;
  uint8_t num;
  uint8_t ret;
  uint8_t prsnt = 0;
  uint8_t prsnt_ori = 0;
  uint8_t curr_state = 0x00;

  while (1) {
    for(fru = FRU_PIM1; fru <= FRU_PIM8; fru++){
      ret = pal_is_fru_prsnt(fru, &prsnt);
      if (ret) {
        goto pim_mon_out;
      }
      /* FRU_PIM1 = 3, FRU_PIM2 = 4, ...., FRU_PIM8 = 10 */
      pal_set_pim_sts_led(fru);
      /* Get original prsnt state PIM1 @bit0, PIM2 @bit1, ..., PIM8 @bit7 */
      num = fru - 2;
      prsnt_ori = GETBIT(curr_state, (num - 1));
      /* 1 is prsnt, 0 is not prsnt. */
      if (prsnt != prsnt_ori) {
        if (prsnt) {
          syslog(LOG_WARNING, "PIM %d is plugged in.", num);
          ret = pim_driver_add(num);
          if(ret){
              syslog(LOG_WARNING, "DOMFPGA/MAX34461 of PIM %d is not ready "
                                  "or sensor cannot be mounted.", num);
          }
        } else {
          syslog(LOG_WARNING, "PIM %d is unplugged.", num);
          ret = pim_driver_del(num);
        }
        /* Set PIM1 prsnt state @bit0, PIM2 @bit1, ..., PIM8 @bit7 */
        curr_state = prsnt ? SETBIT(curr_state, (num - 1))
                           : CLEARBIT(curr_state, (num - 1));
      }
      /* Set PIM number into DOM FPGA 0x03 register to control LED stream. */
      /* 1 is prsnt, 0 is not prsnt. */
      if (prsnt) {
        ret = set_pim_slot_id(num);
        if (ret) {
            syslog(LOG_WARNING,
                   "Cannot set slot id into FPGA register of PIM %d" ,num);
        }
      }
    }
pim_mon_out:
    sleep(1);
  }
  return 0;
}

void
exithandler(int signum) {
  int brd_rev;
  pal_get_board_rev(&brd_rev);
  set_sled(brd_rev, SLED_CLR_YELLOW, SLED_SMB);
  exit(0);
}

// Thread for monitoring sim LED
static void *
simLED_monitor_handler(void *unused) {
  int brd_rev;
  uint8_t sys_ug = 0, fan_ug = 0, psu_ug = 0, smb_ug = 0;
  pal_get_board_rev(&brd_rev);
  init_led();
  while(1) {
	sleep(1);
    pal_mon_fw_upgrade(brd_rev, &sys_ug, &fan_ug, &psu_ug, &smb_ug);
	if(sys_ug == 0) {
      set_sys_led(brd_rev);
	}
	if(fan_ug == 0) {
	  set_fan_led(brd_rev);
	}
	if(psu_ug == 0) {
	  set_psu_led(brd_rev);
	}
	if(smb_ug == 0) {
	  set_smb_led(brd_rev);
	}
  }
  return 0;
}

// Thread for monitoring debug card hotswap
static void *
debug_card_handler(void *unused) {
  int curr = -1;
  int prev = -1;
  int ret;
  uint8_t prsnt = 0;
  uint8_t lpc;

  while (1) {
 
    // Check if debug card present or not
    ret = pal_is_debug_card_prsnt(&prsnt);

    if (ret) {
      goto debug_card_out;
    }
    curr = prsnt;

    // Check if Debug Card was either inserted or removed
    if (curr != prev) {
      if (!curr) {
        // Debug Card was removed
        syslog(LOG_WARNING, "Debug Card Extraction\n");
      } else {
        // Debug Card was inserted
        syslog(LOG_WARNING, "Debug Card Insertion\n");
      }
    }

    // If Debug Card is present
    if (curr) {

      // Enable POST codes for scm slot
      ret = pal_post_enable(IPMB_BUS);
      if (ret) {
        goto debug_card_done;
      }

      // Get last post code and display it
      ret = pal_post_get_last(IPMB_BUS, &lpc);
      if (ret) {
        goto debug_card_done;
      }

      ret = pal_post_handle(IPMB_BUS, lpc);
      if (ret) {
        goto debug_card_out;
      }
    }

debug_card_done:
    prev = curr;
debug_card_out:
    if (curr == 1)
      msleep(500);
    else
      sleep(1);
  }

  return 0;
}

int
main (int argc, char * const argv[]) {
  pthread_t tid_scm_monitor;
  pthread_t tid_pim_monitor;
  pthread_t tid_debug_card;
  pthread_t tid_simLED_monitor;
  int rc;
  int pid_file;
  int brd_rev;
  signal(SIGTERM, exithandler);
  pid_file = open("/var/run/front-paneld.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      printf("Another front-paneld instance is running...\n");
      exit(-1);
    }
  } else {
   openlog("front-paneld", LOG_CONS, LOG_DAEMON);
  }

  if (pal_get_board_rev(&brd_rev)) {
    syslog(LOG_WARNING, "Get board revision failed\n");
    exit(-1);
  }

  if (pthread_create(&tid_scm_monitor, NULL, scm_monitor_handler, NULL) != 0) {
    syslog(LOG_WARNING, "pthread_create for scm monitor error\n");
    exit(1);
  }
  
  if (pthread_create(&tid_pim_monitor, NULL, pim_monitor_handler, NULL) != 0) {
    syslog(LOG_WARNING, "pthread_create for pim monitor error\n");
    exit(1);
  }
  
  if (pthread_create(&tid_simLED_monitor, NULL, simLED_monitor_handler, NULL) 
	  != 0) {
    syslog(LOG_WARNING, "pthread_create for simLED monitor error\n");
    exit(1);
  }

  if (brd_rev != BOARD_REV_EVTA) {
    if (pthread_create(&tid_debug_card, NULL, debug_card_handler, NULL) != 0) {
        syslog(LOG_WARNING, "pthread_create for debug card error\n");
        exit(1);
    }
    pthread_join(tid_debug_card, NULL);
  }

  pthread_join(tid_scm_monitor, NULL);
  pthread_join(tid_pim_monitor, NULL);
  pthread_join(tid_simLED_monitor, NULL);

  return 0;
}
