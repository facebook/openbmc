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
#include <getopt.h>
#include <syslog.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <facebook/fby35_common.h>
#include <facebook/fby35_fruid.h>
#include <facebook/bic.h>
#include <openbmc/pal.h>

#define LAST_RECORD_ID 0xFFFF
#define BYTES_ENTIRE_RECORD 0xFF

#define MAX_RETRY 6           // 180 secs = 1500 * 6
#define MAX_RETRY_CNT 1500    // A senond can run about 50 times, 30 secs = 30 * 50

#define FRUID_RISER 1

int
remote_fruid_cache_init(uint8_t slot_id, uint8_t fru_id, uint8_t intf) {
  int ret=0;
  int fru_size=0;
  char fruid_temp_path[64] = {0};
  char fruid_path[64] = {0};

  sprintf(fruid_temp_path, "/tmp/tfruid_slot%d.%d.bin", slot_id, intf);
  if (intf == FEXP_BIC_INTF) {
    sprintf(fruid_path, "/tmp/fruid_slot%d_dev%d.bin", slot_id, BOARD_1OU);
  } else if (intf == REXP_BIC_INTF) {
    sprintf(fruid_path, "/tmp/fruid_slot%d_dev%d.bin", slot_id, BOARD_2OU);
  } else if (intf == EXP3_BIC_INTF) {
    sprintf(fruid_path, "/tmp/fruid_slot%d_dev%d.bin", slot_id, BOARD_3OU);
  } else if (intf == EXP4_BIC_INTF) {
    sprintf(fruid_path, "/tmp/fruid_slot%d_dev%d.bin", slot_id, BOARD_4OU);
  } else {
    sprintf(fruid_path, "/tmp/fruid_slot%d.%d.bin", slot_id, intf);
  }

  ret = bic_read_fruid(slot_id, fru_id, fruid_temp_path, &fru_size, intf);
  if (ret) {
    syslog(LOG_WARNING, "%s(): slot%d returns %d, fru_size: %d\n", __func__, slot_id, ret, fru_size);
  }

  if (intf == BB_BIC_INTF) {
    sprintf(fruid_path, FRU_BB_BIN);
  }

  if (intf == NONE_INTF && fru_id == FRUID_RISER) {
    sprintf(fruid_path, "/tmp/fruid_slot%d_dev%d.bin", slot_id, BOARD_2OU_X16);
  }

  rename(fruid_temp_path, fruid_path);
  return ret;
}

int
fruid_cache_init(uint8_t slot_id) {
  // Initialize Slot0's fruid
  int ret=0, present = 0, remote_f_ret = 0, remote_r_ret = 0;
  int fru_size=0;
  char fruid_temp_path[64] = {0};
  char fruid_path[64] = {0};
  uint8_t bmc_location = 0;
  uint8_t type_1ou = UNKNOWN_BOARD;
  uint8_t type_2ou = UNKNOWN_BOARD;

  sprintf(fruid_temp_path, "/tmp/tfruid_slot%d.bin", slot_id);
  sprintf(fruid_path, "/tmp/fruid_slot%d.bin", slot_id);

  ret = bic_read_fruid(slot_id, 0, fruid_temp_path, &fru_size, NONE_INTF);
  if (ret) {
    syslog(LOG_WARNING, "fruid_cache_init: bic_read_fruid slot%d returns %d, fru_size: %d\n", slot_id, ret, fru_size);
  }

  rename(fruid_temp_path, fruid_path);

  // Get remote FRU
  present = bic_is_exp_prsnt(slot_id);
  if (present < 0) {
    syslog(LOG_WARNING, "%s: Couldn't get the status of 1OU/2OU", __func__);
    return present;
  }
  fby35_common_get_bmc_location(&bmc_location);
  if (bmc_location == NIC_BMC) { //NIC BMC
    remote_f_ret = remote_fruid_cache_init(slot_id, 0, BB_BIC_INTF);
    if (PRESENT_2OU == (PRESENT_2OU & present)) {
      // dump 2ou board fru
      remote_r_ret = remote_fruid_cache_init(slot_id, 0, REXP_BIC_INTF);
    }
    return (remote_f_ret + remote_r_ret);
  } else { // Baseboard BMC
    // PROT FRU
    if( fby35_common_is_prot_card_prsnt(slot_id) ) {
      char path[128] = {0};
      int path_len = sizeof(path);
      char dev_path[MAX_FRU_PATH_LEN] = {0};
      snprintf(path, path_len, EEPROM_PATH, FRU_DEVICE_BUS(slot_id), PROT_FRU_ADDR);
      snprintf(dev_path, sizeof(dev_path), FRU_DEV_PATH, slot_id, BOARD_PROT);
      if ( copy_eeprom_to_bin(path, dev_path) < 0 ) {
        syslog(LOG_WARNING, "%s() Failed to get PROT FRU\n", __func__);
      }
    }

    if (PRESENT_1OU == (PRESENT_1OU & present)) {
      // dump 1ou board fru
      ret = bic_get_1ou_type(slot_id, &type_1ou);
      if (ret == 0) {
        switch (type_1ou) {
          case TYPE_1OU_RAINBOW_FALLS:
          case TYPE_1OU_VERNAL_FALLS_WITH_AST:
          case TYPE_1OU_NIAGARA_FALLS:
            remote_f_ret = remote_fruid_cache_init(slot_id, FRUID_0, FEXP_BIC_INTF);
            break;
          case TYPE_1OU_OLMSTEAD_POINT:
            if (slot_id != FRU_SLOT1) {
              syslog(LOG_WARNING, "%s() slot %x is not expected on Type 8 system\n", __func__, slot_id);
              return -1;
            }
            remote_f_ret += remote_fruid_cache_init(slot_id, FRUID_0, FEXP_BIC_INTF);
            remote_f_ret += remote_fruid_cache_init(slot_id, FRUID_0, REXP_BIC_INTF);
            remote_f_ret += remote_fruid_cache_init(slot_id, FRUID_0, EXP3_BIC_INTF);
            remote_f_ret += remote_fruid_cache_init(slot_id, FRUID_0, EXP4_BIC_INTF);
            break;
          default:
            remote_f_ret = 0;
        }
      }
      else {
        syslog(LOG_WARNING, "%s() Failed to get 1OU board type\n", __func__);
      }
    }
    if (PRESENT_2OU == (PRESENT_2OU & present)) {
      if ( fby35_common_get_2ou_board_type(slot_id, &type_2ou) < 0 ) {
        syslog(LOG_WARNING, "%s() Failed to get 2OU board type\n", __func__);
      }
      if ((type_2ou & DPV2_X16_BOARD) == DPV2_X16_BOARD) {
        // read riser board fru form sb_bic, fruid = 1
        remote_r_ret = remote_fruid_cache_init(slot_id, FRUID_RISER, NONE_INTF);
      } else {
        if ((type_2ou & DPV2_X8_BOARD) != DPV2_X8_BOARD) {
          remote_r_ret = remote_fruid_cache_init(slot_id, 0, REXP_BIC_INTF);
        }
      }
    }
    return (remote_f_ret + remote_r_ret);
  }

  return ret;
}

int
remote_sdr_cache_init(uint8_t slot_id, uint8_t intf) {
  int ret = 0, rc;
  int fd;
  int retry = 0;
  uint8_t rlen;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  char *path = NULL;
  char sdr_temp_path[64] = {0};
  char sdr_path[64] = {0};
  ssize_t bytes_wr;

  sprintf(sdr_temp_path, "/tmp/tsdr_slot%d.%d.bin", slot_id, intf);
  sprintf(sdr_path, "/tmp/sdr_slot%d.%d.bin", slot_id, intf);

  ipmi_sel_sdr_req_t req;
  ipmi_sel_sdr_res_t *res = (ipmi_sel_sdr_res_t *) rbuf;

  req.rsv_id = 0;
  req.rec_id = 0;
  req.offset = 0;
  req.nbytes = BYTES_ENTIRE_RECORD;

  // Read Slot0's SDR records and store
  path = sdr_temp_path;
  unlink(path);
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s: open fails for path: %s\n", __func__, path);
    ret = -2;
    return ret;
  }

  rc = pal_flock_retry(fd);
  if (rc == -1) {
   syslog(LOG_WARNING, "%s: failed to flock on %s", __func__, path);
   close(fd);
   return rc;
  }

  retry = 0;
  while (retry < MAX_RETRY) {
    ret = bic_get_sdr(slot_id, &req, res, &rlen, intf);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s: rsv_id: 0x%x, record_id: 0x%x, sdr_size: %d, intf: %x, retry: %d\n", __func__, req.rsv_id, req.rec_id, rlen, intf, retry);
      retry++;
      continue;
    }

    sdr_full_t *sdr = (sdr_full_t *)res->data;

    bytes_wr = write(fd, sdr, sizeof(sdr_full_t));
    if (bytes_wr != sizeof(sdr_full_t)) {
      syslog(LOG_ERR, "%s: write sdr failed\n", __func__);
      return -1;
    }

    req.rec_id = res->next_rec_id;
    if (req.rec_id == LAST_RECORD_ID) {
      // syslog(LOG_INFO, "This record is LAST record\n");
      break;
    }
  }

  if (retry == MAX_RETRY) {   // if exceed 30 secs, exit this step
    syslog(LOG_WARNING, "Fail on getting the sdr of slot%u via BIC. intf:%x", slot_id, intf);
  }

  rc = pal_unflock_retry(fd);
  if (rc == -1) {
   syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
   close(fd);
   return rc;
  }

  close(fd);
  rename(sdr_temp_path, sdr_path);

  sprintf(sdr_temp_path, "/tmp/sdr_slot%d.bin", slot_id);

  FILE *fp1 = fopen(sdr_temp_path, "a");
  FILE *fp2 = fopen(sdr_path, "rb");
  int c;
  int count = 0, filesize = 0;

  if (fp1 == NULL || fp2 == NULL) {
    return ret;
  }

  fseek(fp2, 0L, SEEK_END);
  filesize = ftell(fp2);
  rewind(fp2);

  while ((c = fgetc(fp2)) != EOF && filesize != count) {
    fputc(c, fp1);
    count++;
  }

  fclose(fp1);
  fclose(fp2);

  return ret;

}

int
sdr_cache_init(uint8_t slot_id) {
  int ret = 0, remote_f_ret = 0, remote_r_ret = 0, present = 0, rc;
  int fd;
  int retry = 0;
  uint8_t rlen;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  char *path = NULL;
  char sdr_temp_path[64] = {0};
  char sdr_path[64] = {0};
  ssize_t bytes_wr;
  uint8_t bmc_location = 0;
  uint8_t type_1ou = UNKNOWN_BOARD;
  uint8_t type_2ou = UNKNOWN_BOARD;

  sprintf(sdr_temp_path, "/tmp/tsdr_slot%d.bin", slot_id);
  sprintf(sdr_path, "/tmp/sdr_slot%d.bin", slot_id);

  ipmi_sel_sdr_req_t req;
  ipmi_sel_sdr_res_t *res = (ipmi_sel_sdr_res_t *) rbuf;

  req.rsv_id = 0;
  req.rec_id = 0;
  req.offset = 0;
  req.nbytes = BYTES_ENTIRE_RECORD;

  // Read Slot0's SDR records and store
  path = sdr_temp_path;
  unlink(path);
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s: open fails for path: %s\n", __func__, path);
    ret = -2;
    return ret;
  }

  rc = pal_flock_retry(fd);
  if (rc == -1) {
   syslog(LOG_WARNING, "%s: failed to flock on %s", __func__, path);
   close(fd);
   return rc;
  }

  retry = 0;
  while (retry < MAX_RETRY) {
    ret = bic_get_sdr(slot_id, &req, res, &rlen, NONE_INTF);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s: rsv_id: 0x%x, record_id: 0x%x, sdr_size: %d, retry: %d\n", __func__, req.rsv_id, req.rec_id, rlen, retry);
      retry++;
      continue;
    }

    sdr_full_t *sdr = (sdr_full_t *)res->data;

    bytes_wr = write(fd, sdr, sizeof(sdr_full_t));
    if (bytes_wr != sizeof(sdr_full_t)) {
      syslog(LOG_ERR, "%s: write sdr failed\n", __func__);
      return -1;
    }

    req.rec_id = res->next_rec_id;
    if (req.rec_id == LAST_RECORD_ID) {
      // syslog(LOG_INFO, "This record is LAST record\n");
      break;
    }
  }

  if (retry == MAX_RETRY) {   // if exceed 30 secs, exit this step
    syslog(LOG_WARNING, "Fail on getting the sdr of slot%u via BIC", slot_id);
  }

  rc = pal_unflock_retry(fd);
  if (rc == -1) {
   syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
   close(fd);
   return rc;
  }

  close(fd);
  rename(sdr_temp_path, sdr_path);

  // Get remote SDR
  present = bic_is_exp_prsnt(slot_id);
  if (present < 0) {
    syslog(LOG_WARNING, "%s: Couldn't get the status of 1OU/2OU", __func__);
    return present;
  }
  fby35_common_get_bmc_location(&bmc_location);
  if (bmc_location == NIC_BMC) {
    remote_f_ret = remote_sdr_cache_init(slot_id, BB_BIC_INTF);
    if (present == 2 || present == 3) {
      remote_r_ret = remote_sdr_cache_init(slot_id, REXP_BIC_INTF);
    }
  } else {
    if (PRESENT_1OU == (PRESENT_1OU & present)) {
      ret = bic_get_1ou_type(slot_id, &type_1ou);
      if (ret == 0) {
        switch (type_1ou) {
          case TYPE_1OU_RAINBOW_FALLS:
          case TYPE_1OU_VERNAL_FALLS_WITH_AST:
          case TYPE_1OU_NIAGARA_FALLS:
            remote_f_ret = remote_sdr_cache_init(slot_id, FEXP_BIC_INTF);
            break;
          case TYPE_1OU_OLMSTEAD_POINT:
            if (slot_id != FRU_SLOT1) {
              syslog(LOG_WARNING, "%s() slot %x is not expected on Type 8 system\n", __func__, slot_id);
              return -1;
            }
            remote_f_ret += remote_sdr_cache_init(slot_id, FEXP_BIC_INTF);
            remote_f_ret += remote_sdr_cache_init(slot_id, REXP_BIC_INTF);
            remote_f_ret += remote_sdr_cache_init(slot_id, EXP3_BIC_INTF);
            remote_f_ret += remote_sdr_cache_init(slot_id, EXP4_BIC_INTF);
            break;
          default:
            remote_f_ret = 0;
        }
      }
      else {
        syslog(LOG_WARNING, "%s() Failed to get 1OU board type\n", __func__);
      }
    }
    if (PRESENT_2OU == (PRESENT_2OU & present)) {
      if ( fby35_common_get_2ou_board_type(slot_id, &type_2ou) < 0 ) {
          syslog(LOG_WARNING, "%s() Failed to get 2OU board type\n", __func__);
      }
      if (((type_2ou & DPV2_X8_BOARD) != DPV2_X8_BOARD) &&
          ((type_2ou & DPV2_X16_BOARD) != DPV2_X16_BOARD)) {
        remote_r_ret = remote_sdr_cache_init(slot_id, REXP_BIC_INTF);
      }
    }
  }

  if (remote_f_ret != 0 || remote_r_ret != 0) {
    syslog(LOG_WARNING, "Failed to get the remote sdr of slot%u. remote_f_ret: %d, remote_r_ret:%d, %d", slot_id, remote_f_ret, remote_r_ret, (remote_f_ret+remote_r_ret));
    return (remote_f_ret + remote_r_ret);
  }

  return ret;
}

int
parse_args(int argc, char * const argv[], bool *do_fru, bool *do_sdr, bool *do_remote, uint8_t *intf) {
  int opt;
  int optind_long;
  uint8_t fru = 0, sdr = 0;
  static const char *optstring = "fsr:";
  static const struct option long_options[] = {
    {"fruid", no_argument, 0, 'f'},
    {"sdr", no_argument, 0, 's'},
    {"remote sdr", required_argument/*no_argument*/, 0, 'r'},
    {0, 0, 0, 0}
  };

  while ((opt = getopt_long(argc, argv, optstring, long_options, &optind_long)) != -1) {
    switch (opt) {
      case 'f':
        fru = 1;
        break;
      case 's':
        sdr = 1;
        break;
      case 'r':
        *intf = (uint8_t)strtol(optarg, NULL, 16);
        *do_remote = true;
        *do_sdr = true;
        break;
      default:
        return -1;
    }
  }

  if (argc < optind + 1) {
    return -1;
  }

  if (fru ^ sdr) {
    if (fru) {
      *do_fru = true;
      *do_sdr = false;
    } else {
      *do_fru = false;
      *do_sdr = true;
    }
  } else {
    *do_fru = true;
    *do_sdr = true;
  }

  return optind;
}

int
main (int argc, char * const argv[])
{
  int ret = 0;
  int pid_file = 0;
  uint8_t intf = 0;
  uint8_t slot_id = 0;
  uint8_t self_test_result[2] = {0};
  int retry = 0;
  int max_retry = MAX_RETRY; // need more time for accessing GPv3 FRU after sled cycle
  char path[128];
  bool fruid_dump = true, sdr_dump = true, remote_dump = false;
  int index = 0;

  index = parse_args(argc, argv, &fruid_dump, &sdr_dump, &remote_dump, &intf);
  if ((index < 0) || (index >= argc)) {
    printf("Usage: bic-cached [Option] <SLOT_NUM>\n");
    printf("\n");
    printf("           Option:\n");
    printf("                 -f update FRU cache\n");
    printf("                 -s update SDR cache\n");
    printf("                 -r update remote SDR cache\n");
    return -1;
  }

  //get the slot_id
  ret = fby35_common_get_slot_id(argv[index], &slot_id);
  if ( ret <  0 ) {
    syslog(LOG_WARNING, "%s() slot is incorrect %s\n", __func__, argv[index]);
  }

  sprintf(path, BIC_CACHED_PID, slot_id);
  pid_file = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (pid_file < 0) {
    syslog(LOG_WARNING, "%s: fails to open path: %s\n", __func__, path);
  }

  ret = pal_flock_retry(pid_file);
  if (ret) {
   syslog(LOG_WARNING, "%s: failed to flock on %s", __func__, path);
  }

  // Check BIC Self Test Result
  do {
    ret = bic_get_self_test_result(slot_id, (uint8_t *)&self_test_result, NONE_INTF);
    if (ret == 0) {
      syslog(LOG_INFO, "bic_get_self_test_result of slot%u: %X %X", slot_id, self_test_result[0], self_test_result[1]);
      break;
    }
    sleep(5);
  } while (ret != 0);

  // Get the sdr of slot
  if ( sdr_dump == true ) {
    if ( remote_dump == true ) {
      ret = remote_sdr_cache_init(slot_id, intf);
    } else {
      ret = sdr_cache_init(slot_id);
    }

    if ( ret < 0 ) {
      syslog(LOG_CRIT, "Fail on getting slot%u SDR", slot_id);
    }
  }

  // Get Server FRU
  if (fruid_dump == true) {
    retry = 0;
    do {
      ret = fruid_cache_init(slot_id);
      if (ret == 0)
        break;

      retry++;
      sleep(10);
    } while (retry < max_retry);

    if (ret != 0) {
      syslog(LOG_CRIT, "Fail on getting slot%u FRU", slot_id);
    }
  }

  ret = pal_unflock_retry(pid_file);
  if (ret == -1) {
   syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
  }

  close(pid_file);
  remove(path);
  return 0;
}
