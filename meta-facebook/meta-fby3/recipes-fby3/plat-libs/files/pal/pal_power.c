#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <string.h>
#include <ctype.h>
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include "pal.h"

#define CPLD_PWR_CTRL_BUS "/dev/i2c-12"
#define CPLD_PWR_CTRL_ADDR 0x1F

enum {
  POWER_STATUS_ALREADY_OK = 1,
  POWER_STATUS_OK = 0,
  POWER_STATUS_ERR = -1,
  POWER_STATUS_FRU_ERR = -2,
};

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

static int
server_power_12v_on(uint8_t fru) {
  int i2cfd = 0;
  char cmd[64] = {0};
  uint8_t tbuf[2] = {0};
  uint8_t rbuf[2] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;

  i2cfd = open(CPLD_PWR_CTRL_BUS, O_RDWR);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %s", __func__, CPLD_PWR_CTRL_BUS);
    goto error_exit;
  }

  tbuf[0] = 0x0;
  tlen = 1;
  rlen = 1;
  ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_PWR_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    goto error_exit;
  }

  msleep(100);

  tbuf[0] = 0x0;
  tbuf[1] = (rbuf[0] | (0x1 << (fru-1)));
  tlen = 2;
  ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_PWR_CTRL_ADDR, tbuf, tlen, NULL, 0);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    goto error_exit;
  }

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
  
  ret = server_power_on(fru);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to do server_power_on fru%d", __func__, fru);
    goto error_exit;
  }

error_exit:
  if ( i2cfd > 0 ) close(i2cfd);

  return ret; 
}

static int
server_power_12v_off(uint8_t fru) {
  int i2cfd = 0;
  char cmd[64] = {0};
  uint8_t tbuf[2] = {0};
  uint8_t rbuf[2] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;

  i2cfd = open(CPLD_PWR_CTRL_BUS, O_RDWR);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %s", __func__, CPLD_PWR_CTRL_BUS);
    goto error_exit;
  }

  tbuf[0] = 0x0;
  tlen = 1;
  rlen = 1;
  ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_PWR_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer fails, tlen=%d", __func__, tlen);
    goto error_exit;
  }

  msleep(100);

  tbuf[0] = 0x0;
  tbuf[1] = (rbuf[0] &~(0x1 << (fru-1)));
  tlen = 2;
  ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_PWR_CTRL_ADDR, tbuf, tlen, NULL, 0);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer fails, tlen=%d", __func__, tlen);
    goto error_exit;
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

  ret = fby3_common_check_slot_id(fru);
  if ( ret < 0 ) {
    return POWER_STATUS_FRU_ERR;
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
      return bic_server_power_reset(fru);
      break;

    case SERVER_12V_ON:
      if ( pal_get_server_12v_power(fru, &status) < 0 ) {
        return POWER_STATUS_ERR;
      } 
      return (status == SERVER_POWER_ON)?POWER_STATUS_ALREADY_OK:server_power_12v_on(fru);  
      break;

    case SERVER_12V_OFF:
      if ( pal_get_server_12v_power(fru, &status) < 0 ) {
        return POWER_STATUS_ERR;
      }
      return (status == SERVER_POWER_OFF)?POWER_STATUS_ALREADY_OK:server_power_12v_off(fru);
      break;

    case SERVER_12V_CYCLE:
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
