#include <syslog.h>
#include <pal_power.h>
#include <openbmc/libgpio.h>
#include <facebook/fbwc_common.h>
#include <facebook/bic_ipmi.h>
#include <facebook/expander.h>

#define DELAY_POWER_ON 1
#define DELAY_GRACEFUL_SHUTDOWN 1
#define DELAY_POWER_OFF 6
#define DELAY_POWER_CYCLE 10
#define DELAY_RESET 1
#define DELAY_12V_CYCLE 5

#define POWER_STATUS_ALREADY_OK 1


static int
power_btn_out_pulse(uint8_t secs) {
  int ret = 0;
/*  gpio_desc_t *gdesc;

  gdesc = gpio_open_by_shadow("FM_BMC_PWRBTN_OUT_N");
  if (!gdesc) {
    return -1;
  }

  do {
    ret = gpio_set_value(gdesc, GPIO_VALUE_HIGH);
    if (ret) {
      break;
    }
    ret = gpio_set_value(gdesc, GPIO_VALUE_LOW);
    if (ret) {
      break;
    }
    sleep(secs);
    ret = gpio_set_value(gdesc, GPIO_VALUE_HIGH);
  } while (0);

  gpio_close(gdesc);
*/


  ret = bic_set_gpio_passthrough(FRU_MB, DISABLE);
  if (ret) {
    syslog(LOG_WARNING, "%s(), bic_set_gpio_passthrough disable fail", __func__);
    return -1;
  }

  ret = bic_set_single_gpio(FRU_MB, 0x17, 1);
  if (ret) {
    syslog(LOG_WARNING, "%s(), set gpio 0x17 high fail", __func__);
    return -1;
  }
  ret = bic_set_single_gpio(FRU_MB, 0x17, 0);
  if (ret) {
    syslog(LOG_WARNING, "%s(), set gpio 0x17 low fail", __func__);
    return -1;
  }
  sleep(secs);
  ret = bic_set_single_gpio(FRU_MB, 0x17, 1);
  if (ret) {
    syslog(LOG_WARNING, "%s(), set gpio 0x17 high fail", __func__);
    return -1;
  }
  
  ret = bic_set_gpio_passthrough(FRU_MB, ENABLE);
  if (ret) {
    syslog(LOG_WARNING, "%s(), bic_set_gpio_passthrough enable fail", __func__);
    return -1;
  }

  return ret;
}

static int
server_power_on(void) {
  return power_btn_out_pulse(DELAY_POWER_ON);
}

static int
server_power_off(void) {
  return power_btn_out_pulse(DELAY_POWER_OFF);
}

static int
server_graceful_off(void) {
  return power_btn_out_pulse(DELAY_GRACEFUL_SHUTDOWN);
}

static int
server_reset(void) {
  int ret = 0;
/*gpio_desc_t *gdesc;

  gdesc = gpio_open_by_shadow("FM_BMC_RSTBTN_OUT_N");
  if (!gdesc) {
    return -1;
  }

  do {
    ret = gpio_set_value(gdesc, GPIO_VALUE_HIGH);
    if (ret) {
      break;
    }
    ret = gpio_set_value(gdesc, GPIO_VALUE_LOW);
    if (ret) {
      break;
    }
    sleep(DELAY_RESET);
    ret = gpio_set_value(gdesc, GPIO_VALUE_HIGH);
  } while (0);

  gpio_close(gdesc);
*/

  ret = bic_set_gpio_passthrough(FRU_MB, DISABLE);
  if (ret) {
    syslog(LOG_WARNING, "%s(), bic_set_gpio_passthrough disable fail", __func__);
    return -1;
  }

  ret = bic_set_single_gpio(FRU_MB, 17, 1);
  if (ret) {
    syslog(LOG_WARNING, "%s(), set gpio 17 high fail", __func__);
    return -1;
  }
  ret = bic_set_single_gpio(FRU_MB, 17, 0);
  if (ret) {
    syslog(LOG_WARNING, "%s(), set gpio 17 low fail", __func__);
    return -1;
  }
  sleep(1);
  ret = bic_set_single_gpio(FRU_MB, 17, 1);
  if (ret) {
    syslog(LOG_WARNING, "%s(), set gpio 17 high fail", __func__);
    return -1;
  }
  
  ret = bic_set_gpio_passthrough(FRU_MB, ENABLE);
  if (ret) {
    syslog(LOG_WARNING, "%s(), bic_set_gpio_passthrough enable fail", __func__);
    return -1;
  }

  return ret;
}

static int
server_power_12v_on(uint8_t fru) {
  int ret = 0;
  gpio_desc_t *gdesc;

  gdesc = gpio_open_by_shadow("MB_P12V_HS_EN_N");
  if (!gdesc) {
    return -1;
  }
    
  ret = gpio_set_value(gdesc, GPIO_VALUE_LOW);
  gpio_close(gdesc);
  return ret;
}

static int
server_power_12v_off(uint8_t fru) {
  int ret = 0;
  gpio_desc_t *gdesc;

  gdesc = gpio_open_by_shadow("MB_P12V_HS_EN_N");
  if (!gdesc) {
    return -1;
  }

  ret = gpio_set_value(gdesc, GPIO_VALUE_HIGH);
  gpio_close(gdesc);
  return ret;
}

int
pal_get_server_12v_power(uint8_t fru, uint8_t *status) {
	gpio_value_t gpio_value;

  gpio_value = gpio_get_value_by_shadow("MB_P12V_HS_EN_N");
	if (gpio_value == GPIO_VALUE_INVALID) {
		return -1;
	}
	
	if (gpio_value == GPIO_VALUE_HIGH) {
		*status = SERVER_12V_OFF;
	} else {
		*status = SERVER_12V_ON;
	}

  return 0;
}

int
pal_get_server_power(uint8_t fru, uint8_t *status) {
  int ret = -1;
	gpio_value_t gpio_value;

  ret = pal_get_server_12v_power(fru, status);
  if (ret < 0 || (*status) == SERVER_12V_OFF) {
    // return earlier if power state is SERVER_12V_OFF or error happened
    return ret;
  }

	gpio_value = gpio_get_value_by_shadow("PWRGD_CPUPWRGD_LVC1");
	if (gpio_value == GPIO_VALUE_INVALID) {
		return -1;
	}
	
	if (gpio_value == GPIO_VALUE_HIGH) {
		*status = SERVER_POWER_ON;
	} else {
		*status = SERVER_POWER_OFF;
	}

  return ret;
}

int
pal_set_server_power(uint8_t fru, uint8_t cmd) {
  uint8_t status;

  if (pal_get_server_power(fru, &status) < 0) {
    return -1;
  }

  switch(cmd) {
    case SERVER_POWER_ON:
      if (status == SERVER_POWER_ON)
        return POWER_STATUS_ALREADY_OK;
      else
        return server_power_on();
      break;

    case SERVER_POWER_OFF:
      if (status == SERVER_POWER_OFF)
        return POWER_STATUS_ALREADY_OK;
      else
        return server_power_off();
      break;

    case SERVER_POWER_CYCLE:
      if (status == SERVER_POWER_ON) {
        if (server_power_off())
          return -1;

        sleep(DELAY_POWER_CYCLE);

        return server_power_on();
      } else if (status == SERVER_POWER_OFF) {
        return server_power_on();
      }
      break;

    case SERVER_GRACEFUL_SHUTDOWN:
      if (status == SERVER_POWER_OFF)
        return POWER_STATUS_ALREADY_OK;
      else
      	return server_graceful_off();
      break;

    case SERVER_POWER_RESET:
      if (status == SERVER_POWER_ON) {
        return server_reset();
      } else {
        return -1;
      }
      break;

    case SERVER_12V_ON:
      if (status == SERVER_12V_ON) {
        return POWER_STATUS_ALREADY_OK;
      } else {
        return server_power_12v_on(fru);
      }
      break;

    case SERVER_12V_OFF:
      if (status == SERVER_12V_OFF) {
        return POWER_STATUS_ALREADY_OK;
      } else {
        return server_power_12v_off(fru);
      }
      break;

    case SERVER_12V_CYCLE:
      if (status == SERVER_12V_OFF) {
        return server_power_12v_on(fru);
      } else {
        if (server_power_12v_off(fru) < 0) {
          return -1;
        }
        sleep(DELAY_12V_CYCLE);
        if (server_power_12v_on(fru) < 0) {
          return -1;
        }
      }
      break;

    default:
      return -1;
  }

  return 0;
}

int
pal_sled_cycle(void) {
  return exp_sled_cycle(FRU_SCB);
}
