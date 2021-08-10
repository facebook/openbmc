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
#include <openbmc/obmc-mctp.h>
#include <openbmc/ipc.h>
#include <facebook/fbgc_common.h>
#include "iocd.h"

pthread_mutex_t m_ioc    = PTHREAD_MUTEX_INITIALIZER;

static struct {
  uint8_t bus_id;
  uint8_t fru_num;
  char fru_name[MAX_FRU_CMD_STR];
} iocd_config = {
  .bus_id = 0,
  .fru_num = 0,
  .fru_name = {0},
};

struct mctp_smbus_pkt_private *smbus_extra_params = NULL;

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

static int calculate_pec_byte(uint8_t *buf, size_t len)
{
  int pec = 0;

  if (buf == NULL) {
    return -1;
  }

  return pec_calculate(pec, buf, len);
}

// From libobmc-mctp, modify to avoid the hard-coded of destination slave address.
struct obmc_mctp_binding* ioc_mctp_smbus_init(uint8_t bus, uint8_t dest_addr, uint8_t src_eid, int pkt_size,
                                              int dev_fd, int slave_queue_fd)
{
  struct mctp *mctp = NULL;
  struct mctp_binding_smbus *smbus = NULL;
  struct obmc_mctp_binding *mctp_binding = NULL;

  mctp_binding = (struct obmc_mctp_binding *)malloc(sizeof(struct obmc_mctp_binding));
  if (mctp_binding == NULL) {
    syslog(LOG_ERR, "%s: failed to allocate memory for mctp_binding", __func__);
    return NULL;
  }

  if (pkt_size < MCTP_PAYLOAD_SIZE + MCTP_HEADER_SIZE) {
    pkt_size = MCTP_PAYLOAD_SIZE + MCTP_HEADER_SIZE;
  }
  mctp_smbus_set_pkt_size(pkt_size);

  mctp = mctp_init();
  smbus = mctp_smbus_init();
  if (mctp == NULL || smbus == NULL || mctp_smbus_register_bus(smbus, mctp, src_eid) < 0) {
    syslog(LOG_ERR, "%s: MCTP init failed", __func__);
    goto bail;
  }

  smbus_extra_params = (struct mctp_smbus_pkt_private *)
                       malloc(sizeof(struct mctp_smbus_pkt_private));
  if (smbus_extra_params == NULL) {
    syslog(LOG_ERR, "%s: failed to allocate memory for smbus_extra_params", __func__);
    goto bail;
  }

  smbus_extra_params->mux_hold_timeout = 0;
  smbus_extra_params->mux_flags = 0;
  smbus_extra_params->fd = dev_fd;
  smbus_extra_params->slave_addr = dest_addr;

  mctp_smbus_set_in_fd(smbus, slave_queue_fd);

#ifdef DEBUG
  mctp_set_log_syslog();
  mctp_set_tracing_enabled(true);
#endif

  mctp_binding->mctp = mctp;
  mctp_binding->prot = (void *)smbus;
  return mctp_binding;
bail:
  obmc_mctp_smbus_free(mctp_binding);
  return NULL;
}

// From libobmc-mctp. Open and close the I2C device and slave mqueue here to prevent opening too much files.
int send_mctp_command(uint8_t bus, uint16_t src_addr, uint8_t dest_addr, uint8_t src_eid, uint8_t dst_eid,
                      uint8_t *tbuf, int tlen, uint8_t *rbuf, int *rlen)
{
  int ret = -1;
  int dev_fd = 0, slave_queue_fd = 0;
  uint8_t tag = 0;
  struct obmc_mctp_binding *mctp_binding = NULL;
  struct mctp_binding_smbus *smbus = NULL;
  char dev[MAX_PATH_SIZE] = {0}, slave_queue[MAX_PATH_SIZE] = {0};

  snprintf(dev, sizeof(dev), "/dev/i2c-%d", bus);
  dev_fd = open(dev, O_RDWR);
  if (dev_fd < 0) {
    syslog(LOG_ERR, "%s: open %s failed: %s", __func__, dev, strerror(errno));
    goto bail;
  }

  snprintf(slave_queue, sizeof(slave_queue), SYSFS_SLAVE_QUEUE, bus, src_addr);
  slave_queue_fd = open(slave_queue, O_RDONLY);
  if (slave_queue_fd < 0) {
    syslog(LOG_ERR, "%s: open %s failed: %s", __func__, slave_queue, strerror(errno));
    goto bail;
  }

  mctp_binding = ioc_mctp_smbus_init(bus, dest_addr, src_eid, MCTP_MAX_READ_SIZE, dev_fd, slave_queue_fd);
  if (mctp_binding == NULL) {
    syslog(LOG_ERR, "%s: mctp binding failed", __func__);
    goto bail;
  }
  smbus = (struct mctp_binding_smbus *)mctp_binding->prot;

  tag |= MCTP_HDR_FLAG_TO;
  ret = mctp_smbus_send_data(mctp_binding->mctp, dst_eid, tag, smbus, tbuf, tlen);
  if (ret < 0) {
    syslog(LOG_ERR, "%s: send mctp data failed\n", __func__);
    goto bail;
  }

  ret = mctp_smbus_recv_data_timeout_raw(mctp_binding->mctp, dst_eid, smbus, rbuf, -1);
  if (ret < 0) {
    syslog(LOG_ERR, "%s: get mctp response failed\n", __func__);
    goto bail;
  } else {
    *rlen = ret;
    ret = 0;
  }

bail:
  if (dev_fd >= 0) {
    close(dev_fd);
  }

  if (slave_queue_fd >= 0) {
    close(slave_queue_fd);
  }

  if (mctp_binding != NULL) {
    obmc_mctp_smbus_free(mctp_binding);
  }

  return ret;
}

static int 
set_write_buf(uint8_t *write_buf, uint8_t tag, ioc_command command) {
  int pec = 0;
  uint8_t res_len = 0;
  pkt_payload_hdr payload_header = {0};

  if (write_buf == NULL) {
    syslog(LOG_WARNING, "%s(): failed to set write buffer of %s due to NULL parameter.", __func__, ioc_command_map[command].command_name);
    return -1;
  }

  if (command >= ARRAY_SIZE(ioc_command_map)) {
    syslog(LOG_WARNING, "%s(): Failed to write i2c raw due to wrong command.", __func__);
    return -1;
  }

  memset(&payload_header, 0, sizeof(payload_header));

  payload_header.message_type     = MESSAGE_TYPE;
  payload_header.vendor_id[0]     = 0x10;
  payload_header.vendor_id[1]     = 0x00;
  payload_header.payload_id       = ioc_command_map[command].pkt_pl_hdr.payload_id;
  payload_header.msg_seq_count[0] = 0x01;
  payload_header.msg_seq_count[1] = 0x00;
  payload_header.app_msg_tag      = tag;

  memcpy(write_buf, &payload_header, sizeof(payload_header));

  res_len = sizeof(payload_header);

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

  if (res_len > 0) {
    // Calculate VDM PEC
    pec = calculate_pec_byte(write_buf, res_len);
    if (pec < 0) {
      syslog(LOG_WARNING, "%s(): Failed to calculate the PEC of MCTP payload packet.", __func__);
      return -1;
    }
    write_buf[res_len] = (uint8_t)pec;
    res_len++;
  }

  return res_len;
}

int 
mctp_ioc_command(uint8_t slave_addr, uint8_t tag, ioc_command command, uint8_t *resp_buf, uint8_t *resp_len) {
  int ret = 0, write_len = 0, read_len = 0, retry = 0;
  uint8_t write_buf[MCTP_MAX_WRITE_SIZE] = {0};
  uint8_t read_buf[MCTP_MAX_READ_SIZE] = {0};
  uint8_t resp_pl = 0, excepted_pl = 0;

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
    memset(temp, 0, 8);
    sprintf(temp, "%02X ", write_buf[i]);
    strcat(log, temp);
  }
  syslog(LOG_WARNING, "%s(): %s, write data to bus:%d, tag:%02X, write_len:%d, data: %s", __func__, ioc_command_map[command].command_name, iocd_config.bus_id, tag, write_len, log);
#endif

  do {
    memset(read_buf, 0, sizeof(read_buf));
    ret = send_mctp_command(iocd_config.bus_id, BMC_SLAVE_ADDR, slave_addr, DEFAULT_EID, 0x0, write_buf, write_len, read_buf, &read_len);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s(): failed to do '%s' on bus:%d because failed to send MCTP command.", __func__, ioc_command_map[command].command_name, iocd_config.bus_id);
      return -1;
    }

    // Retry if response is not ready.
    if (read_buf[RES_PAYLOAD_OFFSET] == RES_PL_CMD_RESP_NOT_READY) {
      sleep(0.5);
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
        memset(temp, 0, 8);
        sprintf(temp, "%02X ", read_buf[i]);
        strcat(log, temp);
      }
      syslog(LOG_WARNING, "%s(): %s, read data on bus:%d, tag:%02X, retry:%d, read_len:%d, data: %s", __func__, ioc_command_map[command].command_name, iocd_config.bus_id, tag, retry, read_len, log);
#endif
      return -1;
    }
  }

  memcpy(resp_buf, &read_buf, read_len);
  *resp_len = (uint8_t)read_len;

  return 0;
}

static int
vdm_reset(uint8_t slave_addr) {
  bool is_failed = false;
  uint8_t tag = 0xFF, read_len = 0;
  uint8_t read_buf[MCTP_MAX_READ_SIZE] = {0};

  memset(read_buf, 0, sizeof(read_buf));
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

static void *
get_ioc_temp() {
  int value = 0, wait_time = IOC_READY_TIME;
  bool is_failed = true;
  uint8_t tag = 0x01, read_len = 0;
  uint8_t read_buf[MCTP_MAX_READ_SIZE] = {0};

  while (1) {
    // Check IOC is ready to be access
    pthread_mutex_lock(&m_ioc);
    while (1) {
      if (pal_is_ioc_ready(iocd_config.bus_id) == false) {
        wait_time = IOC_READY_TIME;
      } else if (wait_time > 0){
        wait_time--;
      } else {
        break;
      }
      set_ioc_temp(READ_IOC_TEMP_FAILED);
      sleep(1);
    }
    pthread_mutex_unlock(&m_ioc);

    pthread_mutex_lock(&m_ioc);
    if (is_failed == true) {
      if (vdm_reset(IOC_SLAVE_ADDRESS) < 0) {
        syslog(LOG_WARNING, "%s(): failed to get %s IOC temperature because VDM reset failed", __func__, iocd_config.fru_name);

        pthread_mutex_unlock(&m_ioc);
        continue;
      }
      tag = 0x01;
      is_failed = false;
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
      set_ioc_temp(READ_IOC_TEMP_FAILED);
    }

    tag += 1;
    if (tag == 0xFF) {
      tag = 0x01;
    }

    sleep (2);
  }

  return NULL;
}

int
get_ioc_fw_ver(uint8_t slave_addr, uint8_t *res_buf, uint8_t *res_len) {
  bool is_failed = false;
  uint8_t tag = 0, read_len = 0;
  uint8_t read_buf[MCTP_MAX_READ_SIZE] = {0};
  uint8_t ioc_fw_ver[IOC_FW_VER_SIZE] = {0};

  if ((res_buf == NULL) || (res_len == NULL)) {
    syslog(LOG_WARNING, "%s(): Failed to get %s IOC firmware version due to NULL parameter.", __func__, iocd_config.fru_name);
    return -1;
  }

  pthread_mutex_lock(&m_ioc);

  if (vdm_reset(slave_addr) < 0) {
    syslog(LOG_WARNING, "%s(): failed to get %s IOC firmware version because VDM reset failed", __func__, iocd_config.fru_name);
    pthread_mutex_unlock(&m_ioc);
    return -1;
  }

  tag = 0x01;

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
    memset(ioc_fw_ver, 0, sizeof(ioc_fw_ver));
    memcpy(ioc_fw_ver, &read_buf[RES_IOC_FW_VER_OFFSET], sizeof(ioc_fw_ver));
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

  memcpy(res_buf, &ioc_fw_ver, sizeof(ioc_fw_ver));
  *res_len = sizeof(ioc_fw_ver);

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
  char pid_file_path[MAX_PATH_SIZE] = {0};

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
