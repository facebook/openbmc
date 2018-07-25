/*
 * psu-util
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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

#include <facebook/minipack-psu.h>

i2c_info_t psu[] = {
  {49, 0x51, 0x59, PSU1_EEPROM},
  {48, 0x50, 0x58, PSU2_EEPROM},
  {57, 0x51, 0x59, PSU3_EEPROM},
  {56, 0x50, 0x58, PSU4_EEPROM},
};

pmbus_info_t pmbus[] = {
  {"MFR_ID", 0x99},
  {"MFR_MODEL", 0x9a},
  {"MFR_REVISION", 0x9b},
  {"MFR_DATE", 0x9d},
  {"MFR_SERIAL", 0x9e},
  {"PRI_FW_VER", 0xdd},
  {"SEC_FW_VER", 0xd7},
};

static const char *option_list[] = {
  "--get_psu_info",
  "--get_eeprom_info"
};

static void
print_usage(const char *name) {

  int i;

  printf("Usage: %s <psu1|psu2|psu3|psu4> --update <file_path>\n", name);
  printf("Usage: %s <psu1|psu2|psu3|psu4> <option>\n", name);
  printf("       option:\n");
  for (i = 0; i < sizeof(option_list)/sizeof(option_list[0]); i++)
    printf("       %s\n", option_list[i]);
}

static int
i2c_open(uint8_t bus, uint8_t addr) {

  int fd = -1;
  int rc = -1;
  char fn[32];

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);

  if (fd == -1) {
    syslog(LOG_WARNING, "Failed to open i2c device %s", fn);
    return -1;
  }

  rc = ioctl(fd, I2C_SLAVE_FORCE, addr);
  if (rc < 0) {
    syslog(LOG_WARNING, "Failed to open slave @ address 0x%x", addr);
    close(fd);
    return -1;
  }

  rc = ioctl(fd, I2C_PEC, 1);
  if (rc < 0) {
    syslog(LOG_WARNING, "Failed to set I2C PEC @ address 0x%x", addr);
    close(fd);
    return -1;
  }

  return fd;
}

static int
get_psu_info(u_int8_t id) {

  int fd = -1, rc = -1;
  int i = 0, size = 0;
  uint8_t block[I2C_SMBUS_BLOCK_MAX + 1];
  uint16_t word = 0;

	fd = i2c_open(psu[id].bus, psu[id].pmbus_addr);
  if (fd < 0) {
    ERR_PRINT("Fail to open i2c");
    return -1;
  }

  size = sizeof(pmbus)/sizeof(pmbus[0]);
  for (i = 0; i < size; i++) {
    if (i < size -2) {
      memset(block, 0, sizeof(block));
      rc = i2c_smbus_read_block_data(fd, pmbus[i].reg, block);
      if (rc < 0) {
        ERR_PRINT("get_psu_info()");
        return -1;
      }
      printf("%s: %s\n", pmbus[i].item, block);
    }
    else {
      word = i2c_smbus_read_word_data(fd, pmbus[i].reg);
      printf("%s: %d.%d\n", pmbus[i].item, (word & 0xf), ((word >> 8) & 0xf));
    }
  }
  close(fd);

  return 0;
}

/* Print the FRUID in detail */
static void
print_fruid_info(fruid_info_t *fruid, const char *name)
{
  /* Print format */
  printf("%-27s: %s", "\nFRU Information",
      name /* Name of the FRU device */ );
  printf("%-27s: %s", "\n---------------", "------------------");

  if (fruid->chassis.flag) {
    printf("%-27s: %s", "\nChassis Type",fruid->chassis.type_str);
    printf("%-27s: %s", "\nChassis Part Number",fruid->chassis.part);
    printf("%-27s: %s", "\nChassis Serial Number",fruid->chassis.serial);
    if (fruid->chassis.custom1 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 1",fruid->chassis.custom1);
    if (fruid->chassis.custom2 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 2",fruid->chassis.custom2);
    if (fruid->chassis.custom3 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 3",fruid->chassis.custom3);
    if (fruid->chassis.custom4 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 4",fruid->chassis.custom4);
  }

  if (fruid->board.flag) {
    printf("%-27s: %s", "\nBoard Mfg Date",fruid->board.mfg_time_str);
    printf("%-27s: %s", "\nBoard Mfg",fruid->board.mfg);
    printf("%-27s: %s", "\nBoard Product",fruid->board.name);
    printf("%-27s: %s", "\nBoard Serial",fruid->board.serial);
    printf("%-27s: %s", "\nBoard Part Number",fruid->board.part);
    printf("%-27s: %s", "\nBoard FRU ID",fruid->board.fruid);
    if (fruid->board.custom1 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 1",fruid->board.custom1);
    if (fruid->board.custom2 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 2",fruid->board.custom2);
    if (fruid->board.custom3 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 3",fruid->board.custom3);
    if (fruid->board.custom4 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 4",fruid->board.custom4);
  }

  if (fruid->product.flag) {
    printf("%-27s: %s", "\nProduct Manufacturer",fruid->product.mfg);
    printf("%-27s: %s", "\nProduct Name",fruid->product.name);
    printf("%-27s: %s", "\nProduct Part Number",fruid->product.part);
    printf("%-27s: %s", "\nProduct Version",fruid->product.version);
    printf("%-27s: %s", "\nProduct Serial",fruid->product.serial);
    printf("%-27s: %s", "\nProduct Asset Tag",fruid->product.asset_tag);
    printf("%-27s: %s", "\nProduct FRU ID",fruid->product.fruid);
    if (fruid->product.custom1 != NULL)
      printf("%-27s: %s", "\nProduct Custom Data 1",fruid->product.custom1);
    if (fruid->product.custom2 != NULL)
      printf("%-27s: %s", "\nProduct Custom Data 2",fruid->product.custom2);
    if (fruid->product.custom3 != NULL)
      printf("%-27s: %s", "\nProduct Custom Data 3",fruid->product.custom3);
    if (fruid->product.custom4 != NULL)
      printf("%-27s: %s", "\nProduct Custom Data 4",fruid->product.custom4);
  }

  printf("\n");
}

/* Populate and print fruid_info by parsing the fru's binary dump */
static int 
get_eeprom_info(u_int8_t id) {

  int ret = -1, fd = -1;
  char name[5];
  char cmd[64];
  char model_str[32];
  fruid_info_t fruid;
  uint8_t block[I2C_SMBUS_BLOCK_MAX + 1] = {0};

	fd = i2c_open(psu[id].bus, psu[id].pmbus_addr);
  if (fd < 0) {
    return -1;
  }

  if (get_mfr_model(fd, pmbus[1].reg, block)) {
    close(fd);
    return -1;
  }

  snprintf(model_str, sizeof(model_str), "%s", block);
  if (!strncmp(model_str, "ECD55020006", 11)) {
    snprintf(cmd, sizeof(cmd), DRIVER_ADD,
             "24c64", psu[id].eeprom_addr, psu[id].bus);
  }
  else if (!strncmp(model_str, "PFE1500-12-054NA", 16) ||
           !strncmp(model_str, "D1U54P-W-1500-12-HC4TC", 22)) {
    snprintf(cmd, sizeof(cmd), DRIVER_ADD,
             "24c02", psu[id].eeprom_addr, psu[id].bus);
  }
  else {
    printf("Unsupported device or device is not ready");
    return -1;
  }
  run_command(cmd);
  memset(cmd, 0, sizeof(cmd));

  ret = fruid_parse(psu[id].eeprom_file, &fruid);

  snprintf(cmd, sizeof(cmd), DRIVER_DEL,
           psu[id].eeprom_addr, psu[id].bus);
  run_command(cmd);

  snprintf(name, sizeof(name), "PSU%d", id + 1);
  if (ret) {
    printf("Failed print EEPROM info!\n");
    return -1;
  }
  else {
    print_fruid_info(&fruid, name);
    free_fruid_info(&fruid);
  }

  return 0;
}

int
main(int argc, const char *argv[]) {

  u_int8_t psu_id = 0;

  if (argc < 3) {
    print_usage(argv[0]);
  }

  if (!strcmp(argv[1], "psu1")) {
    psu_id = 0;
  }
  else if (!strcmp(argv[1] , "psu2")) {
    psu_id = 1;
  }
  else if (!strcmp(argv[1] , "psu3")) {
    psu_id = 2;
  }
  else if (!strcmp(argv[1] , "psu4")) {
    psu_id = 3;
  }
  else {
    print_usage(argv[0]);
  }

  if (!strcmp(argv[2], "--get_psu_info")) {
    if (get_psu_info(psu_id)) {
      return -1;
    }
  }
  else if (!strcmp(argv[2], "--get_eeprom_info")) {
    if (get_eeprom_info(psu_id)) {
      return -1;
    }
  }
  else if (!strcmp(argv[2], "--update")) {
    printf("Not support yet!");
  }
  else {
    print_usage(argv[0]);
  }

	return 0;
}
