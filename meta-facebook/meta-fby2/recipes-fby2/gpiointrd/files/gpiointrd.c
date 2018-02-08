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
#include <time.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/file.h>
#include <openbmc/edb.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>
#include <openbmc/gpio.h>
#include <facebook/bic.h>
#include <facebook/fby2_gpio.h>
#include <facebook/fby2_sensor.h>
#include <facebook/fby2_common.h>

#define POLL_TIMEOUT -1 /* Forever */
#define MAX_NUM_SLOTS       4

#define GPIO_VAL "/sys/class/gpio/gpio%d/value"

#define SLOT_RECORD_FILE "/tmp/slot%d.rc"

#define HOTSERVICE_SCRPIT "/usr/local/bin/hotservice-reinit.sh"
#define HOTSERVICE_FILE "/tmp/slot%d_reinit"
#define HSLOT_PID  "/tmp/slot%u_reinit.pid"

#define DEBUG_ME_EJECTOR_LOG 1 // Enable log "GPIO_SLOTX_EJECTOR_LATCH_DETECT_N is 1 and SLOT_12v is ON" before mechanism issue is fixed

static uint8_t IsHotServiceStart[MAX_NODES + 1] = {0};
static void *hsvc_event_handler(void *ptr);
static pthread_mutex_t hsvc_mutex[MAX_NODES + 1];
static struct timespec last_ejector_ts[MAX_NODES + 1];

char *fru_prsnt_log_string[3 * MAX_NUM_FRUS] = {
  // slot1, slot2, slot3, slot4
 "", "Slot1 Removal", "Slot2 Removal", "Slot3 Removal", "Slot4 Removal", "",
 "", "Slot1 Insertion", "Slot2 Insertion", "Slot3 Insertion", "Slot4 Insertion", "",
 "", "Slot1 Removal Without 12V-OFF", "Slot2 Removal Without 12V-OFF", "Slot3 Removal Without 12V-OFF", "Slot4 Removal Without 12V-OFF",
};

const static uint8_t gpio_12v[] = { 0, GPIO_P12V_STBY_SLOT1_EN, GPIO_P12V_STBY_SLOT2_EN, GPIO_P12V_STBY_SLOT3_EN, GPIO_P12V_STBY_SLOT4_EN };

typedef struct {
  uint8_t def_val;
  char name[64];
  uint8_t num;
  char log[256];
} def_chk_info;

struct delayed_log {
  useconds_t usec;
  char msg[256];
};

typedef struct {
  uint8_t slot_id;
  uint8_t action;
} hot_service_info;

enum {
  REMOVAl = 0,
  INSERTION = 1,
};

slot_kv_st slot_kv_list[] = {
  // {slot_key, slot_def_val}
  {"pwr_server%d_last_state", "on"},
  {"sysfw_ver_slot%d",         "0"},
  {"slot%d_por_cfg",         "lps"},
  {"slot%d_sensor_health",     "1"},
  {"slot%d_sel_error",         "1"},
  {"slot%d_boot_order",  "0000000"},
  {"slot%d_cpu_ppin",          "0"},
  {"fru%d_restart_cause",      "3"},
};

// Thread for delay event
static void *
delay_log(void *arg)
{
  struct delayed_log* log = (struct delayed_log*)arg;

  pthread_detach(pthread_self());

  if (arg) {
    usleep(log->usec);
    syslog(LOG_CRIT, log->msg);

    free(arg);
  }

  pthread_exit(NULL);
}

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

static int
write_device(const char *device, const char *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "w");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device for write %s", device);
#endif
    return err;
  }

  rc = fputs(value, fp);
  fclose(fp);

  if (rc < 0) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to write device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static void log_gpio_change(gpio_poll_st *gp, useconds_t log_delay)
{
  if (log_delay == 0) {
    syslog(LOG_CRIT, "%s: %s - %s\n", gp->value ? "DEASSERT": "ASSERT", gp->name, gp->desc);
  } else {
    pthread_t tid_delay_log;
    struct delayed_log *log = (struct delayed_log *)malloc(sizeof(struct delayed_log));
    if (log) {
      log->usec = log_delay;
      snprintf(log->msg, 256, "%s: %s - %s\n", gp->value ? "DEASSERT" : "ASSERT", gp->name, gp->value);
      if (pthread_create(&tid_delay_log, NULL, delay_log, (void *)log)) {
        free(log);
        log = NULL;
      }
    }
    if (!log) {
      syslog(LOG_CRIT, "%s: %s - %s\n", gp->value ? "DEASSERT": "ASSERT", gp->name, gp->desc);
    }
  }
}

// Generic Event Handler for GPIO changes
static void gpio_event_handle(gpio_poll_st *gp)
{
  char cmd[128] = {0};
  int ret=-1;
  uint8_t slot_id;
  int value;
  int status;
  char vpath[80] = {0};
  char locstr[MAX_VALUE_LEN];
  static bool prsnt_assert[MAX_NODES + 1]={0};
  static pthread_t hsvc_action_tid[MAX_NODES + 1];
  hot_service_info hsvc_info[MAX_NODES + 1];
  struct timespec ts;

  if (gp->gs.gs_gpio == gpio_num("GPIOH5")) { // GPIO_FAN_LATCH_DETECT
    if (gp->value == 1) { // low to high
      syslog(LOG_CRIT, "ASSERT: SLED is not seated");
      memset(cmd, 0, sizeof(cmd));
      sprintf(cmd, "sv stop fscd ; /usr/local/bin/fan-util --set 100");
      system(cmd);
    }
    else { // high to low
      syslog(LOG_CRIT, "DEASSERT: SLED is seated");
      memset(cmd, 0, sizeof(cmd));
      sprintf(cmd, "/etc/init.d/setup-fan.sh ; sv start fscd");
      system(cmd);
    }
  }
  else if (gp->gs.gs_gpio == gpio_num("GPIOP0") || gp->gs.gs_gpio == gpio_num("GPIOP1") ||
           gp->gs.gs_gpio == gpio_num("GPIOP2") || gp->gs.gs_gpio == gpio_num("GPIOP3") 
          ) //  GPIO_SLOT1/2/3/4_EJECTOR_LATCH_DETECT_N 
  {
    slot_id = (gp->gs.gs_gpio - GPIO_SLOT1_EJECTOR_LATCH_DETECT_N) + 1;
    if (gp->value == 1) { // low to high
      clock_gettime(CLOCK_MONOTONIC, &ts);
      if (last_ejector_ts[slot_id].tv_sec && ((ts.tv_sec - last_ejector_ts[slot_id].tv_sec) < 5)) {
        return;
      }
      last_ejector_ts[slot_id].tv_sec = ts.tv_sec;

      sprintf(vpath, GPIO_VAL, GPIO_FAN_LATCH_DETECT);
      read_device(vpath, &value);

#if DEBUG_ME_EJECTOR_LOG // Enable log "GPIO_SLOTX_EJECTOR_LATCH_DETECT_N is 1 and SLOT_12v is ON" before mechanism issue is fixed
      syslog(LOG_CRIT,"GPIO_SLOT%d_EJECTOR_LATCH_DETECT_N is \"1\" and SLOT_12v is ON", slot_id);
#endif

      // HOT SERVER event would be detected when SLED is pulled out
      if (value) {
          log_gpio_change(gp, 0);
#if 0
          sprintf(cmd, "/usr/local/bin/power-util slot%u graceful-shutdown", slot_id);
          system(cmd);

          ret = pal_is_server_12v_on(slot_id, &Cur12V_value);
          if (ret < 0)
             printf("%s pal_is_fru_prsnt failed for fru: %d\n", __func__, slot_id);
          if (Cur12V_value) {
             // Log SLOT Removal
             sprintf(str, "Server may be removed without 12V off, Action : 12V off to slot%u",slot_id);
             syslog(LOG_CRIT, str);

             // Active 12V off to protect server/device board
             memset(cmd, 0, sizeof(cmd));
             sprintf(cmd, "/usr/local/bin/power-util slot%u 12V-off", slot_id);
             system(cmd);
          }
#endif
      }
    } //End of low to high
  } //End of GPIO_SLOT1/2/3/4_EJECTOR_LATCH_DETECT_N
  else if (gp->gs.gs_gpio == gpio_num("GPIOZ0") || gp->gs.gs_gpio == gpio_num("GPIOZ1")   ||
           gp->gs.gs_gpio == gpio_num("GPIOZ2") || gp->gs.gs_gpio == gpio_num("GPIOZ3")   ||
           gp->gs.gs_gpio == gpio_num("GPIOAA0") || gp->gs.gs_gpio == gpio_num("GPIOAA1") ||
           gp->gs.gs_gpio == gpio_num("GPIOAA2") || gp->gs.gs_gpio == gpio_num("GPIOAA3")
          ) // GPIO_SLOT1/2/3/4_PRSNT_B_N, GPIO_SLOT1/2/3/4_PRSNT_N

  {
    if (gp->gs.gs_gpio >= GPIO_SLOT1_PRSNT_B_N && gp->gs.gs_gpio <= GPIO_SLOT4_PRSNT_B_N)
       slot_id = (gp->gs.gs_gpio - GPIO_SLOT1_PRSNT_B_N) + 1;
    else if (gp->gs.gs_gpio >= GPIO_SLOT1_PRSNT_N && gp->gs.gs_gpio <= GPIO_SLOT4_PRSNT_N)
       slot_id = (gp->gs.gs_gpio - GPIO_SLOT1_PRSNT_N) + 1;

    // Use flag to ignore the interrupt of pair present pin
    if (prsnt_assert[slot_id])
    {
       prsnt_assert[slot_id] = false;
       return;
    }

    sprintf(vpath, GPIO_VAL, GPIO_FAN_LATCH_DETECT);
    read_device(vpath, &value);

    // HOT SERVER event would be detected when SLED is pulled out
    if (value) {
       prsnt_assert[slot_id] = true;

       if (gp->value == 1) { // SLOT Removal
          hsvc_info[slot_id].slot_id = slot_id;
          hsvc_info[slot_id].action = REMOVAl;

          pthread_mutex_lock(&hsvc_mutex[slot_id]);
          if ( IsHotServiceStart[slot_id] )
          {
#ifdef DEBUG
            syslog(LOG_WARNING, "[%s] Close the previous thread for slot%x\n", __func__, slot_id);
#endif
            pthread_cancel(hsvc_action_tid[slot_id]);
          }

          //Create thread for hsvc event detect
          if (pthread_create(&hsvc_action_tid[slot_id], NULL, hsvc_event_handler, &hsvc_info[slot_id]) < 0) {
            syslog(LOG_WARNING, "[%s] Create hsvc_event_handler thread for slot%x\n",__func__, slot_id);
            pthread_mutex_unlock(&hsvc_mutex[slot_id]);
            return;
          }
          IsHotServiceStart[slot_id] = true;
          pthread_mutex_unlock(&hsvc_mutex[slot_id]);
       }
       else { // SLOT INSERTION
          hsvc_info[slot_id].slot_id = slot_id;
          hsvc_info[slot_id].action = INSERTION;

          pthread_mutex_lock(&hsvc_mutex[slot_id]);
          if ( IsHotServiceStart[slot_id] )
          {
#ifdef DEBUG
            syslog(LOG_WARNING, "[%s] Close the previous thread for slot%x\n", __func__, slot_id);
#endif
            pthread_cancel(hsvc_action_tid[slot_id]);
          }

          //Create thread for hsvc event detect
          if (pthread_create(&hsvc_action_tid[slot_id], NULL, hsvc_event_handler, &hsvc_info[slot_id]) < 0) {
            syslog(LOG_WARNING, "[%s] Create hsvc_event_handler thread for slot%x\n",__func__, slot_id);
            pthread_mutex_unlock(&hsvc_mutex[slot_id]);
            return;
          }
          IsHotServiceStart[slot_id] = true;
          pthread_mutex_unlock(&hsvc_mutex[slot_id]);
       }
    }
  } // End of GPIO_SLOT1/2/3/4_PRSNT_B_N, GPIO_SLOT1/2/3/4_PRSNT_N
  else if (gp->gs.gs_gpio == gpio_num("GPIOO3")) {
    if (pal_get_hand_sw(&slot_id)) {
      slot_id = HAND_SW_BMC;
    }

    slot_id = (slot_id >= HAND_SW_BMC) ? HAND_SW_SERVER1 : (slot_id + 1);
    sprintf(locstr, "%u", slot_id);
    edb_cache_set("spb_hand_sw", locstr);
    syslog(LOG_INFO, "change hand_sw location to FRU %s by button", locstr);
  }
}

static void *
hsvc_event_handler(void *ptr) {

  int ret=-1;
  uint8_t value;
  uint8_t status;
  char vpath[80] = {0};
  char hspath[80] = {0};
  char cmd[128] = {0};
  char slotrcpath[80] = {0};
  char hslotpid[80] = {0};
  char slot_kv[80] = {0};
  int i=0;
  hot_service_info *hsvc_info = (hot_service_info *)ptr;
  pthread_detach(pthread_self());


  switch(hsvc_info->action)
  {
    case REMOVAl :
      usleep(250000);   //Delay 250ms to wait for slot present pin status stable
      ret = pal_is_fru_prsnt(hsvc_info->slot_id, &value);
      if (ret < 0) {
        syslog(LOG_ERR, "%s pal_is_fru_prsnt failed for fru: %d\n", __func__, hsvc_info->slot_id);
        break;
      }
      if (value) {
        printf("Not remove entirely\n");
      }
      else {   //Card has been removed
        pal_baseboard_clock_control(hsvc_info->slot_id, "1"); // Disable baseboard clock passing buffer to prevent voltage leakage
        ret = pal_is_server_12v_on(hsvc_info->slot_id, &value);    /* Check whether the system is 12V off or on */
        if (ret < 0) {
          syslog(LOG_ERR, "pal_get_server_power: pal_is_server_12v_on failed");
          break;
        }
        if (value) {

          syslog(LOG_CRIT, fru_prsnt_log_string[2*MAX_NUM_FRUS + hsvc_info->slot_id]);     //Card removal without 12V-off
          memset(vpath, 0, sizeof(vpath));
          sprintf(vpath, GPIO_VAL, gpio_12v[hsvc_info->slot_id]);
          if (write_device(vpath, "0")) {        /* Turn off 12V to given slot when Server/GP/CF be removed brutally */
            break;
          }
          ret = pal_slot_pair_12V_off(hsvc_info->slot_id);  /* Turn off 12V to pair of slots when Server/GP/CF be removed brutally with pair config */
          if (0 != ret)
            printf("pal_slot_pair_12V_off failed for fru: %d\n", hsvc_info->slot_id);
        }
        else
          syslog(LOG_CRIT, fru_prsnt_log_string[hsvc_info->slot_id]);     //Card removal with 12V-off

        // Re-init kv list
        for(i=0; i < sizeof(slot_kv_list)/sizeof(slot_kv_st); i++) {
          memset(slot_kv, 0, sizeof(slot_kv));
          sprintf(slot_kv, slot_kv_list[i].slot_key, hsvc_info->slot_id);
          if ((ret = pal_set_key_value(slot_kv, slot_kv_list[i].slot_def_val)) < 0) {
            syslog(LOG_WARNING, "%s pal_set_def_key_value: kv_set failed. %d", __func__, ret);
          }
        }

        // Create file for 12V-on re-init
        sprintf(hspath, HOTSERVICE_FILE, hsvc_info->slot_id);
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd,"touch %s",hspath);
        system(cmd);

        // Assign slot type
        sprintf(slotrcpath, SLOT_RECORD_FILE, hsvc_info->slot_id);
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd, "echo %d > %s", fby2_get_slot_type(hsvc_info->slot_id), slotrcpath);
        system(cmd);
      }
      break;
    case INSERTION :
      sleep(5);   //Delay 5 seconds to wait for slot present pin status stable
      ret = pal_is_fru_prsnt(hsvc_info->slot_id, &value);
      if (ret < 0) {
        syslog(LOG_ERR, "%s pal_is_fru_prsnt failed for fru: %d\n", __func__, hsvc_info->slot_id);
        break;
      }
      if (value) {      //Card has been inserted
        syslog(LOG_CRIT, fru_prsnt_log_string[MAX_NUM_FRUS + hsvc_info->slot_id]);

        // Create file for 12V-on re-init
        sprintf(hspath, HOTSERVICE_FILE, hsvc_info->slot_id);
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd,"touch %s",hspath);
        system(cmd);

        // Assign slot type
        sprintf(slotrcpath, SLOT_RECORD_FILE, hsvc_info->slot_id);
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd, "echo %d > %s", fby2_get_slot_type(hsvc_info->slot_id), slotrcpath);
        system(cmd);

        // Remove pid_file
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd,"rm -f %s",hslotpid);
        system(cmd);

        /* Check whether the system is 12V off or on */
        ret = pal_is_server_12v_on(hsvc_info->slot_id, &status);
        if (ret < 0) {
          syslog(LOG_ERR, "pal_get_server_power: pal_is_server_12v_on failed");
          break;
        }
        if (!status) {
          sprintf(cmd, "/usr/local/bin/power-util slot%u 12V-on &",hsvc_info->slot_id);
          system(cmd);
        }
      }
      else {
        printf("Not insert entirely\n");
      }
      break;
  }

  pthread_mutex_lock(&hsvc_mutex[hsvc_info->slot_id]);
  IsHotServiceStart[hsvc_info->slot_id] = false;
  pthread_mutex_unlock(&hsvc_mutex[hsvc_info->slot_id]);

  pthread_exit(0);
}

static gpio_poll_st g_gpios[] = {
  // {{gpio, fd}, edge, gpioValue, call-back function, GPIO description}
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOH5",  "GPIO_FAN_LATCH_DETECT"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOP0",  "GPIO_SLOT1_EJECTOR_LATCH_DETECT_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOP1",  "GPIO_SLOT2_EJECTOR_LATCH_DETECT_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOP2",  "GPIO_SLOT3_EJECTOR_LATCH_DETECT_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOP3",  "GPIO_SLOT4_EJECTOR_LATCH_DETECT_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOZ0",  "GPIO_SLOT1_PRSNT_B_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOZ1",  "GPIO_SLOT2_PRSNT_B_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOZ2",  "GPIO_SLOT3_PRSNT_B_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOZ3",  "GPIO_SLOT4_PRSNT_B_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOAA0", "GPIO_SLOT1_PRSNT_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOAA1", "GPIO_SLOT2_PRSNT_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOAA2", "GPIO_SLOT3_PRSNT_N"},
  {{0, 0}, GPIO_EDGE_BOTH,    0, gpio_event_handle, "GPIOAA3", "GPIO_SLOT4_PRSNT_N"},
  {{0, 0}, GPIO_EDGE_FALLING, 0, gpio_event_handle, "GPIOO3",  "GPIO_UART_SEL"},
};

static int g_count = sizeof(g_gpios) / sizeof(gpio_poll_st);

#if DEBUG_ME_EJECTOR_LOG // Enable log "GPIO_SLOTX_EJECTOR_LATCH_DETECT_N is 1 and SLOT_12v is ON" before mechanism issue is fixed
static def_chk_info def_gpio_chk[] = {
  // { default value, gpio name, gpio num, log }
  { 0, "GPIO_SLOT1_EJECTOR_LATCH_DETECT_N", GPIO_SLOT1_EJECTOR_LATCH_DETECT_N, "GPIO_SLOT1_EJECTOR_LATCH_DETECT_N is \"1\" and SLOT_12v is ON" },
  { 0, "GPIO_SLOT2_EJECTOR_LATCH_DETECT_N", GPIO_SLOT2_EJECTOR_LATCH_DETECT_N, "GPIO_SLOT2_EJECTOR_LATCH_DETECT_N is \"1\" and SLOT_12v is ON" },
  { 0, "GPIO_SLOT3_EJECTOR_LATCH_DETECT_N", GPIO_SLOT3_EJECTOR_LATCH_DETECT_N, "GPIO_SLOT3_EJECTOR_LATCH_DETECT_N is \"1\" and SLOT_12v is ON" },
  { 0, "GPIO_SLOT4_EJECTOR_LATCH_DETECT_N", GPIO_SLOT4_EJECTOR_LATCH_DETECT_N, "GPIO_SLOT4_EJECTOR_LATCH_DETECT_N is \"1\" and SLOT_12v is ON" },
  { 0, "GPIO_FAN_LATCH_DETECT",             GPIO_FAN_LATCH_DETECT,             "ASSERT: SLED is not seated"                                    },
};

static void default_gpio_check(void) {
  int i;
  int value;
  char vpath[80] = {0};

  for (i=0; i<sizeof(def_gpio_chk)/sizeof(def_chk_info); i++) {
    sprintf(vpath, GPIO_VAL, def_gpio_chk[i].num);
    read_device(vpath, &value);
    if (value != def_gpio_chk[i].def_val)
      syslog(LOG_CRIT, def_gpio_chk[i].log);
  }
}
#endif

int
main(int argc, void **argv) {
  int dev, rc, pid_file;
  uint8_t status = 0;
  int i;

  for(i=1 ;i<MAX_NODES + 1; i++)
  {
    pthread_mutex_init(&hsvc_mutex[i], NULL);
    last_ejector_ts[i].tv_sec = 0;
  }

#if DEBUG_ME_EJECTOR_LOG // Enable log "GPIO_SLOTX_EJECTOR_LATCH_DETECT_N is 1 and SLOT_12v is ON" before mechanism issue is fixed
  default_gpio_check();
#endif

  pid_file = open("/var/run/gpiointrd.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      syslog(LOG_ERR, "Another gpiointrd instance is running...\n");
      exit(-1);
    }
  } else {
    openlog("gpiointrd", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "gpiointrd: daemon started");

    gpio_poll_open(g_gpios, g_count);
    gpio_poll(g_gpios, g_count, POLL_TIMEOUT);
    gpio_poll_close(g_gpios, g_count);
  }

  for(i=1; i<MAX_NODES + 1; i++)
  {
    pthread_mutex_destroy(&hsvc_mutex[i]);
  }

  return 0;
}
