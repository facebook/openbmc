/*
 * fw-util
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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <openbmc/pal.h>

#define PAGE_SIZE  0x1000
#define AST_WDT_BASE 0x1e785000
#define WDT2_TIMEOUT_OFFSET 0x30

int flash_idx = -1;

static int
get_wdt30_value(uint32_t *wdt30_value){
  uint32_t wdt_fd;
  void *wdt_reg_addr;
  void *wdt30_reg_addr;

  wdt_fd = open("/dev/mem", O_RDWR | O_SYNC );
  if (wdt_fd < 0)
    return -1;

  wdt_reg_addr = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, wdt_fd,
         AST_WDT_BASE);
  wdt30_reg_addr = (char*)wdt_reg_addr + WDT2_TIMEOUT_OFFSET;
  *wdt30_value = *(volatile uint32_t*) wdt30_reg_addr;

  munmap(wdt_reg_addr, PAGE_SIZE);
  close(wdt_fd);

  return 0;
}

static int
get_boot_flash_num(int *flash_num){
  uint32_t wdt30_value = 0xFF;

  if (get_wdt30_value(&wdt30_value) == -1)
    return -1;

  if (((wdt30_value >> 1) & 0x1) == 0) {  // boot from flash 0
    *flash_num = 0;
  } else {                                // boot from flash 1
    *flash_num = 1;
  }

  return 0;
}

static void
print_usage_help(void) {
  if (flash_idx == 1) {
    printf("Usage: fw-util <--version>\n");
    printf("       fw-util <--update> flash1 <path>\n");
  } else {
    printf("Usage: fw-util <--version>\n");
    printf("       fw-util <--update> <flash0|flash1> <path>\n");
  }
}

static void
print_fw_bmc_ver(void) {
  FILE *fp = NULL;
  char str[64];

  fp = fopen("/etc/issue", "r");
  if (fp != NULL) {
    fseek(fp, 16, SEEK_SET);
    memset(str, 0, sizeof(char) * 64);
    if (!fscanf(fp, "%s", str)) {
      strcpy(str, "NA");
    }
    fclose(fp);
    printf("Flash%d BMC Version: %s\n", flash_idx, str);
  }

  return;
}

int
get_mtd_name(const char* name, char* dev)
{
  FILE* partitions = fopen("/proc/mtd", "r");
  char line[256], mnt_name[32];
  unsigned int mtdno;
  int found = 0;

  dev[0] = '\0';
  while (fgets(line, sizeof(line), partitions)) {
    if(sscanf(line, "mtd%d: %*x %*x %s",
                 &mtdno, mnt_name) == 2) {
      if(!strcmp(name, mnt_name)) {
        sprintf(dev, "/dev/mtd%d", mtdno);
        found = 1;
        break;
      }
    }
  }
  fclose(partitions);
  return found;
}

void sig_handler(int signo) {
  //Catch SIGINT, SIGTERM. If recived signal remove FW Update Flag File.
  //Kill flashcp process if there is any.
  char singal[10]={0};
  int retry = 3;
  bool is_flashcp_running = false;

  if (signo == SIGINT)
    sprintf(singal, "SIGINT");
  else if (signo == SIGTERM)
    sprintf(singal, "SIGTERM");
  else
    sprintf(singal, "UNKNOWN");


  if (signo == SIGINT || signo == SIGTERM) {

    printf("Got signal %d(%s). Preparinging to stop fw update.\n", signo, singal);
    syslog(LOG_WARNING, "Got signal %d(%s). Preparinging to stop fw update.\n", signo, singal);

    //If there is flashcp process running, then kill it
    do {
      if (!run_command("pidof flashcp  &> /dev/null")){ // flashcp is running
        is_flashcp_running = true;
        run_command("killall flashcp");
      } else { // flashcp is not running
        break;
      }

      retry--;
      sleep(1);
    } while (is_flashcp_running && (retry >= 0));

    if ((retry == 0) && is_flashcp_running){
      printf("Fail to kill flashcp...\n");
      syslog(LOG_WARNING, "Fail to kill flashcp...\n");
    }

    pal_set_fw_update_ongoing(FRU_ALL, 0);
    exit(0);
  }
}

int
main(int argc, char **argv) {
  int ret = -2;
  char cmd[80];
  char dev[12];
  char flash_mtd_name[32];
  bool is_bmc_update_flag = false;

  //Catch signals
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  if (get_boot_flash_num(&flash_idx) == -1)
    goto exit;
  printf("Boot from flash%d.\n", flash_idx);

  // Check for border conditions
  if ((argc != 2) && (argc != 4))
    goto exit;

  // check operation to perform
  if (!strcmp(argv[1], "--version")) {
    print_fw_bmc_ver();
    return 0;
  } else if (!strcmp(argv[1], "--update")) {
    if (argc != 4) {
      goto exit;
    }

    // Normal updating time is less than 4 minutes.
    pal_set_fw_update_ongoing(FRU_ALL, 4*60*3);

    if (!strcmp(argv[2], "flash0")) {
      if (flash_idx == 0) {
        sprintf(flash_mtd_name, "\"flash0\"");
      } else {
        // AST2400 limitation: if boots from CS1, we can't see CS0.
        printf("Can't update flash0 when BMC boot from flash1.\n");
        ret = -1;
        goto exit;
      }
      if (!get_mtd_name(flash_mtd_name, dev)) {
        printf("Error: Cannot find %s MTD partition in /proc/mtd\n", flash_mtd_name);
        ret = -1;
        goto exit;
      }
      printf("Flashing to device: %s\n", dev);
      snprintf(cmd, sizeof(cmd), "flashcp -v %s %s", argv[3], dev);
      ret = run_command(cmd);
      if (ret == 0) {
        syslog(LOG_CRIT, "BMC %s firmware update successfully", flash_mtd_name);
        printf("Updated BMC %s successfully, start to reboot BMC.\n", flash_mtd_name);
        is_bmc_update_flag = true;
      }
      ret = -1;
      goto exit;
    } else if (!strcmp(argv[2], "flash1")) {
      sprintf(flash_mtd_name, "\"flash1\"");

      if (!get_mtd_name(flash_mtd_name, dev)) {
        printf("Note: Cannot find %s MTD partition in /proc/mtd\n", flash_mtd_name);
        ret = -1;
        goto exit;
      }

      printf("Flashing to device: %s\n", dev);
      snprintf(cmd, sizeof(cmd), "flashcp -v %s %s", argv[3], dev);
      ret = run_command(cmd);
      if (ret == 0) {
        syslog(LOG_CRIT, "BMC %s firmware update successfully", flash_mtd_name);
        printf("Updated BMC %s successfully, start to reboot BMC.\n", flash_mtd_name);
        is_bmc_update_flag = true;
      }
      ret = -1;
      goto exit;
    }
  }

exit:
  if (ret == -2) {
    print_usage_help();
  } else {
    //clear fw update timer
    pal_set_fw_update_ongoing(FRU_ALL, 0);

    if (is_bmc_update_flag) {
      //clear the data partition after doing firmware update
      run_command("rm -rf /mnt/data/*");
      sleep(5);
      run_command("reboot");
    }
  }
  return ret;
}
