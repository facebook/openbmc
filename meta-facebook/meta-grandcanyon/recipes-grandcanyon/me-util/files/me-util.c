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

const uint32_t NM_INTEL_MANUFANCTURER_ID = 0x000157;
const uint32_t NM_POLICY_CORRECT_TIME_LIMIT = 0x000003e8;
const uint16_t NM_POLICY_TRIGGER_LIMIT = 0x0000;
const uint16_t NM_POLICY_STATISTICS_PERIOD = 0x0001;
#define NM_STATISTICS_MODE_GLOBAL_POWER       0x01
#define NM_STATISTICS_DOMAIN_ID               0x00
#define NM_STATISTICS_POLICY_ID               0x00
#define NM_POLICY_DOMAIN_ID                   0x00
#define NM_POLICY_POLICY_ID                   0x02
#define NM_POLICY_POLICY_TYPE                 0xb0
#define NM_POLICY_POLICY_EXCEPTION_ACTION     0x00
#define NM_POLICY_ENABLE_FLAG                 0x05

enum {
  ME_ENABLE_DISABLE_NODE_MANAGER_POLICY = 0xC0,
  ME_SET_NODE_MANAGER_POLICY = 0xC1,
  ME_GET_NODE_MANAGER_STATISTICS= 0xC8,
};

typedef struct {
  ipmi_req_t_common_header header;
  uint8_t intel_manufacturer_id[3];
  uint8_t mode;
  uint8_t domain_id;
  uint8_t plicy_id;
} me_get_nm_statistics_req;

typedef struct {
  uint8_t cc;
  uint8_t intel_manufacturer_id[3];
  uint8_t statistics_data[8];
  uint8_t timestamp[4];
  uint8_t statistics_period[4];
  uint8_t domain_id;
} me_get_nm_statistics_res;

typedef struct {
  ipmi_req_t_common_header header;
  uint8_t intel_manufacturer_id[3];
  uint8_t domain_id;
  uint8_t policy_id;
  uint8_t policy_type;
  uint8_t policy_exception_actions;
  uint8_t policy_target_limit[2];
  uint8_t correction_time_limit[4];
  uint8_t policy_trigger_limit[2];
  uint8_t statistics_period[2];
} me_set_nm_policy_req;

typedef struct {
  ipmi_req_t_common_header header;
  uint8_t intel_manufacturer_id[3];
  uint8_t policy_enable_disable_flag;
  uint8_t domain_id;
  uint8_t policy_id;
} me_enable_disable_nm_policy_req;

static void
print_usage_help(void) {
  printf("Usage: me-util server <[0..n]data_bytes_to_send>\n");
  printf("Usage: me-util server <cmd>\n");
  printf("  cmd list:\n");
  printf("    --get_dev_id                         Get device ID\n");
  printf("    --file <file>                        Input commands from file\n");
  printf("    --get_nm_power_statistics            Get node manager power statistics\n");
  printf("    --set_nm_power_policy <limit power>  Set node manager power policy\n");
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
me_get_nm_power_statistic() {
  int ret = 0;
  me_get_nm_statistics_req req;
  me_get_nm_statistics_res res;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tlen = 0, rlen = 0;
  uint32_t timestamp = 0;
  char time[MAX_VALUE_LEN] = {0};
  struct tm ts = {0};

  memset(&req, 0, sizeof(me_get_nm_statistics_req));
  req.header.netfn_lun = IPMI_NETFN_SHIFT(NETFN_NM_REQ);
  req.header.cmd = ME_GET_NODE_MANAGER_STATISTICS;
  memcpy(&req.intel_manufacturer_id, (uint8_t *) &NM_INTEL_MANUFANCTURER_ID, sizeof(req.intel_manufacturer_id));
  req.mode = NM_STATISTICS_MODE_GLOBAL_POWER;
  req.domain_id = NM_STATISTICS_DOMAIN_ID;
  req.plicy_id = NM_STATISTICS_POLICY_ID;
  tlen = sizeof(me_get_nm_statistics_req);

  ret = bic_me_xmit((uint8_t *)(&req), tlen, (uint8_t *)rbuf, &rlen);

  if (ret < 0) {
    printf("%s(): ME transmission failed\n", __func__);
    return ret;
  }

  if (rlen == sizeof(me_get_nm_statistics_res)) {
    memset(&res, 0, rlen);
    memcpy(&res, rbuf, rlen);
  } else {
    printf("Invalid response length of get node manager statistic: %d, expected: %zu\n", rlen, sizeof(me_get_nm_statistics_res));
    return -1;
  }

  if (res.cc != CC_SUCCESS) {
    printf("Fail to get node manager statistic: Completion Code: %02X\n", res.cc);
    return -1;
  }

  timestamp |= res.timestamp[0];
  timestamp |= res.timestamp[1] << 8;
  timestamp |= res.timestamp[2] << 16;
  timestamp |= res.timestamp[3] << 24;
  snprintf(time, sizeof(time), "%u", timestamp);
  memset(&ts, 0, sizeof(struct tm));
  strptime(time, "%s", &ts);
  memset(&time, 0, sizeof(time));
  strftime(time, sizeof(time), "%Y-%m-%d %H:%M:%S", &ts);

  printf("Manufacturer ID:                  %02X %02X %02X\n", res.intel_manufacturer_id[0], res.intel_manufacturer_id[1],
                                                               res.intel_manufacturer_id[2]);
  printf("Statistics Power data:\n");
  printf("        Current Value:            %3d Watts\n", (res.statistics_data[1] << 8) + res.statistics_data[0]);
  printf("        Minimum Value:            %3d Watts\n", (res.statistics_data[3] << 8) + res.statistics_data[2]);
  printf("        Maximum Value:            %3d Watts\n", (res.statistics_data[5] << 8) + res.statistics_data[4]);
  printf("        Average Value:            %3d Watts\n", (res.statistics_data[7] << 8) + res.statistics_data[6]);
  printf("Timestamp:                        %s\n", time);
  printf("Statistics Reporting Period:      %02X %02X %02X %02X\n", res.statistics_period[0], res.statistics_period[1],
                                                                    res.statistics_period[2], res.statistics_period[3]);
  printf("Domain ID | Policy State:         %02X\n", res.domain_id);

  return ret;
}

static int
me_set_nm_power_policy(char *limit_power) {
  me_set_nm_policy_req set_req;
  me_enable_disable_nm_policy_req enable_req;
  uint16_t power = 0;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tlen = 0, rlen = 0;
  int ret = 0;

  if (limit_power == NULL) {
    printf("Fail to set node manager policy because parameter *limit_power is NULL\n");
    return -1;
  }

  if ((atoi(limit_power) >= 0) && (atoi(limit_power) < 0xFFFF)) {
    power = atoi(limit_power);
  } else {
    printf("Fail to set node manager policy because limit: %s out of range 0~0xFFFF\n", limit_power);
    return -1;
  }

  // set node manager policy config
  memset(&set_req, 0, sizeof(me_set_nm_policy_req));
  set_req.header.netfn_lun = IPMI_NETFN_SHIFT(NETFN_NM_REQ);
  set_req.header.cmd = ME_SET_NODE_MANAGER_POLICY;
  memcpy(&set_req.intel_manufacturer_id, (uint8_t *) &NM_INTEL_MANUFANCTURER_ID, sizeof(set_req.intel_manufacturer_id));
  set_req.domain_id = NM_POLICY_DOMAIN_ID;
  set_req.policy_id = NM_POLICY_POLICY_ID;
  set_req.policy_type = NM_POLICY_POLICY_TYPE;
  set_req.policy_exception_actions = NM_POLICY_POLICY_EXCEPTION_ACTION;
  memcpy(&set_req.policy_target_limit, &power, sizeof(set_req.policy_target_limit));
  memcpy(&set_req.correction_time_limit, (uint8_t *) &NM_POLICY_CORRECT_TIME_LIMIT, sizeof(set_req.correction_time_limit));
  memcpy(&set_req.policy_trigger_limit, (uint8_t *) &NM_POLICY_TRIGGER_LIMIT, sizeof(set_req.policy_trigger_limit));
  memcpy(&set_req.statistics_period, (uint8_t *) &NM_POLICY_STATISTICS_PERIOD, sizeof(set_req.statistics_period));
  tlen = sizeof(me_set_nm_policy_req);

  ret = bic_me_xmit((uint8_t *)(&set_req), tlen, (uint8_t *)rbuf, &rlen);
  if (ret < 0) {
    printf("%s(): ME transmission failed cmd:%d\n", __func__, ME_SET_NODE_MANAGER_POLICY);
    return ret;
  }

  if (rbuf[0] != CC_SUCCESS) {
    printf("Fail to set node manager policy config: Completion Code: %02X\n", rbuf[0]);
    return -1;
  }

  // enable node manager policy
  memset(&enable_req, 0, sizeof(me_enable_disable_nm_policy_req));
  enable_req.header.netfn_lun = IPMI_NETFN_SHIFT(NETFN_NM_REQ);
  enable_req.header.cmd = ME_ENABLE_DISABLE_NODE_MANAGER_POLICY;
  memcpy(&enable_req.intel_manufacturer_id, (uint8_t *) &NM_INTEL_MANUFANCTURER_ID, sizeof(enable_req.intel_manufacturer_id));
  enable_req.policy_enable_disable_flag = NM_POLICY_ENABLE_FLAG;
  enable_req.domain_id = NM_POLICY_DOMAIN_ID;
  enable_req.policy_id = NM_POLICY_POLICY_ID;
  tlen = sizeof(me_enable_disable_nm_policy_req);

  ret = bic_me_xmit((uint8_t *)(&enable_req), tlen, (uint8_t *)rbuf, &rlen);
  if (ret < 0) {
    printf("%s(): ME transmission failed cmd:%d\n", __func__, ME_SET_NODE_MANAGER_POLICY);
    return ret;
  }

  if (rbuf[0] != CC_SUCCESS) {
    printf("Fail to enable node manager policy config: Completion Code: %02X\n", rbuf[0]);
    return -1;
  }

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
  } else if (strcmp(cmd, "--get_nm_power_statistics") == 0) {
    return me_get_nm_power_statistic();
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
  } else if (strcmp(argv[2], "--set_nm_power_policy") == 0) {
    if (argc < 4) {
      goto err_exit;
    }
    return me_set_nm_power_policy(argv[3]);
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
