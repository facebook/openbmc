/*
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <openbmc/pal.h>
#include <openbmc/libgpio.h>

#define BIOS_VER_REGION_SIZE (4*1024*1024)
#define BIOS_ERASE_PKT_SIZE  (64*1024)
#define BIOS_FRU_ID          (1)

static int
get_bios_mtd_name(char* dev)
{
  const char *name = "\"bios0\"";
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

static int validate_bios_image(const char *path) {
  uint8_t board_info;
  uint8_t sku;
  int rc, fd, ret = -1, i = 0, rcnt, end;
  struct stat st;
  uint8_t *buf;
  size_t size;
  uint8_t ver_sig[] = { 0x46, 0x49, 0x44, 0x04, 0x78, 0x00 };
  const char *bios_project_tags[2] = {
    "F08_",
    "TP"
  };
  const char *exp_proj_tag;

  rc = pal_get_platform_id(&board_info);
  if(rc < 0) {
    return rc;
  }
  sku = board_info & 0x1;
  if (stat(path, &st) ||
    (fd = open(path, O_RDONLY)) < 0) {
    return -1;
  }
  exp_proj_tag = bios_project_tags[sku];

  size = st.st_size;
  if (size < BIOS_VER_REGION_SIZE) {
    goto close_bail;
  }

  buf = (uint8_t *)malloc(BIOS_VER_REGION_SIZE);
  if (!buf) {
    goto close_bail;
  }

  lseek(fd, size - BIOS_VER_REGION_SIZE, SEEK_SET);
  while (i < BIOS_VER_REGION_SIZE) {
    rcnt = read(fd, (buf + i), BIOS_ERASE_PKT_SIZE);
    if (rcnt < 0) {
      if (errno == EINTR)
        continue;
      goto free_bail;
    }
    i += rcnt;
  }

  end = BIOS_VER_REGION_SIZE -
            (sizeof(ver_sig) + strlen(exp_proj_tag));
  for(i = 0; i < end; i++) {
    if (!memcmp(buf + i, ver_sig, sizeof(ver_sig))) {
      char *project_tag = (char *)(buf + i + sizeof(ver_sig));
      if (memcmp(project_tag, exp_proj_tag, strlen(exp_proj_tag))) {
        // Temporary workaround TODO. Remove once no longer required.
        // If BIOS is for SKU == 1, just make sure that the project
        // tag does not match sku == 0.
        if (sku == 1 && memcmp(project_tag, bios_project_tags[0],
                strlen(bios_project_tags[0]))) {
          ret = 0;
          break;
        }
        goto free_bail;
      }
      ret = 0;
      break;
    }
  }
free_bail:
  free(buf);
close_bail:
  close(fd);
  return ret;
}

int bios_get_ver(uint8_t slot_id, char *ver)
{
  uint8_t sysfw_ver[32] = {0};
  int i;

  if (slot_id != FRU_MB)
    return -1;

  ver[0] = '\0';
  if (pal_get_sysfw_ver(BIOS_FRU_ID, sysfw_ver)) {
    return -1;
  }

  // BIOS version response contains the length at offset 2 followed by ascii string
  for (i = 3; i < 3 + sysfw_ver[2]; i++) {
    *ver++ = sysfw_ver[i];
  }
  *ver++ = '\0';
  return 0;
}

int bios_program(uint8_t slot_id, const char *file, bool check)
{
  int exit_code = -1;
  char cmd[80];
  char mtddev[32];
  gpio_desc_t *desc;

  if (slot_id != FRU_MB) {
    printf("ERROR: Slot does not support BIOS upgrade\n");
    return -1;
  }

  if(check == true && validate_bios_image(file)) {
    printf("ERROR: Invalid image: %s\n", file);
    return -1;
  }
  pal_set_server_power(slot_id, SERVER_POWER_OFF);
  sleep(10);
  pal_set_fw_update_ongoing(FRU_MB, 60*15);
  system("/usr/local/bin/me-util 0xB8 0xDF 0x57 0x01 0x00 0x01");
  sleep(1);

  if (gpio_export_by_name(GPIO_CHIP_ASPEED, "GPION5", "BMC_BIOS_FLASH_CTRL")) {
    printf("ERROR: Export of SPI-Switch GPIO failed\n");
    goto bail_export;
  }
  desc = gpio_open_by_shadow("BMC_BIOS_FLASH_CTRL");
  if (!desc) {
    printf("ERROR: Opening SPI-Switch GPIO failed\n");
    goto bail_open;
  }
  if (gpio_set_direction(desc, GPIO_DIRECTION_OUT) ||
      gpio_set_value(desc, GPIO_VALUE_HIGH)) {
    printf("ERROR: Switching BIOS to BMC failed\n");
    goto switch_bail;
  }
  system("echo -n \"spi1.0\" > /sys/bus/spi/drivers/m25p80/bind");
  if (!get_bios_mtd_name(mtddev)) {
    printf("ERROR: Could not get MTD device for the BIOS!\n");
    goto mtd_bail;
  }
  snprintf(cmd, sizeof(cmd), "flashcp -v %s %s", file, mtddev);
  exit_code = system(cmd);
  exit_code = exit_code == -1 ? 127 : WEXITSTATUS(exit_code);
mtd_bail:
  system("echo -n \"spi1.0\" > /sys/bus/spi/drivers/m25p80/unbind");
switch_bail:
  gpio_set_value(desc, GPIO_VALUE_LOW);
  gpio_close(desc);
bail_open:
  gpio_unexport("BMC_BIOS_FLASH_CTRL");
bail_export:
  sleep(1);
  pal_PBO();
  sleep(10);
  pal_set_server_power(slot_id, SERVER_POWER_ON);
  pal_set_fw_update_ongoing(FRU_MB, 0);
  return exit_code;
}
