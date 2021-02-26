/*
 * gpiod
 *
 * Copyright 2021-present Facebook. All Rights Reserved.
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
#include <sys/un.h>
#include <sys/file.h>
#include <openbmc/libgpio.h>
#include <openbmc/pal.h>
#include <openbmc/pal_sensors.h>
#include <facebook/fbgc_gpio.h>

#define MONITOR_FRUS_PRESENT_STATUS_INTERVAL    60 // seconds

#define E1S_IOCM_SLOT_NUM 2

static void *
fru_missing_monitor() {
  uint8_t fru_present_flag = 0, chassis_type = 0, uic_location_id = 0;
  uint8_t fru_present_status[FRU_CNT] = {FRU_PRESENT};
  uint8_t e1s_iocm_present_status[E1S_IOCM_SLOT_NUM] = {FRU_PRESENT};
  char fru_name[MAX_FRU_NAME_STR] = {0};
  char uic_location = '?';
  int fru_id = 0, e1s_iocm_slot_id = 0;
  
  memset(&fru_present_status, FRU_PRESENT, sizeof(fru_present_status));
  memset(&e1s_iocm_present_status, FRU_PRESENT, sizeof(e1s_iocm_present_status));
  memset(&fru_name, 0, sizeof(fru_name));
  
  while(1) {
    for (fru_id = FRU_SERVER; fru_id < FRU_CNT; fru_id++) {
      if ((fru_id == FRU_SERVER) || (fru_id == FRU_SCC)) {
        if (pal_is_fru_prsnt(fru_id, &fru_present_flag) < 0) {
          syslog(LOG_WARNING, "%s(): fail to get fru: %d present status\n", __func__, fru_id);
        } else {
          if (pal_get_fru_name(fru_id, fru_name) < 0) {
            syslog(LOG_WARNING, "%s(): fail to get fru: %d name\n", __func__, fru_id);
          } else {
            // fru insert
            if ((fru_present_flag == FRU_PRESENT) && (fru_present_status[fru_id] == FRU_ABSENT)) {
              syslog(LOG_CRIT, "DEASSERT: %s missing\n", fru_name);
              fru_present_status[fru_id] = FRU_PRESENT;
          
            // fru remove
            } else if ((fru_present_flag == FRU_ABSENT) && (fru_present_status[fru_id] == FRU_PRESENT)) {
              syslog(LOG_CRIT, "ASSERT: %s missing\n", fru_name);
              fru_present_status[fru_id] = FRU_ABSENT;
            }
          } 
        }
      } 
      
      if (fru_id == FRU_E1S_IOCM) {
        if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
          chassis_type = -1;
        }
        
        //Type 7
        if (chassis_type == CHASSIS_TYPE7) {
          if ((is_e1s_iocm_present(T5_E1S0_T7_IOC_AVENGER) == true) && (is_e1s_iocm_present(T5_E1S1_T7_IOCM_VOLT) == true)) {
            if (fru_present_status[fru_id] == FRU_ABSENT) {
              syslog(LOG_CRIT, "DEASSERT: iocm missing\n");
              fru_present_status[fru_id] = FRU_PRESENT;
              e1s_iocm_present_status[T5_E1S0_T7_IOC_AVENGER] = FRU_PRESENT;
              e1s_iocm_present_status[T5_E1S1_T7_IOCM_VOLT] = FRU_PRESENT;
            }
          
          } else {
            if (fru_present_status[fru_id] == FRU_PRESENT) {
              syslog(LOG_CRIT, "ASSERT: iocm missing\n");
              fru_present_status[fru_id] = FRU_ABSENT;
              
              if (is_e1s_iocm_present(T5_E1S0_T7_IOC_AVENGER) == false) {
                e1s_iocm_present_status[T5_E1S0_T7_IOC_AVENGER] = FRU_ABSENT;
              }
              
              if (is_e1s_iocm_present(T5_E1S1_T7_IOCM_VOLT) == false) {
                e1s_iocm_present_status[T5_E1S1_T7_IOCM_VOLT] = FRU_ABSENT;
              }
            }
          }
        
        // Type 5 and Type unknown
        } else {
          if (pal_get_uic_location(&uic_location_id) < 0) {
            syslog(LOG_WARNING, "%s(): fail to get uic location\n", __func__);
            uic_location = '?';
          } else {
            if(uic_location_id == UIC_SIDEA) {
              uic_location = 'a';
            } else if(uic_location_id == UIC_SIDEB) {
              uic_location = 'b';
            } else {
              uic_location = '?';
            }
          }
            
          for (e1s_iocm_slot_id = T5_E1S0_T7_IOC_AVENGER; e1s_iocm_slot_id < E1S_IOCM_SLOT_NUM; e1s_iocm_slot_id++) {
            if ((is_e1s_iocm_present(e1s_iocm_slot_id) == true) && (e1s_iocm_present_status[e1s_iocm_slot_id] == FRU_ABSENT)) {
              if (chassis_type == CHASSIS_TYPE5) {
                syslog(LOG_CRIT, "DEASSERT: e1.s %c%d missing\n", uic_location, e1s_iocm_slot_id);
              } else {
                syslog(LOG_CRIT, "DEASSERT: chassis type unknown, e1.s %d or iocm missing\n", e1s_iocm_slot_id);
              }
              e1s_iocm_present_status[e1s_iocm_slot_id] = FRU_PRESENT;
            }
          
            if ((is_e1s_iocm_present(e1s_iocm_slot_id) == false) && (e1s_iocm_present_status[e1s_iocm_slot_id] == FRU_PRESENT)) {
              if (chassis_type == CHASSIS_TYPE5) {
                syslog(LOG_CRIT, "ASSERT: e1.s %c%d missing\n", uic_location, e1s_iocm_slot_id);
              } else {
                syslog(LOG_CRIT, "ASSERT: chassis type unknown, e1.s %d or iocm missing\n", e1s_iocm_slot_id);
              }
              e1s_iocm_present_status[e1s_iocm_slot_id] = FRU_ABSENT;
              fru_present_status[fru_id] = FRU_ABSENT;
            }
          }
            
          if ((is_e1s_iocm_present(T5_E1S0_T7_IOC_AVENGER) == true) && (is_e1s_iocm_present(T5_E1S1_T7_IOCM_VOLT) == true)) {
            fru_present_status[fru_id] = FRU_PRESENT;
          }
        }
      }
      
    } // for loop end
    
    sleep(MONITOR_FRUS_PRESENT_STATUS_INTERVAL);
  } // while loop end
  
  pthread_exit(NULL);
}

static void
print_usage() {
  printf("Usage: gpiod [ %s ]\n", pal_server_list);
}

static void
run_gpiod(int argc, char **argv) {
  pthread_t tid_fru_missing_monitor;
  
  if (argv == NULL) {
    syslog(LOG_ERR, "fail to execute gpiod because NULL parameter: **argv\n");
    exit(EXIT_FAILURE);
  }
  
  // Monitor fru missing by polling (server, SCC, E1.S, IOCM)
  if (pthread_create(&tid_fru_missing_monitor, NULL, fru_missing_monitor, NULL) < 0) {
    syslog(LOG_ERR, "fail to creat thread for monitor fru missing\n");
  }
  
  pthread_join(tid_fru_missing_monitor, NULL);
}
 
int
main(int argc, char **argv) {
  int rc = 0, pid_file = 0;
  
  if (argv == NULL) {
    syslog(LOG_ERR, "fail to execute gpiod because NULL parameter: **argv\n");
    exit(EXIT_FAILURE);
  }

  if (argc < 2) {
    print_usage();
    exit(EXIT_FAILURE);
  }

  pid_file = open("/var/run/gpiod.pid", O_CREAT | O_RDWR, 0666);
  rc = pal_flock_retry(pid_file);
  if (rc < 0) {
    if (EWOULDBLOCK == errno) {
      printf("another gpiod instance is running...\n");
    } else {
      syslog(LOG_ERR, "fail to execute gpiod because %s\n", strerror(errno));
    }
    
    pal_unflock_retry(pid_file);
    close(pid_file);
    exit(EXIT_FAILURE);
    
  } else {
    syslog(LOG_INFO, "daemon started");
    run_gpiod(argc, argv);
  }
  
  pal_unflock_retry(pid_file);
  close(pid_file);
  return 0;
}
