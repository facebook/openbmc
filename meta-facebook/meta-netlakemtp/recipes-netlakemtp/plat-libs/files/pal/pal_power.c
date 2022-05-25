#include <stdio.h>
#include <syslog.h>
#include <openbmc/libgpio.h>
#include <facebook/netlakemtp_common.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/kv.h>

// export to power-util
const char pal_server_list[] = "server";
const char pal_dev_pwr_list[] = "";
const char pal_dev_pwr_option_list[] = "";

#define ERROR     -1
#define SUCCESS    0
#define NO_ACTION  1

enum Button {
  LOW = 0,
  HIGH,
};

/**
*  @brief Function of getting power status
*
*  @param fru: FRU ID
*  @param *status: return variable of power status
*
*  @return Status of getting power status
*  0: Success
*  -1: Failed
**/
int
pal_get_server_power(uint8_t fru, uint8_t *status) {
  gpio_desc_t *gpio;
  gpio_value_t val;
  int ret = ERROR;

  if (fru != FRU_SERVER) {
    return ERROR;
  }

  if (status == NULL) {
    syslog(LOG_ERR, "%s: Invalid parameter: status is NULL ", __func__);
    return ERROR;
  }

  gpio = gpio_open_by_shadow("PWRGD_PCH_R_PWROK");
  if (gpio == NULL) {
    return ERROR;
  }
  if (gpio_get_value(gpio, &val) == 0)  {
    ret = SUCCESS;
    *status = ((val == GPIO_VALUE_LOW) ? 0 : 1);
  }
  gpio_close(gpio);
  return ret;
}

static int
server_power_on(void) {
  int ret = ERROR;
  gpio_desc_t *gpio = gpio_open_by_shadow("PWR_BTN_COME_R_N");

  if (gpio == NULL) {
    return ERROR;
  }
  if (gpio_set_value(gpio, GPIO_VALUE_LOW)) {
    goto ERR_EXIST;
  }
  sleep(1);
  if (gpio_set_value(gpio, GPIO_VALUE_HIGH)) {
    goto ERR_EXIST;
  }

  ret = SUCCESS;

ERR_EXIST:
  if (gpio != NULL)
    gpio_close(gpio);
  return ret;
}

static int
server_power_off() {
  int ret = ERROR;
  gpio_desc_t *gpio = gpio_open_by_shadow("PWR_BTN_COME_R_N");

  if (gpio == NULL) {
    return ERROR;
  }

  if (gpio_set_value(gpio, GPIO_VALUE_LOW)) {
    goto ERR_EXIST;
  }
  sleep(4);
  if (gpio_set_value(gpio, GPIO_VALUE_HIGH)) {
    goto ERR_EXIST;
  }

  ret = SUCCESS;

ERR_EXIST:
  if (gpio != NULL)
    gpio_close(gpio);
  return ret;
}

static int
server_graceful_shutdown(void) {
  int ret = ERROR;
  gpio_desc_t *gpio = gpio_open_by_shadow("PWR_BTN_COME_R_N");

  if (gpio == NULL) {
    return ERROR;
  }
  if (gpio_set_value(gpio, GPIO_VALUE_LOW)) {
    goto ERR_EXIST;
  }
  sleep(1);
  if (gpio_set_value(gpio, GPIO_VALUE_HIGH)) {
    goto ERR_EXIST;
  }

  ret = SUCCESS;

ERR_EXIST:
  if (gpio != NULL)
    gpio_close(gpio);
  return ret;
}

// Update the Reset button input to the server at given slot
int
pal_set_rst_btn(uint8_t slot, uint8_t status) {
  gpio_desc_t *desc;
  int ret = ERROR;

  if (slot != FRU_SERVER) {
    return ERROR;
  }
  desc = gpio_open_by_shadow("RST_BTN_COME_R_N");
  if (!desc) {
    return ERROR;
  }
  if (gpio_set_value(desc,((status != 0) ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW)) == 0) {
    ret = SUCCESS;
  }
  gpio_close(desc);
  return ret;
}

int
pal_set_server_power(uint8_t fru, uint8_t cmd) {
  uint8_t status;
  uint8_t ret;

  if (pal_get_server_power(fru, &status) < 0) {
    return ERROR;
  }

  switch(cmd) {
    case SERVER_POWER_ON:
      if (status == SERVER_POWER_ON)
        return NO_ACTION;
      else
        return server_power_on();
      break;

    case SERVER_POWER_OFF:
      if (status == SERVER_POWER_OFF)
        return NO_ACTION;
      else
        return server_power_off();
      break;

    case SERVER_POWER_CYCLE:
      if (status == SERVER_POWER_ON) {
        if (server_power_off())
          return ERROR;
        sleep(2);
        return server_power_on();

      } else if (status == SERVER_POWER_OFF) {
        return server_power_on();
      } else {
        syslog(LOG_ERR, "%s: Server power cycle - Unknwon status", __func__);
        return ERROR;
      }
      break;

    case SERVER_POWER_RESET:
      if (status == SERVER_POWER_ON) {
        if (pal_set_rst_btn(fru, LOW) < 0)
          return ERROR;
        sleep(1);
        if (pal_set_rst_btn(fru, HIGH) < 0)
          return ERROR;
      } else if (status == SERVER_POWER_OFF) {
        printf("Current Power is OFF, therefore cannot reset.\n");
        return NO_ACTION;
      } else {
        syslog(LOG_ERR, "%s: Server power cycle - Unknwon status", __func__);
        return ERROR;
      }
      break;

    case SERVER_GRACEFUL_SHUTDOWN:
      if (status == SERVER_POWER_OFF) {
        printf("Current Power is OFF, therefore cannot graceful shutdown.\n");
        return NO_ACTION;
      } else {
        if (server_graceful_shutdown() < 0) {
          return ERROR;
        }
      }
      break;

    default:
      return ERROR;
  }

  return SUCCESS;
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
  snprintf(key, MAX_KEY_LEN, "server_power_on_reset");
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
pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len) {
  char buff[MAX_VALUE_LEN] = {0};
  int policy = POWER_CFG_UKNOWN;
  uint8_t status = 0;
  int ret = 0;
  unsigned char *data = res_data;

  if (res_data == NULL) {
    syslog(LOG_WARNING, "%s() NULL pointer: *res_data", __func__);
  }

  if (res_len == NULL) {
    syslog(LOG_WARNING, "%s() NULL pointer: *res_len", __func__);
  }

  memset(&buff, 0, sizeof(buff));
  if (pal_get_key_value("server_power_on_reset", buff) == 0) {
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

int
pal_sled_cycle(void) {
  int ret = 0;
  int fd = 0;
  uint8_t tlen = 1;
  uint8_t power_cycle_reg = MTP_HSC_POWER_CYCLE_REG;

  fd = i2c_cdev_slave_open(MTP_HSC_BUS, MTP_HSC_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
  if ( fd < 0 ) {
    printf("Failed to open HSC 0x%x\n", MTP_HSC_ADDR);
    return -1;
  }

  ret = i2c_rdwr_msg_transfer(fd, MTP_HSC_ADDR, (uint8_t*)&power_cycle_reg, tlen, NULL, 0);
  close(fd);

  if (ret != 0) {
    printf("Failed to do i2c_rdwr_msg_transfer()\n");
  }

  return ret;
}
