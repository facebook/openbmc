/*
 * sensord
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/file.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>
#include <facebook/bic.h>
#include <facebook/fby2_gpio.h>

#define MAX_NUM_SLOTS       4
#define DELAY_GPIOD_READ    1000000
#define SOCK_PATH_GPIO      "/tmp/gpio_socket"

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"
#define PWR_UTL_LOCK "/var/run/power-util_%d.lock"

/* To hold the gpio info and status */
typedef struct {
  uint8_t flag;
  uint8_t status;
  uint8_t ass_val;
  char name[32];
} gpio_pin_t;

static gpio_pin_t gpio_slot1[MAX_GPIO_PINS] = {0};
static gpio_pin_t gpio_slot2[MAX_GPIO_PINS] = {0};
static gpio_pin_t gpio_slot3[MAX_GPIO_PINS] = {0};
static gpio_pin_t gpio_slot4[MAX_GPIO_PINS] = {0};

static bool smi_count_start[MAX_NUM_SLOTS] = {0};

static int
read_device(const char *device, int *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%d", value);
  fclose(fp);
  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static void *
smi_timer(void *ptr) {
  uint8_t fru = (int)ptr;
  int smi_timeout_count = 1;
  int smi_timeout_threshold = 90;
  bool is_issue_event = false;
  int ret;
  uint8_t status;

#ifdef SMI_DEBUG
  syslog(LOG_WARNING, "[%s][%lu] Timer is started.\n", __func__, pthread_self());
#endif

  while(1)
  {
    // Check 12V status
    if (0 == pal_is_server_12v_on(fru, &status)) {
      if (0 == status) {
        smi_count_start[fru-1] = false; // reset smi count
      }
    } else {
      syslog(LOG_ERR, "%s: pal_is_server_12v_on failed", __func__);
      continue;
    }

    // Check Power status
    if (0 == pal_get_server_power(fru, &status)) {
      if (SERVER_POWER_OFF == status) {
        smi_count_start[fru-1] = false; // reset smi count
      }
    } else {
      syslog(LOG_ERR, "%s: pal_get_server_power failed", __func__);
      continue;
    }

    if ( smi_count_start[fru-1] == true )
    {
      smi_timeout_count++;
    }
    else
    {
      smi_timeout_count = 0;
    }

#ifdef SMI_DEBUG
    syslog(LOG_WARNING, "[%s][%lu] smi_timeout_count[%d] == smi_timeout_threshold[%d]\n", __func__, pthread_self(), smi_timeout_count, smi_timeout_threshold);
#endif

    if ( smi_timeout_count == smi_timeout_threshold )
    {
      syslog(LOG_CRIT, "ASSERT: SMI signal is stuck low for %d sec on slot%d\n", smi_timeout_threshold, fru);
      is_issue_event = true;
    }
    else if ( (is_issue_event == true) && (smi_count_start[fru-1] == false) )
    {
      syslog(LOG_CRIT, "DEASSERT: SMI signal is stuck low for %d sec on slot%d\n", smi_timeout_threshold, fru);
      is_issue_event = false;
    }

    //sleep periodically.
    sleep(1);
#ifdef SMI_DEBUG
    syslog(LOG_WARNING, "[%s][%lu] smi_count_start flag is %d. count=%d\n", __func__, pthread_self(), smi_count_start[fru-1], smi_timeout_count);
#endif
  }

  return NULL;
}

/* Returns the pointer to the struct holding all gpio info for the fru#. */
static gpio_pin_t *
get_struct_gpio_pin(uint8_t fru) {

  gpio_pin_t *gpios;

  switch (fru) {
    case FRU_SLOT1:
      gpios = gpio_slot1;
      break;
    case FRU_SLOT2:
      gpios = gpio_slot2;
      break;
    case FRU_SLOT3:
      gpios = gpio_slot3;
      break;
    case FRU_SLOT4:
      gpios = gpio_slot4;
      break;
    default:
      syslog(LOG_WARNING, "get_struct_gpio_pin: Wrong SLOT ID %d\n", fru);
      return NULL;
  }

  return gpios;
}

static int
enable_gpio_intr_config(uint8_t fru, uint8_t gpio) {
  int ret;

  bic_gpio_config_t cfg = {0};
  bic_gpio_config_t verify_cfg = {0};


  ret =  bic_get_gpio_config(fru, gpio, &cfg);
  if (ret < 0) {
    syslog(LOG_ERR, "enable_gpio_intr_config: bic_get_gpio_config failed"
        "for slot_id: %u, gpio pin: %u", fru, gpio);
    return -1;
  }

  cfg.ie = 1;

  ret = bic_set_gpio_config(fru, gpio, &cfg);
  if (ret < 0) {
    syslog(LOG_ERR, "enable_gpio_intr_config: bic_set_gpio_config failed"
        "for slot_id: %u, gpio pin: %u", fru, gpio);
    return -1;
  }

  ret =  bic_get_gpio_config(fru, gpio, &verify_cfg);
  if (ret < 0) {
    syslog(LOG_ERR, "enable_gpio_intr_config: verification bic_get_gpio_config"
        "for slot_id: %u, gpio pin: %u", fru, gpio);
    return -1;
  }

  if (verify_cfg.ie != cfg.ie) {
    syslog(LOG_WARNING, "Slot_id: %u,Interrupt enabling FAILED for GPIO pin# %d",
        fru, gpio);
    return -1;
  }

  return 0;
}

/* Enable the interrupt mode for all the gpio sensors */
static void
enable_gpio_intr(uint8_t fru) {

  int i, ret;
  gpio_pin_t *gpios;

  gpios = get_struct_gpio_pin(fru);
  if (gpios == NULL) {
    syslog(LOG_WARNING, "enable_gpio_intr: get_struct_gpio_pin failed.");
    return;
  }

  for (i = 0; i < gpio_pin_cnt; i++) {

    gpios[i].flag = 0;

    ret = enable_gpio_intr_config(fru, gpio_pin_list[i]);
    if (ret < 0) {
      syslog(LOG_WARNING, "enable_gpio_intr: Slot: %d, Pin %d interrupt enabling"
          " failed", fru, gpio_pin_list[i]);
      syslog(LOG_WARNING, "enable_gpio_intr: Disable check for Slot %d, Pin %d",
          fru, gpio_pin_list[i]);
    } else {
      gpios[i].flag = 1;
#ifdef DEBUG
      syslog(LOG_WARNING, "enable_gpio_intr: Enabled check for Slot: %d, Pin %d",
          fru, gpio_pin_list[i]);
#endif /* DEBUG */
    }
  }
}

static void
populate_gpio_pins(uint8_t fru) {

  int i, ret;
  gpio_pin_t *gpios;

  gpios = get_struct_gpio_pin(fru);
  if (gpios == NULL) {
    syslog(LOG_WARNING, "populate_gpio_pins: get_struct_gpio_pin failed.");
    return;
  }

  // Only monitor the PWRGD_COREPWR & FM_SMI_BMC_N & FM_BIOS_POST_COMPT_N pin
  gpios[PWRGD_COREPWR].flag = 1;
  gpios[FM_SMI_BMC_N].flag = 1;
  gpios[FM_BIOS_POST_COMPT_N].flag = 1;

  for (i = 0; i < MAX_GPIO_PINS; i++) {
    if (gpios[i].flag) {
      gpios[i].ass_val = GETBIT(gpio_ass_val, i);
      ret = fby2_get_gpio_name(fru, i, gpios[i].name);
      if (ret < 0)
        continue;
    }
  }
}

/* Wrapper function to configure and get all gpio info */
static void
init_gpio_pins() {
  int fru;

  for (fru = FRU_SLOT1; fru < (FRU_SLOT1 + MAX_NUM_SLOTS); fru++) {
    populate_gpio_pins(fru);
  }

}

/* Monitor the gpio pins */
static void *
gpio_monitor_poll(void *ptr) {
  int i, ret;
  uint8_t fru = (int)ptr;
  uint8_t slot_12v = 1;
  uint64_t revised_pins, n_pin_val = 0, o_pin_val = 0;
  gpio_pin_t *gpios;
  char pwr_state[MAX_VALUE_LEN];
  char path[128];

  /* Check for initial Asserts */
  gpios = get_struct_gpio_pin(fru);
  if (gpios == NULL) {
    syslog(LOG_WARNING, "gpio_monitor_poll: get_struct_gpio_pin failed for fru %u", fru);
    pthread_exit(NULL);
  }

  // Inform BIOS that BMC is ready
  bic_set_gpio(fru, BMC_READY_N, 0);
  ret = bic_get_gpio(fru, (bic_gpio_t *) &o_pin_val);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "gpio_monitor_poll: bic_get_gpio failed for fru %u", fru);
#endif
  }

  //Init POST status
  gpios[FM_BIOS_POST_COMPT_N].status = GETBIT(o_pin_val, FM_BIOS_POST_COMPT_N);
  gpios[PWRGD_COREPWR].status = GETBIT(o_pin_val, PWRGD_COREPWR);
  if (gpios[FM_BIOS_POST_COMPT_N].status == gpios[FM_BIOS_POST_COMPT_N].ass_val) {
    // POST is not ongoing
    pal_set_fru_post(fru,0);
  } else {
    // POST is ongoing
    if (gpios[PWRGD_COREPWR].status == 1) { //on
      syslog(LOG_WARNING, "FRU: %d, POST already start", fru);
      pal_set_fru_post(fru,1);
    } else { //off
      pal_set_fru_post(fru,0);
    }
  }

  while (1) {
    memset(pwr_state, 0, MAX_VALUE_LEN);
    pal_get_last_pwr_state(fru, pwr_state);

    /* Get the GPIO pins */
    if ((ret = bic_get_gpio(fru, (bic_gpio_t *) &n_pin_val)) < 0) {
#ifdef DEBUG
      /* log the error message only when the CPU is on but not reachable. */
      if (!(strcmp(pwr_state, "on"))) {
        syslog(LOG_WARNING, "gpio_monitor_poll: bic_get_gpio failed for fru %u", fru);
      }
#endif

      if ((pal_is_server_12v_on(fru, &slot_12v) != 0) || slot_12v) {
        usleep(DELAY_GPIOD_READ);
        continue;
      }

      // 12V-off
      gpios[PWRGD_COREPWR].status = 0;
      if (gpios[FM_BIOS_POST_COMPT_N].status) {
        gpios[FM_BIOS_POST_COMPT_N].status = 0;
        pal_set_fru_post(fru,0);
      }
      o_pin_val = 0;
      n_pin_val = o_pin_val;
    }

    if (o_pin_val == n_pin_val) {
      usleep(DELAY_GPIOD_READ);
      continue;
    }

    revised_pins = (n_pin_val ^ o_pin_val);

    for (i = 0; i < MAX_GPIO_PINS; i++) {
      if (GETBIT(revised_pins, i) && (gpios[i].flag == 1)) {
        gpios[i].status = GETBIT(n_pin_val, i);

        // Check if the new GPIO val is ASSERT
        if (gpios[i].status == gpios[i].ass_val) {

          if (PWRGD_COREPWR == i) {
            /*
             * GPIO - PWRGOOD_CPU assert indicates that the CPU is turned off or in a bad shape.
             * Raise an error and change the LPS from on to off or vice versa for deassert.
             */
            if (strcmp(pwr_state, "off")) {
              if ((pal_is_server_12v_on(fru, &slot_12v) != 0) || slot_12v) {
                // Check if power-util is still running to ignore getting incorrect power status
                sprintf(path, PWR_UTL_LOCK, fru);
                if (access(path, F_OK) != 0) {
                  pal_set_last_pwr_state(fru, "off");
                }
              }
            }
            pal_set_fru_post(fru,0);
            syslog(LOG_CRIT, "FRU: %d, System powered OFF", fru);

            // Inform BIOS that BMC is ready
            bic_set_gpio(fru, BMC_READY_N, 0);
          } else if (i == FM_SMI_BMC_N) {
            smi_count_start[fru-1] = true;
          } else if (i == FM_BIOS_POST_COMPT_N) {
            //Set Post is not ongoing
            pal_set_fru_post(fru,0);
            //set POST End timestamp
            pal_set_post_end_timestamp(fru);
          }
        } else {
          if (PWRGD_COREPWR == i) {
            if (strcmp(pwr_state, "on")) {
              if ((pal_is_server_12v_on(fru, &slot_12v) != 0) || slot_12v) {
                // Check if power-util is still running to ignore getting incorrect power status
                sprintf(path, PWR_UTL_LOCK, fru);
                if (access(path, F_OK) != 0) {
                  pal_set_last_pwr_state(fru, "on");
                }
              }
            }
            if (GETBIT(n_pin_val, FM_BIOS_POST_COMPT_N) == 1) {
              pal_set_fru_post(fru,1);
              //set POST Start timestamp
            } else {
              pal_set_fru_post(fru,0);
            }
            syslog(LOG_CRIT, "FRU: %d, System powered ON", fru);
          } else if (i == FM_SMI_BMC_N) {
            smi_count_start[fru-1] = false;
          } else if (i == FM_BIOS_POST_COMPT_N) {
            //Set Post is ongoing
            if (gpios[PWRGD_COREPWR].status == 1) { //on
              pal_set_fru_post(fru,1);
              //set POST Start timestamp
            } else { //off
              pal_set_fru_post(fru,0);
            }
          }
        }
      }
    }

    o_pin_val = n_pin_val;
    usleep(DELAY_GPIOD_READ);
  } /* while loop */
} /* function definition*/

static void
print_usage() {
  printf("Usage: gpiod [ %s ]\n", pal_server_list);
}

/* Spawns a pthread for each fru to monitor all the sensors on it */
static void
run_gpiod(int argc, void **argv) {
  int i, ret, slot;
  uint8_t fru_flag, fru;
  pthread_t tid_gpio[MAX_NUM_SLOTS];
  pthread_t tid_smi_timer[MAX_NUM_SLOTS];

  /* Check for which fru do we need to monitor the gpio pins */
  fru_flag = 0;
  for (i = 1; i < argc; i++) {
    ret = pal_get_fru_id(argv[i], &fru);
    if (ret < 0) {
      print_usage();
      exit(-1);
    }

    if ((fru >= FRU_SLOT1) && (fru < (FRU_SLOT1 + MAX_NUM_SLOTS))) {
      fru_flag = SETBIT(fru_flag, fru);
      slot = fru;

      //Create thread for SMI check
      if (pthread_create(&tid_smi_timer[fru-1], NULL, smi_timer, (void *)fru) < 0) {
        syslog(LOG_WARNING, "pthread_create for smi_handler fail fru%d\n", fru);
      }

      if (pthread_create(&tid_gpio[fru-1], NULL, gpio_monitor_poll, (void *)slot) < 0) {
        syslog(LOG_WARNING, "pthread_create for gpio_monitor_poll failed\n");
      }
    }
  }

  for (fru = FRU_SLOT1; fru < (FRU_SLOT1 + MAX_NUM_SLOTS); fru++) {
    if (GETBIT(fru_flag, fru)) {
      pthread_join(tid_smi_timer[fru-1], NULL);
      pthread_join(tid_gpio[fru-1], NULL);
    }
  }
}

int
main(int argc, void **argv) {
  int dev, rc, pid_file;

  if (argc < 2) {
    print_usage();
    exit(-1);
  }

  pid_file = open("/var/run/gpiod.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      printf("Another gpiod instance is running...\n");
      exit(-1);
    }
  } else {

    init_gpio_pins();

    openlog("gpiod", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "gpiod: daemon started");
    run_gpiod(argc, argv);
  }

  return 0;
}
