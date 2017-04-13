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

#define SETBIT(x, y)        (x | (1 << y))
#define GETBIT(x, y)        ((x & (1 << y)) > y)
#define CLEARBIT(x, y)      (x & (~(1 << y)))
#define GETMASK(y)          (1 << y)

#define MAX_NUM_SLOTS       4
#define DELAY_GPIOD_READ    500000 // Polls each slot gpio values every 4*x usec
#define SOCK_PATH_GPIO      "/tmp/gpio_socket"

#define GPIO_BMC_READY_N    0

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"

/* To hold the gpio info and status */
typedef struct {
  uint8_t flag;
  uint8_t status;
  uint8_t ass_val;
  char name[32];
} gpio_pin_t;

/* For set spb gpio default configuration */
typedef struct {
  int num;
  uint8_t flag;
  uint8_t ass_val;
  char name[32];
} gpio_spb_init_t;

typedef struct {
  uint8_t flag;
  uint8_t status;
  uint8_t ass_val;
  char name[32];
} gpio_spb_t;

static gpio_pin_t gpio_slot1[MAX_GPIO_PINS] = {0};
static gpio_pin_t gpio_slot2[MAX_GPIO_PINS] = {0};
static gpio_pin_t gpio_slot3[MAX_GPIO_PINS] = {0};
static gpio_pin_t gpio_slot4[MAX_GPIO_PINS] = {0};

// SPB gpio default configuration
static gpio_spb_init_t gpio_spb_init[] = { { GPIO_FAN_LATCH_DETECT,             1, 1, "GPIO_FAN_LATCH_DETECT"           },
                                           { GPIO_SLOT1_EJECTOR_LATCH_DETECT_N, 1, 0, "GPIO_SLOT1_EJECTOR_LATCH_DETECT" },
                                           { GPIO_SLOT2_EJECTOR_LATCH_DETECT_N, 1, 0, "GPIO_SLOT2_EJECTOR_LATCH_DETECT" },
                                           { GPIO_SLOT3_EJECTOR_LATCH_DETECT_N, 1, 0, "GPIO_SLOT3_EJECTOR_LATCH_DETECT" },
                                           { GPIO_SLOT4_EJECTOR_LATCH_DETECT_N, 1, 0, "GPIO_SLOT4_EJECTOR_LATCH_DETECT" },
                                         };

static gpio_spb_t gpio_spb[MAX_SPB_GPIO_NUM] = {0};

char *fru_prsnt_log_string[2 * MAX_NUM_FRUS] = {
  // slot1, slot2, slot3, slot4
 "", "Slot1 Removal", "Slot2 Removal", "Slot3 Removal", "Slot4 Removal",
 "", "Slot1 Insertion", "Slot2 Insertion", "Slot3 Insertion", "Slot4 Insertion",
};

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

int
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

  for(i = 0; i < gpio_pin_cnt; i++) {
    // Only monitor the PWRGOOD_CPU pin
    if (i == PWRGOOD_CPU)
      gpios[gpio_pin_list[i]].flag = 1;
  }


  for(i = 0; i < MAX_GPIO_PINS; i++) {
    if (gpios[i].flag) {
      gpios[i].ass_val = GETBIT(gpio_ass_val, i);
      ret = fby2_get_gpio_name(fru, i, gpios[i].name);
      if (ret < 0)
        continue;
    }
  }
}

// GPIO SPB Action
int gpio_spb_action(int gpio_num, int n_pin_val)
{
    char cmd[80];

    switch(gpio_num)
    {
        case GPIO_FAN_LATCH_DETECT:
            if (n_pin_val == gpio_spb[gpio_num].ass_val) {
                syslog(LOG_CRIT, "SLED is pulled out");
                memset(cmd, 0, sizeof(cmd));
                sprintf(cmd, "sv stop fscd | usr/local/bin/fan-util --set 100");
                system(cmd);
            }
            else if (n_pin_val != gpio_spb[gpio_num].ass_val) {
                syslog(LOG_CRIT, "SLED is pulled in");
                memset(cmd, 0, sizeof(cmd));
                sprintf(cmd, "sv start fscd");
                system(cmd);
            }
            break;
        case GPIO_SLOT1_EJECTOR_LATCH_DETECT_N:
            if (n_pin_val == gpio_spb[gpio_num].ass_val) {
                //syslog(LOG_CRIT, "ASSERT : SLOT1 12V is off");
            }
            else if (n_pin_val != gpio_spb[gpio_num].ass_val) {
                //syslog(LOG_CRIT, "DEASSERT : SLOT1 12V is on");
            }
            break;
        case GPIO_SLOT2_EJECTOR_LATCH_DETECT_N:
            if (n_pin_val == gpio_spb[gpio_num].ass_val) {
                //syslog(LOG_CRIT, "ASSERT : SLOT2 12V is off");
            }
            else if (n_pin_val != gpio_spb[gpio_num].ass_val) {
                //syslog(LOG_CRIT, "DEASSERT : SLOT2 12V is on");
            }
            break;
        case GPIO_SLOT3_EJECTOR_LATCH_DETECT_N:
            if (n_pin_val == gpio_spb[gpio_num].ass_val) {
                //syslog(LOG_CRIT, "ASSERT : SLOT3 12V is off");
            }
            else if (n_pin_val != gpio_spb[gpio_num].ass_val) {
                //syslog(LOG_CRIT, "DEASSERT : SLOT3 12V is on");
            }
            break;
        case GPIO_SLOT4_EJECTOR_LATCH_DETECT_N:
            if (n_pin_val == gpio_spb[gpio_num].ass_val) {
                //syslog(LOG_CRIT, "ASSERT : SLOT4 12V is off");
            }
            else if (n_pin_val != gpio_spb[gpio_num].ass_val) {
                //syslog(LOG_CRIT, "DEASSERT : SLOT4 12V is on");
            }
            break;
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

/* Detect SPB GPIO status */
static void
monitor_spb_gpio_sts() {
  int slot_id, gpio_num;
  int ret;
  int i;
  uint8_t val;
  uint8_t n_pin_val;
  char str[256];
  char cmd[80];
  char vpath[64] = {0};

  static int o_pin_val_slot[MAX_NUM_SLOTS + 1] = {0};
  static int o_pin_val_spb[MAX_SPB_GPIO_NUM] = {0};
  static bool init_spb_flag = false;
  static bool init_slot_flag = false;

  // Init SPB GPIO
  if (!init_spb_flag) {

        for(i=0; i < sizeof(gpio_spb_init)/sizeof(gpio_spb_init_t); i++) {
            // flag
            gpio_spb[gpio_spb_init[i].num].flag = gpio_spb_init[i].flag;

            // status
            sprintf(vpath, GPIO_VAL, gpio_spb_init[i].num);
            read_device(vpath, &val);
            gpio_spb[gpio_spb_init[i].num].status = val;
            o_pin_val_spb[gpio_spb_init[i].num] = val;

            // ass_val
            gpio_spb[gpio_spb_init[i].num].ass_val = gpio_spb_init[i].ass_val;

            // name
            strcpy(gpio_spb[gpio_spb_init[i].num].name, gpio_spb_init[i].name);
        }
        init_spb_flag = true;
  }

  // SPB GPIO detect
  // Currently just only detect GPIO_FAN_LATCH_DETECT
  for(gpio_num=0; gpio_num < sizeof(gpio_spb)/sizeof(gpio_spb_t); gpio_num++) {

	if(!gpio_spb[gpio_num].flag)
            continue;

	sprintf(vpath, GPIO_VAL, gpio_num);
	read_device(vpath, &n_pin_val);
	gpio_spb[gpio_num].status = n_pin_val;

	// Currently only detect GPIO FAN_LATCH_DETECT
	if (o_pin_val_spb[gpio_num]^n_pin_val)
	    gpio_spb_action(gpio_num, n_pin_val);

        o_pin_val_spb[gpio_num] = n_pin_val;
  }

  // Detect SLOT Removal/Insertion
  // It`s with 2 PRSNT PIN for SLOT present, so we could use pal_is_fru_prsnt directly.
  for(slot_id = FRU_SLOT1; slot_id < (FRU_SLOT1 + MAX_NUM_SLOTS); slot_id++) {
    ret = pal_is_fru_prsnt(slot_id, &n_pin_val);
    if (ret < 0)
        printf("%s pal_is_fru_prsnt failed for fru: %d\n", __func__, slot_id);

    if (!init_slot_flag) { // Init old pin value, just init this one time
        o_pin_val_slot[slot_id] = n_pin_val;
        init_slot_flag = true;
        continue;
    }

    // HOT SERVER event would be detected when SLED is pulled out
    if(gpio_spb[GPIO_FAN_LATCH_DETECT].status) {

        if (o_pin_val_slot[slot_id]^n_pin_val) {

            if (n_pin_val) { // SLOT Removal
                if (gpio_spb[GPIO_SLOT1_EJECTOR_LATCH_DETECT_N + (slot_id-1)].status) {
                    // Log SLOT Removal
      	            sprintf(str, "%s without 12V off, Action : 12V off to slot%u", fru_prsnt_log_string[slot_id],slot_id);
                    syslog(LOG_CRIT, str);

                    // Active 12V off to protect server/device board
                    sprintf(cmd, "/usr/local/bin/power-util slot%u 12V-off", slot_id);
                    system(cmd);
                }
                else {
                    syslog(LOG_CRIT, fru_prsnt_log_string[slot_id]);
                }
            }
            else {          // SLOT Insertion
                syslog(LOG_CRIT, fru_prsnt_log_string[MAX_NUM_FRUS + slot_id]);
            }

            o_pin_val_slot[slot_id] = n_pin_val;
        }
    }
  }
}

/* Monitor the gpio pins */
static int
gpio_monitor_poll(uint8_t fru_flag) {
  int i, ret;
  uint8_t fru;
  uint32_t revised_pins, n_pin_val, o_pin_val[MAX_NUM_SLOTS + 1] = {0};
  gpio_pin_t *gpios;
  char pwr_state[MAX_VALUE_LEN];

  uint32_t status;
  bic_gpio_t gpio = {0};

  /* Check for initial Asserts */
  for (fru = 1; fru <= MAX_NUM_SLOTS; fru++) {
    if (GETBIT(fru_flag, fru) == 0)
      continue;

    // Inform BIOS that BMC is ready
    bic_set_gpio(fru, GPIO_BMC_READY_N, 0);

    ret = bic_get_gpio(fru, &gpio);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "gpio_monitor_poll: bic_get_gpio failed for "
        " fru %u", fru);
#endif
      continue;
    }

    gpios = get_struct_gpio_pin(fru);
    if  (gpios == NULL) {
      syslog(LOG_WARNING, "gpio_monitor_poll: get_struct_gpio_pin failed for"
          " fru %u", fru);
      continue;
    }

    memcpy(&status, (uint8_t *) &gpio, sizeof(status));

    o_pin_val[fru] = 0;

    for (i = 0; i < MAX_GPIO_PINS; i++) {

      if (gpios[i].flag == 0)
        continue;

      gpios[i].status = GETBIT(status, i);

      if (gpios[i].status)
        o_pin_val[fru] = SETBIT(o_pin_val[fru], i);
    }
  }

  /* Keep monitoring each fru's gpio pins every 4 * GPIOD_READ_DELAY seconds */
  while(1) {

    // Detect SPB GPIO status
    monitor_spb_gpio_sts();

    for (fru = 1; fru <= MAX_NUM_SLOTS; fru++) {
      if (!(GETBIT(fru_flag, fru))) {
        usleep(DELAY_GPIOD_READ);
        continue;
      }

      gpios = get_struct_gpio_pin(fru);
      if  (gpios == NULL) {
        syslog(LOG_WARNING, "gpio_monitor_poll: get_struct_gpio_pin failed for"
            " fru %u", fru);
        continue;
      }

      memset(pwr_state, 0, MAX_VALUE_LEN);
      pal_get_last_pwr_state(fru, pwr_state);

      /* Get the GPIO pins */
      if ((ret = bic_get_gpio(fru, (bic_gpio_t *) &n_pin_val)) < 0) {
        /* log the error message only when the CPU is on but not reachable. */
        if (!(strcmp(pwr_state, "on"))) {
#ifdef DEBUG
          syslog(LOG_WARNING, "gpio_monitor_poll: bic_get_gpio failed for "
              " fru %u", fru);
#endif
        }
        continue;
      }

      if (o_pin_val[fru] == n_pin_val) {
        o_pin_val[fru] = n_pin_val;
        usleep(DELAY_GPIOD_READ);
        continue;
      }

      revised_pins = (n_pin_val ^ o_pin_val[fru]);

      for (i = 0; i < MAX_GPIO_PINS; i++) {
        if (GETBIT(revised_pins, i) && (gpios[i].flag == 1)) {
          gpios[i].status = GETBIT(n_pin_val, i);

          // Check if the new GPIO val is ASSERT
          if (gpios[i].status == gpios[i].ass_val) {
            /*
             * GPIO - PWRGOOD_CPU assert indicates that the CPU is turned off or in a bad shape.
             * Raise an error and change the LPS from on to off or vice versa for deassert.
             */
            if (!(strcmp(pwr_state, "on")))
              pal_set_last_pwr_state(fru, "off");

            syslog(LOG_CRIT, "FRU: %d, System powered OFF", fru);

            // Inform BIOS that BMC is ready
            bic_set_gpio(fru, GPIO_BMC_READY_N, 0);
          } else {

            if (!(strcmp(pwr_state, "off")))
              pal_set_last_pwr_state(fru, "on");

            syslog(LOG_CRIT, "FRU: %d, System powered ON", fru);
          }
        }
      }

      o_pin_val[fru] = n_pin_val;
      usleep(DELAY_GPIOD_READ);

    } /* For Loop for each fru */
  } /* while loop */
} /* function definition*/

static void
print_usage() {
  printf("Usage: gpiod [ %s ]\n", pal_server_list);
}

/* Spawns a pthread for each fru to monitor all the sensors on it */
static void
run_gpiod(int argc, void **argv) {

  //gpio_monitor();

  int i, ret;
  uint8_t fru_flag, fru;

  /* Check for which fru do we need to monitor the gpio pins */
  fru_flag = 0;
  for (i = 1; i < argc; i++) {
    ret = pal_get_fru_id(argv[i], &fru);
    if (ret < 0) {
      print_usage();
      exit(-1);
    }
    fru_flag = SETBIT(fru_flag, fru);
  }

  gpio_monitor_poll(fru_flag);
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

    daemon(0,1);
    openlog("gpiod", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "gpiod: daemon started");
    run_gpiod(argc, argv);
  }

  return 0;
}
