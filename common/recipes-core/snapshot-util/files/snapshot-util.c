/*
 * snapshot-util
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
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <openbmc/pal.h>

#define MAX_FRU_NAME_LEN  64

#define MAX_REC_NUM 4
#define MAX_RI_NUM  3
#define MAX_MFI_NUM 1
#define BLOCK_SIZE  16

#define TYPE_RMA 0
#define TYPE_MFG 1

#define IH_SIZE        0x20  // Info Header
#define RIH_MAGIC_TAG  "/@RMA_ss"
#define MFIH_MAGIC_TAG "/@MFG_ss"

#define MAX_REASON_DESC 1024
#define DEFAULT_REASON_FILE_NAME "/tmp/snapshot-reason-dft"

// extra pw to prevent accidental clear of RMA data
#define CLEAR_PW      "571932"

#define TMP_SS_PATH   "/tmp/_snapshot"
#define TMP_LOG_FILE  TMP_SS_PATH"/log.txt"
#define TMP_POST_FILE TMP_SS_PATH"/postcode.txt"
#define TMP_TARBALL   TMP_SS_PATH"/ss.tgz"

#define OEM_REC_TYPE 0xFA

#ifndef EEPROM_BUS
#define EEPROM_BUS  3     // i2c_1
#endif

#ifndef EEPROM_ADDR
#define EEPROM_ADDR 0xA2  // 8-bit address
#endif

#define log_system(cmd)                                                     \
do {                                                                        \
  int sysret = system(cmd);                                                 \
  if (sysret)                                                               \
    syslog(LOG_WARNING, "%s: system command failed, cmd: \"%s\",  ret: %d", \
            __func__, cmd, sysret);                                         \
} while(0)

typedef struct _info_hdr {
  uint8_t magic_tag[8];
  uint16_t version;
  uint16_t checksum;
  int size;
} info_hdr;

typedef struct _info_rec {
  uint16_t offset;
  uint16_t size;
} info_rec;

static info_rec m_info_rec[MAX_REC_NUM] = {
  {0x0400, 0x0C00},
  {0x1000, 0x0C00},
  {0x1C00, 0x0C00},
  {0x2800, 0x0C00},
};


static void
print_usage_help(void) {
  uint8_t slot_num = 0;

  pal_get_num_slots(&slot_num);

  printf("Usage: snapshot-util [fru] --set <type> <reason_file>\n");
  printf("       snapshot-util [fru] --set <type> \"<inline reason string>\"\n");
  printf("       snapshot-util [fru] --get <type> <info_num> <dump_file>\n");
  printf("       snapshot-util [fru] --get --all <dump_file>\n");
  printf("       snapshot-util [fru] --clear <type> <info_num>\n\n");
  if (slot_num == 1) {
    printf("       [fru]: server\n");
  } else {
    printf("       [fru]: slot1, slot2, slot3, slot4\n");
  }
  printf("       <type>:\n");
  printf("         --rma  RMA information\n");
  printf("         --mfg  MFG failure information\n");
  printf("       <info_num>: 1 ~ 3 for RMA information; 1 for MFG failure information\n");
}

static int
check_info_rec(uint8_t slot_id) {
  int ret = 0;
  uint8_t buf[256], sum, cnt, idx, i;
  FILE *fp;
  info_rec rec[MAX_REC_NUM];

  ret = pal_get_fruid_path(slot_id, (char *)buf);

  if (ret < 0) {
    syslog(LOG_ERR, "%s: failed to get the fruid path of FRU: %d.", __func__, slot_id);
    return -1;
  }

  fp = fopen((char *)buf, "rb");
  if (fp == NULL) {
    syslog(LOG_ERR, "unable to get the %s fp %s", (char *)buf, strerror(errno));
    return errno;
  }

  if (fread(buf, 1, 8, fp) != 8) {
    syslog(LOG_ERR, "read common header failed %s", strerror(errno));
    fclose(fp);
    return errno;
  }

  sum = 0;
  for (i = 0; i < 8; i++) {
    sum += buf[i];
  }
  if (sum != 0x00) {
    syslog(LOG_ERR, "invalid common header");
    fclose(fp);
    return -1;
  }

  if (!buf[5]) {
    fclose(fp);
    return 0;
  }

  fseek(fp, buf[5]*8, SEEK_SET);
  for (idx = 0; idx < MAX_REC_NUM; idx++) {
    if (fread(buf, 1, 5, fp) != 5) {
      syslog(LOG_ERR, "read multi-record header failed %s", strerror(errno));
      break;
    }

    sum = 0;
    for (i = 0; i < 5; i++) {
      sum += buf[i];
    }
    if (sum != 0x00) {
      syslog(LOG_ERR, "invalid multi-record header");
      break;
    }

    if (buf[0] != OEM_REC_TYPE) {
      break;
    }

    cnt = buf[2];
    if (fread(&buf[5], 1, cnt, fp) != cnt) {
      syslog(LOG_ERR, "read multi-record failed %s", strerror(errno));
      break;
    }

    sum = buf[3];
    cnt += 5;
    for (i = 5; i < cnt; i++) {
      sum += buf[i];
    }
    if (sum != 0x00) {
      syslog(LOG_ERR, "invalid multi-record");
      break;
    }

    memcpy(&rec[idx], &buf[5], 4);
  }
  fclose(fp);

  if (idx >= MAX_REC_NUM) {
    for (i = 0; i < MAX_REC_NUM; i++) {
      m_info_rec[i].offset = rec[i].offset;
      m_info_rec[i].size = rec[i].size;
      syslog(LOG_INFO, "info_rec[%u]: offset=0x%04x, size=0x%04x", i, m_info_rec[i].offset, m_info_rec[i].size);
    }
  }

  return 0;
}

static int
is_ih_exist(uint8_t slot_id, uint8_t info_type, uint8_t idx, uint16_t *checksum, int *fsize) {
  uint8_t wbuf[32], rbuf[64];
  char *magic_tag;
  int ret, offset;
  info_hdr *ih;

  if (info_type == TYPE_MFG) {
    idx = MAX_RI_NUM;
    magic_tag = MFIH_MAGIC_TAG;
  } else {
    magic_tag = RIH_MAGIC_TAG;
  }

  offset = m_info_rec[idx].offset;
  wbuf[0] = (offset >> 8) & 0xFF;
  wbuf[1] = offset & 0xFF;
  ret = bic_master_write_read(slot_id, EEPROM_BUS, EEPROM_ADDR, wbuf, 2, rbuf, BLOCK_SIZE);
  if (ret != 0) {
    printf("read failed 0x%x, len = %d\n", offset, BLOCK_SIZE);
    return 0;
  }

  ih = (info_hdr *)rbuf;
  if (!memcmp(ih->magic_tag, magic_tag, 8) && (ih->version == 0x01)) {
    if (checksum) {
      memcpy(checksum, &ih->checksum, 2);
    }

    if (fsize) {
      memcpy(fsize, &ih->size, 4);
    }
    return 1;
  }

  return 0;
}

static int
util_create_default_reason(char *buf_data) {
  FILE *f = fopen(DEFAULT_REASON_FILE_NAME, "w");
  if (f == NULL) {
      printf("Error creating default reason file!\n");
      return -1;
  }

  // store at most MAX_REASON_DESC characters to file
  fprintf(f, "%.*s\n", MAX_REASON_DESC, buf_data);

  fclose(f);
  return 0;
}


static int
util_store_snapshot(uint8_t slot_id, uint8_t info_type, char *cmdline_opt) {
  uint8_t wbuf[64], rbuf[64], ih_buf[IH_SIZE], ih_offs, idx, max_idx;
  uint16_t sum;
  char cmd[256], *magic_tag;
  int ret, fsize, offset, len, i;
  info_hdr *ih = (info_hdr *)ih_buf;
  struct stat st = {0};
  FILE *fp;
  char *reason_file = cmdline_opt;
  char fru_name[MAX_FRU_NAME_LEN] = {0};

  memset(fru_name, 0, sizeof(fru_name));

  // check if user specified a file containing "reason string"
  if (stat(cmdline_opt, &st) != 0) {
    // file doesn't exist, treat it as stdin and create a file instead
    printf("Reason file doesn't exist, assume stdin\n");
    ret = util_create_default_reason(reason_file);
    if (!ret) {
      reason_file = DEFAULT_REASON_FILE_NAME;
    } else {
      return ret;
    }
  }

  if (st.st_size > MAX_REASON_DESC) {
    printf("%s is too large\n", reason_file);
    return -1;
  }

  max_idx = (info_type == TYPE_MFG) ? MAX_MFI_NUM : MAX_RI_NUM;
  for (idx = 0; idx < max_idx; idx++) {
    if (is_ih_exist(slot_id, info_type, idx, NULL, NULL) != 1)
      break;
  }
  if (idx >= max_idx) {
    printf("all Info areas are occupied\n");
    return -1;
  }

  snprintf(cmd, sizeof(cmd), "rm -rf %s", TMP_SS_PATH);
  log_system(cmd);
  mkdir(TMP_SS_PATH, 0755);

  snprintf(cmd, sizeof(cmd), "cp %s %s/", reason_file, TMP_SS_PATH);
  log_system(cmd);

  if (pal_get_fru_name(slot_id, fru_name) < 0) {
    syslog(LOG_ERR, "%s: failed to get fru name", __func__);
    return -1;
  }

  printf("Getting logs...\n");
  snprintf(cmd, sizeof(cmd), "/usr/local/bin/log-util all --print | /usr/bin/tail -n 50 > %s",
           TMP_LOG_FILE);
  log_system(cmd);

  printf("Getting POST codes...\n");
  snprintf(cmd, sizeof(cmd), "/usr/local/bin/bios-util %s --postcode get > %s", fru_name, TMP_POST_FILE);
  log_system(cmd);

  if (chdir(TMP_SS_PATH)) {
    syslog(LOG_WARNING, "chdir failed, dir path: %s", TMP_SS_PATH);
    return -1;
  }
  snprintf(cmd, sizeof(cmd), "tar czf %s ./*", TMP_TARBALL);
  log_system(cmd);

  fp = fopen(TMP_TARBALL, "rb");
  if (fp == NULL) {
    printf("unable to get the %s fp %s\n", TMP_TARBALL, strerror(errno));
    return errno;
  }

  if (info_type == TYPE_MFG) {
    idx = MAX_RI_NUM;
    magic_tag = MFIH_MAGIC_TAG;
  } else {
    magic_tag = RIH_MAGIC_TAG;
  }

  fseek(fp, 0L, SEEK_END);
  fsize = ftell(fp);
  printf("File Size: %d\n", fsize);
  if ((fsize + IH_SIZE) > m_info_rec[idx].size) {
    printf("file is too large\n");
    fclose(fp);
    return -1;
  }
  printf("Storing to EEPROM...\n");

  sum = 0;
  rewind(fp);
  offset = m_info_rec[idx].offset + IH_SIZE;
  while ((len = fread(&wbuf[2], 1, BLOCK_SIZE, fp)) > 0) {
    wbuf[0] = (offset >> 8) & 0xFF;
    wbuf[1] = offset & 0xFF;
    ret = bic_master_write_read(slot_id, EEPROM_BUS, EEPROM_ADDR, wbuf, 2+len, rbuf, 0);
    if (ret != 0) {
      printf("write failed 0x%x, len = %d\n", offset, len);
      fclose(fp);
      return ret;
    }

    for (i = 0; i < len; i++) {
      sum += wbuf[i+2];
    }

    offset += len;
    msleep(10);
  }
  fclose(fp);

  memset(ih_buf, 0x00, IH_SIZE);
  memcpy(ih->magic_tag, magic_tag, 8);
  ih->version = 0x01;
  ih->checksum = sum;
  ih->size = fsize;

  offset = m_info_rec[idx].offset;
  ih_offs = 0;
  fsize = IH_SIZE;
  while (fsize > 0) {
    if (ih_offs) {
      msleep(10);
    }

    wbuf[0] = (offset >> 8) & 0xFF;
    wbuf[1] = offset & 0xFF;
    len = (fsize > BLOCK_SIZE) ? BLOCK_SIZE : fsize;
    memcpy(&wbuf[2], &ih_buf[ih_offs], len);
    ret = bic_master_write_read(slot_id, EEPROM_BUS, EEPROM_ADDR, wbuf, 2+len, rbuf, 0);
    if (ret != 0) {
      printf("write failed 0x%x, len = %d\n", offset, len);
      return ret;
    }

    offset += len;
    ih_offs += len;
    fsize -= len;
  }

  return 0;
}

static int
util_dump_snapshot(uint8_t slot_id, uint8_t info_type, uint8_t idx, char *dump_file) {
  FILE *fp;
  uint8_t wbuf[32], rbuf[64];
  uint16_t checksum = 0, sum;
  int ret, fsize = 0, offset, len, i;

  if (is_ih_exist(slot_id, info_type, idx, &checksum, &fsize) != 1) {
    printf("no data to read\n");
    return -1;
  }

  printf("Filename: %s\n", dump_file);
  printf("File Size: %d\n", fsize);

  fp = fopen(dump_file, "wb");
  if (fp == NULL) {
    printf("unable to get the %s fp %s\n", dump_file, strerror(errno));
    return errno;
  }
  printf("Dumping from EEPROM...\n");

  sum = 0;
  offset = m_info_rec[idx].offset + IH_SIZE;
  while (fsize > 0) {
    len = (fsize > BLOCK_SIZE) ? BLOCK_SIZE : fsize;
    wbuf[0] = (offset >> 8) & 0xFF;
    wbuf[1] = offset & 0xFF;
    ret = bic_master_write_read(slot_id, EEPROM_BUS, EEPROM_ADDR, wbuf, 2, rbuf, len);
    if (ret != 0) {
      printf("read failed 0x%x, len = %d\n", offset, len);
      fclose(fp);
      return ret;
    }

    if ((ret = fwrite(rbuf, 1, len, fp)) != len) {
      printf("write file failed 0x%x, len = (%d / %d)\n", offset, ret, len);
      fclose(fp);
      return ret;
    }

    for (i = 0; i < len; i++) {
      sum += rbuf[i];
    }

    offset += len;
    fsize -= len;
  }
  fclose(fp);

  if (checksum != sum) {
    printf("mismatched checksum, 0x%04x / 0x%04x\n", checksum, sum);
    return -1;
  }

  return 0;
}



static int
util_dump_snapshot_all(uint8_t slot_id, char *dump_file) {
  int idx, ret;
  #define MAX_DUMP_FILE_PATH  255
  char dump_file_name[MAX_DUMP_FILE_PATH] = {0};

  memset(dump_file_name, 0, sizeof(dump_file_name));

  // dump all rma data into dump_file-rmaX
  for (idx = 0; idx < MAX_RI_NUM; ++idx) {
    snprintf(dump_file_name, sizeof(dump_file_name), "%s-rma%d", dump_file, idx+1);
    ret = util_dump_snapshot(slot_id, TYPE_RMA, idx, dump_file_name);
  }

  // dump all mfi data into dump_file-mfgX
  for (idx = 0; idx < MAX_MFI_NUM; ++idx) {
    snprintf(dump_file_name, sizeof(dump_file_name), "%s-mfg%d", dump_file, idx+1);
    ret = util_dump_snapshot(slot_id, TYPE_MFG, idx+MAX_RI_NUM, dump_file_name);
  }

  // in "dump all" case, there may be unexisted RMA or MFG data, or checksum
  // error in certain files. Code will continue and just return success for now
  (void) ret;
  return 0;
}

static int
util_clear_snapshot(uint8_t slot_id, uint8_t idx) {
  uint8_t wbuf[64], rbuf[64], ih_buf[IH_SIZE], ih_offs;
  int ret, fsize, offset, len;

  memset(ih_buf, 0xff, IH_SIZE);
  offset = m_info_rec[idx].offset;
  ih_offs = 0;
  fsize = IH_SIZE;
  while (fsize > 0) {
    if (ih_offs) {
      msleep(10);
    }

    wbuf[0] = (offset >> 8) & 0xFF;
    wbuf[1] = offset & 0xFF;
    len = (fsize > BLOCK_SIZE) ? BLOCK_SIZE : fsize;
    memcpy(&wbuf[2], &ih_buf[ih_offs], len);
    ret = bic_master_write_read(slot_id, EEPROM_BUS, EEPROM_ADDR, wbuf, 2+len, rbuf, 0);
    if (ret != 0) {
      printf("write failed 0x%x, len = %d\n", offset, len);
      return ret;
    }

    offset += len;
    ih_offs += len;
    fsize -= len;
  }

  return 0;
}

int
main(int argc, char **argv) {
  uint8_t slot_id, info_type = TYPE_RMA;
  int ret = 0, idx, max_idx = 0;

  if (argc < 5) {
    printf("Error: invalid number of arguments\n");
    goto err_exit;
  }

  ret = pal_get_fru_id(argv[1], &slot_id);

  if (ret < 0) {
    printf("Error: invalid FRU/slot number\n");
    goto err_exit;
  }

  if (!strcmp(argv[3], "--rma")) {
    info_type = TYPE_RMA;
    max_idx = MAX_RI_NUM;
  } else if (!strcmp(argv[3], "--mfg")) {
    info_type = TYPE_MFG;
    max_idx = MAX_MFI_NUM;
  } else if (!(!strcmp(argv[2], "--get") && !strcmp(argv[3], "--all"))) {
    printf("Error: invalid <type>\n");
    goto err_exit;
  }

  check_info_rec(slot_id);

  if (!strcmp(argv[2], "--set")) {
    // snapshot-util treats argv[4] as either "reason file" or "reason string"
    // and ignores everything after that.
    // A wrong command line can lead to  incorrect reason being stored
    //
    // Here we perform a basic check to ensure command line is valid, e.g
    // Good:
    //     snapshot-util slot1 --set --rma /tmp/reason.txt
    //     snapshot-util slot1 --set --rma "cpu malfunction"
    // Bad:
    //     snapshot-util slot1 --set --rma 3 "cpu malfunction"
    //     snapshot-util slot1 --set --rma 1  /tmp/reason.txt
    //     snapshot-util slot1 --set --rma cpu malfunction
    if (argc != 5) {
      printf("Error: invalid number of arguments for --set <type> <reason>\n");
      goto err_exit;
    }
    ret = util_store_snapshot(slot_id, info_type, argv[4]);
  } else if (!strcmp(argv[2], "--get")) {
    if (!strcmp(argv[3], "--all")) {
      ret = util_dump_snapshot_all(slot_id, argv[4]);
    } else {
      if (argc < 6) {
        printf("Error: invalid number of arguments for --get <type>\n");
        goto err_exit;
      }

      idx = atoi(argv[4]);
      if ((idx < 1) || (idx > max_idx)) {
        printf("Error: invalid <info_num>\n");
        goto err_exit;
      }

      idx = (info_type == TYPE_MFG) ? MAX_RI_NUM : (idx-1);
      ret = util_dump_snapshot(slot_id, info_type, idx, argv[5]);
    }
  } else if (!strcmp(argv[2], "--clear")) {
    idx = atoi(argv[4]);
    if ((idx < 1) || (idx > max_idx)) {
      printf("Error: invalid <info_num>\n");
      goto err_exit;
    }

    // check for clear pw
    if ((argc < 6) || (strcmp(argv[5], CLEAR_PW) != 0)) {
      printf("Error: incorrect --clear command\n");
      goto err_exit;
    }

    idx = (info_type == TYPE_MFG) ? MAX_RI_NUM : (idx-1);
    ret = util_clear_snapshot(slot_id, idx);
  } else {
    printf("Error: invalid command\n");
    goto err_exit;
  }

  printf("%s\n", (!ret) ? "Finished" : "Failed");
  return ret;

err_exit:
  print_usage_help();
  return -1;
}
