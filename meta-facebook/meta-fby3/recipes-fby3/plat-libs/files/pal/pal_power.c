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

#define CPLD_PWR_CTRL_BUS 12
#define CPLD_PWR_CTRL_ADDR 0x1F

#define REG_CWC_CPLD_CWC_HSC 0x01
#define REG_CWC_CPLD_GPV3_HSC 0x02
#define REG_CWC_CPLD_CARD_PWROK 0x0C
#define DELAY_POWER_CYCLE 5 //sec
#define MAX_POWER_CYCLE_DWELL 20  //sec
#define DELAY_EXP_POWER 2 //sec
#define MAX_EXP_POWER_DELAY 6  //sec
#define MAX_PWR_RETRY 3

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
  int ret = 0;
  uint8_t bmc_location = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return POWER_STATUS_ERR;
  }
  return bic_server_power_off(fru);
}

// Power On the server
static int
server_power_on(uint8_t fru) {
  int ret = 0;
  uint8_t bmc_location = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return POWER_STATUS_ERR;
  }
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
  uint8_t rbuf[2] = {0};
  uint8_t rlen = 0;
  int ret = 0, retry= 0;

  i2cfd = i2c_cdev_slave_open(CPLD_PWR_CTRL_BUS, CPLD_PWR_CTRL_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %d", __func__, CPLD_PWR_CTRL_BUS);
    goto error_exit;
  }

  tbuf[0] = 0x09 + (fru-1);
  tlen = 1;
  rlen = 1;
  while (retry < MAX_READ_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_PWR_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      break;
    }
  }
  if (retry == MAX_READ_RETRY) {
    syslog(LOG_WARNING, "%s()%d Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, __LINE__, tlen);
    goto error_exit;
  }

  if (rbuf[0] == AC_ON) {
    tbuf[1] = AC_OFF;
    tlen = 2;
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
      syslog(LOG_WARNING, "%s()%d Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, __LINE__, tlen);
      goto error_exit;
    }
    sleep(2);
  }

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

  // vr cached info was removed when 12v_off was performed
  // we generate it again to avoid accessing VR devices at the same time.
  snprintf(cmd, sizeof(cmd), "/usr/bin/fw-util slot%d --version vr > /dev/null 2>&1", fru);
  if (system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
    ret = PAL_ENOTSUP;
    goto error_exit;
  }

  // SiC45X setting on 1/2ou was set in runtime
  // it was lost when 12v_off was performed,
  // need to reconfigure it again
  snprintf(cmd, sizeof(cmd), "/etc/init.d/setup-sic.sh slot%d > /dev/null 2>&1", fru);
  if (system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
    ret = PAL_ENOTSUP;
    goto error_exit;
  }

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

  ret = fby3_common_set_fru_i2c_isolated(fru, GPIO_VALUE_LOW);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to disable the i2c of fru%d", __func__, fru);
    goto error_exit;
  }

  sleep(1);

  i2cfd = i2c_cdev_slave_open(CPLD_PWR_CTRL_BUS, CPLD_PWR_CTRL_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %d", __func__, CPLD_PWR_CTRL_BUS);
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
pal_server_set_nic_power(const uint8_t expected_pwr) {
  int i = 0;
  int ret = 0;
  uint8_t svr_sts = 0;
  uint8_t pwr_sts = 0;

  for ( i = FRU_SLOT1; i <= FRU_SLOT4; i++ ) {
    ret = fby3_common_is_fru_prsnt(i, &svr_sts);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s(): Failed to get the prsnt sts. fru%d", __FUNCTION__, i);
      return PAL_ENOTSUP;
    }

    //it's prsnt
    if ( svr_sts != 0 ) {
      //read slot/12V pwr
      switch (expected_pwr) {
        case SERVER_POWER_ON:
        case SERVER_POWER_OFF:
          ret = pal_get_server_12v_power(i, &pwr_sts);
          if ( ret == PAL_EOK && pwr_sts == SERVER_12V_ON ) {
            ret = bic_get_server_power_status(i, &pwr_sts);
            if ( ret == PAL_EOK && pwr_sts == SERVER_POWER_ON ) {
              return PAL_ENOTREADY;
            }
          }

          if ( ret < 0 ) {
            if (expected_pwr == SERVER_POWER_ON) {
              syslog(LOG_WARNING, "%s(SERVER_POWER_ON): Failed to get the pwr sts. fru%d", __FUNCTION__, i);
            } else if (expected_pwr == SERVER_POWER_OFF) {
              syslog(LOG_WARNING, "%s(SERVER_POWER_OFF): Failed to get the pwr sts. fru%d", __FUNCTION__, i);
              return PAL_ENOTSUP;
            }
          }
          break;

        default:
          return PAL_ENOTSUP;
      }
    }
  }

  //change the power mode of NIC to expected_pwr
  int fd = -1;
  uint8_t tbuf[2] = {0x0f, (expected_pwr&0x1)};
  uint8_t tlen = 1;
  uint8_t rbuf[1];
  uint8_t rlen = 1;
  int pid_file = 0;

  if ( SERVER_POWER_ON == expected_pwr ) {
    pid_file = open(SET_NIC_PWR_MODE_LOCK, O_CREAT | O_RDWR, 0666);
  }

  fd = i2c_cdev_slave_open(CPLD_PWR_CTRL_BUS, CPLD_PWR_CTRL_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( fd < 0 ) {
    syslog(LOG_WARNING, "Failed to open bus 12");
    return PAL_ENOTSUP;
  }

  ret = i2c_rdwr_msg_transfer(fd, CPLD_PWR_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
  if ( ret < 0) {
    syslog(LOG_WARNING, "Failed to get NIC Power mode in CPLD");
  } else {
    // change the power mode of NIC when orginal mode in cpld is different
    if (rbuf[0] != expected_pwr) {
      tlen = 2;
      ret = i2c_rdwr_msg_transfer(fd, CPLD_PWR_CTRL_ADDR, tbuf, tlen, NULL, 0);
      if ( ret < 0 ) {
        syslog(LOG_WARNING, "Failed to change NIC Power mode");
      } else {
        //if one of them want to wake up, we need to set it and sleep 2s to wait for PERST#
        //2s is enough for CPLD
        if ( SERVER_POWER_ON == expected_pwr ) {
          syslog(LOG_CRIT, "NIC Power is set to VMAIN");
          sleep(2);
        } else if ( SERVER_POWER_OFF == expected_pwr ) {
          syslog(LOG_CRIT, "NIC Power is set to VAUX");
        }
      }
    }
  }

  if ( pid_file > 0 ) {
    close(pid_file);
    remove(SET_NIC_PWR_MODE_LOCK);
  }
  if ( fd > 0 ) close(fd);

  return (ret < 0)?PAL_ENOTSUP:PAL_EOK;
}

int
pal_get_server_power(uint8_t fru, uint8_t *status) {
  int ret;

  if (fru == FRU_CWC || fru == FRU_2U_TOP || fru == FRU_2U_BOT) {
    if (pal_is_cwc() == PAL_EOK)
      return pal_get_exp_power(fru, status);
    else
      return POWER_STATUS_FRU_ERR;
  }

  ret = fby3_common_check_slot_id(fru);
  if ( ret < 0 ) {
    return POWER_STATUS_FRU_ERR;
  }

  ret = pal_get_server_12v_power(fru, status);
  if ( ret < 0 || (*status) == SERVER_12V_OFF ) {
    // return earlier if power state is SERVER_12V_OFF or error happened
    return ret;
  }

  if (bic_get_server_power_status(fru, status) < 0) {
    // if bic not responsing, we reset status to SERVER_12V_ON
    *status = SERVER_12V_ON;
  }

  return ret;
}

// Power Off, Power On, or Power Reset the server in given slot
int
pal_set_server_power(uint8_t fru, uint8_t cmd) {
  uint8_t status;
  int ret = 0;
  uint8_t bmc_location = 0;

  if (fru == FRU_CWC || fru == FRU_2U_TOP || fru == FRU_2U_BOT) {
    if (pal_is_cwc() == PAL_EOK)
      return pal_set_exp_power(fru, cmd);
    else
      return POWER_STATUS_FRU_ERR;
  }

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
      if (status == SERVER_12V_OFF) {
        // discard the commands if power state is 12V-off
        return POWER_STATUS_FRU_ERR;
      }
      break;
  }

  switch(cmd) {
    case SERVER_POWER_ON:
      if ( status == SERVER_POWER_ON ) return POWER_STATUS_ALREADY_OK;
      if ( bmc_location != NIC_BMC) {
        ret = pal_server_set_nic_power(SERVER_POWER_ON); //only check it on class 1
        if (ret == PAL_ENOTSUP) {
          syslog(LOG_WARNING,"%s(SERVER_POWER_ON): fru%d pal_server_set_nic_power fail", __FUNCTION__, fru);
          return POWER_STATUS_ERR;
        }
      } else ret = PAL_EOK;

      return server_power_on(fru);
      break;

    case SERVER_POWER_OFF:
      return (status == SERVER_POWER_OFF)?POWER_STATUS_ALREADY_OK:server_power_off(fru);
      break;

    case SERVER_POWER_CYCLE:
      if (status == SERVER_POWER_ON) {
        return bic_server_power_cycle(fru);
      } else if (status == SERVER_POWER_OFF) {
        if ( bmc_location != NIC_BMC) {
          ret = pal_server_set_nic_power(SERVER_POWER_ON); //only check it on class 1
        } else ret = PAL_EOK;
        return (ret == PAL_ENOTSUP)?POWER_STATUS_ERR:server_power_on(fru);
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
      if (status == SERVER_12V_ON) {
        pal_power_policy_control(fru, NULL);
        return POWER_STATUS_ALREADY_OK;
      } else {
        return server_power_12v_on(fru);
      }

      break;

    case SERVER_12V_OFF:
      if ( bmc_location == NIC_BMC || pal_get_server_12v_power(fru, &status) < 0 ) {
        return POWER_STATUS_ERR;
      }
      if ( status == SERVER_12V_OFF ) return POWER_STATUS_ALREADY_OK;
      return server_power_12v_off(fru);
      break;

    case SERVER_12V_CYCLE:
      if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
        if ( pal_get_server_12v_power(fru, &status) < 0 ) {
          return POWER_STATUS_ERR;
        }

        if (status == SERVER_12V_OFF) {
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
        if ((bmc_location == NIC_BMC) && (true == pal_is_fw_update_ongoing_system()) ) {
          printf("Please make sure no firmware update is ongoing\n");
          return POWER_STATUS_ERR;
        }
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
  int ret = PAL_EOK;
  uint8_t bmc_location = 0;
  uint8_t hsc_det = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return POWER_STATUS_ERR;
  }

  if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
    if ( fby3_common_get_hsc_bb_detect(&hsc_det) ) {
      return -1;
    }
    switch (hsc_det) {
      case HSC_DET_ADM1278:
        ret = system("i2cset -y 11 0x40 0xd9 c &> /dev/null");
        break;
      case HSC_DET_LTC4282:
        ret = system("i2cset -y -f 11 0x40 0x1d 0x80 b &> /dev/null");
        break;
      case HSC_DET_MP5990:
        ret = system("i2cset -y 11 0x40 0xf c &> /dev/null");
        break;
      case HSC_DET_ADM1276:
        ret = system("i2cset -y 11 0x20 0xd9 c &> /dev/null");
        break;
      default:
        syslog(LOG_WARNING, "%s Invalid HSC detection: %u\n", __func__, hsc_det);
        return -1;
    }
  } else {
    // check power lock flag
    if ( bic_is_crit_act_ongoing(FRU_SLOT1) == true ) {
      printf("power lock flag is asserted, please make sure no firmware update is ongoing\n");
      printf("If you still want to proceed, please clean the flag manually!\n");
      syslog(LOG_CRIT, "SLED_CYCLE failed because a critical activity is ongoing\n");
      return PAL_ENOTSUP;
    }

    if ( bic_inform_sled_cycle() < 0 ) {
      syslog(LOG_WARNING, "Inform another BMC for sled cycle failed.\n");
    }
    // Provide the time for inform another BMC
    sleep(2);
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
      ret = -1;
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

  if (pal_is_cwc() == PAL_EOK && 
      (fru == FRU_CWC || fru == FRU_2U_TOP || fru == FRU_2U_BOT)) {
    sprintf(key, "pwr_server%d_last_state", FRU_SLOT1);
  } else {
    sprintf(key, "pwr_server%d_last_state", (int) fru);
  }

  ret = pal_get_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_get_last_pwr_state: pal_get_key_value failed for "
        "fru %u", fru);
#endif
  }

  return ret;
}

static int
pal_is_valid_expansion_dev(uint8_t slot_id, uint8_t dev_id, uint8_t *rsp) {
  const uint8_t st_2ou_idx = DEV_ID0_2OU;
  const uint8_t end_2ou_yv3_idx = DEV_ID5_2OU;
  const uint8_t end_2ou_gpv3_idx = DEV_ID13_2OU;
  int ret = 0;
  int config_status = 0;
  uint8_t bmc_location = 0;

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return POWER_STATUS_ERR;
  }

  config_status = bic_is_m2_exp_prsnt(slot_id);
  if (config_status < 0) {
    return POWER_STATUS_FRU_ERR;
  }

  // Is 1OU dev?
  if ( (dev_id < st_2ou_idx) && ((config_status & PRESENT_1OU) == PRESENT_1OU) ) {
    if ( bmc_location == NIC_BMC ) return POWER_STATUS_FRU_ERR;
    rsp[0] = dev_id;
    rsp[1] = FEXP_BIC_INTF;
  } else if (( dev_id >= st_2ou_idx && dev_id <= end_2ou_gpv3_idx) && ((config_status & PRESENT_2OU) == PRESENT_2OU)) {
    uint8_t type = 0xff;
    if ( fby3_common_get_2ou_board_type(slot_id, &type) < 0 ) {
      return POWER_STATUS_FRU_ERR;
    } else if ( type != GPV3_MCHP_BOARD && type != GPV3_BRCM_BOARD ) {
      //If dev doesn't belong to GPv3, return
      if ( dev_id > end_2ou_yv3_idx ) return POWER_STATUS_FRU_ERR;
    }
    rsp[0] = dev_id - 4;
    rsp[1] = REXP_BIC_INTF;
  } else if (( dev_id >= DUAL_DEV_ID0_2OU && dev_id <= DUAL_DEV_ID5_2OU) && ((config_status & PRESENT_2OU) == PRESENT_2OU)) {
    uint8_t type = 0xff;
    if ( fby3_common_get_2ou_board_type(slot_id, &type) < 0 ) {
      return POWER_STATUS_FRU_ERR;
    } else if ( type != GPV3_MCHP_BOARD && type != GPV3_BRCM_BOARD ) {
      //If dev doesn't belong to GPv3, return
      if ( dev_id > end_2ou_yv3_idx ) return POWER_STATUS_FRU_ERR;
    }
    rsp[0] = dev_id;
    rsp[1] = REXP_BIC_INTF;
  } else {
    // dev not found
    return POWER_STATUS_FRU_ERR;
  }

  return POWER_STATUS_OK;
}

int
pal_get_exp_device_power(uint8_t exp, uint8_t dev_id, uint8_t *status, uint8_t *type) {
  uint8_t sta = 0;
  int ret = 0;
  uint8_t nvme_ready = 0;
  uint8_t intf = 0;

  if (pal_get_exp_power(exp, &sta)) {
    return POWER_STATUS_ERR;
  }

  if (sta == SERVER_12V_OFF) {
    *status = DEVICE_POWER_OFF;
    syslog(LOG_WARNING, "pal_get_exp_device_power: pal_get_exp_power 12V-off");
    return 0;
  }

  switch (exp) {
    case FRU_2U_TOP:
      if (dev_id >= DEV_ID0_2OU && dev_id <= DEV_ID13_2OU) {
        dev_id -= 4;
        intf = RREXP_BIC_INTF1;
      } else if (dev_id >= DUAL_DEV_ID0_2OU && dev_id <= DUAL_DEV_ID5_2OU) {
        intf = RREXP_BIC_INTF1;
      } else {
        printf("Device not found \n");
        return POWER_STATUS_FRU_ERR;
      }
      break;
    case FRU_2U_BOT:
      if (dev_id >= DEV_ID0_2OU && dev_id <= DEV_ID13_2OU) {
        dev_id -= 4;
        intf = RREXP_BIC_INTF2;
      } else if (dev_id >= DUAL_DEV_ID0_2OU && dev_id <= DUAL_DEV_ID5_2OU) {
        intf = RREXP_BIC_INTF2;
      } else {
        printf("Device not found \n");
        return POWER_STATUS_FRU_ERR;
      }
      break;

    default:
      return POWER_STATUS_FRU_ERR;
  }

  ret = bic_get_dev_power_status(FRU_SLOT1, dev_id, &nvme_ready, status, \
                                  NULL, NULL, NULL, NULL, NULL, NULL, intf);
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

int
pal_get_dual_power(uint8_t slot_id, uint8_t dev_id, uint8_t *status, uint8_t *type) {
  uint8_t dev0 = DEV_ID0_2OU + (dev_id - DUAL_DEV_ID0_2OU) * 2;
  uint8_t dev1 = DEV_ID0_2OU + (dev_id - DUAL_DEV_ID0_2OU) * 2 + 1;

  if (pal_get_device_power(slot_id, dev0, &status[0], type) < 0) {
      return -1;
  }
  if (pal_get_device_power(slot_id, dev1, &status[1], type) < 0) {
      return -1;
  }
  
  return 0;
}

int
pal_get_device_power(uint8_t slot_id, uint8_t dev_id, uint8_t *status, uint8_t *type) {
  int ret = 0;
  uint8_t nvme_ready = 0;
  uint8_t intf = 0;
  uint8_t rsp[2] = {0}; //idx0 = dev id, idx1 = intf

  if (dev_id >= DUAL_DEV_ID0_2OU && dev_id <= DUAL_DEV_ID5_2OU) {
    uint8_t dual_sta[2] = {0};
    if (pal_get_dual_power(slot_id, dev_id, dual_sta, type)) {
      return POWER_STATUS_ERR;
    }
    if (dual_sta[0] == DEVICE_POWER_ON && dual_sta[1] == DEVICE_POWER_ON) {
      *status = DEVICE_POWER_ON;
    } else {
      *status = DEVICE_POWER_OFF;
    }
    return 0;
  }

  if (pal_is_cwc() == PAL_EOK && (slot_id == FRU_2U_TOP || slot_id == FRU_2U_BOT)) {
    return pal_get_exp_device_power(slot_id, dev_id, status, type);
  }

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

    if ( pal_is_valid_expansion_dev(slot_id, dev_id, rsp) < 0 ) {
      printf("Device not found \n");
      return POWER_STATUS_FRU_ERR;
    }

    dev_id = rsp[0];
    intf = rsp[1];
    ret = bic_get_dev_power_status(slot_id, dev_id, &nvme_ready, status, \
                                   NULL, NULL, NULL, NULL, NULL, NULL, intf);
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
pal_set_exp_device_power(uint8_t exp, uint8_t dev_id, uint8_t cmd) {
  uint8_t sta[2] = {0}, type = 0;
  int ret = 0;
  uint8_t intf = 0;

  if (pal_get_exp_power(exp, sta)) {
    return POWER_STATUS_ERR;
  }

  if (sta[0] == SERVER_12V_OFF) {
    printf("Fru:%d is off \n", exp);
    return 0;
  }

  if (pal_get_exp_device_power(exp, dev_id, sta, &type)) {
    return POWER_STATUS_ERR;
  }

  switch (exp) {
    case FRU_2U_TOP:
      if (dev_id >= DEV_ID0_2OU && dev_id <= DEV_ID13_2OU) {
        dev_id -= 4;
        intf = RREXP_BIC_INTF1;
      } else if (dev_id >= DUAL_DEV_ID0_2OU && dev_id <= DUAL_DEV_ID5_2OU) {
        intf = RREXP_BIC_INTF1;
      } else {
        printf("Device not found \n");
        return POWER_STATUS_FRU_ERR;
      }
      break;
    case FRU_2U_BOT:
      if (dev_id >= DEV_ID0_2OU && dev_id <= DEV_ID13_2OU) {
        dev_id -= 4;
        intf = RREXP_BIC_INTF2;
      } else if (dev_id >= DUAL_DEV_ID0_2OU && dev_id <= DUAL_DEV_ID5_2OU) {
        intf = RREXP_BIC_INTF2;
      } else {
        printf("Device not found \n");
        return POWER_STATUS_FRU_ERR;
      }
      break;

    default:
      return POWER_STATUS_FRU_ERR;
  }

  if (dev_id >= DUAL_DEV_ID0_2OU && dev_id <= DUAL_DEV_ID5_2OU) {
    if (cmd == SERVER_POWER_ON) {
      if (sta[0] == DEVICE_POWER_ON && sta[1] == DEVICE_POWER_ON) {
        sta[0] = DEVICE_POWER_ON;
      } else {
        sta[0] = DEVICE_POWER_OFF;
      }
    } else {  //off or cycle
      if (sta[0] == DEVICE_POWER_OFF && sta[1] == DEVICE_POWER_OFF) {
        sta[0] = DEVICE_POWER_OFF;
      } else {
        sta[0] = DEVICE_POWER_ON;
      }
    }
  }

  switch(cmd) {
    case SERVER_POWER_ON:
      if (sta[0] == DEVICE_POWER_ON) {
        return 1;
      } else {
        ret = bic_set_dev_power_status(FRU_SLOT1, dev_id, DEVICE_POWER_ON, intf);
        return ret;
      }
      break;

    case SERVER_POWER_OFF:
      if (sta[0] == DEVICE_POWER_OFF) {
        return 1;
      } else {
        ret = bic_set_dev_power_status(FRU_SLOT1, dev_id, DEVICE_POWER_OFF, intf);
        return ret;
      }
      break;
    case SERVER_POWER_CYCLE:
      if (sta[0] == DEVICE_POWER_ON) {
        ret = bic_set_dev_power_status(FRU_SLOT1, dev_id, DEVICE_POWER_OFF, intf);
        if (ret < 0) {
          return -1;
        }

        sleep(3);

        ret = bic_set_dev_power_status(FRU_SLOT1, dev_id, DEVICE_POWER_ON, intf);
        return ret;
      } else if (sta[0] == DEVICE_POWER_OFF) {
        ret = bic_set_dev_power_status(FRU_SLOT1, dev_id, DEVICE_POWER_ON, intf);
        return ret;
      }
      break;

    default:
      return POWER_STATUS_ERR;
  }

  return 0;
}

int
pal_set_device_power(uint8_t slot_id, uint8_t dev_id, uint8_t cmd) {
  int ret;
  uint8_t status, type, type_1ou = 0;
  uint8_t intf = 0;
  uint8_t rsp[2] = {0}; //idx0 = dev id, idx1 = intf
  uint8_t dual_sta[2] = {0};

  if (pal_is_cwc() == PAL_EOK && (slot_id == FRU_2U_TOP || slot_id == FRU_2U_BOT)) {
    return pal_set_exp_device_power(slot_id, dev_id, cmd);
  }

  ret = fby3_common_check_slot_id(slot_id);
  if ( ret < 0 ) {
    return POWER_STATUS_FRU_ERR;
  }
  if (dev_id >= DUAL_DEV_ID0_2OU && dev_id <= DUAL_DEV_ID5_2OU) {
    if (pal_get_dual_power(slot_id, dev_id, dual_sta, &type) < 0) {
      return -1;
    }
  } else {
    if (pal_get_device_power(slot_id, dev_id, &status, &type) < 0) {
      return -1;
    }
  }

  if ( pal_is_valid_expansion_dev(slot_id, dev_id, rsp) < 0 ) {
    printf("Device not found \n");
    return POWER_STATUS_FRU_ERR;
  }

  dev_id = rsp[0];
  intf = rsp[1];

  if (dev_id >= DUAL_DEV_ID0_2OU && dev_id <= DUAL_DEV_ID5_2OU) {
    if (cmd == SERVER_POWER_ON) {
      if (dual_sta[0] == DEVICE_POWER_ON && dual_sta[1] == DEVICE_POWER_ON) {
        status = DEVICE_POWER_ON;
      } else {
        status = DEVICE_POWER_OFF;
      }
    } else {  //off or cycle
      if (dual_sta[0] == DEVICE_POWER_OFF && dual_sta[1] == DEVICE_POWER_OFF) {
        status = DEVICE_POWER_OFF;
      } else {
        status = DEVICE_POWER_ON;
      }
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

        if (intf == FEXP_BIC_INTF) {
          bic_get_1ou_type_cache(slot_id, &type_1ou);
        }
        if (type_1ou == EDSFF_1U) {
          sleep(6); // EDSFF timing requirement
        } else {
          sleep(3);
        }

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
  int ret = 0;
  uint8_t status = 0;
  uint8_t uart_select = 0x00;
  unsigned char *data = res_data;

  if(slot == 5) {   //handle the case, command sent from debug Card
    ret = pal_get_uart_select_from_kv(&uart_select);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() Failed to get debug_card_uart_select\n", __func__);
    }
    // if uart_select is BMC, the following code will return default data.
    slot = uart_select;
  }

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

static int
pal_slot_ac_cycle(uint8_t slot) {
  int ret, cmd_slot;
  pthread_t tid;

  if (m_slot_pwr_ctrl[slot] != false) {
    return CC_NOT_SUPP_IN_CURR_STATE;
  }

  m_slot_pwr_ctrl[slot] = true;
  cmd_slot = (SERVER_12V_CYCLE << 8) | slot;
  ret = pthread_create(&tid, NULL, slot_pwr_ctrl, (void *)cmd_slot);
  if (ret < 0) {
    syslog(LOG_WARNING, "[%s] Create slot_ac_cycle thread failed!", __func__);
    m_slot_pwr_ctrl[slot] = false;
    return CC_NODE_BUSY;
  }

  return CC_SUCCESS;
}

int
pal_sled_ac_cycle(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {
  uint8_t comp_code = CC_UNSPECIFIED_ERROR;
  uint8_t *data = req_data;

  if ( fby3_common_check_slot_id(slot) < 0 ) {
    return comp_code;
  }

  if ((*data != 0x55) || (*(data+1) != 0x66)) {
    return comp_code;
  }

  switch (*(data+2)) {
    case 0x0f:  //do slot ac cycle
      comp_code = pal_slot_ac_cycle(slot);
      break;
    default:
      comp_code = CC_INVALID_DATA_FIELD;
      break;
  }

  return comp_code;
}

int
pal_get_exp_power(uint8_t fru, uint8_t *status) {
  int i2cfd = BIC_STATUS_FAILURE;
  int ret = BIC_STATUS_FAILURE;
  int bus = 0;
  uint8_t tbuf = 0x0;
  uint8_t rbuf = 0x0;
  uint8_t tlen = 0x0;
  uint8_t rlen = 0x0;
  uint8_t retry = MAX_READ_RETRY;
  uint8_t mask = 0;

  switch (fru) {
    case FRU_CWC:
      bus = FRU_SLOT1 + SLOT_BUS_BASE;
      mask = 0x01;
      break;
    case FRU_2U_TOP:
      bus = FRU_SLOT1 + SLOT_BUS_BASE;
      mask = 0x02;
      break;
    case FRU_2U_BOT:
      bus = FRU_SLOT1 + SLOT_BUS_BASE;
      mask = 0x04;
      break;
    default:
      printf("unknown expantion fru : %d\n", fru);
      return POWER_STATUS_FRU_ERR;
  }

  i2cfd = i2c_cdev_slave_open(bus, CWC_CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open i2c device %d", __func__, CWC_CPLD_ADDRESS);
    return i2cfd;
  }

  tbuf = REG_CWC_CPLD_CARD_PWROK;
  tlen = 1;
  rlen = 1;
  do {
    ret = i2c_rdwr_msg_transfer(i2cfd, CWC_CPLD_ADDRESS, &tbuf, tlen, &rbuf, rlen);
    if ( ret < 0 ) {
      msleep(100);
    } else {
      break;
    }
  } while( retry-- > 0 );

  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
  } else {
    if ( rbuf & mask ) {
      *status = SERVER_12V_ON;
    } else {
      *status = SERVER_12V_OFF;
    }
  }

  if ( i2cfd > 0 ) {
    close(i2cfd);
  }
  return ret;
}

int
pal_get_gpv3_hsc(int bus, uint8_t *val) {
  int i2cfd = BIC_STATUS_FAILURE;
  int ret = BIC_STATUS_FAILURE;
  uint8_t tbuf = 0x0;
  uint8_t rbuf = 0x0;
  uint8_t tlen = 0x0;
  uint8_t rlen = 0x0;
  uint8_t retry = MAX_READ_RETRY;

  i2cfd = i2c_cdev_slave_open(bus, CWC_CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open i2c device %d", __func__, CWC_CPLD_ADDRESS);
    return i2cfd;
  }

  tbuf = REG_CWC_CPLD_GPV3_HSC;
  tlen = 1;
  rlen = 1;
  do {
    ret = i2c_rdwr_msg_transfer(i2cfd, CWC_CPLD_ADDRESS, &tbuf, tlen, &rbuf, rlen);
    if ( ret < 0 ) {
      msleep(100);
    } else {
      break;
    }
  } while( retry-- > 0 );

  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
  } else {
    *val = rbuf;
  }

  if ( i2cfd > 0 ) {
    close(i2cfd);
  }
  return ret;
}

int
pal_set_vr_interrupt(uint8_t fru, uint8_t enable) {
  switch (fru) {
    case FRU_CWC:
      if (bic_enable_vr_fault_monitor(FRU_SLOT1, enable ? true : false, REXP_BIC_INTF)) {
        syslog(LOG_WARNING, "%s() Failed to disable 2U-cwc vr interrupt", __func__);
      }
      if (bic_enable_vr_fault_monitor(FRU_SLOT1, enable ? true : false, RREXP_BIC_INTF1)) {
        syslog(LOG_WARNING, "%s() Failed to disable 2U-top vr interrupt", __func__);
      }
      if (bic_enable_vr_fault_monitor(FRU_SLOT1, enable ? true : false, RREXP_BIC_INTF2)) {
        syslog(LOG_WARNING, "%s() Failed to disable 2U-bot vr interrupt", __func__);
      }
      break;
    case FRU_2U_TOP:
      if (bic_enable_vr_fault_monitor(FRU_SLOT1, enable ? true : false, RREXP_BIC_INTF1)) {
        syslog(LOG_WARNING, "%s() Failed to disable 2U-top vr interrupt", __func__);
      }
      break;
    case FRU_2U_BOT:
      if (bic_enable_vr_fault_monitor(FRU_SLOT1, enable ? true : false, RREXP_BIC_INTF2)) {
        syslog(LOG_WARNING, "%s() Failed to disable 2U-bot vr interrupt", __func__);
      }
      break;
  }
  return 0;
}

int
pal_wait_exp_pwr(uint8_t fru, uint8_t pwr_status) {
  int res = MAX_EXP_POWER_DELAY;
  uint8_t status = 0;
  while (res > 0) {
    sleep(DELAY_EXP_POWER);
    res -= DELAY_EXP_POWER;

    if (pal_get_exp_power(fru, &status) < 0) {
      continue;
    }
    if (status == pwr_status) {
      return 0;
    }
  }
  return POWER_STATUS_ERR;
}

int
pal_set_exp_12v_on(uint8_t fru, bool pwr_on) {
  int i2cfd = BIC_STATUS_FAILURE;
  int ret = BIC_STATUS_FAILURE;
  int ina_alert_ret = BIC_STATUS_SUCCESS;
  int bus = 0;
  uint8_t tbuf[2] = {0x0};
  uint8_t rbuf = 0x0;
  uint8_t tlen = 0x02;
  uint8_t rlen = 0x0;
  uint8_t retry = MAX_READ_RETRY;
  uint8_t gpv3_hsc = 0;

  if (!pwr_on) {
    if (bic_enable_ina233_alert(fru, false)) {
      syslog(LOG_WARNING, "%s() Failed to disable INA233 Alert", __func__);
    }
  }

  switch (fru) {
    case FRU_CWC:
      bus = FRU_SLOT1 + SLOT_BUS_BASE;
      tbuf[0] = REG_CWC_CPLD_CWC_HSC;
      tbuf[1] = pwr_on ? 0x1 : 0x0;
      break;
    case FRU_2U_TOP:
      bus = FRU_SLOT1 + SLOT_BUS_BASE;
      tbuf[0] = REG_CWC_CPLD_GPV3_HSC;

      ina_alert_ret = pal_get_gpv3_hsc(bus, &gpv3_hsc);
      if ( ina_alert_ret < 0 ) {
        syslog(LOG_WARNING, "%s() Failed to get gpv3 hsc", __func__);
        goto error_exit;
      }

      tbuf[1] = pwr_on ? (gpv3_hsc | 0x01) : (gpv3_hsc & ~0x01);
      break;
    case FRU_2U_BOT:
      bus = FRU_SLOT1 + SLOT_BUS_BASE;
      tbuf[0] = REG_CWC_CPLD_GPV3_HSC;

      ina_alert_ret = pal_get_gpv3_hsc(bus, &gpv3_hsc);
      if ( ina_alert_ret < 0 ) {
        syslog(LOG_WARNING, "%s() Failed to get gpv3 hsc", __func__);
        goto error_exit;
      }

      tbuf[1] = pwr_on ? (gpv3_hsc | 0x02) : (gpv3_hsc & ~0x02);
      break;
    default:
      printf("unknown expantion fru : %d\n", fru);
      ina_alert_ret = POWER_STATUS_FRU_ERR;
      goto error_exit;
  }

  if (!pwr_on) {
    pal_set_vr_interrupt(fru, 0);
  }

  i2cfd = i2c_cdev_slave_open(bus, CWC_CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    ina_alert_ret = i2cfd;
    syslog(LOG_WARNING, "%s() Failed to open i2c device %d", __func__, CWC_CPLD_ADDRESS);
    goto error_exit;
  }

  do {
    ret = i2c_rdwr_msg_transfer(i2cfd, CWC_CPLD_ADDRESS, tbuf, tlen, &rbuf, rlen);
    if ( ret < 0 ) {
      msleep(100);
    } else {
      break;
    }
  } while( retry-- > 0 );

  if ( ret < 0 ) {
    ina_alert_ret = ret;
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);

    if (!pwr_on) {
      pal_set_vr_interrupt(fru, 1);
    }
  } else if (pwr_on) {
    msleep(100);
    pal_set_vr_interrupt(fru, 1);
  }

  if ( i2cfd > 0 ) {
    close(i2cfd);
  }

error_exit:
  if ( !pwr_on && ina_alert_ret < 0 ) {
    if (bic_enable_ina233_alert(fru, true)) {
      syslog(LOG_WARNING, "%s() Failed to disable INA233 Alert", __func__);
    }
  }
  return ina_alert_ret;
}

int
pal_set_exp_12v_check(uint8_t fru, bool pwr_on) {
  uint8_t retry = MAX_PWR_RETRY;
  for (; retry > 0; --retry) { 
    if (pal_set_exp_12v_on(fru, pwr_on)) {
      syslog(LOG_WARNING, "%s() Failed to power %s fru : %d", __func__, pwr_on ? "on" : "off", fru);
      return POWER_STATUS_ERR;
    }
    if (pal_wait_exp_pwr(fru, pwr_on ? SERVER_12V_ON : SERVER_12V_OFF) == 0) {
      break;
    }
  }
  return retry == 0 ? POWER_STATUS_ERR : 0;
}

int
pal_set_exp_12v_cycle(uint8_t fru) {
  if (pal_set_exp_12v_check(fru, false)) {
    return POWER_STATUS_ERR;
  }

  return pal_set_exp_12v_check(fru, true);
}

int
pal_set_exp_power(uint8_t fru, uint8_t cmd) {
  uint8_t status = 0, root = 0;

  if (pal_get_root_fru(fru, &root) != PAL_EOK) {
    return POWER_STATUS_ERR;
  }
  if (pal_is_fw_update_ongoing(root)) {
    printf("fw update is on going on fru:%d...\n", root);
    return POWER_STATUS_ERR;
  }
  if (pal_get_exp_power(fru, &status) < 0) {
    return POWER_STATUS_ERR;
  }

  switch (cmd) {
    case SERVER_12V_OFF:
      if (status == SERVER_12V_ON) {
        if (pal_set_exp_12v_check(fru, false)) {
          return POWER_STATUS_ERR;
        }
      } else {
        return 1;
      }
      break;
    case SERVER_12V_ON:
      if (status == SERVER_12V_OFF) {
        if (pal_set_exp_12v_check(fru, true)) {
          return POWER_STATUS_ERR;
        }
      } else {
        return 1;
      }
      break;
    case SERVER_12V_CYCLE:
      if (status == SERVER_12V_ON) {
        if (pal_set_exp_12v_cycle(fru)) {
          return POWER_STATUS_ERR;
        }
      } else {
        if (pal_set_exp_12v_check(fru, true)) {
          return POWER_STATUS_ERR;
        }
      }
      break;
    default:
      printf("command not supported for fru:%d\n", fru);
      return POWER_STATUS_ERR;
  }
  return 0;
}
