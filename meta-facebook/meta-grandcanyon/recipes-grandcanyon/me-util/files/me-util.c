/*
 * me-util
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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>

#define MAX_CMD_LENGTH             64
#define MAX_ME_READ_BUFFER         1024
#define COUNT_ARGC_UTIL_FRU        2

static void
print_usage_help(void) {
  printf("Usage: me-util server <[0..n]data_bytes_to_send>\n");
  printf("Usage: me-util server <cmd>\n");
  printf("  cmd list:\n");
  printf("    --get_dev_id      Get device ID\n");
  printf("    --file <file>     Input commands from file\n");
}

static int
me_get_dev_id() {
  int ret = 0;
  ipmi_req_t_common_header req = {0};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t rlen = 0;
  me_get_dev_id_res *res = (me_get_dev_id_res *)rbuf;

  req.netfn_lun = IPMI_NETFN_SHIFT(NETFN_APP_REQ);
  req.cmd = CMD_APP_GET_DEVICE_ID;

  ret = bic_me_xmit((uint8_t *)(&req), sizeof(ipmi_req_t_common_header), (uint8_t *)res, &rlen);

  if (ret < 0 ) {
    printf("ME transmission failed\n");
    return ret;
  }

  if (res->cc != CC_SUCCESS) {
    printf("Fail to get device id: Completion Code: %02X\n", res->cc);
    return -1;
  }

  if (rlen != sizeof(me_get_dev_id_res)) {
    printf("Invalid response length of get device id: %d, expected: %d\n", rlen, sizeof(me_get_dev_id_res));
    return -1;
  }

  printf("Device ID:                 %02x\n", res->ipmi_dev_id.dev_id);
  printf("Device Revision:           %02x\n", res->ipmi_dev_id.dev_rev);
  printf("Firmware Revision:         %02x %02x\n", res->ipmi_dev_id.fw_rev1, res->ipmi_dev_id.fw_rev2);
  printf("IPMI Version:              %02x\n", res->ipmi_dev_id.ipmi_ver);
  printf("Additional Device Support: %02x\n", res->ipmi_dev_id.dev_support);
  printf("Manufacturer ID:           %02x %02x %02x\n", res->ipmi_dev_id.mfg_id[0], res->ipmi_dev_id.mfg_id[1], res->ipmi_dev_id.mfg_id[2]);
  printf("Product ID:                %02x %02x\n", res->ipmi_dev_id.prod_id[0], res->ipmi_dev_id.prod_id[1]);
  printf("Aux Firmware Revision:     %02x %02x %02x %02x\n", res->ipmi_dev_id.aux_fw_rev[0], res->ipmi_dev_id.aux_fw_rev[1],
                                                             res->ipmi_dev_id.aux_fw_rev[2], res->ipmi_dev_id.aux_fw_rev[3]);
  return ret;
}

static int
process_command(int cmd_length, char **cmd_buf) {
  int i = 0, ret = 0;
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0};
  uint8_t rbuf[MAX_IPMB_RES_LEN + 2] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  me_xmit_res *res = (me_xmit_res *)rbuf;

  if (cmd_length > MAX_IPMB_REQ_LEN) {
    printf("Command length(%d) is out of IPMI Spec.(%d)\n", cmd_length, MAX_IPMB_REQ_LEN);
    return -1;
  }

  if (cmd_buf == NULL) {
    printf("Command buffer is NULL, abort\n");
    return -1;
  }

  for (i = 0; i < cmd_length; i++) {
    tbuf[tlen++] = (uint8_t)strtoul(cmd_buf[i], NULL, 0);
  }

  ret = bic_me_xmit(tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    printf("ME transmission failed\n");
    return ret;
  }

  if (res->cc != CC_SUCCESS) {
    printf("Completion Code: %02X \n", res->cc);
    return -1;
  }

  if (rlen == 0) {
    return ret;
  }

  if (rlen > MAX_IPMB_RES_LEN) {
    printf("Response data length(%u) is out of IPMI Spec. (%d)\n", rlen, MAX_IPMB_RES_LEN);
    return -1;
  }

  for (i = 0; i < (rlen - 1); i++) {
    printf("%02X ", res->data[i]);
  }
  printf("\n");

  return ret;
}

static int
process_file(char *path) {
  FILE *fp = NULL;
  int cmd_length = 0;
  char buf[MAX_ME_READ_BUFFER] = {0};
  char *str = NULL, *next = NULL, *del=" \n";
  char *cmd_buf[MAX_CMD_LENGTH] = {NULL};

  if (path == NULL) {
    printf("%s Invalid parameter: path is null\n", __func__);
    return -1;
  }

  if ((fp = fopen(path, "r")) == NULL) {
    syslog(LOG_WARNING, "Failed to open %s", path);
    return -1;
  }

  while (fgets(buf, sizeof(buf), fp) != NULL) {
    str = strtok_r(buf, del, &next);
    for (cmd_length = 0; cmd_length < MAX_CMD_LENGTH && str; cmd_length++, str = strtok_r(NULL, del, &next)) {
      if (str[0] == '#') {
        break;
      }
      if (cmd_length == 0 && strcmp(str, "echo") == 0) {
        if ((*next) != 0) {
          printf("%s", next);
        } else {
          printf("\n");
        }
        break;
      }
      cmd_buf[cmd_length] = str;
    }
    if (cmd_length < 1) {
      continue;
    }
    process_command(cmd_length, cmd_buf);
  }
  fclose(fp);

  return 0;
}

static int
do_cmds(char *cmd) {
  int ret = -1;

  if (cmd == NULL) {
    printf("Invalid parameter: cmd is null\n");
    return ret;
  }

  if ((strcmp(cmd, "--get_dev_id") == 0)) {
    return me_get_dev_id();
  } else {
    print_usage_help();
  } 

  return ret;
}

int
main(int argc, char **argv) {
  uint8_t present_status = 0;
  int cmd_length = 0;

  if (argc < 3) {
    goto err_exit;
  }

  if (strcmp(argv[1], "server") != 0) {
    goto err_exit;
  }

  if (pal_is_fru_prsnt(FRU_SERVER, &present_status) < 0) {
    printf("Fail to get %s present status\n", argv[1]);
  }

  if (present_status == FRU_ABSENT) {
    printf("%s is not present!\n", argv[1]);
    return -1;
  }
  
  if ((strcmp(argv[2], "--file") == 0)) {
    if (argc < 4) {
      goto err_exit;
    }
    return process_file(argv[3]);
  } else if ((start_with(argv[2], "--") == true)) {
    return do_cmds(argv[2]);
  } else {
    if (argc < 4) {
      goto err_exit;
    }
    cmd_length = argc - COUNT_ARGC_UTIL_FRU;

    return process_command(cmd_length, (argv + COUNT_ARGC_UTIL_FRU));
  }

err_exit:
  print_usage_help();
  return -1;
}
