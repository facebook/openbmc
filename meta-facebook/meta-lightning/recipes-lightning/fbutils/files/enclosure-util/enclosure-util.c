/*
 * enclosure-util
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
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <openbmc/pal.h>
#include "enclosure-util.h"

#define MAX_SLOT_NUM 15
#define ERROR_CODE_SSD_FAULT_BASE 70
#define PCIE_SWITCH_FAULT 1
#define PCIE_LINK_LEFT 90
#define PCIE_LINK_RIGHT 91

static int bit_map(unsigned char* in_error,unsigned char* data);

static void
print_usage_help(void) {
  printf("Usage: enclosure-util --error\n");
  printf("       enclosure-util --switch-status --set [assert, deassert] [1, 90, 91]\n");
  printf("           Note:\n");
  printf("           1 : %s\n", Error_Code_Description[1]);
  printf("           90: %s\n", Error_Code_Description[90]);
  printf("           91: %s\n", Error_Code_Description[91]);
  printf("       enclosure-util --switch-status --get\n");
  printf("       enclosure-util --drive-status <slot number [0~14], --all>\n");
  printf("       enclosure-util --drive-health\n");
}

void
get_error_code() {
  uint8_t error_code_assert[ERROR_CODE_NUM * 8];
  int p_ret = 0;
  uint8_t error_code_array[ERROR_CODE_NUM] = {0};
  int tmp = 0;

  pal_read_error_code_file(error_code_array);

  memset(error_code_assert, 0, ERROR_CODE_NUM * 8);
  p_ret = bit_map(error_code_array, error_code_assert);
  printf("Error Counter: %d\n", p_ret);
  for (tmp = 0; tmp < p_ret; tmp++) {
    if ( error_code_assert[tmp] < MAX_DESCRIPTION_NUM ) {
      printf("Error Code %d: %s", (uint32_t)error_code_assert[tmp],
                                  Error_Code_Description[error_code_assert[tmp]]);
    }
    else {
      printf("Wrong error number\n");
      syslog(LOG_WARNING, "enclosure-util: %s: Wrong error number", __func__);
    }
    printf("\n");
  }
  printf("\n");
}

void
get_switch_status() {
  uint8_t error_code_assert[ERROR_CODE_NUM * 8];
  int p_ret = 0;
  uint8_t error_code_array[ERROR_CODE_NUM] = {0};
  int tmp = 0;
  bool switchStatus[3] = {0};

  pal_read_error_code_file(error_code_array);

  memset(error_code_assert, 0, ERROR_CODE_NUM * 8);
  p_ret = bit_map(error_code_array, error_code_assert);
  for (tmp = 0; tmp < p_ret; tmp++) {
    if ( error_code_assert[tmp] == PCIE_SWITCH_FAULT)
      switchStatus[0] = 1;
    if ( error_code_assert[tmp] == PCIE_LINK_LEFT)
      switchStatus[1] = 1;
    if ( error_code_assert[tmp] == PCIE_LINK_RIGHT)
      switchStatus[2] = 1;
  }

  printf("Error Code:\n");
  if (switchStatus[0])
    printf("%s: assert\n", Error_Code_Description[PCIE_SWITCH_FAULT]);
  else
    printf("%s: deassert\n", Error_Code_Description[PCIE_SWITCH_FAULT]);
  if (switchStatus[1])
    printf("%s: assert\n", Error_Code_Description[PCIE_LINK_LEFT]);
  else
    printf("%s: deassert\n", Error_Code_Description[PCIE_LINK_LEFT]);
  if (switchStatus[2])
    printf("%s: assert\n", Error_Code_Description[PCIE_LINK_RIGHT]);
  else
    printf("%s: deassert\n", Error_Code_Description[PCIE_LINK_RIGHT]);
  printf("\n");
}

/* if enclosure-util read SSD,
create a flag file to avoid other process to do any  SSD monitor */
void ssd_monitor_critical_section() {
  FILE *fp = NULL;
  fp = fopen(SKIP_READ_SSD_TEMP, "w");
  fprintf(fp, "%d ", MAX_SLOT_NUM * 4);
  fclose(fp);
  msleep(100);
}

void drive_status(int num) {
  int ret;
  ssd_monitor_critical_section();

  /* read NVMe-MI data */
  ret = pal_read_nvme_data(num, CMD_DRIVE_STATUS);
  if (ret == 1 || ret ==5)
    pal_err_code_disable(ERROR_CODE_SSD_FAULT_BASE + num);
  else {
    pal_err_code_enable(ERROR_CODE_SSD_FAULT_BASE + num);
    if(ret == -1)
      printf("Read slot%d data fail.\n", num);
  }
}

void drive_health() {
  int drive[MAX_SLOT_NUM];
  int i;

  ssd_monitor_critical_section();

  /* read NVMe-MI data */
  for (i = 0; i < MAX_SLOT_NUM; i++) {
    drive[i] = pal_read_nvme_data(i, CMD_DRIVE_HEALTH);
    if (drive[i] == 1 || drive[i] == 5 )
      pal_err_code_disable(ERROR_CODE_SSD_FAULT_BASE + i);
    else
      pal_err_code_enable(ERROR_CODE_SSD_FAULT_BASE + i);
  }

  remove(SKIP_READ_SSD_TEMP);

  /*print result
    0: (U.2) drive is not health
    1: (U.2) drive is health
    2: (M.2) two drives are not health
    3: (M.2) only first drive is health
    4: (M.2) only second drive is health
    5: (M.2) two drives are health*/

  printf("Normal Drives: ");
  for (i = 0; i < MAX_SLOT_NUM; i++)
    if (drive[i] == 1)
      printf("%d ", i);
    else if (drive[i] == 3)
      printf("%d.1 ", i);
    else if (drive[i] == 4)
      printf("%d.2 ", i);
    else if (drive[i] == 5)
      printf("%d.1 %d.2 ", i, i);

  printf("\n");

  printf("Abnormal Drives: ");
  for (i = 0; i < MAX_SLOT_NUM; i++)
    if ((drive[i] == 0) || (drive[i] < 0))
      printf("%d ", i);
    else if (drive[i] == 3)
      printf("%d.2 ", i);
    else if (drive[i] == 4)
      printf("%d.1 ", i);
    else if (drive[i] == 2)
      printf("%d.1 %d.2 ", i, i);
  printf("\n");
}

int
main(int argc, char **argv) {
  if (argc < 2) {
    print_usage_help();
    return -1;
  }

  if ((argc == 2) && !strcmp(argv[1], "--error"))
    get_error_code();

  else if (!strcmp(argv[1], "--switch-status")) {
    if ((argc == 5) && !strcmp(argv[2], "--set")) {
      if (!strcmp(argv[4], "1") || !strcmp(argv[4], "90") || !strcmp(argv[4], "91")) {
        if (!strcmp(argv[3], "assert")) {
          pal_err_code_enable(atoi(argv[4]));
          printf("%s: assert\n", Error_Code_Description[atoi(argv[4])]);
        }

        else if (!strcmp(argv[3], "deassert")) {
          pal_err_code_disable(atoi(argv[4]));
          printf("%s: deassert\n", Error_Code_Description[atoi(argv[4])]);
        }

        else
          print_usage_help();
      }

      else
        print_usage_help();
    }

    else if ((argc == 3) && !strcmp(argv[2], "--get"))
      get_switch_status();

    else
      print_usage_help();
  }

  else if ((argc == 3) && !strcmp(argv[1], "--drive-status")) {
    if (!strcmp(argv[2], "--all")) {
      int i;
      for (i = 0; i < MAX_SLOT_NUM; i++)
        drive_status(i);
      remove(SKIP_READ_SSD_TEMP);
    }

    else if (isdigit(argv[2][0]) && (atoi(argv[2]) > -1) && (atoi(argv[2]) < MAX_SLOT_NUM)) {
      drive_status(atoi(argv[2]));
      remove(SKIP_READ_SSD_TEMP);
    }

    else
      print_usage_help();
  }

  else if ((argc == 2) && !strcmp(argv[1], "--drive-health")) {
    drive_health();
  }

  else
    print_usage_help();

  return 0;
}

static int bit_map(unsigned char* in_error,unsigned char* data)
{
  int ret = 0;
  int ii=0;
  int kk=0;
  if( data == NULL)
    return 0;

  for( ii = 0; ii < ERROR_CODE_NUM; ii++ ) {
    for( kk = 0; kk < 8; kk++ ) {
      if((( in_error[ii] >> kk )&0x01 ) == 1) {
        if( (data + ret) == NULL)
          return ret;
        *(data + ret) = ii*8 + kk;
        ret++;
      }
    }
  }
  return ret;
}
