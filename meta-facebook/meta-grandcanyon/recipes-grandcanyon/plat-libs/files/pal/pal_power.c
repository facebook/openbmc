#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-sensors.h>
#include <facebook/exp.h>
#include <facebook/fbgc_gpio.h>
#include "pal.h"

static int
server_power_12v_on() {
  int i2cfd = 0;
  uint8_t status = 0;
  int ret = 0, retry = 0, times = 0;
  i2c_master_rw_command command;
  
  memset(&command, 0, sizeof(command));
  command.offset = 0x00;
  command.val = STAT_12V_ON;

  i2cfd = i2c_cdev_slave_open(I2C_UIC_FPGA_BUS, UIC_FPGA_SLAVE_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (i2cfd < 0) {
    ret = i2cfd;
    syslog(LOG_WARNING, "%s() Failed to open %d", __func__, I2C_UIC_FPGA_BUS);
    goto exit;
  }

  while (retry < MAX_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, UIC_FPGA_SLAVE_ADDR, (uint8_t *)&command, sizeof(command), NULL, 0);
    if (ret < 0) {
      retry++;
      msleep(100);
    } else {
      break;
    }
  }
  if (retry == MAX_RETRY) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, sizeof(command));
    goto exit;
  }
  
  while(times < WAIT_POWER_STATUS_CHANGE_TIME) {
    ret = pal_get_server_12v_power(&status);
    if ((ret == 0) && (status == SERVER_12V_ON)) {
      goto exit;
    } else {
      times++;
      sleep(1);
    }
  }

exit:
  if (i2cfd >= 0) {
    close(i2cfd);
  }
    
  return ret;
}

static int
server_power_12v_off() {
  int i2cfd = 0;
  uint8_t status = 0;
  int ret = 0, retry = 0, times = 0;
  i2c_master_rw_command command;
  
  memset(&command, 0, sizeof(command));
  command.offset = 0x00;
  command.val = STAT_12V_OFF;

  i2cfd = i2c_cdev_slave_open(I2C_UIC_FPGA_BUS, UIC_FPGA_SLAVE_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (i2cfd < 0) {
    ret = i2cfd;
    syslog(LOG_WARNING, "%s() Failed to open %d", __func__, I2C_UIC_FPGA_BUS);
    goto exit;
  }

  while (retry < MAX_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, UIC_FPGA_SLAVE_ADDR, (uint8_t *)&command, sizeof(command), NULL, 0);
    if (ret < 0) {
      retry++;
      msleep(100);
    } else {
      break;
    }
  }
  if (retry == MAX_RETRY) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer fails, tlen=%d", __func__, sizeof(command));
    goto exit;
  }
  
  while(times < WAIT_POWER_STATUS_CHANGE_TIME) {
    ret = pal_get_server_12v_power(&status);
    if ((ret == 0) && (status == SERVER_12V_OFF)) {
      goto exit;
    } else {
      times++;
      sleep(1);
    }
  }

exit:
  if (i2cfd >= 0) {
    close(i2cfd);
  }
  
  return ret;
}

uint8_t
pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy, uint8_t *res_data) {
  uint8_t completion_code = CC_SUCCESS; // Fill response with default values
  char key[MAX_KEY_LEN] = {0};
  unsigned char policy = 0x00;
  
  if (pwr_policy == NULL) {
    syslog(LOG_WARNING, "%s() NULL pointer: *pwr_policy", __func__);
    return -1;
  }
  
  policy = *pwr_policy & 0x07;  // Power restore policy
  memset(&key, 0, sizeof(key));
  snprintf(key, MAX_KEY_LEN, "server_por_cfg");
  switch (policy)
  {
    case POWER_CFG_OFF:
      if (pal_set_key_value(key, "off") != 0) {
        completion_code = CC_UNSPECIFIED_ERROR;
      }
      break;
    case POWER_CFG_LPS:
      if (pal_set_key_value(key, "lps") != 0) {
        completion_code = CC_UNSPECIFIED_ERROR;
      }
      break;
    case POWER_CFG_ON:
      if (pal_set_key_value(key, "on") != 0) {
        completion_code = CC_UNSPECIFIED_ERROR;
      }
      break;
    case POWER_CFG_UKNOWN:
      // no change (just get present policy support)
      break;
    default:
      completion_code = CC_PARAM_OUT_OF_RANGE;
      break;
  }
  return completion_code;
}

void
pal_get_chassis_status(uint8_t fru, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len) {
  char buff[MAX_VALUE_LEN] = {0};
  int policy = POWER_CFG_UKNOWN;
  uint8_t status = 0, ret = 0;
  unsigned char *data = res_data;

  if (res_data == NULL) {
    syslog(LOG_WARNING, "%s() NULL pointer: *res_data", __func__);
  }
  
  if (res_len == NULL) {
    syslog(LOG_WARNING, "%s() NULL pointer: *res_len", __func__);
  }
  
  memset(&buff, 0, sizeof(buff));
  if (pal_get_key_value("server_por_cfg", buff) == 0) {
    if (!memcmp(buff, "off", strlen("off"))) {
      policy = POWER_CFG_OFF;
    } else if (!memcmp(buff, "lps", strlen("lps"))) {
      policy = POWER_CFG_LPS;
    } else if (!memcmp(buff, "on", strlen("on"))) {
      policy = POWER_CFG_ON;
    } else {
      policy = POWER_CFG_UKNOWN;
    }
  }

  // Current Power State
  ret = pal_get_server_power(FRU_SERVER, &status);
  if (ret >= 0) {
    *data++ = status | (policy << 5);
  } else {
    syslog(LOG_WARNING, "%s: pal_get_server_power failed", __func__);
    *data++ = 0x00 | (policy << 5);
  }

  *data++ = 0x00;   // Last Power Event
  *data++ = 0x40;   // Misc. Chassis Status
  *data++ = 0x00;   // Front Panel Button Disable
  *res_len = data - res_data;
}

static int
set_nic_power_mode(nic_power_control_mode nic_mode) {
  SET_NIC_POWER_MODE_CMD cmd = {0};
  int fd = 0, ret = 0, retry = 0;

  cmd.nic_power_control_cmd_code = CMD_CODE_NIC_POWER_CONTROL;
  cmd.nic_power_control_mode = (uint8_t)nic_mode;

  fd = i2c_cdev_slave_open(I2C_UIC_FPGA_BUS, UIC_FPGA_SLAVE_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open i2c bus %d", __func__, I2C_UIC_FPGA_BUS);
    return -1;
  }

  while (retry < MAX_RETRY) {
    ret = i2c_rdwr_msg_transfer(fd, UIC_FPGA_SLAVE_ADDR, (uint8_t *)&cmd, sizeof(cmd), NULL, 0);
    if (ret < 0) {
      retry++;
      msleep(100);
    } else {
      break;
    }
  }
  if (retry == MAX_RETRY) {
    syslog(LOG_WARNING, "%s() Failed to send \"set NIC power mode\" command to UIC FPGA", __func__);
  }

  if (fd >= 0) {
    close(fd);
  }

  if (ret == 0 && nic_mode == NIC_VMAIN_MODE) {
    sleep(2); //sleep 2s to wait for PERST, 2s is enough for FPGA
  }

  return ret;
}

static int
power_off_pre_actions() {
  if (gpio_set_init_value_by_shadow(fbgc_get_gpio_name(GPIO_E1S_1_P3V3_PG_R), GPIO_VALUE_LOW) < 0) {
    syslog(LOG_ERR, "%s() Failed to disable E1S0/IOCM I2C\n", __func__);
    return -1;
  }
  if (gpio_set_init_value_by_shadow(fbgc_get_gpio_name(GPIO_E1S_2_P3V3_PG_R), GPIO_VALUE_LOW) < 0) {
    syslog(LOG_ERR, "%s() Failed to disable E1S1/IOCM I2C\n", __func__);
    return -1;
  }

  return 0;
}

static int
power_off_post_actions() {
  int ret = 0;

  ret = set_nic_power_mode(NIC_VAUX_MODE);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Failed to set NIC to VAUX mode\n", __func__);
  }

  return ret;
}

static int
power_on_pre_actions() {
  int ret = 0;

  ret = set_nic_power_mode(NIC_VMAIN_MODE);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Failed to set NIC to VMAIN mode, stop power on host\n", __func__);
  }

  return ret;
}

static int
power_on_post_actions() {
  char path[MAX_PATH_LEN] = {0};
  int ret = 0;
  uint8_t chassis_type = 0;

  if (gpio_set_init_value_by_shadow(fbgc_get_gpio_name(GPIO_E1S_1_P3V3_PG_R), GPIO_VALUE_HIGH) < 0) {
    syslog(LOG_ERR, "%s() Failed to enable E1S0/IOCM I2C\n", __func__);
    return -1;
  }
  if (gpio_set_init_value_by_shadow(fbgc_get_gpio_name(GPIO_E1S_2_P3V3_PG_R), GPIO_VALUE_HIGH) < 0) {
    syslog(LOG_ERR, "%s() Failed to enable E1S1/IOCM I2C\n", __func__);
    return -1;
  }

  if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
    syslog(LOG_WARNING, "%s() Failed to get chassis type. If the system is Type 7, will not get IOCM FRU information.\n", __func__);
    return -2;
  }

  if (chassis_type == CHASSIS_TYPE7) {
    if (access(IOCM_EEPROM_BIND_DIR, F_OK) == -1) { // bind device if not bound
      ret = pal_bind_i2c_device(I2C_T5E1S1_T7IOC_BUS, IOCM_EEPROM_ADDR, IOCM_EEPROM_DRIVER_NAME);
      if (ret < 0) {
        syslog(LOG_WARNING, "%s() Failed to bind i2c device on bus: %u, addr: 0x%X, device: %s, ret : %d\n",
          __func__, I2C_T5E1S1_T7IOC_BUS, IOCM_EEPROM_ADDR, IOCM_EEPROM_DRIVER_NAME, ret);
        syslog(LOG_WARNING, "%s() Will not get IOCM FRU information", __func__);
        return -2;
      }
    }

    snprintf(path, sizeof(path), EEPROM_PATH, I2C_T5E1S1_T7IOC_BUS, IOCM_FRU_ADDR);
    if (pal_copy_eeprom_to_bin(path, FRU_IOCM_BIN) < 0) {
      syslog(LOG_WARNING, "%s() Failed to copy %s to %s", __func__, path, FRU_IOCM_BIN);
      return -2;
    }
  }

  return 0;
}

int
pal_get_server_12v_power(uint8_t *status) {
  int ret = 0;
  uint8_t status_12v = 0;
  
  if (status == NULL) {
    syslog(LOG_WARNING, "%s() NULL pointer: *status", __func__);
    return -1;
  }

  ret = fbgc_common_server_stby_pwr_sts(&status_12v);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: fbgc_common_server_stby_pwr_sts failed\n", __func__);
    return ret;
  }
  
  if (status_12v == STAT_AC_ON) {
    *status = SERVER_12V_ON;
  } else {
    *status = SERVER_12V_OFF;
  }

  return ret;
}

int
pal_get_server_power(uint8_t fru, uint8_t *status) {
  int ret = 0;
  
  if (status == NULL) {
    syslog(LOG_WARNING, "%s() NULL pointer: *status", __func__);
    return -1;
  }

  if (fru != FRU_SERVER) {
    syslog(LOG_WARNING, "%s: fru %u not a server\n", __func__, fru);
    return POWER_STATUS_FRU_ERR;
  }

  ret = pal_get_server_12v_power(status);
  if (ret < 0 || (*status) == SERVER_12V_OFF) {
    // return earlier if power state is SERVER_12V_OFF or error happened
    return ret;
  }

  ret = bic_get_server_power_status(status);
  if (ret < 0) {
    // if bic not responding, we reset status to SERVER_12V_ON
    *status = SERVER_12V_ON;
  }

  return ret;
}

// Host DC Off, Host DC On, or Host Reset the server
int
pal_set_server_power(uint8_t fru, uint8_t cmd) {
  uint8_t status = 0;
  int ret = 0;

  if (fru != FRU_SERVER) {
    syslog(LOG_WARNING, "%s: fru %u not a server\n", __func__, fru);
    return POWER_STATUS_FRU_ERR;
  }
  
  if (pal_get_server_power(fru, &status) < 0) {
    syslog(LOG_WARNING, "%s: fru %u get sever power error %u\n", __func__, fru, status);
    return POWER_STATUS_ERR;
  }

  // Discard all the non-12V power control commands if 12V is off
  switch (cmd) {
    case SERVER_12V_OFF:
    case SERVER_12V_ON:
    case SERVER_12V_CYCLE:
      //do nothing
      break;
    default:
      if (status == SERVER_12V_OFF) {
        // discard the commands if power state is 12V-off
        return POWER_STATUS_ERR;
      }
      break;
  }

  switch(cmd) {
    case SERVER_POWER_ON:
      if (status == SERVER_POWER_ON) {
        return POWER_STATUS_ALREADY_OK;
      }
      ret = power_on_pre_actions();
      if (ret < 0) {
        return POWER_STATUS_ERR;
      }
      ret = bic_server_power_ctrl(SET_DC_POWER_ON);
      if (ret == 0) {
        power_on_post_actions();
      }
      return ret;
      
    case SERVER_POWER_OFF:
      if (status == SERVER_POWER_OFF) {
        return POWER_STATUS_ALREADY_OK;
      }
      if (power_off_pre_actions() < 0 ) {
        return POWER_STATUS_ERR;
      }
      ret = bic_server_power_ctrl(SET_DC_POWER_OFF);
      if (ret == 0) {
        power_off_post_actions();
      }
      return ret;

    case SERVER_POWER_CYCLE:
      if (status == SERVER_POWER_ON) {
        if (power_off_pre_actions() < 0 ) {
          return POWER_STATUS_ERR;
        }
        ret = bic_server_power_cycle();
        if (ret == 0) {
          power_on_post_actions();
        }
      }
      ret = bic_server_power_ctrl(SET_DC_POWER_ON);
      if (ret == 0) {
        power_on_post_actions();
      }
      return ret;

    case SERVER_GRACEFUL_SHUTDOWN:
      if (status == SERVER_POWER_OFF) {
        return POWER_STATUS_ALREADY_OK;
      }
      if (power_off_pre_actions() < 0 ) {
        return POWER_STATUS_ERR;
      }
      ret = bic_server_power_ctrl(SET_GRACEFUL_POWER_OFF);
      if (ret == 0) {
        power_off_post_actions();
      }
      return ret;

    case SERVER_POWER_RESET:
      if (status == SERVER_POWER_OFF) {
        return POWER_STATUS_ERR;
      }
      return bic_server_power_ctrl(SET_HOST_RESET);

    case SERVER_12V_ON:
      if (status == SERVER_12V_ON) {
        return POWER_STATUS_ALREADY_OK;
      }
      return server_power_12v_on();

    case SERVER_12V_OFF:
      if (status == SERVER_12V_OFF) {
        return POWER_STATUS_ALREADY_OK;
      }
      ret = server_power_12v_off();
      if (ret == 0) {
        power_off_post_actions();
      }
      return ret;

    case SERVER_12V_CYCLE:
      if (status == SERVER_12V_OFF) {
        return server_power_12v_on();
      } else {
        if (server_power_12v_off() < 0) {
          return POWER_STATUS_ERR;
        }
        sleep(SERVER_AC_CYCLE_DELAY);
        if (server_power_12v_on() < 0) {
          return POWER_STATUS_ERR;
        }
      }
      break;

    default:
      syslog(LOG_WARNING, "%s() wrong cmd: 0x%02X", __func__, cmd);
      return POWER_STATUS_ERR;
  }

  return POWER_STATUS_OK;
}

int
pal_sled_cycle(void) {
  int ret = 0;
  
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rlen = 0, tlen = 0;

  ret = expander_ipmb_wrapper(NETFN_OEM_REQ, CME_OEM_EXP_SLED_CYCLE, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() expander_ipmb_wrapper failed. ret: %d\n", __func__, ret);
    return PAL_ENOTSUP;
  }

  return POWER_STATUS_OK;
}

int
pal_set_last_pwr_state(uint8_t fru, char *state) {
  int ret = 0;
  
  if (state == NULL) {
    syslog(LOG_WARNING, "%s() NULL pointer: *state", __func__);
    return -1;
  }

  ret = pal_set_key_value("pwr_server_last_state", state);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: pal_set_key_value failed", __func__);
  }

  return ret;
}

int
pal_get_last_pwr_state(uint8_t fru, char *state) {
  int ret = 0;
  
  if (state == NULL) {
    syslog(LOG_WARNING, "%s() NULL pointer: *state", __func__);
    return -1;
  }

  ret = pal_get_key_value("pwr_server_last_state", state);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: pal_get_key_value failed", __func__);
  }

  return ret;
}
