/*
 * cpldupdate-i2c.c - Update Lattice CPLD through I2c
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <openbmc/obmc-i2c.h>

// #define DEBUG

#ifdef DEBUG
#define CPLD_BEBUG(fmt, args...) printf(fmt, ##args);
#else
#define CPLD_BEBUG(fmt, args...)
#endif

#define ERR_PRINT(fmt, args...) \
        fprintf(stderr, fmt ": %s\n", ##args, strerror(errno));

#define BUSY_RETRIES 15

typedef struct _i2c_info_t {
  int fd;
  uint8_t bus;
  uint8_t addr;
} i2c_info_t;

enum {
  TRANSPARENT_MODE = 0x74,
  OFFLINE_MODE = 0xC6,
  CFG_PAGE = 0x46,
  UFM_PAGE = 0x47,
};

static void
print_usage(const char *name) {
  printf("Usage: %s <bus> <address> <img_path>\n", name);
}

static int
i2c_open(uint8_t bus_num, uint8_t addr) {
  int fd = -1, rc = -1;
  char fn[32];

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus_num);
  fd = open(fn, O_RDWR);
  if (fd == -1) {
    ERR_PRINT("Failed to open i2c device %s", fn);
    return -1;
  }

  rc = ioctl(fd, I2C_SLAVE, addr);
  if (rc < 0) {
    ERR_PRINT("Failed to open slave @ address 0x%x", addr);
    close(fd);
    return -1;
  }
  return fd;
}

static int
ascii_to_hex(int ascii) {

  ascii = ascii & 0xFF;
  if (ascii >= 0x30 && ascii <= 0x39) {
    return (ascii - 0x30);
  }/*0-9*/
  else if (ascii >= 0x41 && ascii <= 0x46) {
    return (ascii - 0x41 + 10);
  }/*A-F*/
  else if (ascii >= 0x61 && ascii <= 0x66) {
    return (ascii - 0x61 + 10);
  }/*a-f*/
  else {
    return -1;
  }
}

static int
read_device_id(i2c_info_t cpld) {

  uint8_t device_id_cmd[4] = {0xE0, 0x00, 0x00, 0x00};
  uint8_t device_id[4];
  int ret = -1;

  ret = i2c_rdwr_msg_transfer(cpld.fd, cpld.addr << 1, device_id_cmd,
                          sizeof(device_id_cmd), device_id, sizeof(device_id));
  if (ret != 0) {
    ERR_PRINT("read_device_id()");
    return ret;
  }
  CPLD_BEBUG("Read Device ID = 0x%X 0x%X 0x%X 0x%X -\n",
             device_id[0], device_id[1], device_id[2], device_id[3]);
  return 0;
}

static int
read_status(i2c_info_t cpld) {

  uint8_t status_cmd[4] = {0x3C, 0x00, 0x00, 0x00};
  uint8_t status[4];
  int ret = -1;

  ret = i2c_rdwr_msg_transfer(cpld.fd, cpld.addr << 1, status_cmd,
                              sizeof(status_cmd), status, sizeof(status));
  if (ret != 0) {
    ERR_PRINT("read_status()");
    return ret;
  }
  CPLD_BEBUG("Read Status = 0x%X 0x%X 0x%X 0x%X -\n",
              status[0], status[1], status[2], status[3]);
  return 0;
}

static int
read_busy_flag(i2c_info_t cpld) {

  uint8_t busy_flag_cmd[4] = {0xF0, 0x00, 0x00, 0x00};
  uint8_t count = 0;
  uint8_t flag[1];
  int ret = -1;

  for (count = 0; count < BUSY_RETRIES; count++) {
    ret = i2c_rdwr_msg_transfer(cpld.fd, cpld.addr << 1, busy_flag_cmd,
                                sizeof(busy_flag_cmd), flag, sizeof(flag));
    if (ret != 0) {
      ERR_PRINT("read_busy_flag()");
      return ret;
    }
    if (!(flag[0] & 0x80)) {
      return 0;
    }
    sleep(1);
  }
  CPLD_BEBUG("Time Out for Read Busy Flag");

  return -1;
}

static int
enable_program_mode(i2c_info_t cpld, uint8_t mode) {

  uint8_t enable_program_cmd[3] = {mode, 0x08, 0x00};
  int ret = -1;

  printf("Enable Program - ");
  if (mode == TRANSPARENT_MODE) {
    printf("Transparent Mode\n");
  }
  else if (mode == OFFLINE_MODE) {
    printf("Offline Mode\n");
  }
  ret = i2c_rdwr_msg_transfer(cpld.fd, cpld.addr << 1, enable_program_cmd,
                              sizeof(enable_program_cmd), NULL, 0);
  if (ret != 0) {
    ERR_PRINT("enable_program_mode()");
    return ret;
  }
  usleep(5);
  return 0;
}

static int
erase_flash(i2c_info_t cpld, uint8_t location) {
  /*
   * location[0]: SRAM
   * location[1]: FEATURE-ROW
   * location[2]: NVCM0/CFG
   * location[3]: NVCM1/UFM
  */
  uint8_t erase_flash_cmd[4] = {0x0E, location, 0x00, 0x00};
  int ret = -1;

  printf("Erase Flash ");
  if (location & 0x1) {
    printf("SRAM ");
  }
  if (location & 0x2) {
    printf("FEATURE-ROW ");
  }
  if (location & 0x4) {
    printf("NVCM0/CFG ");
  }
  if (location & 0x8) {
    printf("NVCM1/UFM ");
  }
  printf("\n");

  ret = i2c_rdwr_msg_transfer(cpld.fd, cpld.addr << 1, erase_flash_cmd,
                              sizeof(erase_flash_cmd), NULL, 0);
  if (ret != 0) {
    ERR_PRINT("erase_flash()");
    return ret;
  }
  if (read_busy_flag(cpld) != 0) {
    CPLD_BEBUG("Device busy is caused by flash erase\n");
    return -1;
  }
  return 0;
}

static int
get_data_len(const char *file_path, int *cfg_len, int *ufm_len) {

  int fd = -1;
  int i = 0, file_len = 0, ufm_start = 0;
  struct stat st;

  fd = open(file_path, O_RDONLY, 0666);
  if (fd < 0) {
    ERR_PRINT("get_data_len()");
    return -1;
  }

  stat(file_path, &st);
  file_len = st.st_size;
  CPLD_BEBUG("file_len: %d\n", file_len);

  uint8_t file_buf[file_len];

  if (read(fd, file_buf, file_len) < 0) {
    ERR_PRINT("get_data_len()");
  }

  for (i = 0; i < file_len; i+=34) {
    if (file_buf[i+32] == 0x0d && file_buf[i+33] == 0x0a) {
      *cfg_len = ((i + 34) / 34) * 32;
      if (file_buf[i+34] == 0x0d && file_buf[i+35] == 0x0a) {
        break;
      }
    }
    else {
      printf("get_data_len(): Invalid CFG content\n");
      return -1;
    }
  }
  *cfg_len = *cfg_len / 2;
  CPLD_BEBUG("cfg_len: %d\n", *cfg_len);

  if (i >= file_len) {
      printf("get_data_len():Can't get seprator\n");
      return -1;
  }

  ufm_start = i + 34 + 2;
  for (i = ufm_start; i < file_len; i+=34) {
    if (file_buf[i+32] == 0x0d && file_buf[i+33] == 0x0a) {
      *ufm_len = ((i - ufm_start + 34) / 34) * 32;
    }
    else {
      printf("get_data_len(): Invalid UFM content\n");
      return -1;
    }
  }
  *ufm_len = *ufm_len / 2;
  CPLD_BEBUG("ufm_len: %d\n", *ufm_len);
  return 0;
}

static int
get_img_data(const char *file_path, uint8_t cfg_data[], int cfg_len,
             uint8_t ufm_data[], int ufm_len) {

  int fd = -1;
  int i = 0, j = 0, file_len= 0, ufm_start = 0;
  struct stat st;

  fd = open(file_path, O_RDONLY, 0666);
  if (fd < 0) {
    ERR_PRINT("get_img_data()");
    return -1;
  }

  stat(file_path, &st);
  file_len = st.st_size;

  uint8_t file_buf[file_len];

  if (read(fd, file_buf, file_len) < 0) {
    ERR_PRINT("get_img_data()");
    return -1;
  }

  for (i = 0, j = 0; i < cfg_len; i++, j+=2) {
    cfg_data[i] = (ascii_to_hex(file_buf[j])) << 4;
    cfg_data[i] |= ascii_to_hex(file_buf[j+1]);
    if (file_buf[j+2] == 0x0d && file_buf[j+3] == 0x0a) {
      j+=2;
    }
  }
  ufm_start = j + 2;
  for (i = 0 , j = ufm_start; i < ufm_len; i++, j+=2) {
    ufm_data[i] = (ascii_to_hex(file_buf[j])) << 4;
    ufm_data[i] |= ascii_to_hex(file_buf[j+1]);
    if (file_buf[j+2] == 0x0d && file_buf[j+3] == 0x0a) {
      j+=2;
    }
  }
  return 0;
}

/* This function is needed by Transparent Mode */
static int
refresh(i2c_info_t cpld) {

  uint8_t refresh_cmd[3] = {0x79 ,0x00 ,0x00};
  int ret = -1;

  printf("Refreshing CPLD...");
  ret = i2c_rdwr_msg_transfer(cpld.fd, cpld.addr << 1, refresh_cmd,
                              sizeof(refresh_cmd), NULL, 0);
  if (ret != 0) {
    ERR_PRINT("\nrefresh()");
    return ret;
  }
  sleep(1);
  printf("Done\n");
  return 0;
}

static int
verify_flash(i2c_info_t cpld, uint8_t page,
             uint8_t *data, int data_len) {

  uint8_t reset_addr_cmd[4] = {page, 0x00, 0x00, 0x00};
  /* 0x73 0x00: i2c, 0x73 0x10: JTAG/SSPI */
  uint8_t read_page_cmd[4] = {0x73, 0x00, 0x00, 0x01};
  uint8_t page_data[16] = {0};
  int i = 0, byte_index = 0;
  int ret = -1;

  /* Reset Page Address */
  ret = i2c_rdwr_msg_transfer(cpld.fd, cpld.addr << 1, reset_addr_cmd,
                              sizeof(reset_addr_cmd), NULL, 0);
  if (ret != 0) {
    ERR_PRINT("verify_flash(): Reset Page Address");
    return ret;
  }
  if (read_busy_flag(cpld) != 0) {
    CPLD_BEBUG("Device busy is caused by address reset\n");
    return -1;
  }

  if (page == CFG_PAGE) {
    printf("- Verify CFG Page -\n");
  }
  else if (page == UFM_PAGE) {
    printf("- Verify UFM Page -\n");
  }

  for (byte_index = 0; byte_index < data_len; byte_index+=16) {
    /* Read Page Data */
    ret = i2c_rdwr_msg_transfer(cpld.fd, cpld.addr << 1, read_page_cmd,
                          sizeof(read_page_cmd), page_data, sizeof(page_data));
    if (ret != 0) {
      ERR_PRINT("verify_flash(): Read Page Data");
      return ret;
    }
    if (read_busy_flag(cpld) != 0) {
      CPLD_BEBUG("Device busy is caused by Read Page Data\n");
      return -1;
    }
    usleep(200);

    /* Compare Data */
    if (memcmp(page_data, data+byte_index, 16) != 0) {
      CPLD_BEBUG("\nImage_data: ");
      for (i = 0; i < 16; i++) {
        CPLD_BEBUG("0x%2x ", page_data[i]);
      }
      CPLD_BEBUG("\nFlash_data: ");
      for (i = 0; i < 16; i++) {
        CPLD_BEBUG("0x%2x ", data[byte_index+i]);
      }
      printf("\nCompare Fail - Do Clean Up Procedure\n");
      erase_flash(cpld, 0x0C);
      refresh(cpld);
      return 1;
    }

    printf("  (%d/%d) (%d%%/100%%)\r",
          byte_index + 16, data_len, (100 * (byte_index + 16) / data_len));
  }
  printf("\t\t\t\t...Done!\n");
  return 0;
}

static int
program_flash(i2c_info_t cpld, uint8_t page,
              uint8_t *data, int data_len) {

  uint8_t reset_addr_cmd[4] = {page, 0x00, 0x00, 0x00};
  uint8_t write_page_cmd[4] = {0x70, 0x00, 0x00, 0x01};
  uint8_t program_page_cmd[32] = {0};
  int i = 0, byte_index = 0;
  int ret = -1;

  /* Reset Page Address */
  ret = i2c_rdwr_msg_transfer(cpld.fd, cpld.addr << 1, reset_addr_cmd,
                              sizeof(reset_addr_cmd), NULL, 0);
  if (ret != 0) {
    ERR_PRINT("program_flash(): Reset Page Address");
    return ret;
  }
  if (read_busy_flag(cpld) != 0) {
    CPLD_BEBUG("Device busy is caused by address reset.\n");
    return -1;
  }

  if (page == CFG_PAGE) {
    printf("- Program CFG Page -\n");
  }
  else if (page == UFM_PAGE) {
    printf("- Program UFM Page -\n");
  }

  memcpy(&program_page_cmd[0], write_page_cmd, 4);
  for (byte_index = 0; byte_index < data_len; byte_index += 16) {
    memcpy(&program_page_cmd[4], &data[byte_index], 16);

    CPLD_BEBUG("\n");
    for (i = 0; i < 20; i++) {
      CPLD_BEBUG("0x%2x ", program_page_cmd[i]);
    }
    CPLD_BEBUG("\n");

    /* Program Page Data */
    ret = i2c_rdwr_msg_transfer(cpld.fd, cpld.addr << 1, program_page_cmd,
                                sizeof(program_page_cmd), NULL, 0);
    if (ret != 0) {
      ERR_PRINT("program_flash(): Program Page Data");
      return ret;
    }
    if (read_busy_flag(cpld) != 0) {
      CPLD_BEBUG("Device busy is caused by page data program.\n");
      return -1;
    }

    printf("  (%d/%d) (%d%%/100%%)\r",
          byte_index + 16, data_len, (100 * (byte_index + 16) / data_len));
    usleep(200);
  }
  printf("\t\t\t\t...Done!\n");
  return 0;
}

static int
program_done(i2c_info_t cpld) {

  uint8_t program_done_cmd[4] = {0x5E, 0x00, 0x00, 0x00};
  int ret = -1;

  printf("Program Done\n");
  ret = i2c_rdwr_msg_transfer(cpld.fd, cpld.addr << 1, program_done_cmd,
                              sizeof(program_done_cmd), NULL, 0);
  if (ret != 0) {
    ERR_PRINT("program_done()");
    return ret;
  }
  if (read_busy_flag(cpld) != 0) {
    CPLD_BEBUG("Device busy is caused by program done.\n");
    return -1;
  }
  return 0;
}

int
main(int argc, const char *argv[]) {

  int cfg_len = 0, ufm_len = 0, pid_file = 0;
  i2c_info_t cpld;
  u_int8_t *cfg_data;
  u_int8_t *ufm_data;

  if (argc != 4) {
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  pid_file = open("/var/run/cpldupdate-i2c.pid", O_CREAT | O_RDWR, 0666);
  if(flock(pid_file, LOCK_EX | LOCK_NB) && (errno == EWOULDBLOCK)) {
    printf("Another cpldupdate-i2c instance is running...\n");
    exit(EXIT_FAILURE);
  }

  cpld.bus = (uint8_t)strtoul(argv[1], NULL, 0);
  cpld.addr = (uint8_t)strtoul(argv[2], NULL, 0);

  cpld.fd = i2c_open(cpld.bus, cpld.addr);
  if (cpld.fd < 0) {
    return cpld.fd;
  }

  get_data_len(argv[3], &cfg_len, &ufm_len);
  if (cfg_len <= 0 || ufm_len <= 0) {
    close(cpld.fd);
    return -1;
  }

  cfg_data = (uint8_t *) malloc(cfg_len);
  if (!cfg_data) {
    ERR_PRINT("CFG Data");
    close(cpld.fd);
    return ENOMEM;
  }

  ufm_data = (uint8_t *) malloc(ufm_len);
  if (!ufm_data) {
    ERR_PRINT("UFM Data");
    close(cpld.fd);
    return ENOMEM;
  }

  get_img_data(argv[3], cfg_data, cfg_len, ufm_data, ufm_len);
  read_device_id(cpld);
  enable_program_mode(cpld, TRANSPARENT_MODE);
  erase_flash(cpld, 0x0C);

  if (program_flash(cpld, CFG_PAGE, cfg_data, cfg_len) != 0 ||
      program_flash(cpld, UFM_PAGE, ufm_data, ufm_len) != 0) {
    close(cpld.fd);
    free(cfg_data);
    free(ufm_data);
    return -1;
  }
  if (verify_flash(cpld, CFG_PAGE, cfg_data, cfg_len) != 0 ||
      verify_flash(cpld, UFM_PAGE, ufm_data, ufm_len) != 0) {
    close(cpld.fd);
    free(cfg_data);
    free(ufm_data);
    return -1;
  }

  program_done(cpld);
  read_status(cpld);
  refresh(cpld);
  close(cpld.fd);
  free(cfg_data);
  free(ufm_data);

  return 0;
}
