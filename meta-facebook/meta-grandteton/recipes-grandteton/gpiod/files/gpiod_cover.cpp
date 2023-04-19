/*
 * sensord
 *
 * Copyright 2022-present Facebook. All Rights Reserved.
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
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <syslog.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/file.h>
#include <pthread.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/peci_sensors.h>
#include <openbmc/pal.h>
#include <openbmc/pal_def.h>
#include "gpiod.h"


#define MB_CPLD_FAIL_ADDR   (0x28)

static uint8_t g_caterr_irq = 0;
static bool g_mcerr_ierr_assert = false;
static pthread_mutex_t caterr_mutex = PTHREAD_MUTEX_INITIALIZER;


//IERR and MCERR Event Handler
void
cpu_caterr_init(gpiopoll_pin_t *desc, gpio_value_t value) {
  uint8_t status = 0;

  pal_get_server_power(FRU_MB, &status);
  if (status && value == GPIO_VALUE_LOW) {
    g_caterr_irq++;
  }
}

static void
err_caterr_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if(!server_power_check(1))
    return;

  pthread_mutex_lock(&caterr_mutex);
  g_caterr_irq++;
  pthread_mutex_unlock(&caterr_mutex);
}

static int
ierr_mcerr_event_log(bool is_caterr, const char *err_type) {
  char temp_log[128] = {0};
  char temp_syslog[128] = {0};
  char cpu_str[32] = "";
  int num=0;
  int ret=0;

  ret = cmd_peci_get_cpu_err_num(PECI_CPU0_ADDR, &num, is_caterr);
  if (ret != 0) {
    syslog(LOG_ERR, "Can't Read MCA Log\n");
  }

  if (num == 2)
    strcpy(cpu_str, "0/1");
  else if (num != -1)
    sprintf(cpu_str, "%d", num);

  sprintf(temp_syslog, "ASSERT: CPU%s %s\n", cpu_str, err_type);
  sprintf(temp_log, "CPU%s %s", cpu_str, err_type);
  syslog(LOG_CRIT, "FRU: %d %s",FRU_MB, temp_syslog);
  pal_add_cri_sel(temp_log);

  return ret;
}

static void *
ierr_mcerr_event_handler(void *) {
  uint8_t caterr_cnt = 0;
  gpio_value_t value;

  while (1) {
    if (g_caterr_irq > 0) {
      caterr_cnt++;
      if (caterr_cnt == 2) {
        if (g_caterr_irq == 1) {
          g_mcerr_ierr_assert = true;

          value = gpio_get_value_by_shadow(FM_CPU_CATERR_N);
          if (value == GPIO_VALUE_LOW) {
            ierr_mcerr_event_log(true, "IERR/CATERR");
          } else {
            ierr_mcerr_event_log(true, "MCERR/CATERR");
          }

          pthread_mutex_lock(&caterr_mutex);
          g_caterr_irq--;
          pthread_mutex_unlock(&caterr_mutex);
          caterr_cnt = 0;
          if (system("/usr/local/bin/autodump.sh &")) {
            syslog(LOG_WARNING, "Failed to start crashdump\n");
          }

        } else if (g_caterr_irq > 1) {
          while (g_caterr_irq > 1) {
            ierr_mcerr_event_log(true, "MCERR/CATERR");
            pthread_mutex_lock(&caterr_mutex);
            g_caterr_irq--;
            pthread_mutex_unlock(&caterr_mutex);
          }
          caterr_cnt = 1;
        }
      }
    }
    usleep(25000); //25ms
  }
  return NULL;
}



//CPLD power fail event.
static void*
dump_cpld_reg(void* arg) {
  int fd = 0, ret = -1;
  uint8_t tlen, rlen;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t addr = MB_CPLD_FAIL_ADDR;
  uint8_t bus = I2C_BUS_7;
  const char *power_fail_log[] = {
    "PSU_PWR_FAULT",
    "BMC_PWR_FAULT",
    "PCH_PWR_FAULT",
    "PCH_PWR_FAULT_PVNN",
    "PCH_PWR_FAULT_P1V05",
    "MEM0_PWR_FAULT",
    "MEM1_PWR_FAULT",
    "CPU_MEM_PWR_FAULT",

    "CPU0_MEM_PWR_FAULT",
    "CPU0_PVPPHBMFAULT",
    "CPU0_PVCCFAEHVFAULT",
    "CPU0_PVCCFAFIVRFAULT",
    "CPU0_PVCCINFAONFAULT",
    "CPU0_PVNNMAINFAULT",
    "CPU0_PVCCINFAULT",
    "CPU0_PVCCDHVFAULT",

    "CPU1_MEM_PWR_FAULT",
    "CPU1_PVPPHBM_FAULT",
    "CPU1_PVCCFAEHV_FAULT",
    "CPU1_PVCCFAFIVR_FAULT",
    "CPU1_PVCCINFAON_FAULT",
    "CPU1_PVNNMAIN_FAULT",
    "CPU1_PVCCIN_FAULT",
    "CPU1_PVCCDHV_FAULT",

    "A4/C4/A5/C5_FAULT",
    "A2/C2/A3/C3_FAULT",
    "A6/C6/A7/C7_FAULT",
    "A0/C0/A1/C1_FAULT",
    "B0/D0/B1/D1_FAULT",
    "B2/D2/B3/D3_FAULT",
    "B4/D4/B5/D5_FAULT",
    "B6/D6/B7/D7_FAULT",

    "HPDB_HSC_PWRGD_ISO_R_FAULT",
    "GPU_FPGA_READY_ISO_R_FAULT",
    "FM_GPU_PWRGD_ISO_R_FAULT",
    "GPU_PWR_FAULT",
    "SWB_HSC_PWRGD_ISO_R_FAULT",
    "FM_SWB_PWRGD_ISO_R_FAULT",
    "RESERVE",
    "SWB_PWR_FAULT",
  };


  pthread_detach(pthread_self());

  fd = i2c_cdev_slave_open(bus, addr>>1 , I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open %d", __func__, bus);
    goto exit;
  }

  tbuf[0] = 0x03;

  tlen = 1;
  rlen = 5;

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
  if (ret == -1) {
    syslog(LOG_WARNING, "%s bus=%x slavaddr=%x \n", __func__, bus, addr >> 1);
    goto close;
  }

  for(int i=0; i<rlen; i++) {
    uint8_t val = rbuf[i];
    for(int j=0; j<8; j++) {
      if((val & (1 <<j)) == (1 << j)) {
        syslog(LOG_CRIT, "Power Fail Event: %s Assert\n", power_fail_log[j]);
      }
    }
  }

close:
  i2c_cdev_slave_close(fd);
exit:
  pthread_exit(NULL);
}

static
void cpld_get_fail_reg (void) {
  pthread_t tid_dump_cpld_reg;

  if (pthread_create(&tid_dump_cpld_reg, NULL, dump_cpld_reg, 0)) {
    syslog(LOG_WARNING, "pthread_create for dump_cpld_reg");
  }
}

void
cpld_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if (!sgpio_valid_check())
    return;

  if(curr == GPIO_VALUE_HIGH)
    cpld_get_fail_reg();
}



// GPIO table to be monitored
struct gpiopoll_config gpios_list[] = {
  // shadow, description, edge, handler, oneshot
  {FP_RST_BTN_IN_N,         "GPIOP2",         GPIO_EDGE_BOTH,    pwr_reset_handler,          NULL},
  {FP_PWR_BTN_IN_N,         "GPIOP0",         GPIO_EDGE_BOTH,    pwr_button_handler,         NULL},
  {FM_UARTSW_LSB_N,         "SGPIO106",       GPIO_EDGE_BOTH,    uart_select_handle,         NULL},
  {FM_UARTSW_MSB_N,         "SGPIO108",       GPIO_EDGE_BOTH,    uart_select_handle,         NULL},
  {FM_POST_CARD_PRES_N,     "GPIOZ6",         GPIO_EDGE_BOTH,    usb_dbg_card_handler,       NULL},
  {IRQ_UV_DETECT_N,         "SGPIO188",       GPIO_EDGE_BOTH,    uv_detect_handler,          NULL},
  {IRQ_OC_DETECT_N,         "SGPIO178",       GPIO_EDGE_BOTH,    oc_detect_handler,          NULL},
  {IRQ_HSC_FAULT_N,         "SGPIO36",        GPIO_EDGE_BOTH,    sgpio_event_handler,        NULL},
  {IRQ_HSC_ALERT_N,         "SGPIO2",         GPIO_EDGE_BOTH,    sml1_pmbus_alert_handler,   NULL},
  {FM_CPU_CATERR_N,         "GPIOC5",         GPIO_EDGE_FALLING, err_caterr_handler,         cpu_caterr_init},
  {FM_CPU0_PROCHOT_N,       "SGPIO202",       GPIO_EDGE_BOTH,    cpu_prochot_handler,        NULL},
  {FM_CPU1_PROCHOT_N,       "SGPIO186",       GPIO_EDGE_BOTH,    cpu_prochot_handler,        NULL},
  {FM_CPU0_THERMTRIP_N,     "SGPIO136",       GPIO_EDGE_FALLING, cpu_thermtrip_handler,      NULL},
  {FM_CPU1_THERMTRIP_N,     "SGPIO118",       GPIO_EDGE_FALLING, cpu_thermtrip_handler,      NULL},
  {FM_CPU_ERR0_N,           "SGPIO144",       GPIO_EDGE_FALLING, cpu_error_handler,          NULL},
  {FM_CPU_ERR1_N,           "SGPIO142",       GPIO_EDGE_FALLING, cpu_error_handler,          NULL},
  {FM_CPU_ERR2_N,           "SGPIO140",       GPIO_EDGE_FALLING, cpu_error_handler,          NULL},
  {FM_CPU0_MEM_HOT_N,       "SGPIO138",       GPIO_EDGE_BOTH,    gpio_event_pson_3s_handler, NULL},
  {FM_CPU1_MEM_HOT_N,       "SGPIO124",       GPIO_EDGE_BOTH,    gpio_event_pson_3s_handler, NULL},
  {FM_CPU0_MEM_THERMTRIP_N, "SGPIO122",       GPIO_EDGE_FALLING, mem_thermtrip_handler,      NULL},
  {FM_CPU1_MEM_THERMTRIP_N, "SGPIO158",       GPIO_EDGE_FALLING, mem_thermtrip_handler,      NULL},
  {FM_SYS_THROTTLE,         "SGPIO20",        GPIO_EDGE_BOTH,    gpio_event_pson_handler,    NULL},
  {FM_PCH_THERMTRIP_N,      "SGPIO28",        GPIO_EDGE_BOTH,    pch_thermtrip_handler,      NULL},
  {FM_CPU0_VCCIN_VR_HOT,    "SGPIO154",       GPIO_EDGE_BOTH,    cpu0_pvccin_handler,        cpu_vr_hot_init},
  {FM_CPU1_VCCIN_VR_HOT,    "SGPIO120",       GPIO_EDGE_BOTH,    cpu1_pvccin_handler,        cpu_vr_hot_init},
  {FM_CPU0_VCCD_VR_HOT,     "SGPIO12",        GPIO_EDGE_BOTH,    cpu0_pvccd_handler,         cpu_vr_hot_init},
  {FM_CPU1_VCCD_VR_HOT,     "SGPIO18",        GPIO_EDGE_BOTH,    cpu1_pvccd_handler,         cpu_vr_hot_init},
  {RST_PLTRST_N,            "SGPIO200",       GPIO_EDGE_BOTH,    platform_reset_handle,      platform_reset_init},
  {FM_LAST_PWRGD,           "SGPIO116",       GPIO_EDGE_BOTH,    pwr_good_handler,           NULL},
  {FM_CPU0_SKTOCC,          "SGPIO112",       GPIO_EDGE_BOTH,    sgpio_event_handler,        cpu_skt_init},
  {FM_CPU1_SKTOCC,          "SGPIO114",       GPIO_EDGE_BOTH,    sgpio_event_handler,        cpu_skt_init},
  {FP_AC_PWR_BMC_BTN,       "GPIO18A0",       GPIO_EDGE_BOTH,    gpio_event_handler,         NULL},
  {BIC_READY,               "SGPIO32",        GPIO_EDGE_FALLING, bic_ready_handler,          bic_ready_init},
  {HMC_READY,               "SGPIO64",        GPIO_EDGE_BOTH,    hmc_ready_handler,          hmc_ready_init},
  {GPU_FPGA_THERM_OVERT,    "SGPIO40",        GPIO_EDGE_FALLING, nv_event_handler,           NULL},
  {GPU_FPGA_DEVIC_OVERT,    "SGPIO42",        GPIO_EDGE_FALLING, nv_event_handler,           NULL},
  {CPLD_POWER_FAIL_ALERT,   "SGPI240",        GPIO_EDGE_RISING,  cpld_event_handler,         NULL},
  {PEX_FW_VER_UPDATE,       "PEX_VER_UPDATE", GPIO_EDGE_RISING,  pex_fw_ver_handle,          NULL},
};

const uint8_t gpios_cnt = sizeof(gpios_list)/sizeof(gpios_list[0]);



int main(int argc, char **argv)
{
  int rc, pid_file;
  gpiopoll_desc_t *polldesc;
  pthread_t tid_ierr_mcerr_event;
  pthread_t tid_gpio_timer;
  pthread_t tid_iox_gpio_handle;

  pid_file = open("/var/run/gpiod.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if (rc) {
    if (EWOULDBLOCK == errno) {
      syslog(LOG_ERR, "Another gpiod instance is running...\n");
      exit(-1);
    }
  } else {
    openlog("gpiod", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "gpiod: daemon started");

    //Create thread for IERR/MCERR event detect
    if (pthread_create(&tid_ierr_mcerr_event, NULL, ierr_mcerr_event_handler, NULL) < 0) {
      syslog(LOG_WARNING, "pthread_create for ierr_mcerr_event_handler\n");
      exit(1);
    }

    //Create thread for platform reset event filter check
    if (pthread_create(&tid_gpio_timer, NULL, gpio_timer, NULL) < 0) {
      syslog(LOG_WARNING, "pthread_create for platform_reset_filter_handler\n");
      exit(1);
    }

    //Create thread for platform reset event filter check
    if (pthread_create(&tid_iox_gpio_handle, NULL, iox_gpio_handle, NULL) < 0) {
      syslog(LOG_WARNING, "pthread_create for iox_gpio_handler\n");
      exit(1);
    }

    polldesc = gpio_poll_open(gpios_list, gpios_cnt);
    if (!polldesc) {
      syslog(LOG_CRIT, "FRU: %d Cannot start poll operation on GPIOs", FRU_MB);
    } else {
      if (gpio_poll(polldesc, POLL_TIMEOUT)) {
        syslog(LOG_CRIT, "FRU: %d Poll returned error", FRU_MB);
      }
      gpio_poll_close(polldesc);
    }
  }

  return 0;
}
