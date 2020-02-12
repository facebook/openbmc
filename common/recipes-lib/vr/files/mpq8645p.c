/*
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
 *
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
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include "mpq8645p.h"

#define LARGEST_DEVICE_NAME 120

static int write_attr(const char *attr, unsigned int value)
{
  FILE *fp;
  char val[16] = {0};

  fp = fopen(attr, "w");
  if (fp == NULL) {
    syslog(LOG_WARNING, "failed to write attr %s", attr);
    return -1;
  }

  snprintf(val, 16, "0x%x\n", value);
  fputs(val, fp);
  fclose(fp);

  return 0;
}

static int read_attr(const char *attr, unsigned int *value)
{
  FILE *fp;
  int ret = 0;

  fp = fopen(attr, "r");
  if (fp == NULL) {
    syslog(LOG_WARNING, "failed to read attr %s", attr);
    return -1;
  }

  if (fscanf(fp, "%x", value) != 1) {
    syslog(LOG_WARNING, "failed to read attr %s", attr);
    ret = -1;
  }
  fclose(fp);
  return ret;
}

static int get_mpq8645p_ver(struct vr_info *info, char *key, char *ver)
{
  int *hwmon_idxp = (int*)info->private_data;
  int hwmon_idx = *hwmon_idxp;
  int ret;
  unsigned int val;
  char dev_name[LARGEST_DEVICE_NAME] = {0};

  snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS, hwmon_idx, MPQ8645P_MFR_REVISION);
  if ((ret = read_attr(dev_name, &val)) < 0)
    goto exit;

  snprintf(ver, MAX_VER_STR_LEN, "0x%x", val);
  kv_set(key, ver, 0, 0);

exit:
  return ret;
}

static struct config_data* create_data(char *buf)
{
  int val;
  char *token;
  struct config_data *tmp = (struct config_data *)malloc(sizeof(struct config_data));

  token = strtok(buf, ",");
  if (sscanf(token, "0x%02X", &val) < 0)
    goto error;
  tmp->addr = (uint8_t)val;

  token = strtok(NULL, ",");
  if (sscanf(token, "0x%02X", &val) < 0)
    goto error;
  tmp->reg = (uint8_t)val;

  token = strtok(NULL, ",");
  strcpy(tmp->name, token);

  token = strtok(NULL, ",");
  tmp->bytes = (uint8_t)atoi(token);

  token = strtok(NULL, ",");
  tmp->writable = strcmp(token, "WR")? 0: 1;

  token = strtok(NULL, ",");
  tmp->value = strtoul(token, NULL, 16);

  return tmp;

error:
  free(tmp);
  return NULL;
}

void mpq8645p_free_configs(struct mpq8645p_config *head)
{
  struct mpq8645p_config *tmp, *curr;

  curr = head;
  while (curr != NULL) {
    tmp = curr;
    curr = curr->next;
    free(tmp->data);
    free(tmp);
  }

  head = NULL;
}

static int program_mpq8645p(int hwmon_idx, struct config_data *data)
{
  char dev_name[LARGEST_DEVICE_NAME] = {0};

  switch (data->reg) {
    case PMBUS_VOUT_COMMAND:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_VOUT_COMMAND);
      break;
    case PMBUS_VOUT_SCALE_LOOP:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_VOUT_SCALE_LOOP);
      break;
    case PMBUS_IOUT_OC_FAULT_LIMIT:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_IOUT_OC_FAULT_LIMIT);
      break;
    case PMBUS_IOUT_OC_WARN_LIMIT:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_IOUT_OC_WARN_LIMIT);
      break;
    case PMBUS_MFR_REVISION:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_MFR_REVISION);
      break;
    case MPQ8645P_REG_MFR_CTRL_COMP:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_MFR_CTRL_COMP);
      break;
    case MPQ8645P_REG_MFR_CTRL_VOUT:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_MFR_CTRL_VOUT);
      break;
    case MPQ8645P_REG_MFR_CTRL_OPS:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_MFR_CTRL_OPS);
      break;
    case MPQ8645P_REG_MFR_OC_PHASE_LIMIT:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_MFR_OC_PHASE_LIMIT);
      break;
    default:
      syslog(LOG_WARNING, "Register 0x%x is not supported", data->reg);
      return 0;
  };

  return write_attr(dev_name, data->value);
}

static int verify_mpq8645p(int hwmon_idx, struct config_data *data)
{
  unsigned int val;
  char dev_name[LARGEST_DEVICE_NAME] = {0};

  switch (data->reg) {
    case PMBUS_VOUT_COMMAND:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_VOUT_COMMAND);
      break;
    case PMBUS_VOUT_SCALE_LOOP:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_VOUT_SCALE_LOOP);
      break;
    case PMBUS_IOUT_OC_FAULT_LIMIT:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_IOUT_OC_FAULT_LIMIT);
      break;
    case PMBUS_IOUT_OC_WARN_LIMIT:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_IOUT_OC_WARN_LIMIT);
      break;
    case PMBUS_MFR_REVISION:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_MFR_REVISION);
      break;
    case MPQ8645P_REG_MFR_CTRL_COMP:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_MFR_CTRL_COMP);
      break;
    case MPQ8645P_REG_MFR_CTRL_VOUT:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_MFR_CTRL_VOUT);
      break;
    case MPQ8645P_REG_MFR_CTRL_OPS:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_MFR_CTRL_OPS);
      break;
    case MPQ8645P_REG_MFR_OC_PHASE_LIMIT:
      snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
	       hwmon_idx, MPQ8645P_MFR_OC_PHASE_LIMIT);
      break;
    default:
      syslog(LOG_WARNING, "Register 0x%x is not supported", data->reg);
      return 0;
  };

  if (read_attr(dev_name, &val) < 0)
    return -1;

  if (val != data->value) {
    syslog(LOG_WARNING, "VR register = 0x%x != expected = 0x%x", val, data->value);
    return -1;
  }

  return 0;
}

static int restore_user(int hwmon_idx)
{
  char dev_name[LARGEST_DEVICE_NAME] = {0};

  snprintf(dev_name, LARGEST_DEVICE_NAME, MPQ8645P_DEBUGFS,
           hwmon_idx, MPQ8645P_RESTORE_USER_ALL);

  return write_attr(dev_name, 1);
}

int mpq8645p_get_fw_ver(struct vr_info *info, char *ver_str)
{
  char key[MAX_KEY_LEN], tmp_str[MAX_VER_STR_LEN] = {0};

  snprintf(key, sizeof(key), "vr_%02xh_ver", info->addr);
  if (kv_get(key, tmp_str, NULL, 0)) {
    if (get_mpq8645p_ver(info, key, ver_str) < 0) {
      return -1;
    }
  } else {
    snprintf(ver_str, MAX_VER_STR_LEN, "%s", tmp_str);
  }

  return 0;
}

void* mpq8645p_parse_file(struct vr_info *info, const char *path)
{
  FILE* fp;
  char *token;
  char raw[128] = {0};
  char buf[128] = {0};
  struct mpq8645p_config *new_config;
  struct mpq8645p_config *head = NULL;
  struct mpq8645p_config *curr = NULL;
  struct config_data *data;

  fp = fopen(path, "r");
  if (fp == NULL) {
    return NULL;
  }

  while (fgets(raw, sizeof(raw), fp) != NULL) {
    strcpy(buf, raw);
    token = strtok(raw, ",");
    if (!token || !strcmp(token, "Device Address")) {
      continue;
    }

    data = create_data(buf);
    if (data == NULL) {
      goto error;
    }

    new_config = (struct mpq8645p_config *)malloc(sizeof(struct mpq8645p_config));
    if (new_config == NULL) {
      goto error;
    }

    new_config->data = data;
    new_config->next = NULL;

    if (head != NULL) {
      curr->next = new_config;
    } else {
      head = new_config;
    }

    curr = new_config;
    memset(raw, 0, sizeof(raw));
  }

  fclose(fp);
  return (void*)head;

error:
  fclose(fp);
  mpq8645p_free_configs(head);
  return NULL;
}

int mpq8645p_fw_update(struct vr_info *info, void *configs)
{
  uint8_t addr = info->addr;
  int *hwmon_idxp = (int*)info->private_data;
  int hwmon_idx = *hwmon_idxp;
  int ret = VR_STATUS_SKIP;
  char buf[64] = {0};
  struct mpq8645p_config *config = (struct mpq8645p_config *)configs;
  struct config_data *data;

  while (config != NULL) {
    data = config->data;
    if (info->addr == data->addr && data->writable) {
      if (data->reg == PMBUS_MFR_REVISION) {
        printf("Update VR %s to version 0x%x\n", info->dev_name, data->value);
      }
      if ((ret = program_mpq8645p(hwmon_idx, data)) < 0) {
        break;
      }
    }
    config = config->next;
  }

  if (ret == 0) {
    restore_user(hwmon_idx);
    snprintf((char *)buf, sizeof(buf), "/tmp/cache_store/vr_%02xh_ver", addr);
    unlink((char *)buf);
  }

  return ret == VR_STATUS_SKIP? 0: ret;
}

int mpq8645p_fw_verify(struct vr_info *info, void *configs)
{
  int *hwmon_idxp = (int*)info->private_data;
  int hwmon_idx = *hwmon_idxp;
  int ret = 0;
  struct mpq8645p_config *config = (struct mpq8645p_config *)configs;
  struct config_data *data;

  while (config != NULL) {
    data = config->data;
    if (info->addr == data->addr) {
      if ((ret = verify_mpq8645p(hwmon_idx, data)) < 0) {
        break;
      }
    }
    config = config->next;
  }

  return ret;
}

int mpq8645p_validate_file(struct vr_info *info, const char *file)
{
  char *p = strrchr(file, '.');
  if (strcmp(p, ".csv")) {
    printf("Only support .csv format\n");
    return -1;
  }
  return 0;
}
