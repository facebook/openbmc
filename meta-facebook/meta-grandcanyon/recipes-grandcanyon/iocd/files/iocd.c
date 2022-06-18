/*
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
 
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/ipc.h>
#include <facebook/fbgc_common.h>
#include "iocd.h"

pthread_mutex_t m_ioc    = PTHREAD_MUTEX_INITIALIZER;

static struct {
  uint8_t bus_id;
  uint8_t fru_num;
  bool check_pec;
  bool is_support_pec;
  char fru_name[MAX_FRU_CMD_STR];
  char ioc_ver_key[MAX_KEY_LEN];
  int i2c_write_failed_cnt;
  int i2c_fd;
  i2c_mslave_t *bmc_slave;
} iocd_config = {
  .bus_id = 0,
  .fru_num = 0,
  .check_pec = false,
  .is_support_pec = true,
  .fru_name = {0},
  .ioc_ver_key = {0},
  .i2c_write_failed_cnt = 0,
  .i2c_fd = -1,
  .bmc_slave = NULL,
};

static int
parse_cmdline_args(int argc, char* const argv[])
{
  uint8_t i2c_bus = 0;

  if (argv == NULL) {
    syslog(LOG_WARNING, "%s() Fail to set iocd config due to NULL parameter", __func__);
    return -1;
  }

  i2c_bus = (uint8_t)strtoul(argv[1], NULL, 0);
  memset(iocd_config.fru_name, 0, sizeof(iocd_config.fru_name));

  if (i2c_bus == I2C_T5IOC_BUS) {
    iocd_config.fru_num = FRU_SCC;
  } else if (i2c_bus == I2C_T5E1S0_T7IOC_BUS) {
    iocd_config.fru_num = FRU_E1S_IOCM;
  } else {
    syslog(LOG_WARNING, "%s() Fail to set iocd config due to wrong I2C bus:%d.", __func__, i2c_bus);
    return -1;
  }

  iocd_config.bus_id = i2c_bus;

  if (pal_get_fru_name(iocd_config.fru_num, iocd_config.fru_name) < 0) {
    syslog(LOG_WARNING, "%s() Fail to get fru%d name", __func__, iocd_config.fru_num);
    return -1;
  }

  return 0;
}

int
mctp_i2c_write(uint8_t slave_addr, uint8_t *buf, uint8_t len) {
  int ret = 0;
  int i = 0;

  if (buf == NULL) {
    syslog(LOG_WARNING, "%s(): Failed to write i2c raw to bus:%d due to NULL parameter", __func__, iocd_config.bus_id);
    return -1;
  }

  for (i = 0; i < I2C_RETRIES_MAX; i++) {
    ret = i2c_rdwr_msg_transfer(iocd_config.i2c_fd, slave_addr, buf, len, NULL, 0);
    if (ret < 0) {
      msleep(100);
      continue;
    } else {
      break;
    }
  }

  if (ret < 0) {
    syslog(LOG_WARNING, "%s(): Failed to write i2c raw to bus:%d", __func__, iocd_config.bus_id);
    return -1;
  }

  return 0;
}

int
mctp_i2c_read(uint8_t *buf, uint8_t *len) {

  int i = 0, read_len = 0;

  if ((buf == NULL) || (len == NULL)) {
    syslog(LOG_WARNING, "%s(): Failed to read i2c raw from bus:%d due to NULL parameters", __func__, iocd_config.bus_id);
    return -1;
  }

  for (i = 0; i < I2C_RETRIES_MAX; i++) {
    read_len = i2c_mslave_read(iocd_config.bmc_slave, buf, MCTP_MAX_READ_SIZE);
    if (read_len <= 0) {
      i2c_mslave_poll(iocd_config.bmc_slave, I2C_MSLAVE_POLL_TIME); /* 100 milliseconds */
      continue;
    } else {
      break;
    }
  }

  if (read_len <= 0) {
    syslog(LOG_WARNING, "%s(): Failed to read i2c raw from bus:%d because response length is wrong: %d", __func__, iocd_config.bus_id, read_len);
    return -1;
  } else {
    *len = read_len;
  }

  return 0;
}

static int
clear_slave_mqueue() {

  uint8_t buf[MCTP_MAX_READ_SIZE] = {0};
  int read_len = 0;
  int i = 0;

  for (i = 0; i < I2C_RETRIES_MAX; i++) {
    read_len = i2c_mslave_read(iocd_config.bmc_slave, buf, MCTP_MAX_READ_SIZE);
    if (read_len <= 0) {
      i2c_mslave_poll(iocd_config.bmc_slave, I2C_MSLAVE_POLL_TIME); /* 100 milliseconds */
      continue;
    } else {
      break;
    }
  }

  return 0;
}

static uint8_t crc8_calculate(uint16_t d)
{
  const uint32_t poly_check = 0x1070 << 3;
  int i = 0;

  for (i = 0; i < 8; i++) {
    if (d & 0x8000) {
      d = d ^ poly_check;
    }
    d = d << 1;
  }

  return (uint8_t)(d >> 8);
}

/* Incremental CRC8 over count bytes in the array pointed to by p */
static int pec_calculate(uint8_t crc, uint8_t *p, size_t count)
{
  int i = 0;

  if (p == NULL) {
    return -1;
  }

  for (i = 0; i < count; i++) {
    crc = crc8_calculate((crc ^ p[i]) << 8);
  }

  return (int)crc;
}

static int calculate_pec_byte(uint8_t *buf, size_t len, bool is_i2c_pec)
{
  int ret = 0;
  uint8_t pec = 0, address = 0;

  if (buf == NULL) {
    return -1;
  }

  if (is_i2c_pec == true) {
    address = IOC_SLAVE_ADDRESS;
    ret = pec_calculate(0, &address, 1);
    if (ret < 0) {
      return -1;
    }
    pec = (uint8_t) ret;
  }

  return pec_calculate(pec, buf, len);
}

static int 
set_write_buf(uint8_t *write_buf, uint8_t tag, ioc_command command) {
  int pec = 0;
  uint8_t res_len = 0;
  struct mctp_smbus_header_tx smbus_header = {0};
  struct mctp_hdr mctp_header = {0};
  pkt_payload_hdr payload_header = {0};
  size_t mctp_pkt_header_size = 0;
  bool is_i2c_pec = false;

  if (write_buf == NULL) {
    syslog(LOG_WARNING, "%s(): failed to set write buffer of %s due to NULL parameter.", __func__, ioc_command_map[command].command_name);
    return -1;
  }

  if (command >= ARRAY_SIZE(ioc_command_map)) {
    syslog(LOG_WARNING, "%s(): Failed to write i2c raw due to wrong command.", __func__);
    return -1;
  }

  memset(&smbus_header, 0, sizeof(smbus_header));
  memset(&mctp_header, 0, sizeof(mctp_header));
  memset(&payload_header, 0, sizeof(payload_header));
  
  smbus_header.command_code         = SMBUS_COMMAND_CODE;
  smbus_header.source_slave_address = (BMC_SLAVE_ADDR << 1) + 1;
  // Add 1 byte of Source Slave Address 
  smbus_header.byte_count           = (sizeof(mctp_header) + sizeof(payload_header) + 1);

  if (ioc_command_map[command].pkt_pl_hdr.is_need_request) {
    smbus_header.byte_count += (PKT_PAYLOAD_HDR_EXTRA_SIZE + ioc_command_map[command].pkt_pl_hdr.payload_len[0]);
  }

  mctp_header.ver                 = HEADER_VERSION;
  mctp_header.dest                = 0x00;
  mctp_header.src                 = 0x00;
  mctp_header.flags_seq_tag       = BRCM_MSG_TAG;

  if (iocd_config.is_support_pec == true) {
    // Add 1 byte of PEC
    smbus_header.byte_count += 1;
    payload_header.message_type = MESSAGE_TYPE_PEC;
  } else {
    payload_header.message_type = MESSAGE_TYPE;
  }
  payload_header.vendor_id[0]     = 0x10;
  payload_header.vendor_id[1]     = 0x00;
  payload_header.payload_id       = ioc_command_map[command].pkt_pl_hdr.payload_id;
  payload_header.msg_seq_count[0] = 0x01;
  payload_header.msg_seq_count[1] = 0x00;
  payload_header.app_msg_tag      = tag;

  memcpy(write_buf, &smbus_header, sizeof(smbus_header));
  memcpy(write_buf + sizeof(smbus_header), &mctp_header, sizeof(mctp_header));
  memcpy(write_buf + sizeof(smbus_header) + sizeof(mctp_header), &payload_header, sizeof(payload_header));

  mctp_pkt_header_size = sizeof(smbus_header) + sizeof(mctp_header);
  res_len = mctp_pkt_header_size + sizeof(payload_header);

  switch (command)
  {
    case COMMAND_RESUME:
    case COMMAND_DONE:
      break;
    case VDM_RESET:
    case GET_IOC_TEMP:
    case GET_IOC_FW:
      memcpy(write_buf + res_len, &ioc_command_map[command].pkt_pl_hdr.opcode, PKT_PAYLOAD_HDR_EXTRA_SIZE);
      res_len += PKT_PAYLOAD_HDR_EXTRA_SIZE;

      if (command == GET_IOC_TEMP) {
        memcpy(write_buf + res_len, &get_ioc_temp_payload, sizeof(get_ioc_temp_payload));
        res_len += sizeof(get_ioc_temp_payload);
      } else if (command == GET_IOC_FW) {
        memcpy(write_buf + res_len, &get_ioc_fw_version_payload, sizeof(get_ioc_fw_version_payload));
        res_len += sizeof(get_ioc_fw_version_payload);
      }
      break;
    default:
      res_len = -1;
      break;
  }

  if ((res_len > 0) && (iocd_config.is_support_pec == true)) {
    // Calculate VDM PEC
    pec = calculate_pec_byte(&write_buf[mctp_pkt_header_size], (res_len - mctp_pkt_header_size), is_i2c_pec);
    if (pec < 0) {
      syslog(LOG_WARNING, "%s(): Failed to calculate the PEC of MCTP payload packet.", __func__);
      return -1;
    }
    write_buf[res_len] = (uint8_t)pec;
    res_len++;

    // Calculate I2C PEC
    is_i2c_pec = true;
    pec = calculate_pec_byte(write_buf, res_len, is_i2c_pec);
    if (pec < 0) {
      syslog(LOG_WARNING, "%s(): Failed to calculate the PEC of whole packet.", __func__);
      return -1;
    }
    write_buf[res_len] = (uint8_t)pec;
    res_len++;
  }

  return res_len;
}

int 
mctp_ioc_command(uint8_t slave_addr, uint8_t tag, ioc_command command, uint8_t *resp_buf, uint8_t *resp_len) {
  int ret = 0, write_len = 0, retry = 0;
  uint8_t write_buf[MCTP_MAX_WRITE_SIZE] = {0};
  uint8_t read_buf[MCTP_MAX_READ_SIZE] = {0};
  uint8_t read_len = 0, resp_pl = 0, excepted_pl = 0;

  if ((resp_buf == NULL) || (resp_len == NULL)) {
    syslog(LOG_WARNING, "%s(): Failed to write i2c raw due to NULL parameter.", __func__);
    return -1;
  }

  *resp_len = 0;

  if (command >= ARRAY_SIZE(ioc_command_map)) {
    syslog(LOG_WARNING, "%s(): Failed to write i2c raw to bus:%d due to wrong command.", __func__, iocd_config.bus_id);
    return -1;
  }

  write_len = set_write_buf(write_buf, tag, command);

  if (write_len < 0) {
    syslog(LOG_WARNING, "%s(): Failed to initialize write buffer.", __func__);
    return -1;
  }

#ifdef DEBUG
  // Print write buffer data
  int i = 0;
  char temp[8] = {0}, log[MCTP_MAX_READ_SIZE*2] = {0};

  for (i = 0; i < write_len; i++) {
    memset(temp, 0, sizeof(temp));
    sprintf(temp, "%02X ", write_buf[i]);
    strcat(log, temp);
  }
  syslog(LOG_WARNING, "%s(): %s, write data to bus:%d, tag:%02X, write_len:%d, data: %s", __func__, ioc_command_map[command].command_name, iocd_config.bus_id, tag, write_len, log);
#endif

  do {
    ret = mctp_i2c_write(slave_addr, write_buf, write_len);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s(): failed to do '%s' on bus:%d because i2c write failed.", __func__, ioc_command_map[command].command_name, iocd_config.bus_id);
      return WRITE_I2C_RAW_FAILED;
    }

    memset(read_buf, 0, sizeof(read_buf));
    ret = mctp_i2c_read(read_buf, &read_len);
    if ((ret < 0) || (read_len < MCTP_MIN_READ_SIZE)) {
      syslog(LOG_WARNING, "%s(): failed to do '%s' on bus:%d because i2c read failed.", __func__, ioc_command_map[command].command_name, iocd_config.bus_id);
      return -1;
    }

    // Clear the second package of the response of "command resume"
    if (command == COMMAND_RESUME) {
      if (clear_slave_mqueue() < 0) {
        syslog(LOG_WARNING, "%s(): Failed to clear bus:%d slave-mqueue", __func__, iocd_config.bus_id);
        return -1;
      }
    }

    // Retry if response is not ready.
    if (read_buf[RES_PAYLOAD_OFFSET] == RES_PL_CMD_RESP_NOT_READY) {
      msleep(500);
    } else {
      break;
    }
  } while (++retry < I2C_RETRIES_MAX);

  // Check respond payload ID
  if (command != VDM_RESET) {
    resp_pl = read_buf[RES_PAYLOAD_OFFSET];
    excepted_pl = ioc_command_map[command].pkt_pl_hdr.res_payload_id;

    if (resp_pl != excepted_pl) {
       syslog(LOG_WARNING, "%s(): failed to do '%s' on bus:%d due to the wrong payload ID: %x, expected: %x", __func__, ioc_command_map[command].command_name, iocd_config.bus_id, resp_pl, excepted_pl);
#ifdef DEBUG
      // Print read buffer data
      memset(log, 0, sizeof(log));
      for (i = 0; i < read_len; i++) {
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "%02X ", read_buf[i]);
        strcat(log, temp);
      }
      syslog(LOG_WARNING, "%s(): %s, read data on bus:%d, tag:%02X, read_len:%d, data: %s", __func__, ioc_command_map[command].command_name, iocd_config.bus_id, tag, read_len, log);
#endif
      return -1;
    }
  }

  memcpy(resp_buf, &read_buf, read_len);
  *resp_len = read_len;

  return 0;
}

static int
check_ioc_support_pec(uint8_t slave_addr) {
  uint8_t tag = 0xFF, read_len = 0;
  uint8_t read_buf[MCTP_MAX_READ_SIZE] = {0};
  int ret = 0;

  iocd_config.is_support_pec = true;
  // Clear slave mqueue
  if (clear_slave_mqueue() < 0) {
    syslog(LOG_WARNING, "%s(): Failed to clear bus:%d slave-mqueue", __func__, iocd_config.bus_id);
    return -1;
  }

  // Do "VDM reset" to check if IOC F/w could support PEC.
  ret = mctp_ioc_command(slave_addr, tag, VDM_RESET, read_buf, &read_len);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s(): VDM reset failed, bus: %d", __func__, iocd_config.bus_id);
    return ret;
  }

  if (read_buf[RES_PAYLOAD_OFFSET] == RES_PL_PEC_FAILURE) {
    iocd_config.is_support_pec = false;
  } else if (read_buf[RES_PAYLOAD_OFFSET] == RES_PL_RESET_IN_PROGRESS) {
    // If the response status is "reset in progress", continue to do "command resume" and "command done".
    if (mctp_ioc_command(slave_addr, tag, COMMAND_RESUME, read_buf, &read_len) < 0) {
      iocd_config.is_support_pec = false;
    }

    if (mctp_ioc_command(slave_addr, tag, COMMAND_DONE, read_buf, &read_len) < 0) {
      iocd_config.is_support_pec = false;
    }
  } else if (read_buf[RES_PAYLOAD_OFFSET] != ioc_command_map[VDM_RESET].pkt_pl_hdr.res_payload_id) {
    syslog(LOG_WARNING, "%s(): Failed to check if %s IOC firmware could support PEC byte beacuse wrong payload ID: %02X", __func__, iocd_config.fru_name, read_buf[RES_PAYLOAD_OFFSET]);
    return -1;
  }

  if (iocd_config.is_support_pec == false) {
    syslog(LOG_WARNING, "%s(): %s IOC firmware doesn't support handling PEC byte, bus: %d", __func__, iocd_config.fru_name, iocd_config.bus_id);
  }
  iocd_config.check_pec = true;

  return 0;
}

static int
vdm_reset(uint8_t slave_addr) {
  bool is_failed = false;
  uint8_t tag = 0xFF, read_len = 0;
  uint8_t read_buf[MCTP_MAX_READ_SIZE] = {0};

  if (mctp_ioc_command(slave_addr, tag, VDM_RESET, read_buf, &read_len) < 0) {
    syslog(LOG_WARNING, "%s(): VDM reset failed, bus: %d", __func__, iocd_config.bus_id);
    is_failed = true;
  }

  memset(read_buf, 0, sizeof(read_buf));
  if (mctp_ioc_command(slave_addr, tag, COMMAND_RESUME, read_buf, &read_len) < 0) {
    syslog(LOG_WARNING, "%s(): VDM reset failed because command resume failed, bus: %d", __func__, iocd_config.bus_id);
    is_failed = true;
  }

  memset(read_buf, 0, sizeof(read_buf));
  if (mctp_ioc_command(slave_addr, tag, COMMAND_DONE, read_buf, &read_len) < 0) {
    syslog(LOG_WARNING, "%s(): VDM reset failed because command done failed, bus: %d", __func__, iocd_config.bus_id);
    is_failed = true;
  }

  if (is_failed == true) {
    return -1;
  }

  return 0;
}

static int
set_ioc_temp(int value) {
  char key[MAX_KEY_LEN] = {0};
  char cache_value[MAX_VALUE_LEN] = {0};

  memset(key, 0, sizeof(key));
  memset(cache_value, 0, sizeof(cache_value));

  if (iocd_config.bus_id == I2C_T5IOC_BUS) {
    snprintf(key, sizeof(key), "%s_ioc_sensor%d", iocd_config.fru_name, SCC_IOC_TEMP);
  } else if (iocd_config.bus_id == I2C_T5E1S0_T7IOC_BUS) {
    snprintf(key, sizeof(key), "%s_ioc_sensor%d", iocd_config.fru_name, IOCM_IOC_TEMP);
  } else {
    syslog(LOG_WARNING, "%s() Failed to set %s IOC temperature due to unknown i2c bus: %d", __func__, iocd_config.fru_name, iocd_config.bus_id);
    return -1;
  }

  if (value == READ_IOC_TEMP_FAILED) {
    snprintf(cache_value, sizeof(cache_value), "NA");
  } else {
    snprintf(cache_value, sizeof(cache_value), "%d", value);
  }

  if (kv_set(key, cache_value, 0, 0) < 0) {
    syslog(LOG_WARNING, "%s() set %s IOC temperature failed. key = %s, value = %s.", __func__, iocd_config.fru_name, key, cache_value);
    return -1;
  }

  return 0;
}

static int
reattach_i2c_device(char* slave_device_path) {
  // Detach IOC I2C device
  if (access(slave_device_path, F_OK) == 0) {
    if (i2c_delete_device(iocd_config.bus_id, DEVICE_ADDRESS) != 0) {
      syslog(LOG_ERR, "%s(): Failed to delete i2c device on bus:%d", __func__, iocd_config.bus_id);
      return -1;
    }
  }

  if (iocd_config.i2c_fd >= 0) {
    close(iocd_config.i2c_fd);
  }

  if (iocd_config.bmc_slave != NULL) {
    if (i2c_mslave_close(iocd_config.bmc_slave) < 0){
      syslog(LOG_ERR, "%s(): Failed to close slave device on bus:%d", __func__, iocd_config.bus_id);
      return -1;
    }
  }

  // Attach IOC I2C device
  if (i2c_add_device(iocd_config.bus_id, DEVICE_ADDRESS, DEVICE_NAME) < 0) {
    syslog(LOG_ERR, "%s(): Failed to add i2c device on bus:%d", __func__, iocd_config.bus_id);
    return -1;
  }

  iocd_config.i2c_fd = i2c_cdev_slave_open(iocd_config.bus_id, IOC_SLAVE_ADDRESS, I2C_SLAVE);
  if (iocd_config.i2c_fd < 0) {
    syslog(LOG_ERR, "%s(): Failed to open i2c device on bus:%d", __func__, iocd_config.bus_id);
    return -1;
  }

  iocd_config.bmc_slave = i2c_mslave_open(iocd_config.bus_id, BMC_SLAVE_ADDR);
  if (iocd_config.bmc_slave == NULL) {
    syslog(LOG_ERR, "%s(): Failed to open bmc as slave on bus:%d", __func__, iocd_config.bus_id);
    return -1;
  }

  return 0;
}

static void *
get_ioc_temp() {
  int value = 0, ret = 0;
  bool is_failed = true;
  uint8_t tag = 0x01, read_len = 0;
  uint8_t read_buf[MCTP_MAX_READ_SIZE] = {0};
  char slave_dev_path[MAX_FILE_PATH] = {0};

  snprintf(slave_dev_path, sizeof(slave_dev_path), IOC_SLAVE_QUEUE, iocd_config.bus_id, DEVICE_ADDRESS);

  if (access(slave_dev_path, F_OK) == -1) {
    if (i2c_add_device(iocd_config.bus_id, DEVICE_ADDRESS, DEVICE_NAME) < 0) {
      syslog(LOG_WARNING, "%s(): Failed to add i2c device on bus:%d", __func__, iocd_config.bus_id);
      goto cleanup;
    }
  }

  iocd_config.i2c_fd = i2c_cdev_slave_open(iocd_config.bus_id, IOC_SLAVE_ADDRESS, I2C_SLAVE);
  if (iocd_config.i2c_fd < 0) {
    syslog(LOG_WARNING, "%s(): Failed to open i2c device on bus:%d", __func__, iocd_config.bus_id);
    goto cleanup;
  }

  iocd_config.bmc_slave = i2c_mslave_open(iocd_config.bus_id, BMC_SLAVE_ADDR);
  if (iocd_config.bmc_slave == NULL) {
    syslog(LOG_WARNING, "%s(): Failed to open bmc as slave on bus:%d", __func__, iocd_config.bus_id);
    goto cleanup;
  }

  snprintf(iocd_config.ioc_ver_key, sizeof(iocd_config.ioc_ver_key), "ioc_%d_fw_ver", iocd_config.bus_id);
  kv_set(iocd_config.ioc_ver_key, "", 0, 0);

  while (1) {
    // Check IOC is ready to be access
    pthread_mutex_lock(&m_ioc);
    while (1) {
      if (pal_is_ioc_ready(iocd_config.bus_id) == true) {
        if (iocd_config.check_pec == false) {
          ret = check_ioc_support_pec(IOC_SLAVE_ADDRESS);
          if (ret < 0) {
            is_failed = true;
            set_ioc_temp(READ_IOC_TEMP_FAILED);
            kv_set(iocd_config.ioc_ver_key, "", 0, 0);
            // Reattach i2c devcie if i2c write failed 2 times
            if (ret == WRITE_I2C_RAW_FAILED) {
              if (iocd_config.i2c_write_failed_cnt >= 2) {
                syslog(LOG_WARNING, "%s(): Failed to do i2c write, reattach i2c device on bus:%d", __func__, iocd_config.bus_id);
                if (reattach_i2c_device(slave_dev_path) < 0) {
                  syslog(LOG_ERR, "%s(): Failed to reattach i2c device on bus:%d", __func__, iocd_config.bus_id);
                } else {
                  iocd_config.i2c_write_failed_cnt = 0;
                }
              } else {
                iocd_config.i2c_write_failed_cnt++;
              }
            }
            sleep (10);
            continue;
          }
        }
        break;
      }
      is_failed = true;
      set_ioc_temp(READ_IOC_TEMP_FAILED);
      kv_set(iocd_config.ioc_ver_key, "", 0, 0);
      sleep(1);
    }

    if (is_failed == true) {
      if (vdm_reset(IOC_SLAVE_ADDRESS) < 0) {
        syslog(LOG_WARNING, "%s(): failed to get %s IOC temperature because VDM reset failed", __func__, iocd_config.fru_name);

        pthread_mutex_unlock(&m_ioc);
        iocd_config.check_pec = false;
        sleep (1);
        continue;
      }
      tag = 0x01;
      is_failed = false;
      iocd_config.i2c_write_failed_cnt = 0;
    }

    value = 0;

    memset(read_buf, 0, sizeof(read_buf));
    if (mctp_ioc_command(IOC_SLAVE_ADDRESS, tag, GET_IOC_TEMP, read_buf, &read_len) < 0) {
      syslog(LOG_WARNING, "%s(): failed to get %s IOC temperature", __func__, iocd_config.fru_name);
      is_failed = true;
    }

    memset(read_buf, 0, sizeof(read_buf));
    if (mctp_ioc_command(IOC_SLAVE_ADDRESS, tag, COMMAND_RESUME, read_buf, &read_len) < 0) {
      syslog(LOG_WARNING, "%s(): failed to get %s IOC temperature because command resume failed", __func__, iocd_config.fru_name);
      is_failed = true;
    }

    if ((is_failed == false) && (read_buf[RES_NUM_SENSOR_OFFSET] == 0x01) && (read_buf[RES_IOC_TEMP_VALID_OFFSET] == 0x01)) {
      value = (int)read_buf[RES_IOC_TEMP_OFFSET];
    } else {
      syslog(LOG_WARNING, "%s(): failed to get the correct %s IOC temperature", __func__, iocd_config.fru_name);
    }

    memset(read_buf, 0, sizeof(read_buf));
    if (mctp_ioc_command(IOC_SLAVE_ADDRESS, tag, COMMAND_DONE, read_buf, &read_len) < 0) {
      syslog(LOG_WARNING, "%s(): failed to get %s IOC temperature because command done failed", __func__, iocd_config.fru_name);
      is_failed = true;
    }
    pthread_mutex_unlock(&m_ioc);

    if (is_failed == false) {
      set_ioc_temp(value);
    } else {
      iocd_config.check_pec = false;
      set_ioc_temp(READ_IOC_TEMP_FAILED);
    }

    tag += 1;
    if (tag == 0xFF) {
      tag = 0x01;
    }

    sleep (2);
  }

cleanup:
  if (access(slave_dev_path, F_OK) == 0) {
    i2c_delete_device(iocd_config.bus_id, DEVICE_ADDRESS);
  }

  if (iocd_config.i2c_fd >= 0) {
    close(iocd_config.i2c_fd);
  }

  if (iocd_config.bmc_slave != NULL) {
    i2c_mslave_close(iocd_config.bmc_slave);
  }

  return NULL;
}

int
get_ioc_fw_ver(uint8_t slave_addr, uint8_t *res_buf, uint8_t *res_len) {
  bool is_failed = false;
  uint8_t tag = 0x01, read_len = 0;
  uint8_t read_buf[MCTP_MAX_READ_SIZE] = {0};
  ioc_fw_ver fw_ver = {0};
  char ioc_fw_ver_str[MAX_VALUE_LEN] = {0};

  if ((res_buf == NULL) || (res_len == NULL)) {
    syslog(LOG_WARNING, "%s(): Failed to get %s IOC firmware version due to NULL parameter.", __func__, iocd_config.fru_name);
    return -1;
  }

  if (iocd_config.check_pec == false) {
    syslog(LOG_WARNING, "%s(): Failed to get %s IOC firmware version because IOC firmware is not ready.", __func__, iocd_config.fru_name);
    return -1;
  }

  if (pal_get_cached_value(iocd_config.ioc_ver_key, ioc_fw_ver_str) != 0) {
    syslog(LOG_WARNING, "%s(): Failed to get %s IOC firmware version due to kv get failed.", __func__, iocd_config.fru_name);
    return -1;
  }

  // Get IOC firmware version from IOC if the cache is empty
  if (strncmp(ioc_fw_ver_str, "", sizeof(ioc_fw_ver_str)) == 0) {
    pthread_mutex_lock(&m_ioc);

    if (vdm_reset(slave_addr) < 0) {
      syslog(LOG_WARNING, "%s(): failed to get %s IOC firmware version because VDM reset failed", __func__, iocd_config.fru_name);
      pthread_mutex_unlock(&m_ioc);
      return -1;
    }

    memset(read_buf, 0, sizeof(read_buf));
    if (mctp_ioc_command(slave_addr, tag, GET_IOC_FW, read_buf, &read_len) < 0) {
      syslog(LOG_WARNING, "%s(): failed to get %s IOC firmware version", __func__, iocd_config.fru_name);
      is_failed = true;
    }

    memset(read_buf, 0, sizeof(read_buf));
    if (mctp_ioc_command(slave_addr, tag, COMMAND_RESUME, read_buf, &read_len) < 0) {
      syslog(LOG_WARNING, "%s(): failed to get %s IOC firmware version because command resume failed", __func__, iocd_config.fru_name);
      is_failed = true;
    }

    if ((is_failed == false) && (read_len >= ioc_command_map[GET_IOC_FW].pkt_pl_hdr.response_len[0])) {
      memcpy(&fw_ver, &read_buf[RES_IOC_FW_VER_OFFSET], sizeof(fw_ver));
    } else {
      syslog(LOG_WARNING, "%s(): failed to get the correct %s IOC firmware version.", __func__, iocd_config.fru_name);
      is_failed = true;
    }

    memset(read_buf, 0, sizeof(read_buf));
    if (mctp_ioc_command(slave_addr, tag, COMMAND_DONE, read_buf, &read_len) < 0) {
      syslog(LOG_WARNING, "%s(): failed to get %s IOC firmware version because command done failed", __func__, iocd_config.fru_name);
      is_failed = true;
    }
    pthread_mutex_unlock(&m_ioc);

    if (is_failed == true) {
      *res_len = 0;
      return -1;
    }

    snprintf(ioc_fw_ver_str, sizeof(ioc_fw_ver_str), "%d.%d.%d.%d-%05d-%05d",
             fw_ver.gen_major, fw_ver.gen_minor, fw_ver.phase_major, fw_ver.phase_minor, (fw_ver.cus_id_1 << 8) + fw_ver.cus_id_2, (fw_ver.build_num_1 << 8) + fw_ver.build_num_2);
    if (pal_set_cached_value(iocd_config.ioc_ver_key, ioc_fw_ver_str) < 0) {
      return -1;
    }
  }

  res_buf[0] = IOC_FW_VER_SIZE;
  *res_len = 1;

  return 0;
}

int
conn_handler(client_t *cli) {
  uint8_t req_buf[MCTP_MAX_WRITE_SIZE] = {0};
  uint8_t res_buf[MCTP_MAX_READ_SIZE] = {0};
  size_t req_len = IOC_FW_VER_SIZE, res_len = 0;

  memset(req_buf, 0, sizeof(req_buf));
  memset(res_buf, 0, sizeof(res_buf));

  if (cli == NULL) {
    syslog(LOG_WARNING, "%s(): get %s IOC firmware version failed due to NULL parameter", __func__, iocd_config.fru_name);
    return -1;
  }

  if (ipc_recv_req(cli, req_buf, &req_len, TIMEOUT_IOC)) {
    syslog(LOG_WARNING, "%s(): get %s IOC firmware version failed because IPC recvive failed", __func__, iocd_config.fru_name);
    return -1;
  }

  if (get_ioc_fw_ver(IOC_SLAVE_ADDRESS, res_buf, (uint8_t*)&res_len) < 0) {
    syslog(LOG_WARNING, "%s(): get %s IOC firmware version failed", __func__, iocd_config.fru_name);
    return -1;
  }

  if (res_len == 0) {
    syslog(LOG_WARNING, "%s(): get %s IOC firmware version failed due to the wrong response length", __func__, iocd_config.fru_name);
    return -1;
  }

  if(ipc_send_resp(cli, res_buf, res_len)) {
    syslog(LOG_WARNING, "%s(): get %s IOC firmware version failed because IPC send failed", __func__, iocd_config.fru_name);
    return -1;
  }

  return 0;
}

static void
run_iocd() {
  char ioc_socket[MAX_SOCK_PATH_SIZE] = {0};
  pthread_t tid_get_ioc_temp, tid_get_ioc_fw_ver;
  int ret_get_ioc_temp = 0, ret_get_ioc_fw = 0;

  // Create thread for getting IOC temperature
  if (pthread_create(&tid_get_ioc_temp, NULL, get_ioc_temp, (void *)&iocd_config.bus_id) < 0) {
    syslog(LOG_WARNING, "Failed to create thread for getting %s IOC temperature error", iocd_config.fru_name);
    ret_get_ioc_temp = -1;
  }

  // Create IPC socket to receive and send IOC firmware version to fw-util.
  memset(ioc_socket, 0, sizeof(ioc_socket));
  snprintf(ioc_socket, sizeof(ioc_socket), SOCK_PATH_IOC, iocd_config.fru_num);
  if (ipc_start_svc(ioc_socket, conn_handler, MAX_REQUESTS, NULL, &tid_get_ioc_fw_ver) < 0) {
    syslog(LOG_WARNING, "Failed to create ipc socket for %s IOC firmware version", iocd_config.fru_name);
    ret_get_ioc_fw = -1;
  }

  if (ret_get_ioc_temp == 0) {
    pthread_join(tid_get_ioc_temp, NULL);
  }
  if (ret_get_ioc_fw == 0) {
    pthread_join(tid_get_ioc_fw_ver, NULL);
  }
}

int
main (int argc, char * const argv[])
{
  int rc = 0, pid_file = 0;
  char pid_file_path[MAX_PID_PATH_SIZE] = {0};

  // Parse command line arguments.
  if (parse_cmdline_args(argc, argv) != 0) {
    syslog(LOG_ERR, "Fail to execute iocd because fail to set up IOC Daemon config");
    return -1;
  }

  memset(pid_file_path,0, sizeof(pid_file_path));
  snprintf(pid_file_path, sizeof(pid_file_path), "/var/run/iocd_%d.pid", iocd_config.bus_id);

  pid_file = open(pid_file_path, O_CREAT | O_RDWR, 0666);
  rc = pal_flock_retry(pid_file);
  if (rc < 0) {
    if (EWOULDBLOCK == errno) {
      printf("Another iocd_%d instance is running...\n", iocd_config.bus_id);
    } else {
      syslog(LOG_ERR, "Fail to execute iocd_%d because %s", iocd_config.bus_id, strerror(errno));
    }
    pal_unflock_retry(pid_file);
    close(pid_file);
    exit(EXIT_FAILURE);
  } else {
    syslog(LOG_INFO, "IOC bus:%d daemon started", iocd_config.bus_id);
    run_iocd();
  }

  pal_unflock_retry(pid_file);
  close(pid_file);

  return 0;
}
