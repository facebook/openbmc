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
#include "pal.h"

#define CPLD_PWR_CTRL_BUS "/dev/i2c-12"
#define CPLD_PWR_CTRL_ADDR 0x1F
#define MAX_READ_RETRY 5

enum {
  DEVICE_POWER_OFF = 0x0,
  DEVICE_POWER_ON = 0x1,
};

enum {
  DEV_TYPE_UNKNOWN,
  DEV_TYPE_M2,
};

enum {
  POWER_STATUS_ALREADY_OK = 1,
  POWER_STATUS_OK = 0,
  POWER_STATUS_ERR = -1,
  POWER_STATUS_FRU_ERR = -2,
};

static uint8_t m_slot_pwr_ctrl[MAX_NODES+1] = {0};

bool
is_server_off(void) {
  return POWER_STATUS_OK;
}

// Power Off the server
static int
server_power_off(uint8_t fru) {
  return bic_server_power_off(fru);
}

// Power On the server
static int
server_power_on(uint8_t fru) {
  return bic_server_power_on(fru);
}

static void
pal_power_policy_control(uint8_t slot, char *last_ps) {
  uint8_t chassis_sts[8] = {0};
  uint8_t chassis_sts_len;
  uint8_t power_policy = POWER_CFG_UKNOWN;
  char pwr_state[MAX_VALUE_LEN] = {0};

  //get power restore policy
  //defined by IPMI Spec/Section 28.2.
  pal_get_chassis_status(slot, NULL, chassis_sts, &chassis_sts_len);

  //byte[1], bit[6:5]: power restore policy
  power_policy = (*chassis_sts >> 5);

  //Check power policy and last power state
  if (power_policy == POWER_CFG_LPS) {
    if (!last_ps) {
      pal_get_last_pwr_state(slot, pwr_state);
      last_ps = pwr_state;
    }
    if (!(strcmp(last_ps, "on"))) {
      sleep(3);
      pal_set_server_power(slot, SERVER_POWER_ON);
    }
  }
  else if (power_policy == POWER_CFG_ON) {
    sleep(3);
    pal_set_server_power(slot, SERVER_POWER_ON);
  }
}

static int
server_power_12v_on(uint8_t fru) {
  int i2cfd = 0;
  char cmd[64] = {0};
  uint8_t tbuf[2] = {0};
  uint8_t tlen = 0;
  int ret = 0, retry= 0;

  i2cfd = open(CPLD_PWR_CTRL_BUS, O_RDWR);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %s", __func__, CPLD_PWR_CTRL_BUS);
    goto error_exit;
  }

  tbuf[0] = 0x09 + (fru-1);
  tbuf[1] = AC_ON;
  tlen = 2;
  retry = 0;
  while (retry < MAX_READ_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_PWR_CTRL_ADDR, tbuf, tlen, NULL, 0);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      break;
    }
  }
  if (retry == MAX_READ_RETRY) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    goto error_exit;
  }

  sleep(1);

  ret = fby3_common_set_fru_i2c_isolated(fru, GPIO_VALUE_HIGH);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to enable the i2c of fru%d", __func__, fru);
    goto error_exit;
  }

  sleep(1);

  snprintf(cmd, sizeof(cmd), "sv start ipmbd_%d > /dev/null 2>&1", fby3_common_get_bus_id(fru));
  if (system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
    ret = PAL_ENOTSUP;
    goto error_exit;
  }
  sleep(2);
  pal_power_policy_control(fru, NULL);

error_exit:
  if ( i2cfd > 0 ) close(i2cfd);

  return ret;
}

static int
server_power_12v_off(uint8_t fru) {
  int i2cfd = 0;
  char cmd[64] = {0};
  uint8_t tbuf[2] = {0};
  uint8_t tlen = 0;
  int ret = 0, retry= 0;
  
  snprintf(cmd, 64, "rm -f /tmp/cache_store/slot%d_vr*", fru);
  if (system(cmd) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
  }

  snprintf(cmd, sizeof(cmd), "sv stop ipmbd_%d > /dev/null 2>&1", fby3_common_get_bus_id(fru));
  if (system(cmd) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
      ret = PAL_ENOTSUP;
      goto error_exit;
  }

  ret = fby3_common_set_fru_i2c_isolated(fru, GPIO_VALUE_LOW);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to disable the i2c of fru%d", __func__, fru);
    goto error_exit;
  }

  sleep(1);

  i2cfd = open(CPLD_PWR_CTRL_BUS, O_RDWR);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %s", __func__, CPLD_PWR_CTRL_BUS);
    goto error_exit;
  }

  tbuf[0] = 0x09 + (fru-1);
  tbuf[1] = AC_OFF;
  tlen = 2;
  retry = 0;
  while (retry < MAX_READ_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_PWR_CTRL_ADDR, tbuf, tlen, NULL, 0);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      break;
    }
  }
  if (retry == MAX_READ_RETRY) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer fails, tlen=%d", __func__, tlen);
    goto error_exit;
  }

error_exit:
  if ( i2cfd > 0 ) close(i2cfd);

  return ret;
}

int
pal_get_server_12v_power(uint8_t slot_id, uint8_t *status) {
  int ret = 0;

  ret = fby3_common_server_stby_pwr_sts(slot_id, status);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s: Failed to run fby3_common_server_stby_pwr_sts on slot%d\n", __func__, slot_id);
  }

  if ( *status == 1 ) {
    *status = SERVER_12V_ON;
  } else {
    *status = SERVER_12V_OFF;
  }

  return ret;
}

int
pal_get_server_power(uint8_t fru, uint8_t *status) {
  int ret;

  ret = fby3_common_check_slot_id(fru);
  if ( ret < 0 ) {
    return POWER_STATUS_FRU_ERR;
  }

  ret = bic_get_server_power_status(fru, status);
  if ( ret == POWER_STATUS_OK ) {
    return ret;
  }

  return pal_get_server_12v_power(fru, status);
}

// Power Off, Power On, or Power Reset the server in given slot
int
pal_set_server_power(uint8_t fru, uint8_t cmd) {
  uint8_t status;
  int ret = 0;
  uint8_t bmc_location = 0;

  ret = fby3_common_check_slot_id(fru);
  if ( ret < 0 ) {
    return POWER_STATUS_FRU_ERR;
  }

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return POWER_STATUS_ERR;
  }

  switch (cmd) {
    case SERVER_12V_OFF:
    case SERVER_12V_ON:
    case SERVER_12V_CYCLE:
      //do nothing
      break;
    default:
      if (pal_get_server_power(fru, &status) < 0) {
        return POWER_STATUS_ERR;
      }
      break;
  }

  switch(cmd) {
    case SERVER_POWER_ON:
      return (status == SERVER_POWER_ON)?POWER_STATUS_ALREADY_OK:server_power_on(fru);
      break;

    case SERVER_POWER_OFF:
      return (status == SERVER_POWER_OFF)?POWER_STATUS_ALREADY_OK:server_power_off(fru);
      break;

    case SERVER_POWER_CYCLE:
      if (status == SERVER_POWER_ON) {
        return bic_server_power_cycle(fru);
      } else if (status == SERVER_POWER_OFF) {
        return server_power_on(fru);
      }
      break;

    case SERVER_GRACEFUL_SHUTDOWN:
      return (status == SERVER_POWER_OFF)?POWER_STATUS_ALREADY_OK:bic_server_graceful_power_off(fru);
      break;

    case SERVER_POWER_RESET:
      return (status == SERVER_POWER_OFF)?POWER_STATUS_ERR:bic_server_power_reset(fru);
      break;

    case SERVER_12V_ON:
      if ( bmc_location == NIC_BMC || pal_get_server_12v_power(fru, &status) < 0 ) {
        return POWER_STATUS_ERR;
      }
      return (status == SERVER_POWER_ON)?POWER_STATUS_ALREADY_OK:server_power_12v_on(fru);
      break;

    case SERVER_12V_OFF:
      if ( bmc_location == NIC_BMC || pal_get_server_12v_power(fru, &status) < 0 ) {
        return POWER_STATUS_ERR;
      }
      return (status == SERVER_POWER_OFF)?POWER_STATUS_ALREADY_OK:server_power_12v_off(fru);
      break;

    case SERVER_12V_CYCLE:
      if ( bmc_location == BB_BMC ) {
        if ( pal_get_server_12v_power(fru, &status) < 0 ) {
          return POWER_STATUS_ERR;
        }

        if (status == SERVER_POWER_OFF) {
          return server_power_12v_on(fru);
        } else {
          if ( server_power_12v_off(fru) < 0 ) {
            return POWER_STATUS_ERR;
          }
          sleep(2);
          if ( server_power_12v_on(fru) < 0 ) {
            return POWER_STATUS_ERR;
          }
        }
      } else {
        if ( bic_do_12V_cycle(fru) < 0 ) {
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
  int ret;
  uint8_t bmc_location = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return POWER_STATUS_ERR;
  }

  if ( bmc_location == BB_BMC ) {
    ret = system("i2cset -y 11 0x40 0xd9 c &> /dev/null");
  } else {
    int i = 0;
    int retries = 3;
    for (i = 0 ; i < retries; i++) {
      //BMC always sends the command with slot id 1 on class 2
      if ( bic_do_sled_cycle(1) < 0 ) {
        printf("Try to do the sled cycle...\n");
        msleep(100);
      }
    }

    if ( i == retries ) {
      printf("Failed to do the sled cycle. Please check the BIC is alive.\n");
    }
  }

  return ret;
}

int
pal_set_last_pwr_state(uint8_t fru, char *state) {

  int ret;
  char key[MAX_KEY_LEN] = {0};

  sprintf(key, "pwr_server%d_last_state", (int) fru);

  ret = pal_set_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_set_last_pwr_state: pal_set_key_value failed for "
        "fru %u", fru);
#endif
  }

  return ret;
}

int
pal_get_last_pwr_state(uint8_t fru, char *state) {

  int ret;
  char key[MAX_KEY_LEN] = {0};

  sprintf(key, "pwr_server%d_last_state", (int) fru);

  ret = pal_get_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_get_last_pwr_state: pal_get_key_value failed for "
        "fru %u", fru);
#endif
  }

  return ret;
}

int
pal_get_device_power(uint8_t slot_id, uint8_t dev_id, uint8_t *status, uint8_t *type) {
  int ret;
  uint8_t nvme_ready = 0;
  uint8_t intf = 0;
  uint8_t config_status = 0;
  uint8_t bmc_location = 0;

  if (fby3_common_check_slot_id(slot_id) == 0) {
    /* Check whether the system is 12V off or on */
    if ( pal_get_server_12v_power(slot_id, status) < 0 ) {
        return POWER_STATUS_ERR;
    }

    /* If 12V-off, return */
    if ((*status) == SERVER_12V_OFF) {
      *status = DEVICE_POWER_OFF;
      syslog(LOG_WARNING, "pal_get_device_power: pal_is_server_12v_on 12V-off");
      return 0;
    }

    ret = fby3_common_get_bmc_location(&bmc_location);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
      return POWER_STATUS_ERR;
    }

    config_status = bic_is_m2_exp_prsnt(slot_id);
    if (config_status < 0) {
      return POWER_STATUS_FRU_ERR;
    }

    if ( bmc_location == BB_BMC ) {
        if ((dev_id >= 1 && dev_id <= 4) && (config_status == 1 || config_status == 3)) { // 1OU Exp
          intf = FEXP_BIC_INTF;
        } else if ((dev_id >= 5 && dev_id <= 10) && ( config_status == 2 || config_status == 3)) { // 2OU Exp
          intf = REXP_BIC_INTF;
          dev_id = dev_id - 4; //shift to 2OU device id
        } else {
          printf("Device not found \n");
          return POWER_STATUS_FRU_ERR;
        }
    } else {
      if ((dev_id >= 5 && dev_id <= 10) && ( config_status == 2 || config_status == 3)) { // 2OU Exp
        intf = REXP_BIC_INTF;
        dev_id = dev_id - 4; //shift to 2OU device id
      } else {
        printf("Device not found \n");
        return POWER_STATUS_FRU_ERR;
      }
    }

    ret = bic_get_dev_power_status(slot_id, dev_id, &nvme_ready, status, intf);
    if (ret < 0) {
      return -1;
    }

    if (nvme_ready) {
      *type = DEV_TYPE_M2;
    } else {
      *type = DEV_TYPE_UNKNOWN;
    }
    return 0;
  }

  *type = DEV_TYPE_UNKNOWN;

  return -1;
}

int
pal_set_device_power(uint8_t slot_id, uint8_t dev_id, uint8_t cmd) {
  int ret;
  uint8_t status, type;
  uint8_t intf = 0;
  uint8_t config_status = 0;
  uint8_t bmc_location = 0;

  ret = fby3_common_check_slot_id(slot_id);
  if ( ret < 0 ) {
    return POWER_STATUS_FRU_ERR;
  }

  if (pal_get_device_power(slot_id, dev_id, &status, &type) < 0) {
    return -1;
  }

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return POWER_STATUS_ERR;
  }

  config_status = bic_is_m2_exp_prsnt(slot_id);
  if (config_status < 0) {
    return POWER_STATUS_FRU_ERR;
  }

  if ( bmc_location == BB_BMC ) {
      if ((dev_id >= 1 && dev_id <= 4) && (config_status == 1 || config_status == 3)) { // 1OU Exp
        intf = FEXP_BIC_INTF;
      } else if ((dev_id >= 5 && dev_id <= 10) && ( config_status == 2 || config_status == 3)) { // 2OU Exp
        intf = REXP_BIC_INTF;
        dev_id = dev_id - 4; //shift to 2OU device id
      } else {
        printf("Device not found \n");
        return POWER_STATUS_FRU_ERR;
      }
  } else {
    if ((dev_id >= 5 && dev_id <= 10) && ( config_status == 2 || config_status == 3)) { // 2OU Exp
      intf = REXP_BIC_INTF;
      dev_id = dev_id - 4; //shift to 2OU device id
    } else {
      printf("Device not found \n");
      return POWER_STATUS_FRU_ERR;
    }
  }

  switch(cmd) {
    case SERVER_POWER_ON:
      if (status == DEVICE_POWER_ON) {
        return 1;
      } else {
        ret = bic_set_dev_power_status(slot_id, dev_id, DEVICE_POWER_ON, intf);
        return ret;
      }
      break;

    case SERVER_POWER_OFF:
      if (status == DEVICE_POWER_OFF) {
        return 1;
      } else {
        ret = bic_set_dev_power_status(slot_id, dev_id, DEVICE_POWER_OFF, intf);
        return ret;
      }
      break;

    case SERVER_POWER_CYCLE:
      if (status == DEVICE_POWER_ON) {
        ret = bic_set_dev_power_status(slot_id, dev_id, DEVICE_POWER_OFF, intf);
        if (ret < 0) {
          return -1;
        }
        sleep(3);
        ret = bic_set_dev_power_status(slot_id, dev_id, DEVICE_POWER_ON, intf);
        return ret;
      } else if (status == DEVICE_POWER_OFF) {
        ret = bic_set_dev_power_status(slot_id, dev_id, DEVICE_POWER_ON, intf);
        return ret;
      }
      break;

    default:
      return -1;
  }

  return 0;
}

void
pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len) {
  char key[MAX_KEY_LEN];
  char buff[MAX_VALUE_LEN] = {0};
  int policy = 3;
  uint8_t status, ret;
  unsigned char *data = res_data;

  sprintf(key, "slot%u_por_cfg", slot);
  if (pal_get_key_value(key, buff) == 0) {
    if (!memcmp(buff, "off", strlen("off")))
      policy = 0;
    else if (!memcmp(buff, "lps", strlen("lps")))
      policy = 1;
    else if (!memcmp(buff, "on", strlen("on")))
      policy = 2;
    else
      policy = 3;
  }

  // Current Power State
  ret = pal_get_server_power(slot, &status);
  if (ret >= 0) {
    *data++ = status | (policy << 5);
  } else {
    syslog(LOG_WARNING, "pal_get_chassis_status: pal_get_server_power failed for slot%u", slot);
    *data++ = 0x00 | (policy << 5);
  }

  *data++ = 0x00;   // Last Power Event
  *data++ = 0x40;   // Misc. Chassis Status
  *data++ = 0x00;   // Front Panel Button Disable
  *res_len = data - res_data;
}

uint8_t
pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy, uint8_t *res_data) {
  uint8_t comp_code = CC_SUCCESS;
  uint8_t policy = *pwr_policy & 0x07;  // Power restore policy
  char key[MAX_KEY_LEN];

  sprintf(key, "slot%u_por_cfg", slot);
  switch (policy) {
    case 0:
      if (pal_set_key_value(key, "off")) {
        comp_code = CC_UNSPECIFIED_ERROR;
      }
      break;
    case 1:
      if (pal_set_key_value(key, "lps")) {
        comp_code = CC_UNSPECIFIED_ERROR;
      }
      break;
    case 2:
      if (pal_set_key_value(key, "on")) {
        comp_code = CC_UNSPECIFIED_ERROR;
      }
      break;
    case 3:
      // no change (just get present policy support)
      break;
    default:
      comp_code = CC_PARAM_OUT_OF_RANGE;
      break;
  }

  return comp_code;
}

// Do slot power control
static void * slot_pwr_ctrl(void *ptr) {
  uint8_t slot = (int)ptr & 0xFF;
  uint8_t cmd = ((int)ptr >> 8) & 0xFF;
  char pwr_state[MAX_VALUE_LEN] = {0};

  pthread_detach(pthread_self());
  msleep(500);

  if (cmd == SERVER_12V_CYCLE) {
    pal_get_last_pwr_state(slot, pwr_state);
  }

  if (!pal_set_server_power(slot, cmd)) {
    switch (cmd) {
      case SERVER_POWER_OFF:
        syslog(LOG_CRIT, "SERVER_POWER_OFF successful for FRU: %d", slot);
        break;
      case SERVER_POWER_ON:
        syslog(LOG_CRIT, "SERVER_POWER_ON successful for FRU: %d", slot);
        break;
      case SERVER_POWER_CYCLE:
        syslog(LOG_CRIT, "SERVER_POWER_CYCLE successful for FRU: %d", slot);
        break;
      case SERVER_POWER_RESET:
        syslog(LOG_CRIT, "SERVER_POWER_RESET successful for FRU: %d", slot);
        break;
      case SERVER_GRACEFUL_SHUTDOWN:
        syslog(LOG_CRIT, "SERVER_GRACEFUL_SHUTDOWN successful for FRU: %d", slot);
        break;
      case SERVER_12V_CYCLE:
        syslog(LOG_CRIT, "SERVER_12V_CYCLE successful for FRU: %d", slot);
        pal_power_policy_control(slot, pwr_state);
        break;
    }
  }

  m_slot_pwr_ctrl[slot] = false;
  pthread_exit(0);
}

int
pal_chassis_control(uint8_t slot, uint8_t *req_data, uint8_t req_len) {
  uint8_t comp_code = CC_SUCCESS, cmd = 0xFF;
  int ret, cmd_slot;
  pthread_t tid;

  if ((slot < FRU_SLOT1) || (slot > FRU_SLOT4)) {
    return CC_UNSPECIFIED_ERROR;
  }

  if (req_len != 1) {
    return CC_INVALID_LENGTH;
  }

  if (m_slot_pwr_ctrl[slot] != false) {
    return CC_NOT_SUPP_IN_CURR_STATE;
  }

  switch (req_data[0]) {
    case 0x00:  // power off
      cmd = SERVER_POWER_OFF;
      break;
    case 0x01:  // power on
      cmd = SERVER_POWER_ON;
      break;
    case 0x02:  // power cycle
      cmd = SERVER_POWER_CYCLE;
      break;
    case 0x03:  // power reset
      cmd = SERVER_POWER_RESET;
      break;
    case 0x05:  // graceful-shutdown
      cmd = SERVER_GRACEFUL_SHUTDOWN;
      break;
    default:
      comp_code = CC_INVALID_DATA_FIELD;
      break;
  }

  if (comp_code == CC_SUCCESS) {
    m_slot_pwr_ctrl[slot] = true;
    cmd_slot = (cmd << 8) | slot;
    ret = pthread_create(&tid, NULL, slot_pwr_ctrl, (void *)cmd_slot);
    if (ret < 0) {
      syslog(LOG_WARNING, "[%s] Create slot_pwr_ctrl thread failed!", __func__);
      m_slot_pwr_ctrl[slot] = false;
      return CC_NODE_BUSY;
    }
  }

  return comp_code;
}
