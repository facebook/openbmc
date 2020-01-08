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
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/kv.h>
#include "mpq8645p.h"

static int mpq8645p_open(struct vr_info *info)
{
  int fd;

  fd = i2c_cdev_slave_open(info->bus, info->addr, I2C_SLAVE_FORCE_CLAIM | I2C_CLIENT_PEC);
  if (fd < 0) {
    syslog(LOG_ERR, "%s: open %s failed", __func__, info->dev_name);
  }

  return fd;
}

static int get_mpq8645p_ver(struct vr_info *info, char *key, char *ver)
{
  int fd, ret = -1;
  uint8_t rbuf[16];

  if ((fd = mpq8645p_open(info)) < 0) {
    return -1;
  }

  if ((ret = i2c_smbus_read_block_data(fd, MPQ8645P_REG_MFR_REVISION, rbuf)) < 0) {
    goto exit;
  }

  snprintf(ver, MAX_VALUE_LEN, "%02X", rbuf[0]);
  kv_set(key, ver, 0, 0);

exit:
  close(fd);
  return ret;
}

static struct config_data* create_data(char *buf)
{
  int i, val;
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
  for (i = tmp->bytes-1; i >= 0; i--) {
    token += 2;
    if (sscanf(token, "%02x", &val) < 0) {
      goto error;
    }
    tmp->value[i] = (uint8_t)val;
  }

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

static int program_mpq8645p(int fd, struct config_data *data)
{
  int ret;

  if (data->reg == MPQ8645P_REG_MFR_REVISION) {
    if ((ret = i2c_smbus_write_word_data(fd, 0xE7, 0x0001)) < 0) {
      goto exit;
    }
  }

  if (data->reg == MPQ8645P_REG_MFR_ID || data->reg == MPQ8645P_REG_MFR_MODEL ||
      data->reg == MPQ8645P_REG_MFR_REVISION || data->reg == MPQ8645P_REG_MFR_4_DIGIT) {
    ret = i2c_smbus_write_block_data(fd, data->reg, data->bytes, data->value);
  } else if (data->bytes == 1) {
    uint8_t val = data->value[0];

    ret = i2c_smbus_write_byte_data(fd, data->reg, val);
  } else if (data->bytes == 2) {
    uint16_t val;

    if (data->reg != MPQ8645P_REG_MFR_CTRL) {
      memcpy(&val, data->value, 2);
    } else {
      if ((ret = i2c_smbus_read_word_data(fd, MPQ8645P_REG_MFR_CTRL)) < 0) {
        goto exit;
      }
      val = ((ret & ~(0x8)) | (data->value[0] & 0x8));
    }

    ret = i2c_smbus_write_word_data(fd, data->reg, val);
  } else {
    // Shouldn't be here
    return -1;
  }

  if (data->reg == MPQ8645P_REG_MFR_REVISION) {
    ret = i2c_smbus_write_word_data(fd, 0xE7, 0x0000);
  }

exit:
  return ret;
}

static int verify_mpq8645p(int fd, struct config_data *data)
{
  int ret;
  uint8_t val[8];

  if (data->reg == MPQ8645P_REG_MFR_ID || data->reg == MPQ8645P_REG_MFR_MODEL ||
      data->reg == MPQ8645P_REG_MFR_REVISION || data->reg == MPQ8645P_REG_MFR_4_DIGIT) {
    if ((ret = i2c_smbus_read_block_data(fd, data->reg, val)) < 0 ||
        (ret != data->bytes || memcmp(val, data->value, data->bytes))) {
      return -1;
    }
  } else if (data->bytes == 1) {
    if ((ret = i2c_smbus_read_byte_data(fd, data->reg)) < 0 ||
        (ret != data->value[0])) {
      return -1;
    }
  } else if (data->bytes == 2) {
    if ((ret = i2c_smbus_read_word_data(fd, data->reg)) < 0 ||
        ((ret & 0xFF) != data->value[0] || ((ret >> 8) & 0xFF) != data->value[1])) {
      return -1;
    }
  } else {
    // Shouldn't be here
    return -1;
  }

  return 0;
}

static int restore_user(int fd)
{
  int ret, i;
  struct mtp_cmd {
    uint8_t reg; uint16_t value; int delay;
  } cmds[6] = {
    {0xE6, 0x287C, 1},
    {0xE7, 0x0001, 2},
    {0xE7, 0x2001, 167},
    {0xE7, 0x1001, 1},
    {0xE7, 0x4001, 300},
    {0xE7, 0x0000, 10}
  };

  // Write to MTP
  for (i = 0; i < 6; i++) {
    if ((ret = i2c_smbus_write_word_data(fd, cmds[i].reg, cmds[i].value)) < 0) {
      goto exit;
    }
    msleep(cmds[i].delay);
  }

  // MTP->RAM
  ret = i2c_smbus_write_byte(fd, MPQ8645P_REG_RESTORE_USER_ALL);
  msleep(2);

exit:
  return ret;
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
  int fd;
  int ret = 0;
  char buf[64] = {0};
  struct mpq8645p_config *config = (struct mpq8645p_config *)configs;
  struct config_data *data;

  if ((fd = mpq8645p_open(info)) < 0) {
    return -1;
  }

  restore_user(fd);
  while (config != NULL) {
    data = config->data;
    if (info->addr == data->addr && data->writable) {
      if (data->reg == MPQ8645P_REG_MFR_REVISION) {
        printf("Update VR %s to version 0x%2x\n", info->dev_name, data->value[0]);
      }
      if ((ret = program_mpq8645p(fd, data)) < 0) {
        break;
      }
    }
    config = config->next;
  }

  if (ret == 0) {
    restore_user(fd);
    snprintf((char *)buf, sizeof(buf), "/tmp/cache_store/vr_%02xh_ver", addr);
    unlink((char *)buf);
  }

  close(fd);
  return ret;
}

int mpq8645p_fw_verify(struct vr_info *info, void *configs)
{
  int fd;
  int ret = 0;
  struct mpq8645p_config *config = (struct mpq8645p_config *)configs;
  struct config_data *data;

  if ((fd = mpq8645p_open(info)) < 0) {
    return -1;
  }

  while (config != NULL) {
    data = config->data;
    if (info->addr == data->addr) {
      if ((ret = verify_mpq8645p(fd, data)) < 0) {
        goto exit;
      }
    }
    config = config->next;
  }

exit:
  close(fd);
  return ret;
}
