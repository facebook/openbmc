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
#include <openbmc/misc-utils.h>
#include "pal.h"

#define CPLD_PWR_CTRL_BUS 12
#define CPLD_PWR_CTRL_ADDR 0x1F
#define CPLD_PWR_OFF_BIC_REG 0x1E
#define CPLD_PWR_OFF_1OU_REG 0x18

#define DELAY_VDD_CORE_OFF 20

static uint8_t m_slot_pwr_ctrl[MAX_NODES+1] = {0};

static int
pal_set_VF_power_off(uint8_t fru) {
  int i2cfd = 0, bus = 0, ret = 0;
  uint8_t tbuf[2] = {0};
  char gpio_pin_name[128];
  const char *gpio_pin_names[] = {BIC_SRST_SHADOW, BIC_EXTRST_SHADOW};
  const int num_of_gpio_pins = ARRAY_SIZE(gpio_pin_names);
  gpio_desc_t *vf_gpio = NULL;

  for (int i = 0; i < num_of_gpio_pins; i++) {
    snprintf(gpio_pin_name, sizeof(gpio_pin_name), gpio_pin_names[i], fru);
    if ((vf_gpio = gpio_open_by_shadow(gpio_pin_name))) {
      gpio_set_init_value(vf_gpio, GPIO_VALUE_LOW);
      gpio_close(vf_gpio);
    }
  }

  bus = fby35_common_get_bus_id(fru) + 4;
  i2cfd = i2c_cdev_slave_open(bus, CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (i2cfd < 0) {
    printf("Failed to open CPLD 0x%x\n", CPLD_ADDRESS);
    return -1;
  }

  tbuf[0] = CPLD_PWR_OFF_1OU_REG;
  tbuf[1] = 0; // power off
  ret = retry_cond(!i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, 2, NULL, 0),
                   RETRY_TIME, 100);
  close(i2cfd);
  if (ret < 0) {
    syslog(LOG_CRIT, "%s(): Failed to do i2c_rdwr_msg_transfer\n", __func__);
  }

  //Sleep for 20 microsecond wait for 1OU STBY power off
  usleep(DELAY_VDD_CORE_OFF);

  return ret;
}

static void
reinit_vf_ioexp(uint8_t fru) {
  const char *ioexp_pin_names[] = {BIC_SRST_SHADOW, BIC_EXTRST_SHADOW, BIC_FWSPICK_SHADOW};
  char shadow_name[64];
  gpio_desc_t *vf_gpio = NULL;

  for (int i = 0; i < ARRAY_SIZE(ioexp_pin_names); i++) {
    snprintf(shadow_name, sizeof(shadow_name), ioexp_pin_names[i], fru);
    if ((vf_gpio = gpio_open_by_shadow(shadow_name))) {
      gpio_set_direction(vf_gpio, GPIO_DIRECTION_IN);
      gpio_close(vf_gpio);
    }
  }
}

// Power Off the server
static int
server_power_off(uint8_t fru) {
  int ret = 0;
  uint8_t bmc_location = 0;

  ret = fby35_common_get_bmc_location(&bmc_location);
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

  ret = fby35_common_get_bmc_location(&bmc_location);
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
  switch (power_policy) {
    case POWER_CFG_LPS:
      if (!last_ps) {
        pal_get_last_pwr_state(slot, pwr_state);
        last_ps = pwr_state;
      }
      if (strcmp(last_ps, "on") != 0) {
        //do nothing if last pw state is not on
        break;
      }
      // last pw state is on, do power on
    case POWER_CFG_ON:
      sleep(3);
      if (!bic_is_prot_bypass(slot)) {
        printf("waiting for PRoT AUTH_COMPLETE...\n");
        //wait AUTH_COMPLETE signal
        retry_cond(fby35_common_is_prot_auth_complete(slot), 120, 1000);
      }
      retry_cond(pal_set_server_power(slot, SERVER_FORCE_POWER_ON) != POWER_STATUS_ERR, 10, 500);
      break;
    default:
      //do nothing
      break;
  }
}

static int
server_power_12v_on(uint8_t fru) {
  int ret = 0, config_status = 0;
  int i2cfd = 0;
  uint8_t tbuf[4] = {0};
  uint8_t rbuf[4] = {0};
  uint8_t type = TYPE_1OU_UNKNOWN;

  i2cfd = i2c_cdev_slave_open(CPLD_PWR_CTRL_BUS, CPLD_PWR_CTRL_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %d", __func__, CPLD_PWR_CTRL_BUS);
    return -1;
  }

  do {
    tbuf[0] = 0x09 + (fru-1);

    // toggle HSC_EN when the HSC output was turned OFF
    // ex: when HSC_FAULT#, status == SERVER_12V_OFF, but HSC_EN == AC_ON
    ret = retry_cond(!i2c_rdwr_msg_transfer(i2cfd, CPLD_PWR_CTRL_ADDR, tbuf, 1, rbuf, 1),
                     MAX_READ_RETRY, 100);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to read HSC_EN", __func__);
      break;
    }

    if ( rbuf[0] == AC_ON ) {
      tbuf[1] = AC_OFF;
      ret = retry_cond(!i2c_rdwr_msg_transfer(i2cfd, CPLD_PWR_CTRL_ADDR, tbuf, 2, NULL, 0),
                       MAX_READ_RETRY, 100);
      if ( ret < 0 ) {
        syslog(LOG_WARNING, "%s() Failed to write HSC_EN %u", __func__, tbuf[1]);
        break;
      }

      // for the case of CPLD failure, although it shows 12V-off, it's actually 12V-on
      char cmd[64] = {0};
      snprintf(cmd, sizeof(cmd), FRU_ID_CPLD_NEW_VER_KEY, fru);
      kv_del(cmd, 0);
      sleep(DELAY_12V_CYCLE);
    }

    tbuf[1] = AC_ON;
    ret = retry_cond(!i2c_rdwr_msg_transfer(i2cfd, CPLD_PWR_CTRL_ADDR, tbuf, 2, NULL, 0),
                     MAX_READ_RETRY, 100);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to write HSC_EN %u", __func__, tbuf[1]);
      break;
    }
  } while (0);
  close(i2cfd);
  if ( ret < 0 ) {
    return -1;
  }

  sleep(1);
  ret = fby35_common_set_fru_i2c_isolated(fru, GPIO_VALUE_HIGH);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to enable the i2c of fru%d", __func__, fru);
    return ret;
  }

  sleep(1);
  if ((config_status = bic_is_exp_prsnt(fru)) < 0) {
    config_status = 0;
  }
  if (((config_status & PRESENT_1OU) == PRESENT_1OU) &&
      (bic_get_1ou_type(fru, &type) == 0) && (type == TYPE_1OU_VERNAL_FALLS_WITH_AST)) {
    reinit_vf_ioexp(fru);
  }

  pal_power_policy_control(fru, NULL);

  return ret;
}

static int
server_power_12v_off(uint8_t fru) {
  int ret = 0, config_status = 0;
  int i2cfd = 0;
  char cmd[64] = {0};
  uint8_t tbuf[2] = {0};
  uint8_t type = TYPE_1OU_UNKNOWN;

  if ((config_status = bic_is_exp_prsnt(fru)) < 0) {
    config_status = 0;
  }
  if (((config_status & PRESENT_1OU) == PRESENT_1OU) &&
      (bic_get_1ou_type(fru, &type) == 0) && (type == TYPE_1OU_VERNAL_FALLS_WITH_AST)) {
    pal_set_VF_power_off(fru);
  }

  pal_clear_vr_crc(fru);

  snprintf(cmd, sizeof(cmd), FRU_ID_COMPONENT_VER_KEY, fru, "prot");
  kv_del(cmd, 0);
  snprintf(cmd, sizeof(cmd), FRU_ID_CPLD_NEW_VER_KEY, fru);
  kv_del(cmd, 0);

  ret = fby35_common_set_fru_i2c_isolated(fru, GPIO_VALUE_LOW);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to disable the i2c of fru%d", __func__, fru);
    return -1;
  }

  sleep(1);
  i2cfd = i2c_cdev_slave_open(CPLD_PWR_CTRL_BUS, CPLD_PWR_CTRL_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (i2cfd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open %d", __func__, CPLD_PWR_CTRL_BUS);
    return -1;
  }

  tbuf[0] = 0x09 + (fru-1);
  tbuf[1] = AC_OFF;
  ret = retry_cond(!i2c_rdwr_msg_transfer(i2cfd, CPLD_PWR_CTRL_ADDR, tbuf, 2, NULL, 0),
                   MAX_READ_RETRY, 100);
  close(i2cfd);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to write HSC_EN %u", __func__, tbuf[1]);
    return -1;
  }

  return 0;
}

int
pal_get_server_12v_power(uint8_t slot_id, uint8_t *status) {
  int ret = 0;

  ret = fby35_common_server_stby_pwr_sts(slot_id, status);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s: Failed to run fby35_common_server_stby_pwr_sts on slot%d\n", __func__, slot_id);
  }

  if ( *status == 1 ) {
    *status = SERVER_12V_ON;
  } else {
    *status = SERVER_12V_OFF;
  }

  return ret;
}

static int
set_nic_pwr_en_time(void) {
  char value[MAX_VALUE_LEN];
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  snprintf(value, sizeof(value), "%lld", ts.tv_sec + 10);
  if (kv_set("nic_pwr", value, 0, 0) < 0) {
    return -1;
  }

  return 0;
}

static int
check_nic_pwr_en_time(void) {
  char value[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  if (kv_get("nic_pwr", value, NULL, 0)) {
     return 0;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (ts.tv_sec >= strtoul(value, NULL, 10)) {
     return 0;
  }

  return -1;
}

int
pal_server_set_nic_power(const uint8_t expected_pwr) {
  int ret = -1;
  int lockfd = -1, ifd = -1;
  uint8_t tbuf[4] = {0x0f, (expected_pwr&0x1),};
  uint8_t rbuf[4] = {0};

  do {
    if ( SERVER_POWER_ON == expected_pwr ) {
      lockfd = single_instance_lock_blocked("nic_pwr");
    } else {  // SERVER_POWER_OFF
      if ( (lockfd = single_instance_lock("nic_pwr")) < 0 ) {
        break;  // break since there is SERVER_POWER_ON processing
      }
    }

    ifd = i2c_cdev_slave_open(CPLD_PWR_CTRL_BUS, CPLD_PWR_CTRL_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
    if ( ifd < 0 ) {
      syslog(LOG_WARNING, "Failed to open bus %d", CPLD_PWR_CTRL_BUS);
      break;
    }

    if ( SERVER_POWER_ON == expected_pwr ) {
      set_nic_pwr_en_time();
      ret = retry_cond(!i2c_rdwr_msg_transfer(ifd, CPLD_PWR_CTRL_ADDR, tbuf, 1, rbuf, 1),
                       2, 100);
      if ( !ret && (rbuf[0] & 0x1) ) {  // nic power is already ON
        break;
      }
    } else if ( check_nic_pwr_en_time() != 0 ) {  // SERVER_POWER_OFF
      break;  // break since there is SERVER_POWER_ON processing
    }

    ret = retry_cond(!i2c_rdwr_msg_transfer(ifd, CPLD_PWR_CTRL_ADDR, tbuf, 2, NULL, 0),
                     2, 100);
    if ( lockfd >= 0 ) {
      single_instance_unlock(lockfd);
      lockfd = -1;
    }
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "Failed to change NIC Power mode");
      break;
    }

    if ( SERVER_POWER_ON == expected_pwr ) {
      sleep(2);  // wait 2s for PERST# when waking up
    }
  } while (0);

  if ( ifd >= 0 ) {
    close(ifd);
  }
  if ( lockfd >= 0 ) {
    single_instance_unlock(lockfd);
  }

  return (!ret)?PAL_EOK:PAL_ENOTSUP;
}

int
pal_get_server_power(uint8_t fru, uint8_t *status) {
  int ret;

  ret = fby35_common_check_slot_id(fru);
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

static int
pal_set_bic_power_off(int fru) {
  int i2cfd = 0, bus = 0, ret = 0;
  uint8_t tbuf[2] = {0};

  bus = fby35_common_get_bus_id(fru) + 4;
  i2cfd = i2c_cdev_slave_open(bus, CPLD_ADDRESS >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (i2cfd < 0) {
    printf("Failed to open CPLD 0x%x\n", CPLD_ADDRESS);
    return -1;
  }

  tbuf[0] = CPLD_PWR_OFF_BIC_REG;
  tbuf[1] = 0; // power off
  ret = retry_cond(!i2c_rdwr_msg_transfer(i2cfd, CPLD_ADDRESS, tbuf, 2, NULL, 0),
                   RETRY_TIME, 100);
  close(i2cfd);
  if (ret < 0) {
    syslog(LOG_CRIT, "%s(): Failed to do i2c_rdwr_msg_transfer\n", __func__);
  }

  return ret;
}

static bool
pal_is_fan_fail_otp(void) {
  char value[MAX_VALUE_LEN] = {0};

  if (kv_get("fan_fail_otp", value, NULL, 0)) {
    return false;
  }

  if (atoi(value) > 0) {
    return true;
  }

  return false;
}

// Power Off, Power On, or Power Reset the server in given slot
int
pal_set_server_power(uint8_t fru, uint8_t cmd) {
  uint8_t status;
  int ret = 0;
  uint8_t bmc_location = 0;
  char post_complete[MAX_KEY_LEN] = {0};
  bool force = false;

  ret = fby35_common_check_slot_id(fru);
  if ( ret < 0 ) {
    return POWER_STATUS_FRU_ERR;
  }

  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return POWER_STATUS_ERR;
  }

  snprintf(post_complete, sizeof(post_complete), POST_COMPLETE_STR, fru);

  switch (cmd) {
    case SERVER_12V_OFF:
    case SERVER_12V_CYCLE:
    case SERVER_12V_ON:
    case SERVER_FORCE_12V_ON:
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
    case SERVER_FORCE_POWER_ON:
      force = true;
    case SERVER_POWER_ON:
      if (status == SERVER_POWER_ON) {
        return POWER_STATUS_ALREADY_OK;
      }
      if (status != SERVER_POWER_OFF) {
        // unexpected status, BIC is probably unavailable
        return POWER_STATUS_ERR;
      }
      if (!force && pal_is_fan_fail_otp()) {
        printf("Failed to power fru %u to ON state due to fan fail...\n", fru);
        syslog(LOG_CRIT, "SERVER_POWER_ON failed for FRU: %u due to fan fail", fru);
        return POWER_STATUS_FAN_FAIL;
      }
      if (bmc_location != NIC_BMC) {
        if (pal_server_set_nic_power(SERVER_POWER_ON)) {
          return POWER_STATUS_ERR;
        }
      }
      if (server_power_on(fru) < 0) {
        return POWER_STATUS_ERR;
      }
      break;

    case SERVER_POWER_OFF:
      pal_set_key_value(post_complete, STR_VALUE_1);  // reset post complete value
      return (status == SERVER_POWER_OFF)?POWER_STATUS_ALREADY_OK:server_power_off(fru);
      break;

    case SERVER_POWER_CYCLE:
      pal_set_key_value(post_complete, STR_VALUE_1);  // reset post complete value
      if (status == SERVER_POWER_ON) {
        // assuming that user has force power on the fru, so fan_fail is not checked here
        return bic_server_power_cycle(fru);
      }
      if (status != SERVER_POWER_OFF) {
        // unexpected status, BIC is probably unavailable
        return POWER_STATUS_ERR;
      }
      if (pal_is_fan_fail_otp()) {
        printf("Failed to power cycle fru %u due to fan fail...\n", fru);
        syslog(LOG_CRIT, "SERVER_POWER_CYCLE failed for FRU: %u due to fan fail", fru);
        return POWER_STATUS_FAN_FAIL;
      }
      if (bmc_location != NIC_BMC) {
        if (pal_server_set_nic_power(SERVER_POWER_ON)) {
          return POWER_STATUS_ERR;
        }
      }
      if (server_power_on(fru) < 0) {
        return POWER_STATUS_ERR;
      }
      break;

    case SERVER_GRACEFUL_SHUTDOWN:
      pal_set_key_value(post_complete, STR_VALUE_1);  // reset post complete value
      return (status == SERVER_POWER_OFF)?POWER_STATUS_ALREADY_OK:bic_server_graceful_power_off(fru);
      break;

    case SERVER_POWER_RESET:
      pal_set_key_value(post_complete, STR_VALUE_1);  // reset post complete value
      return (status == SERVER_POWER_OFF)?POWER_STATUS_ERR:bic_server_power_reset(fru);
      break;

    case SERVER_FORCE_12V_ON:
      force = true;
    case SERVER_12V_ON:
      if (bmc_location == NIC_BMC || pal_get_server_12v_power(fru, &status) < 0) {
        return POWER_STATUS_ERR;
      }
      if (status == SERVER_12V_ON) {
        return POWER_STATUS_ALREADY_OK;
      }
      if (!force && pal_is_fan_fail_otp()) {
        printf("Failed to 12V Power fru %u to ON state due to fan fail...\n", fru);
        syslog(LOG_CRIT, "SERVER_12V_ON failed for FRU: %u due to fan fail", fru);
        return POWER_STATUS_FAN_FAIL;
      }
      if (server_power_12v_on(fru) < 0) {
        return POWER_STATUS_ERR;
      }
      break;

    case SERVER_12V_OFF:
      // Reset post complete value
      pal_set_key_value(post_complete, STR_VALUE_1);
      if ( bmc_location == NIC_BMC || pal_get_server_12v_power(fru, &status) < 0 ) {
        return POWER_STATUS_ERR;
      }

      if ( status == SERVER_12V_OFF ) return POWER_STATUS_ALREADY_OK;
      pal_set_bic_power_off(fru);
      return server_power_12v_off(fru);
      break;

    case SERVER_12V_CYCLE:
      if (bmc_location == BB_BMC) {
        if (pal_get_server_12v_power(fru, &status) < 0) {
          return POWER_STATUS_ERR;
        }
        if (status == SERVER_12V_ON) {
          // assuming that user has force 12V power on the fru, so fan_fail is not checked here
          pal_set_key_value(post_complete, STR_VALUE_1);  // reset post complete value
          pal_set_bic_power_off(fru);
          if (server_power_12v_off(fru) < 0) {
            return POWER_STATUS_ERR;
          }
          sleep(DELAY_12V_CYCLE);
          if (server_power_12v_on(fru) < 0) {
            return POWER_STATUS_ERR;
          }
        } else {
          if (pal_is_fan_fail_otp()) {
            printf("Failed to 12V Power cycle fru %u due to fan fail...\n", fru);
            syslog(LOG_CRIT, "SERVER_12V_CYCLE failed for FRU: %u due to fan fail", fru);
            return POWER_STATUS_FAN_FAIL;
          }
          if (server_power_12v_on(fru) < 0) {
            return POWER_STATUS_ERR;
          }
        }
      } else {
        pal_set_key_value(post_complete, STR_VALUE_1);  // reset post complete value
        if (bic_do_12V_cycle(fru) < 0) {
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
  int i = 0;
  uint8_t bmc_location = 0, is_fru_present = 0, status = 0;
  uint8_t tbuf[2] = {0};
  int i2cfd = 0;

  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return POWER_STATUS_ERR;
  }

  if (bmc_location == NIC_BMC) {
    if (bic_get_power_lock_status(&status) == 0 && (status != UNLOCK)) {
      printf("Another slot is doing fw update, cannot do sled cycle.\n");
      return POWER_STATUS_ERR;
    }
  }

  ret = system("sv stop sensord > /dev/null 2>&1 &");
  if ( ret < 0 ) {
    printf("Fail to stop sensord\n");
  }

  if ( bmc_location == BB_BMC ) {
    for (i = 1; i <= 4; i++) {
      ret = pal_is_fru_prsnt(i, &is_fru_present);
      if (ret < 0 || is_fru_present == 0) {
        continue;
      }
      ret = pal_get_server_12v_power(i, &status);
      if (ret < 0 || status == SERVER_12V_OFF) {
        continue;
      }
      pal_set_bic_power_off(i);
    }

    i2cfd = i2c_cdev_slave_open(CPLD_PWR_CTRL_BUS, CPLD_PWR_CTRL_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
    if ( i2cfd < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to open %d, error: %s", __func__, CPLD_PWR_CTRL_BUS, strerror(errno));
      return -1;
    }

    tbuf[0] = 0x2B;
    tbuf[1] = 0x01;
    ret = retry_cond(!i2c_rdwr_msg_transfer(i2cfd, CPLD_PWR_CTRL_ADDR, tbuf, 2, NULL, 0),
                     MAX_READ_RETRY, 100);
    close(i2cfd);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() Failed to do sled cycle", __func__);
    }
  } else {
    if ( bic_inform_sled_cycle() < 0 ) {
      syslog(LOG_WARNING, "Inform another BMC for sled cycle failed.\n");
    }
    // Provide the time for inform another BMC
    sleep(2);
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

  if ( system("sv start sensord > /dev/null 2>&1 &") < 0 ) {
    printf("Fail to start sensord\n");
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

static int
pal_is_valid_expansion_dev(uint8_t slot_id, uint8_t dev_id, uint8_t *rsp, int config_status, uint8_t board_type) {
  uint8_t bmc_location = 0;
  uint8_t prsnt = PRESENT;

  if (rsp == NULL) {
    syslog(LOG_ERR, "%s: failed due to NULL pointer\n", __func__);
    return POWER_STATUS_ERR;
  }

  // Is 1OU dev?
  if ((config_status & PRESENT_1OU) == PRESENT_1OU) {
    if (board_type == TYPE_1OU_OLMSTEAD_POINT) {
      if (dev_id <= DEV_ID3_1OU) {
        rsp[0] = dev_id;
        rsp[1] = FEXP_BIC_INTF;
      } else if (dev_id <= DEV_ID4_2OU) {
        rsp[0] = dev_id - DEV_ID3_1OU;
        rsp[1] = REXP_BIC_INTF;
      } else if (dev_id <= DEV_ID3_3OU) {
        rsp[0] = dev_id - DEV_ID4_2OU;
        rsp[1] = EXP3_BIC_INTF;
      } else if (dev_id <= DEV_ID4_4OU) {
        rsp[0] = dev_id - DEV_ID3_3OU;
        rsp[1] = EXP4_BIC_INTF;
      } else {
        return POWER_STATUS_FRU_ERR;
      }
    } else {
      // Vernal Falls
      if (dev_id > DEV_ID3_1OU) {
        return POWER_STATUS_FRU_ERR;
      }
      if (fby35_common_get_bmc_location(&bmc_location) < 0) {
        syslog(LOG_ERR, "%s: Cannot get the location of BMC", __func__);
        return POWER_STATUS_FRU_ERR;
      }
      if (bmc_location == NIC_BMC) {
        return POWER_STATUS_FRU_ERR;
      }

      if (bic_vf_get_e1s_present(slot_id, dev_id - 1, &prsnt) == 0 &&
          prsnt == NOT_PRESENT) {
        return POWER_STATUS_FRU_ERR;
      }

      rsp[0] = dev_id;
      rsp[1] = FEXP_BIC_INTF;
    }
  } else {
    // dev not found
    return POWER_STATUS_FRU_ERR;
  }

  return POWER_STATUS_OK;
}

int
pal_get_device_power(uint8_t slot_id, uint8_t dev_id, uint8_t *status, uint8_t *type) {
  int ret = 0;
  uint8_t nvme_ready = 0;
  uint8_t intf = 0;
  uint8_t rsp[2] = {0}; //idx0 = dev id, idx1 = intf
  int config_status = 0;
  uint8_t board_type = UNKNOWN_BOARD;

  if (fby35_common_check_slot_id(slot_id) == 0) {
    /* Check whether the system is 12V off or on */
    if ( pal_get_server_12v_power(slot_id, status) < 0 ) {
        return POWER_STATUS_ERR;
    }

    /* If 12V-off, return */
    if ((*status) == SERVER_12V_OFF) {
      *status = DEVICE_POWER_OFF;
      syslog(LOG_WARNING, "%s() pal_is_server_12v_on 12V-off", __func__);
      return POWER_STATUS_ERR;
    }

    ret = pal_get_board_type(slot_id, &config_status, &board_type);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() get board config fail", __func__);
      return ret;
    }

    if ( pal_is_valid_expansion_dev(slot_id, dev_id, rsp, config_status, board_type) < 0 ) {
      printf("Device not found \n");
      return POWER_STATUS_FRU_ERR;
    }

    dev_id = rsp[0];
    intf = rsp[1];
    ret = bic_get_dev_power_status(slot_id, dev_id, &nvme_ready, status, \
                                   NULL, NULL, NULL, NULL, NULL, intf, board_type);
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
  uint8_t status, type = 0;
  uint8_t intf = 0;
  uint8_t rsp[2] = {0}; //idx0 = dev id, idx1 = intf
  int config_status = 0;
  uint8_t board_type = UNKNOWN_BOARD;

  ret = fby35_common_check_slot_id(slot_id);
  if ( ret < 0 ) {
    return POWER_STATUS_FRU_ERR;
  }

  ret = pal_get_board_type(slot_id, &config_status, &board_type);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() get board config fail", __func__);
    return ret;
  }

  if ( pal_is_valid_expansion_dev(slot_id, dev_id, rsp, config_status, board_type) < 0 ) {
    printf("Device not found \n");
    return POWER_STATUS_FRU_ERR;
  }

  if (pal_get_device_power(slot_id, dev_id, &status, &type) < 0) {
    return -1;
  }

  dev_id = rsp[0];
  intf = rsp[1];

  switch (cmd) {
    case SERVER_POWER_ON:
      if (status == DEVICE_POWER_ON) {
        return 1;
      } else {
        ret = bic_set_dev_power_status(slot_id, dev_id, DEVICE_POWER_ON, intf, board_type);
        return ret;
      }
      break;

    case SERVER_POWER_OFF:
      if (status == DEVICE_POWER_OFF) {
        return 1;
      } else {
        ret = bic_set_dev_power_status(slot_id, dev_id, DEVICE_POWER_OFF, intf, board_type);
        return ret;
      }
      break;

    case SERVER_POWER_CYCLE:
      if (status == DEVICE_POWER_ON) {
        ret = bic_set_dev_power_status(slot_id, dev_id, DEVICE_POWER_OFF, intf, board_type);
        if (ret < 0) {
          return -1;
        }
        if (((config_status & PRESENT_1OU) == PRESENT_1OU) && ((board_type == TYPE_1OU_VERNAL_FALLS_WITH_AST) || (board_type == TYPE_1OU_OLMSTEAD_POINT))) {
          sleep(6); // EDSFF timing requirement
        } else {
          sleep(3);
        }

        ret = bic_set_dev_power_status(slot_id, dev_id, DEVICE_POWER_ON, intf, board_type);
        return ret;
      } else if (status == DEVICE_POWER_OFF) {
        ret = bic_set_dev_power_status(slot_id, dev_id, DEVICE_POWER_ON, intf, board_type);
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
  uint8_t *data = res_data;

  if (slot == FRU_BB) {  // handle the case, command sent from debug Card
    ret = pal_get_uart_select(&uart_select);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() Failed to get debug_card_uart_select\n", __func__);
    }
    // if uart_select is BMC, the following code will return default data.
    slot = uart_select;
  }

  if (slot == FRU_ALL) {
    *data++ = 0x00 | (policy << 5);
  } else {
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

  if ( fby35_common_check_slot_id(slot) < 0 ) {
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
