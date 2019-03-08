/*
 * cfg-util
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
#include <stdint.h>
#include <string.h>
#include <openbmc/pal.h>
#include <sys/reboot.h>
#include <syslog.h>
#include <sys/mman.h>
#include <openbmc/kv.h>

#define PAGE_SIZE                     0x1000
#define AST_SRAM_BMC_REBOOT_BASE      0x1E721000
#define BMC_REBOOT_BY_CMD(base) *((uint32_t *)(base + 0x204))

#define BIT_RECORD_LOG                (1 << 8)
#define FLAG_CFG_UTIL                 (1 << 1)

static void
print_usage(void) {
  printf("Usage: cfg-util <dump-all|key> <value>\n");
  printf("       cfg-util --clear\n");
}

int
main(int argc, char **argv) {

  int ret;
  int check_key;
  int mem_fd;
  uint8_t key[MAX_KEY_LEN] = {0};
  uint8_t val[MAX_VALUE_LEN] = {0};
  uint8_t *bmc_reboot_base;

  // Handle boundary checks
  if (argc < 2 || argc > 3) {
    goto err_exit;
  }

  // Handle dump of key value data base
  if ((argc == 2) && (!strcmp(argv[1], "dump-all"))){
      pal_dump_key_value();
      return 0;
  }

  // Handle Reset Default factory settings
  if ((argc == 2) && (!strcmp(argv[1], "--clear"))){
      printf("Reset BMC data to default factory settings and BMC will be reset...\n");
      system("rm /mnt/data/* -rf > /dev/null 2>&1");
      sync();

      // Set the flag to identify the reboot is caused by cfg-util
      mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
      if (mem_fd >= 0) {
        bmc_reboot_base = (uint8_t *)mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, AST_SRAM_BMC_REBOOT_BASE);
        if (bmc_reboot_base != 0) {
          BMC_REBOOT_BY_CMD(bmc_reboot_base) |= BIT_RECORD_LOG | FLAG_CFG_UTIL;
          munmap(bmc_reboot_base, PAGE_SIZE);
        }
        close(mem_fd);
      }

      sleep(3);
      reboot(RB_AUTOBOOT);
      return 0;
  }

  // Handle Get the Configuration
  if (argc == 2) {
    snprintf(key, MAX_KEY_LEN, "%s", argv[1]);

    ret = pal_get_key_value(key, val);
    if (ret) {
      goto err_exit;
    }

    printf("%s\n", val);
    return 0;
  }

  // Handle Set the configuration
  snprintf(key, MAX_KEY_LEN, "%s", argv[1]);
  snprintf(val, MAX_VALUE_LEN, "%s", argv[2]);

  // Check the key can be set or not by cfg-util
  check_key = pal_cfg_key_check(key);
  if (check_key == 0) {         // check OK
    ret = pal_set_key_value(key, val);
    if (ret != 0) {
      goto err_exit;
    }
  } else if (check_key == 1) {  // the key is valid but unsupported in cfg-util
    printf("The key does not support to set: %s\n", key);
  } else {                      // the key is invalid
    goto err_exit;
  }

  return 0;

err_exit:
  print_usage();
  exit(-1);
}
