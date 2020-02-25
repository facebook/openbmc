/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include "pal.h"
#include "pal_sensors.h"
#include <openbmc/misc-utils.h>
#include <openbmc/vr.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-sensors.h>
#include <sys/stat.h>
#include <openbmc/libgpio.h>
#include <openbmc/kv.h>
#include <openbmc/sensor-correction.h>

#define BIT(value, index) ((value >> index) & 1)

#define FBTP_PLATFORM_NAME "fbtp"
#define LAST_KEY "last_key"
#define FBTP_MAX_NUM_SLOTS 1

#define PAGE_SIZE  0x1000
#define AST_SCU_BASE 0x1e6e2000
#define PIN_CTRL1_OFFSET 0x80
#define PIN_CTRL2_OFFSET 0x84
#define RST_STS_OFFSET 0x3c

#define AST_LPC_BASE 0x1e789000
#define HICRA_OFFSET 0x9C
#define HICRA_MASK_UART1 0x70000
#define HICRA_MASK_UART2 0x380000
#define HICRA_MASK_UART3 0x1C00000
#define UART1_TO_UART3 0x5
#define UART2_TO_UART3 0x6
#define UART3_TO_UART1 0x5
#define UART3_TO_UART2 0x4
#define DEBUG_TO_UART1 0x0

#define UART1_TXD (1 << 22)
#define UART2_TXD (1 << 30)
#define UART3_TXD (1 << 22)
#define UART4_TXD (1 << 30)

#define DELAY_GRACEFUL_SHUTDOWN 1
#define DELAY_POWER_OFF 6
#define DELAY_POWER_CYCLE 10

#define CRASHDUMP_BIN       "/usr/local/bin/dump.sh"
#define CRASHDUMP_FILE      "/mnt/data/crashdump_"

#define LARGEST_DEVICE_NAME 120


#define EEPROM_RISER     "/sys/bus/i2c/devices/1-0050/eeprom"
#define EEPROM_RETIMER   "/sys/bus/i2c/devices/3-0054/eeprom"

#define MAX_SENSOR_NUM 0xFF
#define ALL_BYTES 0xFF
#define LAST_REC_ID 0xFFFF

#define RISER_BUS_ID 0x1

#define GUID_SIZE 16
#define OFFSET_SYS_GUID 0x17F0
#define OFFSET_DEV_GUID 0x1800
#define FRU_EEPROM     "/sys/bus/i2c/devices/6-0054/eeprom"

#define READING_NA -2
#define READING_SKIP 1

#define NIC_MAX_TEMP 125
#define NIC_MIN_TEMP  0

#define MAX_READ_RETRY 10

#define CPLD_BUS_ID 0x6
#define CPLD_ADDR 0xA0

#define BIOS_VER_REGION_SIZE (4*1024*1024)
#define BIOS_ERASE_PKT_SIZE  (64*1024)

const char pal_fru_list[] = "all, mb, nic, riser_slot2, riser_slot3, riser_slot4";
const char pal_server_list[] = "mb";

size_t pal_pwm_cnt = 2;
size_t pal_tach_cnt = 2;
const char pal_pwm_list[] = "0, 1";
const char pal_tach_list[] = "0, 1";

static uint8_t g_plat_id = 0x0;
static uint8_t postcodes_last[256] = {0};

static int key_func_por_policy (int event, void *arg);
static int key_func_lps (int event, void *arg);

enum key_event {
  KEY_BEFORE_SET,
  KEY_AFTER_INI,
};

struct pal_key_cfg {
  char *name;
  char *def_val;
  int (*function)(int, void*);
} key_cfg[] = {
  /* name, default value, function */
  {"pwr_server_last_state", "on", key_func_lps},
  {"sysfw_ver_server", "0", NULL},
  {"identify_sled", "off", NULL},
  {"timestamp_sled", "0", NULL},
  {"server_por_cfg", "lps", key_func_por_policy},
  {"server_sensor_health", "1", NULL},
  {"nic_sensor_health", "1", NULL},
  {"server_sel_error", "1", NULL},
  {"server_boot_order", "0100090203ff", NULL},
  {"ntp_server", "", NULL},
  /* Add more Keys here */
  {LAST_KEY, LAST_KEY, NULL} /* This is the last key of the list */
};

//control mux based on the bus and channel
int
pal_control_mux_to_target_ch(uint8_t channel, uint8_t bus, uint8_t mux_addr)
{
  int ret;
  int fd;
  char fn[32];
  uint8_t retry;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if (fd < 0)
  {
    syslog(LOG_WARNING,"[%s]Cannot open bus %d", __func__, bus);
    ret = PAL_ENOTSUP;
    goto error_exit;
  }

  if (channel < 4)
  {
    tbuf[0] = 0x04 + channel;
  }
  else
  {
    tbuf[0] = 0x00; // close all channels
  }

  retry = MAX_READ_RETRY;
  while ( retry > 0 )
  {
    ret = i2c_rdwr_msg_transfer(fd, mux_addr, tbuf, 1, rbuf, 0);
    if ( PAL_EOK == ret )
    {
      break;
    }

    msleep(50);
    retry--;
  }

  if ( ret < 0 )
  {
    syslog(LOG_WARNING,"[%s] Cannot switch the mux on bus %d", __func__, bus);
    goto error_exit;
  }

error_exit:

  if ( fd > 0 )
  {
    close(fd);
  }

  return ret;
}

static int
pal_control_mux(int fd, uint8_t addr, uint8_t channel) {
  uint8_t tcount = 1, rcount = 0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  // PCA9544A
  if (channel < 4)
    tbuf[0] = 0x04 + channel;
  else
    tbuf[0] = 0x00; // close all channels

  return i2c_rdwr_msg_transfer(fd, addr, tbuf, tcount, rbuf, rcount);
}

static int
pal_control_switch(int fd, uint8_t addr, uint8_t channel) {
  uint8_t tcount = 1, rcount = 0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  // PCA9846
  if (channel < 4)
    tbuf[0] = 0x01 << channel;
  else
    tbuf[0] = 0x00; // close all channels

  return i2c_rdwr_msg_transfer(fd, addr, tbuf, tcount, rbuf, rcount);
}

// Shared Memory data accrossing processes
struct mux_shm {
  pthread_mutex_t mutex;
  pthread_cond_t unlock, free;
  int locked;
  int using_num;
  time_t expiration;
  uint8_t chan, ipmb_chan;
};

// Data in each process
struct mux {
  pthread_once_t init;
  void (*init_func)(void);
  int bus_fd, addr, wait_time;
  struct mux_shm *shm;
};

static void init_mux_data_riser_mux(void);
struct mux riser_mux = {
  .init = PTHREAD_ONCE_INIT,
  .init_func = init_mux_data_riser_mux,
  .bus_fd = -1,
  .addr = 0xe2,
  .wait_time = 3,
  .shm = NULL,
};
static void init_mux_data_riser_mux(void) {
  int fd;
  struct stat st;
  pthread_mutexattr_t mutex_attr;
  pthread_condattr_t cond_attr;
  char path[128];
  uint8_t slot_cfg;

  fd = shm_open("/mux_data_riser_mux", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  flock(fd, LOCK_EX);

  fstat(fd, &st);
  if (ftruncate(fd, sizeof(struct mux_shm)) != 0) {
    syslog(LOG_ERR, "Setting size of SHM failed!\n");
  }
  riser_mux.shm = (struct mux_shm *)mmap(NULL, sizeof(struct mux_shm),
    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  // Initialize shared memory
  if (st.st_size < sizeof(struct mux_shm)) {
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&mutex_attr, PTHREAD_MUTEX_ROBUST);
    pthread_mutex_init(&riser_mux.shm->mutex, &mutex_attr);
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&riser_mux.shm->unlock, &cond_attr);
    pthread_cond_init(&riser_mux.shm->free, &cond_attr);
    riser_mux.shm->locked = 0;
    riser_mux.shm->using_num = 0;
    riser_mux.shm->expiration = 0;
    riser_mux.shm->chan = 0xff;

    if (pal_get_slot_cfg_id(&slot_cfg) < 0)
      slot_cfg = SLOT_CFG_EMPTY;

    // default channel to top slot
    if(slot_cfg == SLOT_CFG_SS_3x8)
      riser_mux.shm->ipmb_chan = 2;
    else
      riser_mux.shm->ipmb_chan = 1;
  }

  flock(fd, LOCK_UN);
  close(fd);

  snprintf(path, sizeof(path), "/dev/i2c-%d", RISER_BUS_ID);
  riser_mux.bus_fd = open(path, O_RDWR);
  if (riser_mux.bus_fd < 0) {
    syslog(LOG_ERR, "Cannot open %s\n", path);
  }
}

static struct mux_shm *mux_get_shm(struct mux *mux) {
  pthread_once(&(mux->init), mux->init_func);
  return mux->shm;
}

static void shm_lock(struct mux_shm *shm)
{
  int rc = pthread_mutex_lock(&shm->mutex);
  if (rc == EOWNERDEAD) {
    syslog(LOG_WARNING, "Trying to recover riser mutex due to dead-owner\n");
    rc = pthread_mutex_consistent(&shm->mutex);
    if (rc != 0) {
      syslog(LOG_ERR, "Failed to recover riser mutex after dead-owner: %s\n", strerror(rc));
    } else {
      // Switch to defaults.
      shm->locked = 0;
      shm->using_num = 0;
      shm->expiration = 0;
      shm->chan = 0xff;
      syslog(LOG_ERR, "Successfully recovered riser mutex after a dead-owner");
      // Let any other "expired" user's lease expire.
      sleep(1);
    }
  } else if (rc != 0) {
    syslog(LOG_ERR, "Failed to lock riser mux: %s\n", strerror(rc));
  }
}

static void shm_unlock(struct mux_shm *shm)
{
  pthread_mutex_unlock(&shm->mutex);
}

/*
 * Release the mux
 */
static int mux_release (struct mux *mux)
{
  struct mux_shm *shm = mux_get_shm(mux);
  int ret = 0;
  uint8_t old_chan;

  shm_lock(shm);
  old_chan = shm->chan;
  if (shm->chan != shm->ipmb_chan) {
    ret = pal_control_mux(mux->bus_fd, mux->addr, shm->ipmb_chan);
  }
  shm->chan = shm->ipmb_chan;
  shm_unlock(shm);

  //send online command out side of mutex
  if(old_chan != shm->ipmb_chan) {
    if(pal_is_BBV_prsnt())
      notify_BBV_ipmb_offline_online(1,0);
  }
  shm_lock(shm);
  shm->locked = 0;
  pthread_cond_broadcast(&shm->unlock);
  shm_unlock(shm);

  return ret;
}
/*
 * Request to use the mux on the channel
 */
static int mux_using (struct mux *mux)
{
  struct mux_shm *shm = mux_get_shm(mux);
  int ret = -1;
  struct timespec to;

  clock_gettime(CLOCK_REALTIME, &to);
  to.tv_sec += mux->wait_time;

  shm_lock(shm);
  if (shm->locked == 1 && time(NULL) > shm->expiration) {
    mux_release(mux);
  }

  while(1) {
    if (shm->chan == shm->ipmb_chan) {
      ret = 0;
      break;
    }
    ret = pthread_cond_timedwait(&shm->unlock, &shm->mutex, &to);
    if (ret == ETIMEDOUT) {
      syslog(LOG_WARNING, "%s wait unlock timeout\n", __func__);
      break;
    }
  }

  shm->using_num++;
  shm_unlock(shm);

  return ret;
}
/*
 * Notify that using finished
 */
static int mux_finish (struct mux *mux)
{
  struct mux_shm *shm = mux_get_shm(mux);
  shm_lock(shm);
  if (shm->using_num > 0)
    shm->using_num--;
  pthread_cond_broadcast(&shm->free);
  shm_unlock(shm);
  return 0;
}
/*
 * Request to lock the mux on the channel, doesn't allow other to use
 * Return 0 on success, other on failure
 */
static int mux_lock (struct mux *mux, int chan, int lease_time)
{
  struct mux_shm *shm = mux_get_shm(mux);
  int ret = -1;
  struct timespec to;

  clock_gettime(CLOCK_REALTIME, &to);
  to.tv_sec += mux->wait_time;

  shm_lock(shm);
  if (shm->locked == 1 && time(NULL) > shm->expiration) {
    mux_release(mux);
  }

  // Announce to lock this resource
  while(1) {
    if (shm->locked == 0) {
      shm->expiration = time(NULL) + lease_time;
      shm->locked = 1;
      shm_unlock(shm);
      //send offline command out side of mutex
      if(shm->ipmb_chan != chan) {
        if(pal_is_BBV_prsnt())
          notify_BBV_ipmb_offline_online(0,mux->wait_time);
      }
      shm_lock(shm);
      shm->chan = chan;
      ret = 0;
      break;
    }
    ret = pthread_cond_timedwait(&shm->unlock, &shm->mutex, &to);
    if (ret == ETIMEDOUT) {
      ret = -ETIMEDOUT;
      syslog(LOG_WARNING, "%s wait unlock timeout\n", __func__);
      break;
    }
  }

  // Wait for all ipmb transaction finished before switch channel
  if (ret == 0 && shm->ipmb_chan != chan) {
    clock_gettime(CLOCK_REALTIME, &to);
    to.tv_sec += mux->wait_time;
    shm->expiration += mux->wait_time;
    while (1) {
      if (shm->using_num == 0) {
        break;
      }
      ret = pthread_cond_timedwait(&shm->free, &shm->mutex, &to);
      if (ret == ETIMEDOUT) {
        syslog(LOG_WARNING, "%s wait free timeout\n", __func__);
        break;
      }
    }

    // switch MUX no matter bus free or time-out
    ret = pal_control_mux(mux->bus_fd, mux->addr, chan);
    if (ret != 0) {
      // unlock if failed
      pal_control_mux(mux->bus_fd, mux->addr, shm->ipmb_chan);
      shm->chan = shm->ipmb_chan;
      shm->locked = 0;
      pthread_cond_broadcast(&shm->unlock);
    }
  }
  shm_unlock(shm);

  return ret;
}

int pal_ipmb_processing(int bus, void *buf, uint16_t size)
{
  if (bus == RISER_BUS_ID)
    return mux_using(&riser_mux);

  return 0;
}

int pal_ipmb_finished(int bus, void *buf, uint16_t size)
{
  if (bus == RISER_BUS_ID)
    return mux_finish(&riser_mux);

  return 0;
}

int
notify_BBV_ipmb_offline_online(uint8_t on_off, int off_sec) {
  int rlen = 0;

  rlen = ipmb_send(
    RISER_BUS_ID,
    0x2c,
    NETFN_OEM_REQ << 2,
    CMD_OEM_SET_IPMB_OFFONLINE,
    0x4C,
    0x1C,
    0x00,
    on_off,
    off_sec & 0xff,
    off_sec >> 8);

  if ( rlen < 0 )
  {
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(%d): Zero bytes received\n", __func__, __LINE__);
#endif
    return -1;
  }
  else
  {
    if (ipmb_rxb()->cc == 0) {
      return 0;
    } else {
      syslog(LOG_DEBUG, "%s(%d): fail com_code:%x \n", __func__, __LINE__,ipmb_rxb()->cc);
      return -1;
    }
  }
}

// List of MB sensors to be monitored
const uint8_t mb_sensor_list[] = {
  MB_SENSOR_INLET_TEMP,
  MB_SENSOR_OUTLET_TEMP,
  MB_SENSOR_INLET_REMOTE_TEMP,
  MB_SENSOR_OUTLET_REMOTE_TEMP,
  MB_SENSOR_FAN0_TACH,
  MB_SENSOR_FAN1_TACH,
  MB_SENSOR_P3V3,
  MB_SENSOR_P5V,
  MB_SENSOR_P12V,
  MB_SENSOR_P1V05,
  MB_SENSOR_PVNN_PCH_STBY,
  MB_SENSOR_P3V3_STBY,
  MB_SENSOR_P5V_STBY,
  MB_SENSOR_P3V_BAT,
  MB_SENSOR_HSC_IN_VOLT,
  MB_SENSOR_HSC_OUT_CURR,
  MB_SENSOR_HSC_TEMP,
  MB_SENSOR_HSC_IN_POWER,
  MB_SENSOR_CPU0_TEMP,
  MB_SENSOR_CPU0_TJMAX,
  MB_SENSOR_CPU0_PKG_POWER,
  MB_SENSOR_CPU0_THERM_MARGIN,
  MB_SENSOR_CPU1_TEMP,
  MB_SENSOR_CPU1_TJMAX,
  MB_SENSOR_CPU1_PKG_POWER,
  MB_SENSOR_CPU1_THERM_MARGIN,
  MB_SENSOR_PCH_TEMP,
  MB_SENSOR_CPU0_DIMM_GRPA_TEMP,
  MB_SENSOR_CPU0_DIMM_GRPB_TEMP,
  MB_SENSOR_CPU1_DIMM_GRPC_TEMP,
  MB_SENSOR_CPU1_DIMM_GRPD_TEMP,
  MB_SENSOR_VR_CPU0_VCCIN_TEMP,
  MB_SENSOR_VR_CPU0_VCCIN_CURR,
  MB_SENSOR_VR_CPU0_VCCIN_VOLT,
  MB_SENSOR_VR_CPU0_VCCIN_POWER,
  MB_SENSOR_VR_CPU0_VSA_TEMP,
  MB_SENSOR_VR_CPU0_VSA_CURR,
  MB_SENSOR_VR_CPU0_VSA_VOLT,
  MB_SENSOR_VR_CPU0_VSA_POWER,
  MB_SENSOR_VR_CPU0_VCCIO_TEMP,
  MB_SENSOR_VR_CPU0_VCCIO_CURR,
  MB_SENSOR_VR_CPU0_VCCIO_VOLT,
  MB_SENSOR_VR_CPU0_VCCIO_POWER,
  MB_SENSOR_VR_CPU0_VDDQ_GRPA_TEMP,
  MB_SENSOR_VR_CPU0_VDDQ_GRPA_CURR,
  MB_SENSOR_VR_CPU0_VDDQ_GRPA_VOLT,
  MB_SENSOR_VR_CPU0_VDDQ_GRPA_POWER,
  MB_SENSOR_VR_CPU0_VDDQ_GRPB_TEMP,
  MB_SENSOR_VR_CPU0_VDDQ_GRPB_CURR,
  MB_SENSOR_VR_CPU0_VDDQ_GRPB_VOLT,
  MB_SENSOR_VR_CPU0_VDDQ_GRPB_POWER,
  MB_SENSOR_VR_CPU1_VCCIN_TEMP,
  MB_SENSOR_VR_CPU1_VCCIN_CURR,
  MB_SENSOR_VR_CPU1_VCCIN_VOLT,
  MB_SENSOR_VR_CPU1_VCCIN_POWER,
  MB_SENSOR_VR_CPU1_VSA_TEMP,
  MB_SENSOR_VR_CPU1_VSA_CURR,
  MB_SENSOR_VR_CPU1_VSA_VOLT,
  MB_SENSOR_VR_CPU1_VSA_POWER,
  MB_SENSOR_VR_CPU1_VCCIO_TEMP,
  MB_SENSOR_VR_CPU1_VCCIO_CURR,
  MB_SENSOR_VR_CPU1_VCCIO_VOLT,
  MB_SENSOR_VR_CPU1_VCCIO_POWER,
  MB_SENSOR_VR_CPU1_VDDQ_GRPC_TEMP,
  MB_SENSOR_VR_CPU1_VDDQ_GRPC_CURR,
  MB_SENSOR_VR_CPU1_VDDQ_GRPC_VOLT,
  MB_SENSOR_VR_CPU1_VDDQ_GRPC_POWER,
  MB_SENSOR_VR_CPU1_VDDQ_GRPD_TEMP,
  MB_SENSOR_VR_CPU1_VDDQ_GRPD_CURR,
  MB_SENSOR_VR_CPU1_VDDQ_GRPD_VOLT,
  MB_SENSOR_VR_CPU1_VDDQ_GRPD_POWER,
  MB_SENSOR_VR_PCH_PVNN_TEMP,
  MB_SENSOR_VR_PCH_PVNN_CURR,
  MB_SENSOR_VR_PCH_PVNN_VOLT,
  MB_SENSOR_VR_PCH_PVNN_POWER,
  MB_SENSOR_VR_PCH_P1V05_TEMP,
  MB_SENSOR_VR_PCH_P1V05_CURR,
  MB_SENSOR_VR_PCH_P1V05_VOLT,
  MB_SENSOR_VR_PCH_P1V05_POWER,
  MB_SENSOR_CONN_P12V_INA230_VOL,
  MB_SENSOR_CONN_P12V_INA230_CURR,
  MB_SENSOR_CONN_P12V_INA230_PWR,
  MB_SENSOR_HOST_BOOT_TEMP,
};

// List of NIC sensors to be monitored
const uint8_t nic_sensor_list[] = {
  MEZZ_SENSOR_TEMP,
};

// List of MB discrete sensors to be monitored
const uint8_t mb_discrete_sensor_list[] = {
  MB_SENSOR_POWER_FAIL,
  MB_SENSOR_MEMORY_LOOP_FAIL,
  MB_SENSOR_PROCESSOR_FAIL,
};

const uint8_t riser_slot2_sensor_list[] = {
  MB_SENSOR_C2_AVA_FTEMP,
  MB_SENSOR_C2_AVA_RTEMP,
  MB_SENSOR_C2_NVME_CTEMP,
  MB_SENSOR_C2_1_NVME_CTEMP,
  MB_SENSOR_C2_2_NVME_CTEMP,
  MB_SENSOR_C2_3_NVME_CTEMP,
  MB_SENSOR_C2_4_NVME_CTEMP,
  MB_SENSOR_C2_P12V_INA230_VOL,
  MB_SENSOR_C2_P12V_INA230_CURR,
  MB_SENSOR_C2_P12V_INA230_PWR,
};

const uint8_t riser_slot3_sensor_list[] = {
  MB_SENSOR_C3_AVA_FTEMP,
  MB_SENSOR_C3_AVA_RTEMP,
  MB_SENSOR_C3_NVME_CTEMP,
  MB_SENSOR_C3_1_NVME_CTEMP,
  MB_SENSOR_C3_2_NVME_CTEMP,
  MB_SENSOR_C3_3_NVME_CTEMP,
  MB_SENSOR_C3_4_NVME_CTEMP,
  MB_SENSOR_C3_P12V_INA230_VOL,
  MB_SENSOR_C3_P12V_INA230_CURR,
  MB_SENSOR_C3_P12V_INA230_PWR,
};

const uint8_t riser_slot4_sensor_list[] = {
  MB_SENSOR_C4_AVA_FTEMP,
  MB_SENSOR_C4_AVA_RTEMP,
  MB_SENSOR_C4_NVME_CTEMP,
  MB_SENSOR_C4_1_NVME_CTEMP,
  MB_SENSOR_C4_2_NVME_CTEMP,
  MB_SENSOR_C4_3_NVME_CTEMP,
  MB_SENSOR_C4_4_NVME_CTEMP,
  MB_SENSOR_C4_P12V_INA230_VOL,
  MB_SENSOR_C4_P12V_INA230_CURR,
  MB_SENSOR_C4_P12V_INA230_PWR,
};

float mb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float nic_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float riser_slot2_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float riser_slot3_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float riser_slot4_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};


size_t mb_sensor_cnt = sizeof(mb_sensor_list)/sizeof(uint8_t);
size_t nic_sensor_cnt = sizeof(nic_sensor_list)/sizeof(uint8_t);
size_t mb_discrete_sensor_cnt = sizeof(mb_discrete_sensor_list)/sizeof(uint8_t);
size_t riser_slot2_sensor_cnt = sizeof(riser_slot2_sensor_list)/sizeof(uint8_t);
size_t riser_slot3_sensor_cnt = sizeof(riser_slot3_sensor_list)/sizeof(uint8_t);
size_t riser_slot4_sensor_cnt = sizeof(riser_slot4_sensor_list)/sizeof(uint8_t);

char g_sys_guid[GUID_SIZE] = {0};
char g_dev_guid[GUID_SIZE] = {0};

static uint8_t g_board_rev_id = BOARD_REV_EVT;
static uint8_t g_vr_cpu0_vddq_abc;
static uint8_t g_vr_cpu0_vddq_def;
static uint8_t g_vr_cpu1_vddq_ghj;
static uint8_t g_vr_cpu1_vddq_klm;

static char *dimm_label_SS_DVT[12] = {
  "A0", "A1", "A2", "B0", "B1", "B2", "C0", "C1", "C2", "D0", "D1", "D2"};

static char *dimm_label_SS_PVT[12] = {
  "A0", "A1", "A2", "A3", "A4", "A5", "B0", "B1", "B2", "B3", "B4", "B5"};

static char *dimm_label_DS_DVT[24] = {
  "A0", "A3", "A1", "A4", "A2", "A5", "B0", "B3", "B1", "B4", "B2", "B5", \
  "C0", "C3", "C1", "C4", "C2", "C5", "D0", "D3", "D1", "D4", "D2", "D5"};

static char *dimm_label_DS_PVT[24] = {
  "A0", "C0", "A1", "C1", "A2", "C2", "A3", "C3", "A4", "C4", "A5", "C5", \
  "B0", "D0", "B1", "D1", "B2", "D2", "B3", "D3", "B4", "D4", "B5", "D5", };

struct dimm_map {
  unsigned char index;
  char *label;
};

static bool is_cpu_socket_occupy(unsigned int cpu_id);
static void _print_sensor_discrete_log(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t val, char *event);

static void apply_inlet_correction(float *value) {
  int i;
  static int rpm[2] = {0};
  static bool rpm_valid[2] = {false, false};
  static bool inited = false;
  float avg_rpm = 0;
  uint8_t cnt = 0;

  // Get PWM value
  for (i = 0; i < 2; i ++) {
    if (pal_get_fan_speed(i, &rpm[i]) == 0 || rpm_valid[i] == true) {
      rpm_valid[i] = true;
      avg_rpm += (float)rpm[i];
      cnt++;
    }
  }
  if (cnt) {
    avg_rpm = avg_rpm / (float)cnt;
    if (!inited) {
      inited = true;
      sensor_correction_init("/etc/sensor-correction-conf.json");
    }
    sensor_correction_apply(FRU_MB, MB_SENSOR_INLET_REMOTE_TEMP, avg_rpm, value);
  }
}

static void
init_board_sensors(void) {
  pal_get_board_rev_id(&g_board_rev_id);

  if (g_board_rev_id == BOARD_REV_POWERON ||
      g_board_rev_id == BOARD_REV_EVT ) {
    g_vr_cpu0_vddq_abc = VR_CPU0_VDDQ_ABC_EVT;
    g_vr_cpu0_vddq_def = VR_CPU0_VDDQ_DEF_EVT;
    g_vr_cpu1_vddq_ghj = VR_CPU1_VDDQ_GHJ_EVT;
    g_vr_cpu1_vddq_klm = VR_CPU1_VDDQ_KLM_EVT;
  } else {
    g_vr_cpu0_vddq_abc = VR_CPU0_VDDQ_ABC;
    g_vr_cpu0_vddq_def = VR_CPU0_VDDQ_DEF;
    g_vr_cpu1_vddq_ghj = VR_CPU1_VDDQ_GHJ;
    g_vr_cpu1_vddq_klm = VR_CPU1_VDDQ_KLM;
  }
}

//Dynamic change CPU Temp threshold
static void
dyn_sensor_thresh_array_init() {
  static bool init_cpu0 = false;
  static bool init_cpu1 = false;
  static bool init_done = false;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};

  // Return if both cpu thresholds are initialized
  if (init_done) {
    return;
  }

  // Checkd if cpu0 threshold needs to be initialized
  if (init_cpu0) {
    goto dyn_cpu1_init;
  }

  sprintf(key, "mb_sensor%d", MB_SENSOR_CPU0_TJMAX);
  if( kv_get(key,str,NULL,0) >= 0 && (float) (strtof(str, NULL) - 2) > 0) {
    mb_sensor_threshold[MB_SENSOR_CPU0_TEMP][UCR_THRESH] = (float) (strtof(str, NULL) - 2);
    init_cpu0 = true;
  }else{
    mb_sensor_threshold[MB_SENSOR_CPU0_TEMP][UCR_THRESH] = 91;
  }

  // Check if cpu1 threshold needs to be initialized
dyn_cpu1_init:
  if (init_cpu1) {
    goto dyn_thresh_exit;
  }

  sprintf(key, "mb_sensor%d", MB_SENSOR_CPU1_TJMAX);
  if( kv_get(key,str,NULL,0) >= 0 && (float) (strtof(str, NULL) - 2) > 0 ) {
    mb_sensor_threshold[MB_SENSOR_CPU1_TEMP][UCR_THRESH] = (float) (strtof(str, NULL) - 2);
    init_cpu1 = true;
  }else{
    mb_sensor_threshold[MB_SENSOR_CPU1_TEMP][UCR_THRESH] = 91;
  }

  // Mark init complete only if both thresholds are initialized
dyn_thresh_exit:
  if (init_cpu0 && init_cpu1) {
    init_done = true;
  }
}

static void
sensor_thresh_array_init() {
  static bool init_done = false;

  dyn_sensor_thresh_array_init();

  if (init_done)
    return;

  mb_sensor_threshold[MB_SENSOR_INLET_REMOTE_TEMP][UCR_THRESH] = 40;
  mb_sensor_threshold[MB_SENSOR_OUTLET_REMOTE_TEMP][UCR_THRESH] = 90;

  // Assign UCT based on the system is Single Side or Double Side
  if (!(pal_get_platform_id(&g_plat_id)) && !(g_plat_id & PLAT_ID_SKU_MASK)) {
    // Single side 10k RPM fans.
    mb_sensor_threshold[MB_SENSOR_FAN0_TACH][UNC_THRESH] = 8500;
    mb_sensor_threshold[MB_SENSOR_FAN1_TACH][UNC_THRESH] = 8500;
    mb_sensor_threshold[MB_SENSOR_FAN0_TACH][UCR_THRESH] = 11500;
    mb_sensor_threshold[MB_SENSOR_FAN1_TACH][UCR_THRESH] = 11500;
  } else {
    // Double side 15k RPM fans.
    mb_sensor_threshold[MB_SENSOR_FAN0_TACH][UNC_THRESH] = 13500;
    mb_sensor_threshold[MB_SENSOR_FAN1_TACH][UNC_THRESH] = 13500;
    mb_sensor_threshold[MB_SENSOR_FAN0_TACH][UCR_THRESH] = 17000;
    mb_sensor_threshold[MB_SENSOR_FAN1_TACH][UCR_THRESH] = 17000;
  }

  mb_sensor_threshold[MB_SENSOR_FAN0_TACH][LCR_THRESH] = 500;
  mb_sensor_threshold[MB_SENSOR_FAN1_TACH][LCR_THRESH] = 500;

  mb_sensor_threshold[MB_SENSOR_P3V3][UCR_THRESH] = 3.621;
  mb_sensor_threshold[MB_SENSOR_P3V3][LCR_THRESH] = 2.975;
  mb_sensor_threshold[MB_SENSOR_P5V][UCR_THRESH] = 5.486;
  mb_sensor_threshold[MB_SENSOR_P5V][LCR_THRESH] = 4.524;
  mb_sensor_threshold[MB_SENSOR_P12V][UCR_THRESH] = 13.23;
  mb_sensor_threshold[MB_SENSOR_P12V][LCR_THRESH] = 10.773;
  mb_sensor_threshold[MB_SENSOR_P1V05][UCR_THRESH] = 1.15;
  mb_sensor_threshold[MB_SENSOR_P1V05][LCR_THRESH] = 0.94;
  mb_sensor_threshold[MB_SENSOR_PVNN_PCH_STBY][UCR_THRESH] = 1.1;
  mb_sensor_threshold[MB_SENSOR_PVNN_PCH_STBY][LCR_THRESH] = 0.76;
  mb_sensor_threshold[MB_SENSOR_P3V3_STBY][UCR_THRESH] = 3.621;
  mb_sensor_threshold[MB_SENSOR_P3V3_STBY][LCR_THRESH] = 2.975;
  mb_sensor_threshold[MB_SENSOR_P5V_STBY][UCR_THRESH] = 5.486;
  mb_sensor_threshold[MB_SENSOR_P5V_STBY][LCR_THRESH] = 4.524;
  mb_sensor_threshold[MB_SENSOR_P3V_BAT][UCR_THRESH] = 3.738;
  mb_sensor_threshold[MB_SENSOR_P3V_BAT][LCR_THRESH] = 2.73;
  mb_sensor_threshold[MB_SENSOR_HSC_IN_VOLT][UCR_THRESH] = 13.2;
  mb_sensor_threshold[MB_SENSOR_HSC_IN_VOLT][LCR_THRESH] = 10.8;
  mb_sensor_threshold[MB_SENSOR_HSC_OUT_CURR][UCR_THRESH] = 52.8;
  mb_sensor_threshold[MB_SENSOR_HSC_IN_POWER][UCR_THRESH] = 792.0;
  mb_sensor_threshold[MB_SENSOR_PCH_TEMP][UCR_THRESH] = 82;
  mb_sensor_threshold[MB_SENSOR_CPU0_DIMM_GRPA_TEMP][UCR_THRESH] = 81;
  mb_sensor_threshold[MB_SENSOR_CPU0_DIMM_GRPB_TEMP][UCR_THRESH] = 81;
  mb_sensor_threshold[MB_SENSOR_CPU1_DIMM_GRPC_TEMP][UCR_THRESH] = 81;
  mb_sensor_threshold[MB_SENSOR_CPU1_DIMM_GRPD_TEMP][UCR_THRESH] = 81;

  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIN_TEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIN_CURR][UCR_THRESH] = 235;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIN_POWER][UCR_THRESH] = 414;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIN_VOLT][LCR_THRESH] = 1.45;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIN_VOLT][UCR_THRESH] = 2.05;

  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VSA_TEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VSA_CURR][UCR_THRESH] = 20;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VSA_POWER][UCR_THRESH] = 25;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VSA_VOLT][LCR_THRESH] = 0.45;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VSA_VOLT][UCR_THRESH] = 1.2;


  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIO_TEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIO_CURR][UCR_THRESH] = 24;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIO_POWER][UCR_THRESH] = 32;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIO_VOLT][LCR_THRESH] = 0.8;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VCCIO_VOLT][UCR_THRESH] = 1.2;


  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPA_TEMP][UCR_THRESH] = 95;
  if (!(pal_get_platform_id(&g_plat_id)) && !(g_plat_id & PLAT_ID_SKU_MASK)) {
    mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPA_CURR][UCR_THRESH] = 40;
    mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPA_POWER][UCR_THRESH] = 66;
  } else {
    mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPA_CURR][UCR_THRESH] = 95;
    mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPA_POWER][UCR_THRESH] = 115;
  }
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPA_VOLT][LCR_THRESH] = 1.08;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPA_VOLT][UCR_THRESH] = 1.32;

  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPB_TEMP][UCR_THRESH] = 95;
  if (!(pal_get_platform_id(&g_plat_id)) && !(g_plat_id & PLAT_ID_SKU_MASK)) {
    mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPB_CURR][UCR_THRESH] = 40;
    mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPB_POWER][UCR_THRESH] = 66;
  } else {
    mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPB_CURR][UCR_THRESH] = 95;
    mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPB_POWER][UCR_THRESH] = 115;
  }
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPB_VOLT][LCR_THRESH] = 1.08;
  mb_sensor_threshold[MB_SENSOR_VR_CPU0_VDDQ_GRPB_VOLT][UCR_THRESH] = 1.32;

  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIN_TEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIN_CURR][UCR_THRESH] = 235;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIN_POWER][UCR_THRESH] = 420;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIN_VOLT][LCR_THRESH] = 1.45;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIN_VOLT][UCR_THRESH] = 2.05;

  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VSA_TEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VSA_CURR][UCR_THRESH] = 20;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VSA_POWER][UCR_THRESH] = 25;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VSA_VOLT][LCR_THRESH] = 0.45;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VSA_VOLT][UCR_THRESH] = 1.2;

  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIO_TEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIO_CURR][UCR_THRESH] = 24;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIO_POWER][UCR_THRESH] = 32;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIO_VOLT][LCR_THRESH] = 0.8;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VCCIO_VOLT][UCR_THRESH] = 1.2;

  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPC_TEMP][UCR_THRESH] = 95;
  if (!(pal_get_platform_id(&g_plat_id)) && !(g_plat_id & PLAT_ID_SKU_MASK)) {
    mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPC_CURR][UCR_THRESH] = 40;
    mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPC_POWER][UCR_THRESH] = 66;
  } else {
    mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPC_CURR][UCR_THRESH] = 95;
    mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPC_POWER][UCR_THRESH] = 115;
  }
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPC_VOLT][LCR_THRESH] = 1.08;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPC_VOLT][UCR_THRESH] = 1.32;

  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPD_TEMP][UCR_THRESH] = 95;
  if (!(pal_get_platform_id(&g_plat_id)) && !(g_plat_id & PLAT_ID_SKU_MASK)) {
    mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPD_CURR][UCR_THRESH] = 40;
    mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPD_POWER][UCR_THRESH] = 66;
  } else {
    mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPD_CURR][UCR_THRESH] = 95;
    mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPD_POWER][UCR_THRESH] = 115;
  }
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPD_VOLT][LCR_THRESH] = 1.08;
  mb_sensor_threshold[MB_SENSOR_VR_CPU1_VDDQ_GRPD_VOLT][UCR_THRESH] = 1.32;

  mb_sensor_threshold[MB_SENSOR_VR_PCH_PVNN_TEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_PVNN_CURR][UCR_THRESH] = 23;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_PVNN_POWER][UCR_THRESH] = 28;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_PVNN_VOLT][LCR_THRESH] = 0.76;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_PVNN_VOLT][UCR_THRESH] = 1.1;

  mb_sensor_threshold[MB_SENSOR_VR_PCH_P1V05_TEMP][UCR_THRESH] = 95;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_P1V05_CURR][UCR_THRESH] = 19;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_P1V05_POWER][UCR_THRESH] = 26;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_P1V05_VOLT][LCR_THRESH] = 0.94;
  mb_sensor_threshold[MB_SENSOR_VR_PCH_P1V05_VOLT][UCR_THRESH] = 1.15;

  // Set when required
  // mb_sensor_threshold[MB_SENSOR_HOST_BOOT_TEMP][UCR_THRESH] = 100;

  riser_slot2_sensor_threshold[MB_SENSOR_C2_NVME_CTEMP][UCR_THRESH] = 75;
  riser_slot3_sensor_threshold[MB_SENSOR_C3_NVME_CTEMP][UCR_THRESH] = 75;
  riser_slot4_sensor_threshold[MB_SENSOR_C4_NVME_CTEMP][UCR_THRESH] = 75;

  riser_slot2_sensor_threshold[MB_SENSOR_C2_AVA_FTEMP][UCR_THRESH] = 60;
  riser_slot2_sensor_threshold[MB_SENSOR_C2_AVA_RTEMP][UCR_THRESH] = 80;
  riser_slot2_sensor_threshold[MB_SENSOR_C2_1_NVME_CTEMP][UCR_THRESH] = 75;
  riser_slot2_sensor_threshold[MB_SENSOR_C2_2_NVME_CTEMP][UCR_THRESH] = 75;
  riser_slot2_sensor_threshold[MB_SENSOR_C2_3_NVME_CTEMP][UCR_THRESH] = 75;
  riser_slot2_sensor_threshold[MB_SENSOR_C2_4_NVME_CTEMP][UCR_THRESH] = 75;
  riser_slot3_sensor_threshold[MB_SENSOR_C3_AVA_FTEMP][UCR_THRESH] = 60;
  riser_slot3_sensor_threshold[MB_SENSOR_C3_AVA_RTEMP][UCR_THRESH] = 80;
  riser_slot3_sensor_threshold[MB_SENSOR_C3_1_NVME_CTEMP][UCR_THRESH] = 75;
  riser_slot3_sensor_threshold[MB_SENSOR_C3_2_NVME_CTEMP][UCR_THRESH] = 75;
  riser_slot3_sensor_threshold[MB_SENSOR_C3_3_NVME_CTEMP][UCR_THRESH] = 75;
  riser_slot3_sensor_threshold[MB_SENSOR_C3_4_NVME_CTEMP][UCR_THRESH] = 75;
  riser_slot4_sensor_threshold[MB_SENSOR_C4_AVA_FTEMP][UCR_THRESH] = 60;
  riser_slot4_sensor_threshold[MB_SENSOR_C4_AVA_RTEMP][UCR_THRESH] = 80;
  riser_slot4_sensor_threshold[MB_SENSOR_C4_1_NVME_CTEMP][UCR_THRESH] = 75;
  riser_slot4_sensor_threshold[MB_SENSOR_C4_2_NVME_CTEMP][UCR_THRESH] = 75;
  riser_slot4_sensor_threshold[MB_SENSOR_C4_3_NVME_CTEMP][UCR_THRESH] = 75;
  riser_slot4_sensor_threshold[MB_SENSOR_C4_4_NVME_CTEMP][UCR_THRESH] = 75;
  riser_slot2_sensor_threshold[MB_SENSOR_C2_P12V_INA230_VOL][UCR_THRESH] = 12.96;
  riser_slot2_sensor_threshold[MB_SENSOR_C2_P12V_INA230_VOL][LCR_THRESH] = 11.04;
  riser_slot2_sensor_threshold[MB_SENSOR_C2_P12V_INA230_CURR][UCR_THRESH] = 5.5;
  riser_slot2_sensor_threshold[MB_SENSOR_C2_P12V_INA230_PWR][UCR_THRESH] = 75;
  riser_slot3_sensor_threshold[MB_SENSOR_C3_P12V_INA230_VOL][UCR_THRESH] = 12.96;
  riser_slot3_sensor_threshold[MB_SENSOR_C3_P12V_INA230_VOL][LCR_THRESH] = 11.04;
  riser_slot3_sensor_threshold[MB_SENSOR_C3_P12V_INA230_CURR][UCR_THRESH] = 5.5;
  riser_slot3_sensor_threshold[MB_SENSOR_C3_P12V_INA230_PWR][UCR_THRESH] = 75;
  riser_slot4_sensor_threshold[MB_SENSOR_C4_P12V_INA230_VOL][UCR_THRESH] = 12.96;
  riser_slot4_sensor_threshold[MB_SENSOR_C4_P12V_INA230_VOL][LCR_THRESH] = 11.04;
  riser_slot4_sensor_threshold[MB_SENSOR_C4_P12V_INA230_CURR][UCR_THRESH] = 5.5;
  riser_slot4_sensor_threshold[MB_SENSOR_C4_P12V_INA230_PWR][UCR_THRESH] = 75;
  mb_sensor_threshold[MB_SENSOR_CONN_P12V_INA230_VOL][UCR_THRESH] = 12.96;
  mb_sensor_threshold[MB_SENSOR_CONN_P12V_INA230_VOL][LCR_THRESH] = 11.04;
  mb_sensor_threshold[MB_SENSOR_CONN_P12V_INA230_CURR][UCR_THRESH] = 20;
  mb_sensor_threshold[MB_SENSOR_CONN_P12V_INA230_PWR][UCR_THRESH] = 250;
  nic_sensor_threshold[MEZZ_SENSOR_TEMP][UCR_THRESH] = 95;

  init_board_sensors();
  init_done = true;
}
static int
read_nic_temp(float *value)
{
  static unsigned int retry = 0;
  int ret = sensors_read("tmp421-i2c-8-1f", "MEZZ_SENSOR_TEMP", value);
  // Workaround: handle when NICs wrongly report higher temperatures
  if (ret || ( *value > NIC_MAX_TEMP ) || ( *value < NIC_MIN_TEMP )) {
    ret = READING_NA;
  } else {
    retry = 0;
  }
  if (ret == READING_NA && ++retry <= 3)
    ret = READING_SKIP;
  return ret;
}

static int
read_fan_value(const char *fan, float *value)
{
  int ret = sensors_read_fan(fan, value);
  if (ret || *value < 500 || *value > mb_sensor_threshold[MB_SENSOR_FAN0_TACH][UCR_THRESH]) {
    sleep(2);
    ret = sensors_read_fan(fan, value);
  }
  return ret;
}

static int
read_hsc_current_value(float *value) {
  uint8_t bus_id = 0x4; //TODO: ME's address 0x2c in FBTP
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  int rlen = 0;
  float hsc_b = 20475;
  float Rsence;
  ipmb_req_t *req;
  int ret = 0;
  static int retry = 0;
  uint8_t revision_id;
  uint8_t sku_id;

  if (pal_get_platform_id(&sku_id) ||
      pal_get_board_rev_id(&revision_id)) {
    return -1;
  }

  req = ipmb_txb();
  req->res_slave_addr = 0x2C; //ME's Slave Address
  req->netfn_lun = NETFN_NM_REQ<<2;
  req->cmd = CMD_NM_SEND_RAW_PMBUS;
  req->data[0] = 0x57;
  req->data[1] = 0x01;
  req->data[2] = 0x00;
  req->data[3] = 0x86;
  if (revision_id < BOARD_REV_PVT) { //DVT
    //HSC slave addr check for SS and DS
    if ((sku_id & (1 << 4))) { // DS
      req->data[4] = 0x8A;
      Rsence = 0.265;
    }else{    //SS
      req->data[4] = 0x22;
      Rsence = 0.505;
    }
  } else { //PVT and MP
    //HSC slave addr is the same for SS and DS
    req->data[4] = 0x8A;
    Rsence = 0.267;
  }

  req->data[5] = 0x00;
  req->data[6] = 0x00;
  req->data[7] = 0x01;
  req->data[8] = 0x02;
  req->data[9] = 0x8C;
  tlen = 10 + MIN_IPMB_REQ_LEN; // Data Length + Others

  // Invoke IPMB library handler
  rlen = ipmb_send_buf(bus_id, tlen);
  if (rlen > 0) {
    memcpy(rbuf, ipmb_rxb(), rlen);
  }

  if (rlen <= 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "read_hsc_current_value: Zero bytes received\n");
#endif
    ret = READING_NA;
  }
  if (rbuf[6] == 0)
  {
    *value = ((float) (rbuf[11] << 8 | rbuf[10])*10-hsc_b )/(800*Rsence);
    retry = 0;
  } else {
    ret = READING_NA;
  }

  if (ret == READING_NA) {
    retry++;
    if (retry <= 3 )
      ret = READING_SKIP;
  }

  return ret;
}

static int
read_hsc_temp_value(float *value) {
  uint8_t bus_id = 0x4; //TODO: ME's address 0x2c in FBTP
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  float hsc_b = 31880;
  float hsc_m = 42;
  ipmb_req_t *req;
  int ret = 0;
  static int retry = 0;
  uint8_t revision_id;
  uint8_t sku_id;

  if (pal_get_platform_id(&sku_id) ||
      pal_get_board_rev_id(&revision_id)) {
    return -1;
  }

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = 0x2C; //ME's Slave Address
  req->netfn_lun = NETFN_NM_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;

  req->cmd = CMD_NM_SEND_RAW_PMBUS;
  req->data[0] = 0x57;
  req->data[1] = 0x01;
  req->data[2] = 0x00;
  req->data[3] = 0x86;
  if (revision_id < BOARD_REV_PVT) { //DVT
    //HSC slave addr check for SS and DS
    if ((sku_id & (1 << 4))) { // DS
      req->data[4] = 0x8A;
    }else{    //SS
      req->data[4] = 0x22;
    }
  } else { //PVT and MP
    //HSC slave addr is the same for SS and DS
    req->data[4] = 0x8A;
  }
  req->data[5] = 0x00;
  req->data[6] = 0x00;
  req->data[7] = 0x01;
  req->data[8] = 0x02;
  req->data[9] = 0x8D;
  tlen = 16;

  // Invoke IPMB library handler
  lib_ipmb_handle(bus_id, tbuf, tlen+1, rbuf, &rlen);

  if (rlen == 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "read_hsc_temp_value: Zero bytes received\n");
#endif
    ret = READING_NA;
  }
  if (rbuf[6] == 0)
  {
    *value = ((float) (rbuf[11] << 8 | rbuf[10])*10-hsc_b )/(hsc_m);
    retry = 0;
  } else {
    ret = READING_NA;
  }

  if (ret == READING_NA) {
    retry++;
    if (retry <= 3 )
      ret = READING_SKIP;
  }

  return ret;
}

static int
read_sensor_reading_from_ME(uint8_t snr_num, float *value) {
  uint8_t bus_id = 0x4; //TODO: ME's address 0x2c in FBTP
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  ipmb_req_t *req;
  int ret = 0;
  enum {
    e_HSC_PIN,
    e_HSC_VIN,
    e_PCH_TEMP,
    e_MAX,
  };
  static uint8_t retry[e_MAX] = {0};

  req = (ipmb_req_t*)tbuf;
  req->res_slave_addr = 0x2C; //ME's Slave Address
  req->netfn_lun = NETFN_SENSOR_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;
  req->cmd = CMD_SENSOR_GET_SENSOR_READING;
  req->data[0] = snr_num;
  tlen = 7;

  // Invoke IPMB library handler
  lib_ipmb_handle(bus_id, tbuf, tlen+1, rbuf, &rlen);

  if (rlen == 0) {
  //ME no response
#ifdef DEBUG
    syslog(LOG_DEBUG, "read HSC %x from_ME: Zero bytes received\n", snr_num);
#endif
   ret = READING_NA;
  } else {
    if (rbuf[6] == 0)
    {
        if (rbuf[8] & 0x20) {
          //not available
          ret = READING_NA;
        }
    } else {
      ret = READING_NA;
    }
  }

  if(snr_num == MB_SENSOR_HSC_IN_POWER) {
    if (!ret) {
      *value = (((float) rbuf[7])*0x28 + 0 )/10 ;
      retry[e_HSC_PIN] = 0;
    } else {
      retry[e_HSC_PIN]++;
      if (retry[e_HSC_PIN] <= 3)
        ret = READING_SKIP;
    }
  } else if(snr_num == MB_SENSOR_HSC_IN_VOLT) {
    if (!ret) {
      *value = (((float) rbuf[7])*0x02 + (0x5e*10) )/100 ;
      retry[e_HSC_VIN] = 0;
    } else {
      retry[e_HSC_VIN]++;
      if (retry[e_HSC_VIN] <= 3)
        ret = READING_SKIP;
    }
  } else if(snr_num == MB_SENSOR_PCH_TEMP) {
    if (!ret) {
      *value = (float) rbuf[7];
      retry[e_PCH_TEMP] = 0;
    } else {
      retry[e_PCH_TEMP]++;
      if (retry[e_PCH_TEMP] <= 3)
        ret = READING_SKIP;
    }
  }
  return ret;
}

static int
read_cpu_temp(uint8_t snr_num, float *value) {
  int ret = 0;
  uint8_t bus_id = 0x4; //TODO: ME's address 0x2c in FBTP
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf1[256] = {0x00};
  static uint8_t tjmax[2] = {0x00};
  static uint8_t tjmax_flag[2] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  ipmb_req_t *req;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  int cpu_index;
  int16_t dts;
  static uint8_t retry[2] = {0x00}; // CPU0 and CPU1

  switch (snr_num) {
    case MB_SENSOR_CPU0_TEMP:
      cpu_index = 0;
      break;
    case MB_SENSOR_CPU1_TEMP:
      cpu_index = 1;
      break;
    default:
      return -1;
  }

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = 0x2C; //ME's Slave Address

  req->netfn_lun = NETFN_NM_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;

  if( tjmax_flag[cpu_index] == 0 ) { // First time to get CPU0/CPU1 Tjmax reading
    //Get CPU0/CPU1 Tjmax
    req->cmd = CMD_NM_SEND_RAW_PECI;
    req->data[0] = 0x57;
    req->data[1] = 0x01;
    req->data[2] = 0x00;
    req->data[3] = 0x30 + cpu_index;
    req->data[4] = 0x05;
    req->data[5] = 0x05;
    req->data[6] = 0xa1;
    req->data[7] = 0x00;
    req->data[8] = 0x10;
    req->data[9] = 0x00;
    req->data[10] = 0x00;
    tlen = 17;
    // Invoke IPMB library handler
    lib_ipmb_handle(bus_id, tbuf, tlen+1, rbuf1, &rlen);
    if (rlen == 0) {
    //ME no response
#ifdef DEBUG
      syslog(LOG_DEBUG, "%s(%d): Zero bytes received\n", __func__, __LINE__);
#endif
    } else {
      if (rbuf1[6] == 0)
      {
        // If PECI command successes and got a reasonable value
        if ( (rbuf1[10] == 0x40) && rbuf1[13] > 50) {
          tjmax[cpu_index] = rbuf1[13];
          tjmax_flag[cpu_index] = 1;
        }
      }
    }
  }

  //Updated CPU Tjmax cache
  sprintf(key, "mb_sensor%d", (cpu_index?MB_SENSOR_CPU1_TJMAX:MB_SENSOR_CPU0_TJMAX));
  if (tjmax_flag[cpu_index] != 0) {
    sprintf(str, "%.2f",(float) tjmax[cpu_index]);
  } else {
    //ME no response or PECI command completion code error. Set "NA" in sensor cache.
    strcpy(str, "NA");
  }
  kv_set(key, str, 0, 0);

  // Get CPU temp if BMC got TjMax
  ret = READING_NA;
  if (tjmax_flag[cpu_index] != 0) {
    rlen = 0;
    memset( rbuf1,0x00,sizeof(rbuf1) );
    //Get CPU Temp
    req->cmd = CMD_NM_SEND_RAW_PECI;
    req->data[0] = 0x57;
    req->data[1] = 0x01;
    req->data[2] = 0x00;
    req->data[3] = 0x30 + cpu_index;
    req->data[4] = 0x05;
    req->data[5] = 0x05;
    req->data[6] = 0xa1;
    req->data[7] = 0x00;
    req->data[8] = 0x02;
    req->data[9] = 0xff;
    req->data[10] = 0x00;
    tlen = 17;

    // Invoke IPMB library handler
    lib_ipmb_handle(bus_id, tbuf, tlen+1, rbuf1, &rlen);

    if (rlen == 0) {
      //ME no response
#ifdef DEBUG
      syslog(LOG_DEBUG, "%s(%d): Zero bytes received\n", __func__, __LINE__);
#endif
    } else {
      if (rbuf1[6] == 0) { // ME Completion Code
        if ( (rbuf1[10] == 0x40) ) { // PECI Completion Code
          dts = (rbuf1[11] | rbuf1[12] << 8);
          // Intel Doc#554767 p.58: Reserved Values 0x8000~0x81ff
          if (dts <= -32257) {
            ret = READING_NA;
          } else {
            // 16-bit, 2s complement [15]Sign Bit;[14:6]Integer Value;[5:0]Fractional Value
            *value = (float) (tjmax[0] + (dts >> 6));
            ret = 0;
          }
        }
      }
    }
  }

  if (ret != 0) {
    retry[cpu_index]++;
    if (retry[cpu_index] <= 3) {
      ret = READING_SKIP;
    }
  } else
    retry[cpu_index] = 0;

  //Updated CPU Thermal Margin cache
  sprintf(key, "mb_sensor%d", (cpu_index?MB_SENSOR_CPU1_THERM_MARGIN:MB_SENSOR_CPU0_THERM_MARGIN));
  switch (ret) {
    case 0:
      sprintf(str, "%.2f",(float) (dts >> 6));
      kv_set(key, str, 0, 0);
      break;
    case READING_NA:
      strcpy(str, "NA");
      kv_set(key, str, 0, 0);
      break;
    case READING_SKIP:
    default:
      break;
  }

  return ret;
}

bool
pal_is_BIOS_completed(uint8_t fru)
{
  gpio_desc_t *desc;
  gpio_value_t value;
  bool ret = false;

  if ( FRU_MB != fru )
  {
    syslog(LOG_WARNING, "[%s]incorrect fru id: %d", __func__, fru);
    return false;
  }
  desc = gpio_open_by_shadow("FM_BIOS_POST_CMPLT_N");
  if (!desc)
    return false;

  if (gpio_get_value(desc, &value) == 0 && value == GPIO_VALUE_LOW)
    ret = true;
  gpio_close(desc);
  return ret;
}

void
pal_is_dimm_present_check(uint8_t fru, bool *dimm_sts_list)
{
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  int i;
  int DIMM_SLOT_CNT = 12;//only SS
  size_t ret;

  //check dimm info from /mnt/data/sys_config/
  for (i=0; i<DIMM_SLOT_CNT; i++)
  {
    sprintf(key, "sys_config/fru%d_dimm%d_location", fru, i);
    if(kv_get(key, value, &ret, KV_FPERSIST) != 0 || ret < 4)
    {
      syslog(LOG_WARNING,"[%s]Cannot get dimm_slot%d present info", __func__, i);
      return;
    }

#ifdef FSC_DEBUG
    syslog(LOG_WARNING,"[%s]0=%x 1=%x 2=%x 3=%x", __func__, value[0], value[1], value[2], value[3]);
#endif

    if ( 0xff == value[0] )
    {
      dimm_sts_list[i] = false;
#ifdef FSC_DEBUG
      syslog(LOG_WARNING,"[%s]dimm_slot%d is not present", __func__, i);
#endif
    }
    else
    {
      dimm_sts_list[i] = true;
#ifdef FSC_DEBUG
      syslog(LOG_WARNING,"[%s]dimm_slot%d is present", __func__, i);
#endif
    }
  }
}

bool
pal_is_dimm_present(uint8_t sensor_num)
{
  static bool is_check = false;
  static bool dimm_sts_list[12] = {0};
  int i = 0,j;
  uint8_t fru = FRU_MB;

  if ( false == pal_is_BIOS_completed(fru) )
  {
    return false;
  }

  if ( false == is_check )
  {
    is_check = true;
    pal_is_dimm_present_check(fru, dimm_sts_list);
  }

  switch (sensor_num)
  {
    case MB_SENSOR_CPU0_DIMM_GRPA_TEMP:
      i = 0;
    break;

    case MB_SENSOR_CPU0_DIMM_GRPB_TEMP:
      i = 3;
    break;

    case MB_SENSOR_CPU1_DIMM_GRPC_TEMP:
      i = 6;
    break;

    case MB_SENSOR_CPU1_DIMM_GRPD_TEMP:
      i = 9;
    break;

    default:
      syslog(LOG_WARNING, "[%s]Unknown sensor num: 0x%x", __func__, sensor_num);
    break;
  }

  j = i + 3;

  for ( ; i<j; i++ )
  {
    if ( true == dimm_sts_list[i] )
    {
      return true;
    }
  }

  return false;
}

static int
read_dimm_temp(uint8_t snr_num, float *value) {
  int ret = READING_NA;
  uint8_t bus_id = 0x4; //TODO: ME's address 0x2c in FBTP
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf1[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  ipmb_req_t *req;
  int dimm_index, i;
  int max = 0;
  static uint8_t retry[4] = {0x00};
  static int odm_id = -1;
  uint8_t BoardInfo;

  //Use FM_BOARD_SKU_ID0 to identify ODM to apply filter
  if (odm_id == -1) {
    ret = pal_get_platform_id(&BoardInfo);
    if (ret == 0) {
      odm_id = (int) (BoardInfo & 0x1);
      //re-init the ret variable since the sensor reading(NA or value) is affected by it
      //make sure the ret variable used correctly.
      ret = READING_NA;
    }
  }

  //show NA if BIOS has not completed POST.
  if ( false == pal_is_BIOS_completed(FRU_MB) )
  {
    return ret;
  }

  switch (snr_num) {
    case MB_SENSOR_CPU0_DIMM_GRPA_TEMP:
      dimm_index = 0;
      break;
    case MB_SENSOR_CPU0_DIMM_GRPB_TEMP:
      dimm_index = 1;
      break;
    case MB_SENSOR_CPU1_DIMM_GRPC_TEMP:
      dimm_index = 2;
      break;
    case MB_SENSOR_CPU1_DIMM_GRPD_TEMP:
      dimm_index = 3;
      break;
    default:
      return -1;
  }

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = 0x2C; //ME's Slave Address

  req->netfn_lun = NETFN_NM_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;

  for (i=0; i<3; i++) { // Get 3 channel for each DIMM group
    //Get DIMM Temp per channel
    req->cmd = CMD_NM_SEND_RAW_PECI;
    req->data[0] = 0x57;
    req->data[1] = 0x01;
    req->data[2] = 0x00;
    req->data[3] = 0x30 + (dimm_index / 2);
    req->data[4] = 0x05;
    req->data[5] = 0x05;
    req->data[6] = 0xa1;
    req->data[7] = 0x00;
    req->data[8] = 0x0e;
    req->data[9] = 0x00 + (dimm_index % 2 * 3) + i;
    req->data[10] = 0x00;
    tlen = 17;

    // Invoke IPMB library handler
    lib_ipmb_handle(bus_id, tbuf, tlen+1, rbuf1, &rlen);
    if (rlen == 0) {
    //ME no response
#ifdef DEBUG
      syslog(LOG_DEBUG, "%s(%d): Zero bytes received\n", __func__, __LINE__);
#endif
    } else {
      if (rbuf1[6] == 0)
      {
        // If PECI command successes
        if ( (rbuf1[10] == 0x40)) {
          if (rbuf1[11] > max)
            max = rbuf1[11];
          if (rbuf1[12] > max)
            max = rbuf1[11];
        }
      }
    }
  }

  if (odm_id == 1) {
    // Filter abnormal values: 0x0 and 0xFF
    if (max != 0 && max != 0xFF)
      ret = 0;
  } else {
    // Filter abnormal values: 0x0
    if (max != 0)
      ret = 0;
  }

  if (ret != 0) {
    retry[dimm_index]++;
    if (retry[dimm_index] <= 3) {
      ret = READING_SKIP;
      return ret;
    }
  } else
    retry[dimm_index] = 0;

  if (ret == 0) {
    *value = (float)max;
  }

  return ret;
}

static int
read_cpu_package_power(uint8_t snr_num, float *value) {
  int ret = READING_NA;
  uint8_t bus_id = 0x4; //TODO: ME's address 0x2c in FBTP
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf1[256] = {0x00};
  // Energy units: Intel Doc#554767, p37, 2^(-ENERGY UNIT) J, ENERGY UNIT defalut is 14
  // Run Time units: Intel Doc#554767, p33, msec
  // 2^(-14)*1000 = 0.06103515625
  float unit = 0.06103515625f;
  static uint32_t last_pkg_energy[2] = {0}, last_run_time[2] = {0};
  uint32_t pkg_energy, run_time, diff_energy, diff_time;
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  ipmb_req_t *req;
  int cpu_index;
  static uint8_t retry[2] = {0x00}; // CPU0 and CPU1

  switch (snr_num) {
    case MB_SENSOR_CPU0_PKG_POWER:
      cpu_index = 0;
      break;
    case MB_SENSOR_CPU1_PKG_POWER:
      cpu_index = 1;
      break;
    default:
      return -1;
  }

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = 0x2C; //ME's Slave Address

  req->netfn_lun = NETFN_NM_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;

  // Get CPU package power and run time
  rlen = 0;
  memset( rbuf1,0x00,sizeof(rbuf1) );
  //Read Accumulated Energy Pkg and Accumulated Run Time
  req->cmd = CMD_NM_AGGREGATED_SEND_RAW_PECI;
  req->data[0] = 0x57;
  req->data[1] = 0x01;
  req->data[2] = 0x00;
  req->data[3] = 0x30 + cpu_index;
  req->data[4] = 0x05;
  req->data[5] = 0x05;
  req->data[6] = 0xa1;
  req->data[7] = 0x00;
  req->data[8] = 0x03;
  req->data[9] = 0xff;
  req->data[10] = 0x00;
  req->data[11] = 0x30 + cpu_index;
  req->data[12] = 0x05;
  req->data[13] = 0x05;
  req->data[14] = 0xa1;
  req->data[15] = 0x00;
  req->data[16] = 0x1F;
  req->data[17] = 0x00;
  req->data[18] = 0x00;
  tlen = 25;

  // Invoke IPMB library handler
  lib_ipmb_handle(bus_id, tbuf, tlen+1, rbuf1, &rlen);

  if (rlen == 0) {
    //ME no response
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(%d): Zero bytes received\n", __func__, __LINE__);
#endif
    goto error_exit;
  } else {
    if (rbuf1[6] == 0) { // ME Completion Code
      if ( (rbuf1[10] == 0x00) && (rbuf1[11] == 0x40) && // 1st ME CC & PECI CC
           (rbuf1[16] == 0x00) && (rbuf1[17] == 0x40) ){ // 2nd ME CC & PECI CC
        pkg_energy = rbuf1[15];
        pkg_energy = (pkg_energy << 8) | rbuf1[14];
        pkg_energy = (pkg_energy << 8) | rbuf1[13];
        pkg_energy = (pkg_energy << 8) | rbuf1[12];

        run_time = rbuf1[21];
        run_time = (run_time << 8) | rbuf1[20];
        run_time = (run_time << 8) | rbuf1[19];
        run_time = (run_time << 8) | rbuf1[18];

        ret = 0;
      }
    }
  }

  // need at least 2 entries to calculate
  if (last_pkg_energy[cpu_index] == 0 && last_run_time[cpu_index] == 0) {
    if (ret == 0) {
      last_pkg_energy[cpu_index] = pkg_energy;
      last_run_time[cpu_index] = run_time;
    }
    ret = READING_NA;
  }

  if(!ret) {
    if(pkg_energy >= last_pkg_energy[cpu_index])
      diff_energy = pkg_energy - last_pkg_energy[cpu_index];
    else
      diff_energy = pkg_energy + (0xffffffff - last_pkg_energy[cpu_index] + 1);
    last_pkg_energy[cpu_index] = pkg_energy;

    if(run_time >= last_run_time[cpu_index])
      diff_time = run_time - last_run_time[cpu_index];
    else
      diff_time = run_time + (0xffffffff - last_run_time[cpu_index] + 1);
    last_run_time[cpu_index] = run_time;

    if(diff_time == 0)
      ret = READING_NA;
    else
      *value = ((float)diff_energy / (float)diff_time * unit);
  }

error_exit:
  if (ret != 0) {
    retry[cpu_index]++;
    if (retry[cpu_index] <= 3) {
      ret = READING_SKIP;
      return ret;
    }
  } else
    retry[cpu_index] = 0;

  return ret;
}

static int
read_ava_temp(uint8_t sensor_num, float *value) {
  int fd = 0;
  char fn[32];
  int ret = READING_NA;;
  static unsigned int retry[6] = {0};
  uint8_t i_retry;
  uint8_t tcount, rcount, slot_cfg, addr, mux_chan;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  if (pal_get_slot_cfg_id(&slot_cfg) < 0)
    slot_cfg = SLOT_CFG_EMPTY;

  switch(sensor_num) {
    case MB_SENSOR_C2_AVA_FTEMP:
      i_retry = 0; break;
    case MB_SENSOR_C2_AVA_RTEMP:
      i_retry = 1; break;
    case MB_SENSOR_C3_AVA_FTEMP:
      i_retry = 2; break;
    case MB_SENSOR_C3_AVA_RTEMP:
      i_retry = 3; break;
    case MB_SENSOR_C4_AVA_FTEMP:
      i_retry = 4; break;
    case MB_SENSOR_C4_AVA_RTEMP:
      i_retry = 5; break;
    default:
      return READING_NA;
  }

  switch(sensor_num) {
    case MB_SENSOR_C2_AVA_FTEMP:
    case MB_SENSOR_C2_AVA_RTEMP:
      if(slot_cfg == SLOT_CFG_EMPTY)
        return READING_NA;
      mux_chan = 0;
      break;
    case MB_SENSOR_C3_AVA_FTEMP:
    case MB_SENSOR_C3_AVA_RTEMP:
      if(slot_cfg == SLOT_CFG_EMPTY)
        return READING_NA;
      mux_chan = 1;
      break;
    case MB_SENSOR_C4_AVA_FTEMP:
    case MB_SENSOR_C4_AVA_RTEMP:
      if(slot_cfg != SLOT_CFG_SS_3x8)
        return READING_NA;
      mux_chan = 2;
      break;
    default:
      return READING_NA;
  }

  if ( false == pal_is_ava_card(mux_chan) )
    return READING_NA;

  switch(sensor_num) {
    case MB_SENSOR_C2_AVA_FTEMP:
    case MB_SENSOR_C3_AVA_FTEMP:
    case MB_SENSOR_C4_AVA_FTEMP:
      addr = 0x92;
      break;
    case MB_SENSOR_C2_AVA_RTEMP:
    case MB_SENSOR_C3_AVA_RTEMP:
    case MB_SENSOR_C4_AVA_RTEMP:
      addr = 0x90;
      break;
    default:
      return READING_NA;
  }

  //try to control multiplexer to target channel and it will exit if it fail
  ret = mux_lock(&riser_mux, mux_chan, 2);
  if (ret < 0) {
    ret = READING_NA;
    goto error_exit;
  }

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", RISER_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    ret = READING_NA;
    goto release_mux_and_exit;
  }

  // Read 2 bytes from TMP75
  tbuf[0] = 0x00;
  tcount = 1;
  rcount = 2;

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tcount, rbuf, rcount);
  if (ret < 0) {
    ret = READING_NA;
    goto release_mux_and_exit;
  }
  ret = 0;
  retry[i_retry] = 0;

  // rbuf:MSB, LSB; 12-bit value on Bit[15:4], unit: 0.0625
  *value = (float)(signed char)rbuf[0];

//if the channel is locked, unlock it and then exit
release_mux_and_exit:

  mux_release(&riser_mux);

//if the channel is busy, exit the function
error_exit:

  if (fd > 0) {
    close(fd);
  }

  if (ret == READING_NA && ++retry[i_retry] <= 3)
    ret = READING_SKIP;

  return ret;
}

static int
read_INA230 (uint8_t sensor_num, float *value, int pot) {
  int fd = 0;
  char fn[32];
  int ret = READING_NA;;
  static unsigned int retry[12] = {0};
  uint8_t i_retry;
  uint8_t slot_cfg, addr, mux_chan;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t BoardInfo;
  static int odm_id = -1;

  //Use FM_BOARD_SKU_ID0 to identify ODM to apply filter
  if (odm_id == -1) {
    ret = pal_get_platform_id(&BoardInfo);
    if (ret == 0) {
      odm_id = (int) (BoardInfo & 0x1);
    }
  }

  if (pal_get_slot_cfg_id(&slot_cfg) < 0)
    slot_cfg = SLOT_CFG_EMPTY;

  switch(sensor_num) {
    case MB_SENSOR_C2_P12V_INA230_VOL:
      i_retry = 0; break;
    case MB_SENSOR_C2_P12V_INA230_CURR:
      i_retry = 1; break;
    case MB_SENSOR_C2_P12V_INA230_PWR:
      i_retry = 2; break;
    case MB_SENSOR_C3_P12V_INA230_VOL:
      i_retry = 3; break;
    case MB_SENSOR_C3_P12V_INA230_CURR:
      i_retry = 4; break;
    case MB_SENSOR_C3_P12V_INA230_PWR:
      i_retry = 5; break;
    case MB_SENSOR_C4_P12V_INA230_VOL:
      i_retry = 6; break;
    case MB_SENSOR_C4_P12V_INA230_CURR:
      i_retry = 7; break;
    case MB_SENSOR_C4_P12V_INA230_PWR:
      i_retry = 8; break;
    case MB_SENSOR_CONN_P12V_INA230_VOL:
      i_retry = 9; break;
    case MB_SENSOR_CONN_P12V_INA230_CURR:
      i_retry = 10; break;
    case MB_SENSOR_CONN_P12V_INA230_PWR:
      i_retry = 11; break;
  }

  switch(sensor_num) {
    case MB_SENSOR_C2_P12V_INA230_VOL:
    case MB_SENSOR_C2_P12V_INA230_CURR:
    case MB_SENSOR_C2_P12V_INA230_PWR:
    case MB_SENSOR_C3_P12V_INA230_VOL:
    case MB_SENSOR_C3_P12V_INA230_CURR:
    case MB_SENSOR_C3_P12V_INA230_PWR:
    case MB_SENSOR_CONN_P12V_INA230_VOL:
    case MB_SENSOR_CONN_P12V_INA230_CURR:
    case MB_SENSOR_CONN_P12V_INA230_PWR:
      if(slot_cfg == SLOT_CFG_EMPTY)
        return READING_NA;
      break;
    case MB_SENSOR_C4_P12V_INA230_VOL:
    case MB_SENSOR_C4_P12V_INA230_CURR:
    case MB_SENSOR_C4_P12V_INA230_PWR:
      if(slot_cfg != SLOT_CFG_SS_3x8)
        return READING_NA;
      break;
    default:
      return READING_NA;
  }

  //use channel 4
  mux_chan = 0x3;

  //try to control multiplexer to target channel and it will exit if it fail
  ret = mux_lock(&riser_mux, mux_chan, 2);
  if (ret < 0) {
    ret = READING_NA;
    goto error_exit;
  }

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", RISER_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    ret = READING_NA;
    goto release_mux_and_exit;
  }

  switch(sensor_num) {
    case MB_SENSOR_C2_P12V_INA230_VOL:
    case MB_SENSOR_C2_P12V_INA230_CURR:
    case MB_SENSOR_C2_P12V_INA230_PWR:
      addr = 0x80;
      break;
    case MB_SENSOR_C3_P12V_INA230_VOL:
    case MB_SENSOR_C3_P12V_INA230_CURR:
    case MB_SENSOR_C3_P12V_INA230_PWR:
      addr = 0x82;
      break;
    case MB_SENSOR_C4_P12V_INA230_VOL:
    case MB_SENSOR_C4_P12V_INA230_CURR:
    case MB_SENSOR_C4_P12V_INA230_PWR:
      addr = 0x88;
      break;
    case MB_SENSOR_CONN_P12V_INA230_VOL:
    case MB_SENSOR_CONN_P12V_INA230_CURR:
    case MB_SENSOR_CONN_P12V_INA230_PWR:
      addr = 0x8A;
      break;
    default:
        syslog(LOG_WARNING, "read_INA230: undefined sensor number") ;
      break;
  }

  if (odm_id == 0) {
    static unsigned int initialized[4] = {0};
    // If Power On Time == 1, re-initialize INA230
    if (pot == 1 && (i_retry % 3) == 0)
      initialized[i_retry/3] = 0;

    if (initialized[i_retry/3] == 0) {
      //Set Configuration register
      tbuf[0] = 0x00, tbuf[1] = 0x49; tbuf[2] = 0x27;
      ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, 3, rbuf, 0);
      if (ret < 0) {
        ret = READING_NA;
        goto release_mux_and_exit;
      }

      //Set Calibration register
      tbuf[0] = 0x05, tbuf[1] = 0x9; tbuf[2] = 0xd9;
      ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, 3, rbuf, 0);
      if (ret < 0) {
        ret = READING_NA;
        goto release_mux_and_exit;
      }
      initialized[i_retry/3] = 1;
    }

    // Delay for 2 cycles and check INA230 init done
    if(pot < 3 || initialized[i_retry/3] == 0){
      ret = READING_NA;
      goto release_mux_and_exit;
    }

    //Get registers data
    switch(sensor_num) {
      case MB_SENSOR_C2_P12V_INA230_VOL:
      case MB_SENSOR_C3_P12V_INA230_VOL:
      case MB_SENSOR_C4_P12V_INA230_VOL:
      case MB_SENSOR_CONN_P12V_INA230_VOL:
        tbuf[0] = 0x02;
        break;
      case MB_SENSOR_C2_P12V_INA230_CURR:
      case MB_SENSOR_C3_P12V_INA230_CURR:
      case MB_SENSOR_C4_P12V_INA230_CURR:
      case MB_SENSOR_CONN_P12V_INA230_CURR:
        tbuf[0] = 0x04;
        break;
      case MB_SENSOR_C2_P12V_INA230_PWR:
      case MB_SENSOR_C3_P12V_INA230_PWR:
      case MB_SENSOR_C4_P12V_INA230_PWR:
      case MB_SENSOR_CONN_P12V_INA230_PWR:
        tbuf[0] = 0x03;
        break;
      default:
        syslog(LOG_WARNING, "read_INA230: undefined sensor number") ;
        break;
    }

    tbuf[1] = 0x0; tbuf[2] = 0x0;
    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, 1, rbuf, 2);
    if (ret < 0) {
      ret = READING_NA;
      goto release_mux_and_exit;
    }
    int16_t temp;
    switch(sensor_num) {
      case MB_SENSOR_C2_P12V_INA230_VOL:
      case MB_SENSOR_C3_P12V_INA230_VOL:
      case MB_SENSOR_C4_P12V_INA230_VOL:
      case MB_SENSOR_CONN_P12V_INA230_VOL:
        *value = ((rbuf[1] + rbuf[0] *256) *0.00125) ;
        break;
      case MB_SENSOR_C2_P12V_INA230_CURR:
      case MB_SENSOR_C3_P12V_INA230_CURR:
      case MB_SENSOR_C4_P12V_INA230_CURR:
      case MB_SENSOR_CONN_P12V_INA230_CURR:
        temp = rbuf[0];
        temp = (temp <<8) | rbuf[1];
        //*value = (((int16_t)rbuf[0] << 8) | (int16_t)rbuf[1]) * 0.001;
        *value = temp * 0.001;
        if(*value < 0)
          *value = 0;
        break;
      case MB_SENSOR_C2_P12V_INA230_PWR:
      case MB_SENSOR_C3_P12V_INA230_PWR:
      case MB_SENSOR_C4_P12V_INA230_PWR:
      case MB_SENSOR_CONN_P12V_INA230_PWR:
        *value = (rbuf[1] + rbuf[0] * 256)*0.025;
        if(*value < 1)
          *value = 0;
        break;
      default:
        syslog(LOG_WARNING, "read_INA230: undefined sensor number") ;
        break;
    }

  } else {
   /*
    *Rshunt - it is defined by the schematic. It will be 2m ohm in the fbtp
    */
    const float Rshunt = 0.002;    // 2m ohm
    const uint8_t bus_volt_addr = 0x02; // bus voltage address
    const uint8_t shunt_volt_addr = 0x01; // shunt voltage address

    // Delay for 2 cycles and check INA230 init done
    if( pot < 3 )
    {
      ret = READING_NA;
      goto release_mux_and_exit;
    }

    memset(rbuf, 0, sizeof(rbuf));
    memset(tbuf, 0, sizeof(tbuf));

    switch(sensor_num)
    {
      case MB_SENSOR_C2_P12V_INA230_VOL:
      case MB_SENSOR_C3_P12V_INA230_VOL:
      case MB_SENSOR_C4_P12V_INA230_VOL:
      case MB_SENSOR_CONN_P12V_INA230_VOL:
        tbuf[0] = bus_volt_addr;
        break;
      case MB_SENSOR_C2_P12V_INA230_CURR:
      case MB_SENSOR_C3_P12V_INA230_CURR:
      case MB_SENSOR_C4_P12V_INA230_CURR:
      case MB_SENSOR_CONN_P12V_INA230_CURR:
        tbuf[0] = shunt_volt_addr;
        break;
      case MB_SENSOR_C2_P12V_INA230_PWR:
      case MB_SENSOR_C3_P12V_INA230_PWR:
      case MB_SENSOR_C4_P12V_INA230_PWR:
      case MB_SENSOR_CONN_P12V_INA230_PWR:
        tbuf[0] = shunt_volt_addr;
        tbuf[1] = bus_volt_addr;
        break;
      default:
          syslog(LOG_WARNING, "read_INA230: undefined sensor number") ;
        break;
    }

    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, 1, rbuf, 2);
    if (ret < 0)
    {
      ret = READING_NA;
      goto release_mux_and_exit;
    }

    //get the additional data to calculate the power reading
    if ( bus_volt_addr == tbuf[1] )
    {
      tbuf[0] = tbuf[1];

      //use the rbuf[2] and rbuf[3] to store data
      ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, 1, &rbuf[2], 2);
      if (ret < 0)
      {
        ret = READING_NA;
        goto release_mux_and_exit;
      }
    }

    switch(sensor_num)
    {
      case MB_SENSOR_C2_P12V_INA230_VOL:
      case MB_SENSOR_C3_P12V_INA230_VOL:
      case MB_SENSOR_C4_P12V_INA230_VOL:
      case MB_SENSOR_CONN_P12V_INA230_VOL:
          //calculate the bus voltage
          *value = ((rbuf[1] + rbuf[0]*256) * 0.00125);
        break;
      case MB_SENSOR_C2_P12V_INA230_CURR:
      case MB_SENSOR_C3_P12V_INA230_CURR:
      case MB_SENSOR_C4_P12V_INA230_CURR:
      case MB_SENSOR_CONN_P12V_INA230_CURR:
        *value = 0;
        //check the sign bit. If it is a negative value, show 0
        if ( 1 != BIT(rbuf[0], 7) )
        {
          //calculate the shunt voltage
          *value = ((rbuf[1] + rbuf[0]*256) * 0.0000025);

          //use I = V / R to get the current
          *value = (*value) / Rshunt;
        }
        break;
      case MB_SENSOR_C2_P12V_INA230_PWR:
      case MB_SENSOR_C3_P12V_INA230_PWR:
      case MB_SENSOR_C4_P12V_INA230_PWR:
      case MB_SENSOR_CONN_P12V_INA230_PWR:
        {
          float shunt_volt = 0;
          float current = 0;
          float bus_volt = ((rbuf[3] + rbuf[2]*256) * 0.00125);//use rbuf[2] and rbuf[3] to get the bus voltage

          //check the sign bit. If it is a negative value, show 0
          if ( 1 != BIT(rbuf[0], 7) )
          {
            //calculate the shunt voltage
            shunt_volt = ((rbuf[1] + rbuf[0]*256) * 0.0000025);

            //use I = V / R to get the current
            current = shunt_volt / Rshunt;
          }

          //use P = V * I to get the power
          *value = bus_volt * current;
        }
        break;
      default:
          syslog(LOG_WARNING, "read_INA230: undefined sensor number") ;
        break;
    }
  }
    ret = 0;
    retry[i_retry] = 0;

//if the channel is locked, unlock it and then exit
release_mux_and_exit:

  mux_release(&riser_mux);

//if the channel is busy, exit the function
error_exit:

  if (fd > 0) {
    close(fd);
  }

  if (ret == READING_NA && ++retry[i_retry] <= 3)
    ret = READING_SKIP;

  return ret;
}

static int
read_nvme_temp(uint8_t sensor_num, float *value) {
  int fd = 0;
  char fn[32];
  int ret = READING_NA;
  static unsigned int retry[15] = {0};
  uint8_t i_retry;
  uint8_t tcount, rcount, slot_cfg, addr = 0xd4, mux_chan;
  uint8_t switch_chan, switch_addr=0xe6;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  if (pal_get_slot_cfg_id(&slot_cfg) < 0)
    slot_cfg = SLOT_CFG_EMPTY;

  switch(sensor_num) {
    case MB_SENSOR_C2_1_NVME_CTEMP:
      i_retry = 0; break;
    case MB_SENSOR_C2_2_NVME_CTEMP:
      i_retry = 1; break;
    case MB_SENSOR_C2_3_NVME_CTEMP:
      i_retry = 2; break;
    case MB_SENSOR_C2_4_NVME_CTEMP:
      i_retry = 3; break;
    case MB_SENSOR_C3_1_NVME_CTEMP:
      i_retry = 4; break;
    case MB_SENSOR_C3_2_NVME_CTEMP:
      i_retry = 5; break;
    case MB_SENSOR_C3_3_NVME_CTEMP:
      i_retry = 6; break;
    case MB_SENSOR_C3_4_NVME_CTEMP:
      i_retry = 7; break;
    case MB_SENSOR_C4_1_NVME_CTEMP:
      i_retry = 8; break;
    case MB_SENSOR_C4_2_NVME_CTEMP:
      i_retry = 9; break;
    case MB_SENSOR_C4_3_NVME_CTEMP:
      i_retry = 10; break;
    case MB_SENSOR_C4_4_NVME_CTEMP:
      i_retry = 11; break;
    case MB_SENSOR_C2_NVME_CTEMP:
      i_retry = 12; break;
    case MB_SENSOR_C3_NVME_CTEMP:
      i_retry = 13; break;
    case MB_SENSOR_C4_NVME_CTEMP:
      i_retry = 14; break;
    default:
      return READING_NA;
  }

  switch(sensor_num) {
    case MB_SENSOR_C2_NVME_CTEMP:
      if(slot_cfg == SLOT_CFG_EMPTY || ( !pal_is_pcie_ssd_card(0) ))
        return READING_NA;
      mux_chan = 0;
      break;
    case MB_SENSOR_C2_1_NVME_CTEMP:
    case MB_SENSOR_C2_2_NVME_CTEMP:
    case MB_SENSOR_C2_3_NVME_CTEMP:
    case MB_SENSOR_C2_4_NVME_CTEMP:
      if(slot_cfg == SLOT_CFG_EMPTY || (!pal_is_ava_card(0)))
        return READING_NA;
      mux_chan = 0;
      break;
    case MB_SENSOR_C3_NVME_CTEMP:
      if(slot_cfg == SLOT_CFG_EMPTY || ( !pal_is_pcie_ssd_card(1)))
         return READING_NA;
      mux_chan = 1;
      break;
    case MB_SENSOR_C3_1_NVME_CTEMP:
    case MB_SENSOR_C3_2_NVME_CTEMP:
    case MB_SENSOR_C3_3_NVME_CTEMP:
    case MB_SENSOR_C3_4_NVME_CTEMP:
      if(slot_cfg == SLOT_CFG_EMPTY || ( !pal_is_ava_card(1)))
        return READING_NA;
      mux_chan = 1;
      break;
    case MB_SENSOR_C4_NVME_CTEMP:
      if(slot_cfg != SLOT_CFG_SS_3x8 || ( !pal_is_pcie_ssd_card(2)))
        return READING_NA;
      mux_chan = 2;
      break;
    case MB_SENSOR_C4_1_NVME_CTEMP:
    case MB_SENSOR_C4_2_NVME_CTEMP:
    case MB_SENSOR_C4_3_NVME_CTEMP:
    case MB_SENSOR_C4_4_NVME_CTEMP:
      if(slot_cfg != SLOT_CFG_SS_3x8 || ( !pal_is_ava_card(2)))
        return READING_NA;
      mux_chan = 2;
      break;
    default:
      return READING_NA;
  }

  switch(sensor_num) {
    case MB_SENSOR_C2_1_NVME_CTEMP:
    case MB_SENSOR_C3_1_NVME_CTEMP:
    case MB_SENSOR_C4_1_NVME_CTEMP:
      switch_chan = 0;
      break;
    case MB_SENSOR_C2_2_NVME_CTEMP:
    case MB_SENSOR_C3_2_NVME_CTEMP:
    case MB_SENSOR_C4_2_NVME_CTEMP:
      switch_chan = 1;
      break;
    case MB_SENSOR_C2_3_NVME_CTEMP:
    case MB_SENSOR_C3_3_NVME_CTEMP:
    case MB_SENSOR_C4_3_NVME_CTEMP:
      switch_chan = 2;
      break;
    case MB_SENSOR_C2_4_NVME_CTEMP:
    case MB_SENSOR_C3_4_NVME_CTEMP:
    case MB_SENSOR_C4_4_NVME_CTEMP:
      switch_chan = 3;
      break;
    case MB_SENSOR_C2_NVME_CTEMP:
    case MB_SENSOR_C3_NVME_CTEMP:
    case MB_SENSOR_C4_NVME_CTEMP:
      switch_chan = 0xff; // no i2c switch
      break;
    default:
      return READING_NA;
  }

  //try to control I2C multiplexer to target channel and it will exit if it fail
  ret = mux_lock(&riser_mux, mux_chan, 2);
  if (ret < 0) {
    ret = READING_NA;
    goto error_exit;
  }

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", RISER_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    ret = READING_NA;
    goto release_mux_and_exit;
  }

  if (switch_chan != 0xff) {
    // control I2C switch to target channel if it has
    ret = pal_control_switch(fd, switch_addr, switch_chan);
    if (ret < 0) {
      ret = READING_NA;
      goto release_mux_and_exit;
    }
  }

  // Read 8 bytes from NVMe
  tbuf[0] = 0x00;
  tcount = 1;
  rcount = 8;

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tcount, rbuf, rcount);
  if (ret < 0) {
    ret = READING_NA;
    goto release_mux_and_exit;
  }
  ret = 0;
  retry[i_retry] = 0;

  // Cmd 0: length, SFLGS, SMART Warnings, CTemp, PDLU, Reserved, Reserved, PEC
  *value = (float)(signed char)rbuf[3];

//if the channel is locked, unlock it and then exit
release_mux_and_exit:

  mux_release(&riser_mux);

//if the channel is busy, exit the function
error_exit:

  if (fd > 0) {
    if (switch_chan != 0xff)
      pal_control_switch(fd, switch_addr, 0xff); // close

    close(fd);
  }

  if (ret == READING_NA && ++retry[i_retry] <= 3)
    ret = READING_SKIP;

  return ret;
}

//When the power is turned off, three states are 0.
enum
{
  MAIN_STATE_PWR_OFF = 0x0,
  CPU0_STATE_PWR_OFF = 0x0,
  CPU1_STATE_PWR_OFF = 0x0,
};

//CPLD power status register define
enum {
  MAIN_PWR_STS_REG = 0x00,
  CPU0_PWR_STS_REG = 0x01,
  CPU1_PWR_STS_REG = 0x02,
};
//CPLD power status register normal value after power on
enum {
  MAIN_PWR_STS_VAL = 0x04, // MAIN_ON
  CPU0_PWR_STS_VAL = 0x13, // CPUPWRGD
  CPU1_PWR_STS_VAL = 0x13, // CPUPWRGD
};

static struct cpld_reg_desc {
  unsigned char offset;
  unsigned char bit;
  char *name;
} cpld_power_seq[] = {
  { 0x07, 5, "PWRGD_PVNN_P1V05_PCH"},
  { 0x07, 4, "PWRGD_DSW_PWROK"},
  { 0x07, 3, "FM_SLP_SUS_N" },
  { 0x07, 2, "RST_RSMRST_N" },
  { 0x07, 1, "FM_SLPS4_N" },
  { 0x07, 0, "FM_SLPS3_N" },
  { 0x03, 7, "FM_CTNR_PS_ON" },
  { 0x0a, 6, "FM_P12V_MAIN_SW_EN" },
  { 0x03, 6, "PWRGD_P12V_MAIN" },
  { 0x0a, 7, "FM_PS_EN" },
  { 0x03, 5, "PWRGD_P5V" },
  { 0x0a, 5, "FM_P3V3_CPLD_EN" },
  { 0x03, 4, "PWRGD_P3V3" },
  { 0x0a, 4, "FM_PVPP_CPU0_EN" },
  { 0x03, 3, "PWRGD_PVPP_ABC" },
  { 0x03, 2, "PWRGD_PVPP_DEF" },
  { 0x0a, 3, "FM_PVPP_CPU1_EN" },
  { 0x03, 1, "PWRGD_PVPP_GHJ" },
  { 0x03, 0, "PWRGD_PVPP_KLM" },
  { 0x0a, 2, "FM_PVDDQ_ABC_EN" },
  { 0x0a, 1, "FM_PVDDQ_DEF_EN" },
  { 0x04, 7, "PWRGD_PVTT_CPU0" },
  { 0x0a, 0, "FM_PVDDQ_GHJ_EN" },
  { 0x0b, 7, "FM_PVDDQ_KLM_EN" },
  { 0x04, 6, "PWRGD_PVTT_CPU1" },
  { 0x0b, 2, "PWRGD_DRAMPWRGD" },
  { 0x0b, 6, "FM_PVCCIO_CPU0_EN" },
  { 0x04, 5, "PWRGD_PVCCIO_CPU0" },
  { 0x0b, 5, "FM_PVCCIO_CPU1_EN" },
  { 0x04, 4, "PWRGD_PVCCIO_CPU1" },
  { 0x0b, 4, "FM_PVCCIN_PVCCSA_CPU0_EN_LVC1" },
  { 0x04, 3, "PWRGD_PVCCIN_CPU0" },
  { 0x0b, 3, "FM_PVCCIN_PVCCSA_CPU1_EN_LVC1" },
  { 0x04, 2, "PWRGD_PVCCIN_CPU1" },
  { 0x04, 1, "PWRGD_PVSA_CPU0" },
  { 0x04, 0, "PWRGD_PVSA_CPU1" },
  { 0x0b, 1, "PWRGD_PCH_PWROK" },
  { 0x0b, 0, "PWRGD_CPU0_LVC3" },
  { 0x0c, 7, "PWRGD_CPU1_LVC3" },
  { 0x05, 7, "PWRGD_CPUPWRGD" },
  { 0x0c, 6, "PWRGD_SYS_PWROK" },
  { 0x05, 6, "RST_PLTRST_N" },
};
static int cpld_power_seq_num = (sizeof(cpld_power_seq)/sizeof(struct cpld_reg_desc));

static int get_CPLD_power_sts(uint8_t *reg)
{
  int fd = 0;
  char fn[32];
  uint8_t tbuf[16] = {0};
  int ret = PAL_ENOTSUP;
  int i;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", CPLD_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING, "[%s] Cannot open the i2c-%d", __func__, CPLD_BUS_ID);
    ret = PAL_ENOTSUP;
    goto exit;
  }

  // Get the data offset 0x00 to 0x0c
  for(i = 0; i < 0x0d; i++) {
    tbuf[0] = i;
    ret = i2c_rdwr_msg_transfer(fd, CPLD_ADDR, tbuf, 1, &reg[i], 1);
    if (ret < 0) {
      syslog(LOG_WARNING, "[%s] Cannot acces the i2c-%d dev: %x", __func__, CPLD_BUS_ID, CPLD_ADDR);
      ret = PAL_ENOTSUP;
      goto exit;
    }
  }

  ret = PAL_EOK;

exit:
  if (fd > 0)
  {
    close(fd);
  }

  return ret;
}

static bool is_slps4_deassert(void)
{
  gpio_value_t value;
  gpio_desc_t *desc = gpio_open_by_shadow("FM_SLPS4_N");
  if (!desc)
    return false;
  if (gpio_get_value(desc, &value)) {
    gpio_close(desc);
    return false;
  }
  gpio_close(desc);
  return value == GPIO_VALUE_HIGH ? true : false;
}

static int
read_CPLD_power_fail_sts (uint8_t fru, uint8_t sensor_num, float *value, int pot) {
  static uint8_t power_fail = 0;
  static uint8_t power_fail_log = 0;
  int fd = 0;
  char fn[32];
  int ret = READING_NA, i;
  uint8_t tbuf[16] = {0};
  uint8_t data_chk, fail_offset;
  unsigned char reg[16];
  char sensor_name[32] = {0}, event_str[30] = {0};

  // Check SLPS4 is high before start monitor CPLD power fail
  if (!is_slps4_deassert()) {
    // Reset
    power_fail = 0;
    power_fail_log = 0;
    ret = 0;
    goto exit;
  }

  // Already log
  if (power_fail_log) {
    ret = 0;
    goto exit;
  }

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", CPLD_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    syslog(LOG_WARNING, "[%s] Cannot open the i2c-%d", __func__, CPLD_BUS_ID);
    ret = READING_NA;
    goto exit;
  }

  // Check the status register 0 to 2
  for(i=0;i<3;i++) {
    switch(i) {
      case MAIN_PWR_STS_REG :
        data_chk = MAIN_PWR_STS_VAL;
        break;
      case CPU0_PWR_STS_REG :
        data_chk = CPU0_PWR_STS_VAL;
        break;
      case CPU1_PWR_STS_REG :
        data_chk = CPU1_PWR_STS_VAL;
        break;
    }

    tbuf[0] = i;
    ret = i2c_rdwr_msg_transfer(fd, CPLD_ADDR, tbuf, 1, &reg[i], 1);
    if (ret < 0) {
      syslog(LOG_WARNING, "[%s] Cannot acces the i2c-%d dev: %x", __func__, CPLD_BUS_ID, CPLD_ADDR);
      ret = READING_NA;
      goto exit;
    }
    if ( reg[i] != data_chk ) {
      fail_offset = i;
      power_fail++;
      break;
    }
  }

  // All status regs are expected
  if (i == 3) {
    power_fail = 0;
  }

  // Check the status regs later, it might has not finished
  if(power_fail <= 3) {
    ret = 0;
    *value = 0;
    goto exit;
  }

  // Power failed, get the data offset 0x03 to 0x0c
  for(i = 3; i < 0x0d; i++) {
    tbuf[0] = i;
    ret = i2c_rdwr_msg_transfer(fd, CPLD_ADDR, tbuf, 1, &reg[i], 1);
    if (ret < 0) {
      ret = READING_NA;
      goto exit;
    }
  }

  // Check the power sequence one by one in order
  for(i=0; i < cpld_power_seq_num; i++) {
    if (!( reg[cpld_power_seq[i].offset] & (1 << cpld_power_seq[i].bit) )) {
      break;
    }
  }

  if (i == cpld_power_seq_num) {
    sprintf(event_str, "Unknown power rail fails");
    // keep the fail status reg offset(0~2)
  } else {
    sprintf(event_str, "%s power rail fails", cpld_power_seq[i].name);
    fail_offset = cpld_power_seq[i].offset;
  }

  pal_get_sensor_name(fru, sensor_num, sensor_name);

  _print_sensor_discrete_log(fru, sensor_num, sensor_name, fail_offset, event_str);
  pal_add_cri_sel(event_str);
  power_fail_log = 1;

  ret = 0;

exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

void
pal_check_power_sts(void)
{
  uint8_t fail_offset =0xff; //init to 0xff to identify the undefined error
  uint8_t reg[16]={0};
  char *sensor_name = "MB_POWER_FAIL";
  const uint8_t sensor_num = MB_SENSOR_POWER_FAIL;
  const uint8_t fru = FRU_MB;
  //the main_state, cpu0_state, cpu1_state are 0 when the power is turned off normally
  const uint8_t normal_power_off_sts[3]={MAIN_STATE_PWR_OFF, CPU0_STATE_PWR_OFF, CPU1_STATE_PWR_OFF};
  char event_str[30] = {0};
  int i;
  int ret;
  int bit_value = 0; //get the bit from the register
  int power_faill_addr = -1;

  sleep(1);//ensure the power data is updated

  //get the power data when the slp4 falling
  ret = get_CPLD_power_sts(reg);
  if ( ret < 0 )
  {
    syslog(LOG_WARNING, "[%s] Cannot get the power status from CPLD", __func__);
    return;
  }

  //if the power is turned off normally. the value of three machine states are 0
  ret = memcmp(reg, normal_power_off_sts, sizeof(normal_power_off_sts));
  if ( PAL_EOK == ret )
  {
    return;
  }

  // Check the power sequence one by one in order.
  for ( i=0; i < cpld_power_seq_num; i++ )
  {
    bit_value = reg[cpld_power_seq[i].offset] & (1 << cpld_power_seq[i].bit);

    //record the fail index if the power fail occur
    //0 means the power fail
    if ( 0 == bit_value )
    {
      power_faill_addr = i;

      sprintf(event_str, "%s power rail fails", cpld_power_seq[power_faill_addr].name);

      fail_offset = cpld_power_seq[power_faill_addr].offset;

      _print_sensor_discrete_log(fru, sensor_num, sensor_name, fail_offset, event_str);

      pal_add_cri_sel(event_str);
    }
  }

  //Unknnow error
  if ( power_faill_addr < 0 )
  {
    sprintf(event_str, "%s power rail fails", "Unknown");

    _print_sensor_discrete_log(fru, sensor_num, sensor_name, fail_offset, event_str);

    pal_add_cri_sel(event_str);
  }
}



static int
pal_key_index(char *key) {

  int i;

  i = 0;
  while(strcmp(key_cfg[i].name, LAST_KEY)) {

    // If Key is valid, return success
    if (!strcmp(key, key_cfg[i].name))
      return i;

    i++;
  }

#ifdef DEBUG
  syslog(LOG_WARNING, "pal_key_index: invalid key - %s", key);
#endif
  return -1;
}

int
pal_get_key_value(char *key, char *value) {
  int index;

  // Check is key is defined and valid
  if ((index = pal_key_index(key)) < 0)
    return -1;

  return kv_get(key, value, NULL, KV_FPERSIST);
}

int
pal_set_key_value(char *key, char *value) {
  int index, ret;
  // Check is key is defined and valid
  if ((index = pal_key_index(key)) < 0)
    return -1;

  if (key_cfg[index].function) {
    ret = key_cfg[index].function(KEY_BEFORE_SET, value);
    if (ret < 0)
      return ret;
  }

  return kv_set(key, value, 0, KV_FPERSIST);
}

static int fw_getenv(char *key, char *value)
{
  char cmd[MAX_KEY_LEN + 32] = {0};
  char *p;
  FILE *fp;

  sprintf(cmd, "/sbin/fw_printenv -n %s", key);
  fp = popen(cmd, "r");
  if (!fp) {
    return -1;
  }
  if (fgets(value, MAX_VALUE_LEN, fp) == NULL) {
    pclose(fp);
    return -1;
  }
  for (p = value; *p != '\0'; p++) {
    if (*p == '\n' || *p == '\r') {
      *p = '\0';
      break;
    }
  }
  pclose(fp);
  return 0;
}

static int fw_setenv(char *key, char *value)
{
  char old_value[MAX_VALUE_LEN] = {0};
  if (fw_getenv(key, old_value) != 0 ||
      strcmp(old_value, value) != 0) {
    /* Set the env key:value if either the key
     * does not exist or the value is different from
     * what we want set */
    char cmd[MAX_VALUE_LEN] = {0};
    snprintf(cmd, MAX_VALUE_LEN, "/sbin/fw_setenv %s %s", key, value);
    return system(cmd);
  }
  return 0;
}

static int
key_func_por_policy (int event, void *arg)
{
  char value[MAX_VALUE_LEN] = {0};
  int ret = -1;

  switch (event) {
    case KEY_BEFORE_SET:
      if (pal_is_fw_update_ongoing(FRU_MB))
        return -1;
      // sync to env
      if ( !strcmp(arg,"lps") || !strcmp(arg,"on") || !strcmp(arg,"off")) {
        ret = fw_setenv("por_policy", (char *)arg);
      }
      else
        return -1;
      break;
    case KEY_AFTER_INI:
      // sync to env
      kv_get("server_por_cfg", value, NULL, KV_FPERSIST);
      ret = fw_setenv("por_policy", value);
      break;
  }

  return ret;
}

static int
key_func_lps (int event, void *arg)
{
  char value[MAX_VALUE_LEN] = {0};
  int ret = -1;

  switch (event) {
    case KEY_BEFORE_SET:
      if (pal_is_fw_update_ongoing(FRU_MB))
        return -1;
      ret = fw_setenv("por_ls", (char *)arg);
      break;
    case KEY_AFTER_INI:
      kv_get("pwr_server_last_state", value, NULL, KV_FPERSIST);
      ret = fw_setenv("por_ls", value);
      break;
  }

  return ret;
}

static void
FORCE_ADR() {
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN];
  size_t len;
  //char vpath[64] = {0};

  sprintf(key, "%s", "mb_machine_config");
  if (kv_get(key, value, &len, KV_FPERSIST) < 0 || len < 13) {
#ifdef DEBUG
    syslog(LOG_WARNING, "FORCE_ADR: get mb_machine_config failed for fru %u", fru);
#endif
    return;
  }
  if(value[12] == 0 || value[12] > 32)
    return;
#if 0 /*disable the force ADR*/
  {
    gpio_desc_t *desc = gpio_open_by_shadow("FM_FORCE_ADR_N");
    if (!desc)
      return;
    if (gpio_set_value(desc, GPIO_VALUE_HIGH))
      goto bail;
    if (gpio_set_value(desc, GPIO_VALUE_LOW))
      goto bail;
    msleep(10);
    if (gpio_set_value(desc, GPIO_VALUE_HIGH))
      goto bail;
bail:
    gpio_close(desc);
  }
#endif
}

// Power Button Override
int
pal_power_button_override(uint8_t fruid) {
  int ret = -1;
  gpio_desc_t *gpio = gpio_open_by_shadow("FM_BMC_PWRBTN_OUT_N");
  if (!gpio) {
    return -1;
  }
  if (gpio_set_value(gpio, GPIO_VALUE_HIGH)) {
    goto bail;
  }
  if (gpio_set_value(gpio, GPIO_VALUE_LOW)) {
    goto bail;
  }
  sleep(6);
  if (gpio_set_value(gpio, GPIO_VALUE_HIGH)) {
    goto bail;
  }
  ret = 0;
bail:
  gpio_close(gpio);
  return ret;
}

// Power On the server in a given slot
static int
server_power_on(void) {
  int ret = -1;
  gpio_desc_t *gpio = gpio_open_by_shadow("FM_BMC_PWRBTN_OUT_N");

  if (!gpio) {
    return -1;
  }
  if (gpio_set_value(gpio, GPIO_VALUE_HIGH)) {
    goto bail;
  }
  if (gpio_set_value(gpio, GPIO_VALUE_LOW)) {
    goto bail;
  }
  sleep(1);
  if (gpio_set_value(gpio, GPIO_VALUE_HIGH)) {
    goto bail;
  }
  sleep(2);
  if (system("/usr/bin/sv restart fscd >> /dev/null") != 0) {
    syslog(LOG_CRIT, "FSCD Restart failed\n");
    goto bail;
  }
  ret = 0;
bail:
  gpio_close(gpio);
  return ret;
}

// Power Off the server in given slot
static int
server_power_off(bool gs_flag) {
  int ret = -1;
  gpio_desc_t *gpio = gpio_open_by_shadow("FM_BMC_PWRBTN_OUT_N");

  if (!gpio) {
    return -1;
  }

  if (system("/usr/bin/sv stop fscd >> /dev/null") != 0) {
    syslog(LOG_CRIT, "FSCD Stop on server power-off failed\n");
    // Still go ahead in power-off
  }

  if (!gs_flag)
    FORCE_ADR();

  if (gpio_set_value(gpio, GPIO_VALUE_HIGH)) {
    goto bail;
  }
  sleep(1);

  if (gpio_set_value(gpio, GPIO_VALUE_LOW)) {
    goto bail;
  }
  if (gs_flag) {
    sleep(DELAY_GRACEFUL_SHUTDOWN);
  } else {
    sleep(DELAY_POWER_OFF);
  }
  if (gpio_set_value(gpio, GPIO_VALUE_HIGH)) {
    goto bail;
  }
  ret = 0;
bail:
  gpio_close(gpio);
  return ret;
}

// Debug Card's UART and BMC/SoL port share UART port and need to enable only one
static int
control_sol_txd(uint8_t fru) {
  uint32_t lpc_fd;
  uint32_t ctrl;
  void *lpc_reg;
  void *lpc_hicr;

  lpc_fd = open("/dev/mem", O_RDWR | O_SYNC );
  if (lpc_fd < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "control_sol_txd: open fails\n");
#endif
    return -1;
  }

  lpc_reg = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, lpc_fd,
             AST_LPC_BASE);
  lpc_hicr = (char*)lpc_reg + HICRA_OFFSET;

  // Read HICRA register
  ctrl = *(volatile uint32_t*) lpc_hicr;
  // Clear bits for UART2 and UART3 routing
  ctrl &= (~HICRA_MASK_UART2);
  ctrl &= (~HICRA_MASK_UART3);

  // Route UART2 to UART3 for SoL purpose
  ctrl |= (UART2_TO_UART3 << 22);

  // Route DEBUG to UART1 for TXD control
  ctrl |= (UART3_TO_UART2 << 19);

  *(volatile uint32_t*) lpc_hicr = ctrl;

  munmap(lpc_reg, PAGE_SIZE);
  close(lpc_fd);

  return 0;
}

// Platform Abstraction Layer (PAL) Functions
int
pal_get_platform_name(char *name) {
  strcpy(name, FBTP_PLATFORM_NAME);

  return 0;
}

int
pal_get_num_slots(uint8_t *num) {
  *num = FBTP_MAX_NUM_SLOTS;

  return 0;
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
  uint8_t slot_cfg = 0;
  char full_name[LARGEST_DEVICE_NAME + 1]={0};
  FILE *fp;
  *status = 0;

  switch (fru) {
    case FRU_MB:
      *status = 1;
      break;
    case FRU_NIC:
      snprintf(full_name, LARGEST_DEVICE_NAME, "%s", "/sys/bus/i2c/devices/8-001f/hwmon");
      fp = fopen(full_name, "r");
      if (!fp) {
        return -1;
      }
      fclose(fp);
      *status = 1;
      break;
    case FRU_RISER_SLOT2:
    case FRU_RISER_SLOT3:
    case FRU_RISER_SLOT4:
      if (pal_get_slot_cfg_id(&slot_cfg) < 0)
        return -1;
      if (slot_cfg == SLOT_CFG_EMPTY)
        return 0;
      *status = 1;
      break;
    default:
      return -1;
    }
  return 0;
}

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  *status = 1;

  return 0;
}

int
pal_is_slot_server(uint8_t fru) {
  if (fru == FRU_MB)
    return 1;
  return 0;
}

int
pal_is_debug_card_prsnt(uint8_t *status) {
  gpio_desc_t *desc = gpio_open_by_shadow("FM_POST_CARD_PRES_BMC_N");
  gpio_value_t value;
  int ret = -1;

  if (!desc) {
    return -1;
  }
  if (gpio_get_value(desc, &value) == 0) {
    *status = value == GPIO_VALUE_LOW ? 1 : 0;
    ret = 0;
  }
  gpio_close(desc);
  return ret;
}

int
pal_get_server_power(uint8_t fru, uint8_t *status) {
  gpio_desc_t *gpio;
  gpio_value_t val;
  int ret = -1;

  if ( fru != FRU_MB)
    return -1;

  gpio = gpio_open_by_shadow("PWRGD_SYS_PWROK");
  if (!gpio) {
    return -1;
  }
  if (gpio_get_value(gpio, &val) == 0)  {
    ret = 0;
    *status = val == GPIO_VALUE_LOW ? 0 : 1;
  }
  gpio_close(gpio);
  return ret;
}

static bool
is_server_off(void) {
  int ret;
  uint8_t status;
  ret = pal_get_server_power(FRU_MB, &status);
  if (ret) {
    return false;
  }

  if (status == SERVER_POWER_OFF) {
    return true;
  } else {
    return false;
  }
}


// Power Off, Power On, or Power Reset the server in given slot
int
pal_set_server_power(uint8_t fru, uint8_t cmd) {
  uint8_t status;
  bool gs_flag = false;
  uint8_t ret;

  if (pal_get_server_power(fru, &status) < 0) {
    return -1;
  }

  switch(cmd) {
    case SERVER_POWER_ON:
      if (status == SERVER_POWER_ON)
        return 1;
      else
        return server_power_on();
      break;

    case SERVER_POWER_OFF:
      if (status == SERVER_POWER_OFF)
        return 1;
      else
        return server_power_off(gs_flag);
      break;

    case SERVER_POWER_CYCLE:
      if (status == SERVER_POWER_ON) {
        if (server_power_off(gs_flag))
          return -1;

        sleep(DELAY_POWER_CYCLE);

        return server_power_on();

      } else if (status == SERVER_POWER_OFF) {

        return (server_power_on());
      }
      break;

    case SERVER_GRACEFUL_SHUTDOWN:
      if (status == SERVER_POWER_OFF)
        return 1;
      gs_flag = true;
      return server_power_off(gs_flag);
      break;

   case SERVER_POWER_RESET:
      if (status == SERVER_POWER_ON) {
        FORCE_ADR();
        ret = pal_set_rst_btn(fru, 0);
        if (ret < 0)
          return ret;
        msleep(100); //some server miss to detect a quick pulse, so delay 100ms between low high
        ret = pal_set_rst_btn(fru, 1);
        if (ret < 0)
          return ret;
      } else if (status == SERVER_POWER_OFF)
        return -1;
      break;

    default:
      return -1;
  }

  return 0;
}

int
pal_sled_cycle(void) {
  // Send command to HSC power cycle
  if (system("i2cset -y 7 0x45 0xd9 c &> /dev/null") != 0) {
    syslog(LOG_CRIT, "SLED Cycle failed\n");
    return -1;
  }
  return 0;
}

// Return the front panel's Reset Button status
int
pal_get_rst_btn(uint8_t *status) {
  int ret = -1;
  gpio_value_t value;
  gpio_desc_t *desc = gpio_open_by_shadow("FM_THROTTLE_N");
  if (!desc) {
    return -1;
  }
  if (0 == gpio_get_value(desc, &value)) {
    *status = value == GPIO_VALUE_HIGH ? 0 : 1;
    ret = 0;
  }
  gpio_close(desc);
  return ret;
}

// Update the Reset button input to the server at given slot
int
pal_set_rst_btn(uint8_t slot, uint8_t status) {
  gpio_desc_t *desc;
  int ret = -1;

  if (slot != FRU_MB) {
    return -1;
  }
  desc = gpio_open_by_shadow("RST_BMC_SYSRST_BTN_OUT_N");
  if (!desc) {
    return -1;
  }
  if (gpio_set_value(desc, status ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW) == 0) {
    ret = 0;
  }
  gpio_close(desc);
  return ret;
}

// Update the LED for the given slot with the status
int 
pal_set_sled_led(uint8_t fru, uint8_t status) {
  int ret = -1;

  gpio_desc_t *gpio = gpio_open_by_shadow("SERVER_POWER_LED");
  if (!gpio) {
    return -1;
  }

  //TODO: Need to check power LED control from CPLD
  if (gpio_set_value(gpio, status ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW) == 0) {
    ret = 0;
  }
  gpio_close(gpio);
  return ret;
}

// Update Heartbeet LED
int
pal_set_hb_led(uint8_t status) {
  char cmd[64] = {0};
  char *val;

  if (status) {
    val = "1";
  } else {
    val = "0";
  }

  sprintf(cmd, "devmem 0x1e6c0064 32 %s", val);

  if (system(cmd) != 0) {
    syslog(LOG_ERR, "Setting HB LED failed!\n");
    return -1;
  }

  return 0;
}

// Update the Identification LED for the given fru with the status
int
pal_set_id_led(uint8_t fru, uint8_t status) {
  return pal_set_sled_led(fru, status);
}

// Switch the UART mux to the given fru
int
pal_switch_uart_mux(uint8_t fru) {
  return control_sol_txd(fru);
}

static int
check_postcodes(uint8_t fru_id, uint8_t sensor_num, float *value) {
  static int log_asserted = 0;
  const int loop_threshold = 3;
  const int longest_loop_code = 4;
  size_t len;
  int i, check_from, check_until;
  uint8_t buff[256];
  uint8_t location, maj_err, min_err, loop_convention;
  int ret = READING_NA, rc;
  static unsigned int retry=0;
  char sensor_name[32] = {0};
  char str[32] = {0};

  if (fru_id != 1) {
    syslog(LOG_ERR, "Not Supported Operation for fru %d", fru_id);
    goto error_exit;
  }

  if (is_server_off()) {
    log_asserted = 0;
    goto error_exit;
  }

  len = 0; // clear higher bits
  rc = pal_get_80port_record(FRU_MB, buff, sizeof(buff), &len);
  if (rc != PAL_EOK)
    goto error_exit;

  loop_convention = 0; // NO loop
  check_from = len - (longest_loop_code * (loop_threshold) );
  check_until = len - (longest_loop_code * (loop_threshold+1) );
  // Check post code from last 12 to last 16 codes
  for(i = check_from; i >= 0 && i >= check_until; i--) {

    if (i > 0 && buff[i-1] == 0x00) {
      // If previous code is 00h, find the 1st 00h.
      continue;
    }

    // Check Loop Convention1, 0x00 -> DIMM Location -> Major Error Code - > Minor Error Code -> Loop
    // Major shall not be 0x00, and it also shall not be 0xA0 ~ 0xDF
    // DIMM Location and Minor might be 0x00, but they shall not be 0x00 at the same time.
    // When Minor is 0x00, the DIMM location shall be 0xA0 ~ 0xDF
    if (buff[i] == 0x00 &&
        buff[i+4] == 0x00 &&
        !memcmp(&buff[i], &buff[i+4], len - i - 4) ) {

      // Check special minor 00h case
      if (buff[i+1] == 0x00 &&
          buff[i+2] >= 0xA0 &&
          buff[i+2] <= 0xDF ) {
        // This is minor 00h case, the 2nd 00h is starting 00h
        i = i + 1;
      }

      // PostCode looping
      loop_convention = 1;
      location = buff[i+1];
      maj_err = buff[i+2];
      min_err = buff[i+3];
      break;
    }
    // Check Loop Convention2, 0x00 -> Major Error Code -> Minor Error Code ->Loop
    // Major and Minor shall not be 0x00 in this convention
    else if (buff[i] == 0x00 &&
             buff[i+3] == 0x00 &&
             !memcmp(&buff[i], &buff[i+3], len - i - 3) ) {
      // PostCode looping
      loop_convention = 2;
      location = 0x00;
      maj_err = buff[i+1];
      min_err = buff[i+2];
      break;
    }
  }

  if (loop_convention != 0) {
    if (!log_asserted) {
      pal_get_sensor_name(fru_id, sensor_num, sensor_name);
      if (loop_convention == 1) { // Convention1
        snprintf(str, sizeof(str), "Location:%02X Err:%02X %02X",location, maj_err, min_err);
        _print_sensor_discrete_log(fru_id, sensor_num, sensor_name, 0x01, str);
        snprintf(str, sizeof(str), "DIMM %02X initial fails",location);
        pal_add_cri_sel(str);
        //syslog(LOG_CRIT, "Memory training failure at %02X MajErr:%02X, MinErr:%02X", location, maj_err, min_err);
    } else { // Convention2
        snprintf(str, sizeof(str), "Location Unknown Err:%02X %02X", maj_err, min_err);
        _print_sensor_discrete_log(fru_id, sensor_num, sensor_name, 0x01, str);
        //syslog(LOG_CRIT, "Memory training failure MajErr:%02X, MinErr:%02X", maj_err, min_err);
        snprintf(str, sizeof(str), "DIMM XX initial fails");
        pal_add_cri_sel(str);
      }
    }
    log_asserted = 1;
  }
  else
  {
    log_asserted = 0;
  }
  *value = (float)log_asserted;
  ret = 0;

error_exit:
  if ((ret == READING_NA) && (retry < MAX_READ_RETRY)){
    ret = READING_SKIP;
    retry++;
  } else {
    retry = 0;
  }

  return ret;
}

static int
check_frb3(uint8_t fru_id, uint8_t sensor_num, float *value) {
  static unsigned int retry = 0;
  static uint8_t frb3_fail = 0x10; // bit 4: FRB3 failure
  static time_t rst_time = 0;
  uint8_t postcodes[256] = {0};
  struct stat file_stat;
  int ret = READING_NA, rc;
  size_t len = 0;
  char sensor_name[32] = {0};
  char error[32] = {0};

  if (fru_id != 1) {
    syslog(LOG_ERR, "Not Supported Operation for fru %d", fru_id);
    return READING_NA;
  }

  if (stat("/tmp/rst_touch", &file_stat) == 0 && file_stat.st_mtime > rst_time) {
    rst_time = file_stat.st_mtime;
    // assume fail till we know it is not
    frb3_fail = 0x10; // bit 4: FRB3 failure
    retry = 0;
    // cache current postcode buffer
    if (stat("/tmp/DWR", &file_stat) != 0) {
      memset(postcodes_last, 0, sizeof(postcodes_last));
      pal_get_80port_record(FRU_MB, postcodes_last, sizeof(postcodes_last), &len);
    }
  }

  if (frb3_fail) {
    // KCS transaction
    if (stat("/tmp/kcs_touch", &file_stat) == 0 && file_stat.st_mtime > rst_time)
      frb3_fail = 0;

    // Port 80 updated
    memset(postcodes, 0, sizeof(postcodes));
    rc = pal_get_80port_record(FRU_MB, postcodes, sizeof(postcodes), &len);
    if (rc == PAL_EOK && memcmp(postcodes_last, postcodes, 256) != 0) {
      frb3_fail = 0;
    }

    // BIOS POST COMPLT, in case BMC reboot when system idle in OS
    if (pal_is_BIOS_completed(FRU_MB))
      frb3_fail = 0;
  }

  if (frb3_fail)
    retry++;
  else
    retry = 0;

  if (retry == MAX_READ_RETRY) {
    pal_get_sensor_name(fru_id, sensor_num, sensor_name);
    snprintf(error, sizeof(error), "FRB3 failure");
    _print_sensor_discrete_log(fru_id, sensor_num, sensor_name, frb3_fail, error);
  }

  *value = (float)frb3_fail;
  ret = 0;

  return ret;
}

int
pal_get_fru_list(char *list) {

  strcpy(list, pal_fru_list);
  return 0;
}

int
pal_get_fru_id(char *str, uint8_t *fru) {
  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "mb")) {
    *fru = FRU_MB;
  } else if (!strcmp(str, "nic")) {
    *fru = FRU_NIC;
  } else if (!strcmp(str, "riser_slot2")) {
    *fru = FRU_RISER_SLOT2;
  } else if (!strcmp(str, "riser_slot3")) {
    *fru = FRU_RISER_SLOT3;
  } else if (!strcmp(str, "riser_slot4")) {
    *fru = FRU_RISER_SLOT4;
  } else if (!strncmp(str, "fru", 3)) {
    *fru = atoi(&str[3]);
    if (*fru <= FRU_NIC || *fru > MAX_NUM_FRUS)
      return -1;
  } else {
    syslog(LOG_WARNING, "pal_get_fru_id: Wrong fru#%s", str);
    return -1;
  }

  return 0;
}

int
pal_get_fru_name(uint8_t fru, char *name) {
  switch(fru) {
    case FRU_MB:
      strcpy(name, "mb");
      break;
    case FRU_NIC:
      strcpy(name, "nic");
      break;
    case FRU_RISER_SLOT2:
      strcpy(name, "riser_slot2");
      break;
    case FRU_RISER_SLOT3:
      strcpy(name, "riser_slot3");
      break;
    case FRU_RISER_SLOT4:
      strcpy(name, "riser_slot4");
      break;
    default:
      if (fru > MAX_NUM_FRUS)
        return -1;
      sprintf(name, "fru%d", fru);
      break;
  }
  return 0;
}

int
pal_devnum_to_fruid(int devnum)
{
  return FRU_MB;
}

int
pal_channel_to_bus(int channel)
{
  switch (channel) {
    case 0:
      return 9; // USB (LCD Debug Board)
    case 6:
      return 4; // ME
    case 9:
      return 1; // Riser (Big Basin)
  }

  // Debug purpose, map to real bus number
  if (channel & 0x80) {
    return (channel & 0x7f);
  }

  return channel;
}


int
pal_get_fru_sdr_path(uint8_t fru, char *path) {
  return -1;
}

int
pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  return -1;
}

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  switch(fru) {
  case FRU_MB:
    *sensor_list = (uint8_t *) mb_sensor_list;
    *cnt = mb_sensor_cnt;
    break;
  case FRU_NIC:
    *sensor_list = (uint8_t *) nic_sensor_list;
    *cnt = nic_sensor_cnt;
    break;
  case FRU_RISER_SLOT2:
    *sensor_list = (uint8_t *) riser_slot2_sensor_list;
    *cnt = riser_slot2_sensor_cnt;
    break;
  case FRU_RISER_SLOT3:
    *sensor_list = (uint8_t *) riser_slot3_sensor_list;
    *cnt = riser_slot3_sensor_cnt;
    break;
  case FRU_RISER_SLOT4:
    *sensor_list = (uint8_t *) riser_slot4_sensor_list;
    *cnt = riser_slot4_sensor_cnt;
    break;
  default:
    if (fru > MAX_NUM_FRUS)
      return -1;
    // Nothing to read yet.
    *sensor_list = NULL;
    *cnt = 0;
  }

  return 0;
}

int
pal_fruid_write(uint8_t fru, char *path)
{
  int fru_size = 0;
  char command[128]={0};
  char device_name[16]={0};
  uint8_t bus = 0;
  uint8_t device_addr = 0;
  uint8_t device_type = 0;
  uint8_t acutal_riser_slot = 0;
  int ret=PAL_EOK;
  FILE *fruid_fd;

  fruid_fd = fopen(path, "rb");
  if ( NULL == fruid_fd ) {
    syslog(LOG_WARNING, "[%s] unable to open the file: %s", __func__, path);
    return PAL_ENOTSUP;
  }

  fseek(fruid_fd, 0, SEEK_END);
  fru_size = (uint32_t) ftell(fruid_fd);
  fclose(fruid_fd);
  printf("[%s]FRU Size: %d\n", __func__, fru_size);
  switch (fru)
  {
    case FRU_RISER_SLOT2:
    case FRU_RISER_SLOT3:
    case FRU_RISER_SLOT4:
      acutal_riser_slot = fru - FRU_RISER_SLOT2;//make the slot start from 0
      if ( true == pal_is_ava_card( acutal_riser_slot ) )
      {
        device_type = FOUND_AVA_DEVICE;
        device_addr = 0x50;
        bus = RISER_BUS_ID;
        sprintf(device_name, "24c64");
        sprintf(command, "dd if=%s of=%s bs=%d count=1", path, EEPROM_RISER, fru_size);
      }
      else if ( true == pal_is_retimer_card( acutal_riser_slot ) )
      {
        device_type = FOUND_RETIMER_DEVICE;
        device_addr = 0x55;
        bus = 0x3;
        sprintf(device_name, "24c02");
        sprintf(command, "dd if=%s of=%s bs=%d count=1", path, EEPROM_RETIMER, fru_size);
      }
      else
      {
        //if there is no riser card, return
        syslog(LOG_WARNING, "[%s] There is no fru on the riser slot %d ", __func__, fru);
        return PAL_ENOTSUP;
      }
    break;

    default:
      //if there is an unknown device on the slot, return
      syslog(LOG_WARNING, "[%s] Unknown device on the fru %d ", __func__, fru);
      return PAL_ENOTSUP;
    break;
  }

  switch (device_type)
  {
    case FOUND_AVA_DEVICE:
      ret = mux_lock(&riser_mux, acutal_riser_slot, 5);
      if ( PAL_EOK == ret )
      {
        pal_add_i2c_device(bus, device_name, device_addr);
        if (system(command) != 0) {
          syslog(LOG_WARNING, "%s failed\n", command);
        }
        //compare the in and out data
        ret=pal_compare_fru_data((char*)EEPROM_RISER, path, fru_size);
        if (ret < 0)
        {
          ret = PAL_ENOTSUP;
          if (system("i2cdetect -y -q 1 > /tmp/AVA_FRU_FAIL.log") != 0) {
            syslog(LOG_ERR, "Gathering I2C detect failure log failed\n");
          }
          syslog(LOG_ERR, "[%s] AVA FRU Write Fail", __func__);
        }

        pal_del_i2c_device(bus, device_addr);
        mux_release(&riser_mux);
      }
    break;

    case FOUND_RETIMER_DEVICE:
      ret = pal_control_mux_to_target_ch(acutal_riser_slot, 0x3/*bus number*/, 0xe2/*mux address*/);
      if ( PAL_EOK == ret )
      {
        pal_add_i2c_device(bus, device_name, device_addr);
        if (system(command) != 0) {
          syslog(LOG_ERR, "%s failed", command);
        }
        ret = pal_compare_fru_data((char*)EEPROM_RETIMER, path, fru_size);
        if ( ret < 0 )
        {
          ret = PAL_ENOTSUP;
          if (system("i2cdetect -y -q 3 > /tmp/RETIMER_FRU_FAIL.log") != 0) {
            syslog(LOG_ERR, "Gathering I2C detect failure log failed");
          }
          syslog(LOG_ERR, "[%s] RETIMER FRU Write Fail", __func__);
        }
        pal_del_i2c_device(bus, device_addr);
      }
      ret = PAL_EOK;
    break;

  }

  return ret;
}

int
pal_get_sensor_poll_interval(uint8_t fru, uint8_t sensor_num, uint32_t *value)
{
  //default poll interval
  *value = 2;

  switch (fru)
  {
    case FRU_MB:
      if ( MB_SENSOR_P3V_BAT == sensor_num )
      {
        *value = 3600;
      }
      break;

    case FRU_NIC:
    case FRU_RISER_SLOT2:
    case FRU_RISER_SLOT3:
    case FRU_RISER_SLOT4:
      break;
  }

  return PAL_EOK;
}

static int
read_battery_status(float *value)
{
  int ret = -1;
  gpio_desc_t *gp_batt = gpio_open_by_shadow("FM_BATTERY_SENSE_EN_N");
  if (!gp_batt) {
    return -1;
  }
  if (gpio_set_value(gp_batt, GPIO_VALUE_LOW)) {
    goto bail;
  }
  msleep(10);
  ret = sensors_read_adc("MB_P3V_BAT", value);
  gpio_set_value(gp_batt, GPIO_VALUE_HIGH);
bail:
  gpio_close(gp_batt);
  return ret;
}


int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char fru_name[32];
  int ret;
  static uint8_t poweron_10s_flag = 0;
  bool server_off;

  pal_get_fru_name(fru, fru_name);
  sprintf(key, "%s_sensor%d", fru_name, sensor_num);
  switch(fru) {
  case FRU_MB:
  case FRU_RISER_SLOT2:
  case FRU_RISER_SLOT3:
  case FRU_RISER_SLOT4:
    server_off = is_server_off();
    if (server_off) {
      poweron_10s_flag = 0;
      // Power is OFF, so only some of the sensors can be read
      switch(sensor_num) {
      // Temp. Sensors
      case MB_SENSOR_INLET_TEMP:
        ret = sensors_read("tmp421-i2c-6-4e", "MB_INLET_TEMP", (float *)value);
        break;
      case MB_SENSOR_OUTLET_TEMP:
        ret = sensors_read("tmp421-i2c-6-4f", "MB_OUTLET_TEMP", (float *)value);
        break;
      case MB_SENSOR_INLET_REMOTE_TEMP:
        ret = sensors_read("tmp421-i2c-6-4e", "MB_INLET_REMOTE_TEMP", (float *)value);
        if (!ret)
          apply_inlet_correction((float *) value);
        break;
      case MB_SENSOR_OUTLET_REMOTE_TEMP:
        ret = sensors_read("tmp421-i2c-6-4f", "MB_OUTLET_REMOTE_TEMP", (float *)value);
        break;
      case MB_SENSOR_P12V:
        ret = sensors_read_adc("MB_P12V", (float *)value);
        break;
      case MB_SENSOR_P1V05:
        ret = sensors_read_adc("MB_P1V05", (float *)value);
        break;
      case MB_SENSOR_PVNN_PCH_STBY:
        ret = sensors_read_adc("MB_PVNN_PCH_STBY", (float *)value);
        break;
      case MB_SENSOR_P3V3_STBY:
        ret = sensors_read_adc("MB_P3V3_STBY", (float *)value);
        break;
      case MB_SENSOR_P5V_STBY:
        ret = sensors_read_adc("MB_P5V_STBY", (float *)value);
        break;
      case MB_SENSOR_P3V_BAT:
        ret = read_battery_status((float *)value);
        break;

      // Hot Swap Controller
      case MB_SENSOR_HSC_IN_VOLT:
        ret = read_sensor_reading_from_ME(MB_SENSOR_HSC_IN_VOLT, (float*) value);
        break;
      case MB_SENSOR_HSC_OUT_CURR:
        ret = read_hsc_current_value((float*) value);
        break;
      case MB_SENSOR_HSC_TEMP:
        ret = read_hsc_temp_value((float*) value);
        break;
      case MB_SENSOR_HSC_IN_POWER:
        ret = read_sensor_reading_from_ME(MB_SENSOR_HSC_IN_POWER, (float*) value);
        break;
      case MB_SENSOR_POWER_FAIL:
        ret = read_CPLD_power_fail_sts (fru, sensor_num, (float*) value, poweron_10s_flag);
        break;
      default:
        ret = READING_NA;
        break;
      }
    } else {
      if((poweron_10s_flag < 5) && ((sensor_num == MB_SENSOR_HSC_IN_VOLT) ||
         (sensor_num == MB_SENSOR_HSC_OUT_CURR) || (sensor_num == MB_SENSOR_HSC_IN_POWER) ||
         (sensor_num == MB_SENSOR_HSC_TEMP) ||
         (sensor_num == MB_SENSOR_FAN0_TACH) || (sensor_num == MB_SENSOR_FAN1_TACH))) {
        if(sensor_num == MB_SENSOR_HSC_IN_POWER){
          poweron_10s_flag++;
        }
        ret = READING_NA;
        break;
      }
      switch(sensor_num) {
      // Temp. Sensors
      case MB_SENSOR_INLET_TEMP:
        ret = sensors_read("tmp421-i2c-6-4e", "MB_INLET_TEMP", (float *)value);
        break;
      case MB_SENSOR_OUTLET_TEMP:
        ret = sensors_read("tmp421-i2c-6-4f", "MB_OUTLET_TEMP", (float *)value);
        break;
      case MB_SENSOR_INLET_REMOTE_TEMP:
        ret = sensors_read("tmp421-i2c-6-4e", "MB_INLET_REMOTE_TEMP", (float *)value);
        if (!ret)
          apply_inlet_correction((float *) value);
        break;
      case MB_SENSOR_OUTLET_REMOTE_TEMP:
        ret = sensors_read("tmp421-i2c-6-4f", "MB_OUTLET_REMOTE_TEMP", (float *)value);
        break;
      // Fan Sensors
      case MB_SENSOR_FAN0_TACH:
        ret = read_fan_value("fan1", (float *)value);
        break;
      case MB_SENSOR_FAN1_TACH:
        ret = read_fan_value("fan3", (float *)value);
        break;
      // Various Voltages
      case MB_SENSOR_P3V3:
        ret = sensors_read_adc("MB_P3V3", (float *)value);
        break;
      case MB_SENSOR_P5V:
        ret = sensors_read_adc("MB_P5V", (float *)value);
        break;
      case MB_SENSOR_P12V:
        ret = sensors_read_adc("MB_P12V", (float *)value);
        break;
      case MB_SENSOR_P1V05:
        ret = sensors_read_adc("MB_P1V05", (float *)value);
        break;
      case MB_SENSOR_PVNN_PCH_STBY:
        ret = sensors_read_adc("MB_PVNN_PCH_STBY", (float *)value);
        break;
      case MB_SENSOR_P3V3_STBY:
        ret = sensors_read_adc("MB_P3V3_STBY", (float *)value);
        break;
      case MB_SENSOR_P5V_STBY:
        ret = sensors_read_adc("MB_P5V_STBY", (float *)value);
        break;
      case MB_SENSOR_P3V_BAT:
        ret = read_battery_status((float *)value);
        break;

      // Hot Swap Controller
      case MB_SENSOR_HSC_IN_VOLT:
        ret = read_sensor_reading_from_ME(MB_SENSOR_HSC_IN_VOLT, (float*) value);
        break;
      case MB_SENSOR_HSC_OUT_CURR:
        ret = read_hsc_current_value((float*) value);
        break;
      case MB_SENSOR_HSC_TEMP:
        ret = read_hsc_temp_value((float*) value);
        break;
      case MB_SENSOR_HSC_IN_POWER:
        ret = read_sensor_reading_from_ME(MB_SENSOR_HSC_IN_POWER, (float*) value);
        break;
      //CPU, DIMM, PCH Temp
      case MB_SENSOR_CPU0_TEMP:
      case MB_SENSOR_CPU1_TEMP:
        ret = read_cpu_temp(sensor_num, (float*) value);
        break;
      case MB_SENSOR_CPU0_DIMM_GRPA_TEMP:
      case MB_SENSOR_CPU0_DIMM_GRPB_TEMP:
      case MB_SENSOR_CPU1_DIMM_GRPC_TEMP:
      case MB_SENSOR_CPU1_DIMM_GRPD_TEMP:
        ret = read_dimm_temp(sensor_num, (float*) value);
        break;
      case MB_SENSOR_CPU0_PKG_POWER:
      case MB_SENSOR_CPU1_PKG_POWER:
        ret = read_cpu_package_power(sensor_num, (float*) value);
        break;
      case MB_SENSOR_PCH_TEMP:
        ret = read_sensor_reading_from_ME(MB_SENSOR_PCH_TEMP, (float*) value);
        break;
      //VR Sensors
      case MB_SENSOR_VR_CPU0_VCCIN_TEMP:
        ret = vr_read_temp(VR_CPU0_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VCCIN_CURR:
        ret = vr_read_curr(VR_CPU0_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VCCIN_VOLT:
        ret = vr_read_volt(VR_CPU0_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VCCIN_POWER:
        ret = vr_read_power(VR_CPU0_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VSA_TEMP:
        ret = vr_read_temp(VR_CPU0_VSA, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VSA_CURR:
        ret = vr_read_curr(VR_CPU0_VSA, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VSA_VOLT:
        ret = vr_read_volt(VR_CPU0_VSA, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VSA_POWER:
        ret = vr_read_power(VR_CPU0_VSA, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VCCIO_TEMP:
        ret = vr_read_temp(VR_CPU0_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VCCIO_CURR:
        ret = vr_read_curr(VR_CPU0_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VCCIO_VOLT:
        ret = vr_read_volt(VR_CPU0_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VCCIO_POWER:
        ret = vr_read_power(VR_CPU0_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_GRPA_TEMP:
        ret = vr_read_temp(g_vr_cpu0_vddq_abc, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_GRPA_CURR:
        ret = vr_read_curr(g_vr_cpu0_vddq_abc, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_GRPA_VOLT:
        ret = vr_read_volt(g_vr_cpu0_vddq_abc, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_GRPA_POWER:
        ret = vr_read_power(g_vr_cpu0_vddq_abc, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_GRPB_TEMP:
        ret = vr_read_temp(g_vr_cpu0_vddq_def, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_GRPB_CURR:
        ret = vr_read_curr(g_vr_cpu0_vddq_def, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_GRPB_VOLT:
        ret = vr_read_volt(g_vr_cpu0_vddq_def, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU0_VDDQ_GRPB_POWER:
        ret = vr_read_power(g_vr_cpu0_vddq_def, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_CPU1_VCCIN_TEMP:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_temp(VR_CPU1_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VCCIN_CURR:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_curr(VR_CPU1_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VCCIN_VOLT:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_volt(VR_CPU1_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VCCIN_POWER:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_power(VR_CPU1_VCCIN, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VSA_TEMP:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_temp(VR_CPU1_VSA, VR_LOOP_PAGE_1, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VSA_CURR:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_curr(VR_CPU1_VSA, VR_LOOP_PAGE_1, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VSA_VOLT:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_volt(VR_CPU1_VSA, VR_LOOP_PAGE_1, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VSA_POWER:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_power(VR_CPU1_VSA, VR_LOOP_PAGE_1, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VCCIO_TEMP:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_temp(VR_CPU1_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VCCIO_CURR:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_curr(VR_CPU1_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VCCIO_VOLT:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_volt(VR_CPU1_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VCCIO_POWER:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_power(VR_CPU1_VCCIO, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GRPC_TEMP:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_temp(g_vr_cpu1_vddq_ghj, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GRPC_CURR:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_curr(g_vr_cpu1_vddq_ghj, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GRPC_VOLT:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_volt(g_vr_cpu1_vddq_ghj, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GRPC_POWER:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_power(g_vr_cpu1_vddq_ghj, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GRPD_TEMP:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_temp(g_vr_cpu1_vddq_klm, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GRPD_CURR:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_curr(g_vr_cpu1_vddq_klm, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GRPD_VOLT:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_volt(g_vr_cpu1_vddq_klm, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_CPU1_VDDQ_GRPD_POWER:
        if (is_cpu_socket_occupy(1))
          ret = vr_read_power(g_vr_cpu1_vddq_klm, VR_LOOP_PAGE_0, (float*) value);
        else
          ret = READING_NA;
        break;
      case MB_SENSOR_VR_PCH_PVNN_TEMP:
        ret = vr_read_temp(VR_PCH_PVNN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_PCH_PVNN_CURR:
        ret = vr_read_curr(VR_PCH_PVNN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_PCH_PVNN_VOLT:
        ret = vr_read_volt(VR_PCH_PVNN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_PCH_PVNN_POWER:
        ret = vr_read_power(VR_PCH_PVNN, VR_LOOP_PAGE_0, (float*) value);
        break;
      case MB_SENSOR_VR_PCH_P1V05_TEMP:
        ret = vr_read_temp(VR_PCH_P1V05, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_PCH_P1V05_CURR:
        ret = vr_read_curr(VR_PCH_P1V05, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_PCH_P1V05_VOLT:
        ret = vr_read_volt(VR_PCH_P1V05, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_VR_PCH_P1V05_POWER:
        ret = vr_read_power(VR_PCH_P1V05, VR_LOOP_PAGE_1, (float*) value);
        break;
      case MB_SENSOR_C2_AVA_FTEMP:
      case MB_SENSOR_C2_AVA_RTEMP:
      case MB_SENSOR_C3_AVA_FTEMP:
      case MB_SENSOR_C3_AVA_RTEMP:
      case MB_SENSOR_C4_AVA_FTEMP:
      case MB_SENSOR_C4_AVA_RTEMP:
        ret = read_ava_temp(sensor_num, (float*) value);
        break;
      case MB_SENSOR_C2_NVME_CTEMP:
      case MB_SENSOR_C3_NVME_CTEMP:
      case MB_SENSOR_C4_NVME_CTEMP:
      case MB_SENSOR_C2_1_NVME_CTEMP:
      case MB_SENSOR_C2_2_NVME_CTEMP:
      case MB_SENSOR_C2_3_NVME_CTEMP:
      case MB_SENSOR_C2_4_NVME_CTEMP:
      case MB_SENSOR_C3_1_NVME_CTEMP:
      case MB_SENSOR_C3_2_NVME_CTEMP:
      case MB_SENSOR_C3_3_NVME_CTEMP:
      case MB_SENSOR_C3_4_NVME_CTEMP:
      case MB_SENSOR_C4_1_NVME_CTEMP:
      case MB_SENSOR_C4_2_NVME_CTEMP:
      case MB_SENSOR_C4_3_NVME_CTEMP:
      case MB_SENSOR_C4_4_NVME_CTEMP:
        ret = read_nvme_temp(sensor_num, (float*) value);
        break;
      case MB_SENSOR_C2_P12V_INA230_VOL:
      case MB_SENSOR_C3_P12V_INA230_VOL:
      case MB_SENSOR_C4_P12V_INA230_VOL:
      case MB_SENSOR_CONN_P12V_INA230_VOL:
      case MB_SENSOR_C2_P12V_INA230_CURR:
      case MB_SENSOR_C3_P12V_INA230_CURR:
      case MB_SENSOR_C4_P12V_INA230_CURR:
      case MB_SENSOR_CONN_P12V_INA230_CURR:
      case MB_SENSOR_C2_P12V_INA230_PWR:
      case MB_SENSOR_C3_P12V_INA230_PWR:
      case MB_SENSOR_C4_P12V_INA230_PWR:
      case MB_SENSOR_CONN_P12V_INA230_PWR:
        ret = read_INA230 (sensor_num, (float*) value, poweron_10s_flag);
        break;
      case MB_SENSOR_POWER_FAIL:
        ret = read_CPLD_power_fail_sts (fru, sensor_num, (float*) value, poweron_10s_flag);
        break;
      case MB_SENSOR_MEMORY_LOOP_FAIL:
        ret = check_postcodes(FRU_MB, sensor_num, (float*) value);
        break;
      case MB_SENSOR_PROCESSOR_FAIL:
        ret = check_frb3(FRU_MB, sensor_num, (float*) value);
        break;

      default:
        return -1;
      }
    }
    if (is_server_off() != server_off) {
      /* server power status changed while we were reading the sensor.
       * this sensor is potentially NA. */
      return pal_sensor_read_raw(fru, sensor_num, value);
    }
    break;
  case FRU_NIC:
    sprintf(key, "nic_sensor%d", sensor_num);
    switch(sensor_num) {
    case MEZZ_SENSOR_TEMP:
      ret = read_nic_temp((float*) value);
      break;
    default:
      return -1;
    }
    break;
  default:
    return -1;
  }

  if (ret) {
    if (ret == READING_NA || ret == -1) {
      strcpy(str, "NA");
    } else {
      return ret;
    }
  } else {
    sprintf(str, "%.2f",*((float*)value));
  }
  if(kv_set(key, str, 0, 0) < 0) {
#ifdef DEBUG
     syslog(LOG_WARNING, "pal_sensor_read_raw: cache_set key = %s, str = %s failed.", key, str);
#endif
    return -1;
  } else {
    return ret;
  }

  return 0;
}

int
pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag) {

  return 0;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  float *val = (float*) value;
  sensor_thresh_array_init();
  switch(fru) {
  case FRU_MB:
    *val = mb_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_NIC:
    *val = nic_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_RISER_SLOT2:
    *val = riser_slot2_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_RISER_SLOT3:
    *val = riser_slot3_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_RISER_SLOT4:
    *val = riser_slot4_sensor_threshold[sensor_num][thresh];
    break;
  default:
    return -1;
  }
  return 0;
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  switch(fru) {
  case FRU_MB:
  case FRU_RISER_SLOT2:
  case FRU_RISER_SLOT3:
  case FRU_RISER_SLOT4:
    switch(sensor_num) {
    case MB_SENSOR_INLET_TEMP:
      sprintf(name, "MB_INLET_TEMP");
      break;
    case MB_SENSOR_OUTLET_TEMP:
      sprintf(name, "MB_OUTLET_TEMP");
      break;
    case MB_SENSOR_INLET_REMOTE_TEMP:
      sprintf(name, "MB_INLET_REMOTE_TEMP");
      break;
    case MB_SENSOR_OUTLET_REMOTE_TEMP:
      sprintf(name, "MB_OUTLET_REMOTE_TEMP");
      break;
    case MB_SENSOR_FAN0_TACH:
      sprintf(name, "MB_FAN0_TACH");
      break;
    case MB_SENSOR_FAN1_TACH:
      sprintf(name, "MB_FAN1_TACH");
      break;
    case MB_SENSOR_P3V3:
      sprintf(name, "MB_P3V3");
      break;
    case MB_SENSOR_P5V:
      sprintf(name, "MB_P5V");
      break;
    case MB_SENSOR_P12V:
      sprintf(name, "MB_P12V");
      break;
    case MB_SENSOR_P1V05:
      sprintf(name, "MB_P1V05");
      break;
    case MB_SENSOR_PVNN_PCH_STBY:
      sprintf(name, "MB_PVNN_PCH_STBY");
      break;
    case MB_SENSOR_P3V3_STBY:
      sprintf(name, "MB_P3V3_STBY");
      break;
    case MB_SENSOR_P5V_STBY:
      sprintf(name, "MB_P5V_STBY");
      break;
    case MB_SENSOR_P3V_BAT:
      sprintf(name, "MB_P3V_BAT");
      break;
    case MB_SENSOR_HSC_IN_VOLT:
      sprintf(name, "MB_HSC_IN_VOLT");
      break;
    case MB_SENSOR_HSC_OUT_CURR:
      sprintf(name, "MB_HSC_OUT_CURR");
      break;
    case MB_SENSOR_HSC_TEMP:
      sprintf(name, "MB_HSC_TEMP");
      break;
    case MB_SENSOR_HSC_IN_POWER:
      sprintf(name, "MB_HSC_IN_POWER");
      break;
    case MB_SENSOR_CPU0_TEMP:
      sprintf(name, "MB_CPU0_TEMP");
      break;
    case MB_SENSOR_CPU0_TJMAX:
      sprintf(name, "MB_CPU0_TJMAX");
      break;
    case MB_SENSOR_CPU0_PKG_POWER:
      sprintf(name, "MB_CPU0_PKG_POWER");
      break;
    case MB_SENSOR_CPU0_THERM_MARGIN:
      sprintf(name, "MB_CPU0_THERM_MARGIN");
      break;
    case MB_SENSOR_CPU1_TEMP:
      sprintf(name, "MB_CPU1_TEMP");
      break;
    case MB_SENSOR_CPU1_TJMAX:
      sprintf(name, "MB_CPU1_TJMAX");
      break;
    case MB_SENSOR_CPU1_PKG_POWER:
      sprintf(name, "MB_CPU1_PKG_POWER");
      break;
    case MB_SENSOR_CPU1_THERM_MARGIN:
      sprintf(name, "MB_CPU1_THERM_MARGIN");
      break;
    case MB_SENSOR_PCH_TEMP:
      sprintf(name, "MB_PCH_TEMP");
      break;
    case MB_SENSOR_CPU0_DIMM_GRPA_TEMP:
      sprintf(name, "MB_CPU0_DIMM_GRPA_TEMP");
      break;
    case MB_SENSOR_CPU0_DIMM_GRPB_TEMP:
      sprintf(name, "MB_CPU0_DIMM_GRPB_TEMP");
      break;
    case MB_SENSOR_CPU1_DIMM_GRPC_TEMP:
      sprintf(name, "MB_CPU1_DIMM_GRPC_TEMP");
      break;
    case MB_SENSOR_CPU1_DIMM_GRPD_TEMP:
      sprintf(name, "MB_CPU1_DIMM_GRPD_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VCCIN_TEMP:
      sprintf(name, "MB_VR_CPU0_VCCIN_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VCCIN_CURR:
      sprintf(name, "MB_VR_CPU0_VCCIN_CURR");
      break;
    case MB_SENSOR_VR_CPU0_VCCIN_VOLT:
      sprintf(name, "MB_VR_CPU0_VCCIN_VOLT");
      break;
    case MB_SENSOR_VR_CPU0_VCCIN_POWER:
      sprintf(name, "MB_VR_CPU0_VCCIN_POWER");
      break;
    case MB_SENSOR_VR_CPU0_VSA_TEMP:
      sprintf(name, "MB_VR_CPU0_VSA_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VSA_CURR:
      sprintf(name, "MB_VR_CPU0_VSA_CURR");
      break;
    case MB_SENSOR_VR_CPU0_VSA_VOLT:
      sprintf(name, "MB_VR_CPU0_VSA_VOLT");
      break;
    case MB_SENSOR_VR_CPU0_VSA_POWER:
      sprintf(name, "MB_VR_CPU0_VSA_POWER");
      break;
    case MB_SENSOR_VR_CPU0_VCCIO_TEMP:
      sprintf(name, "MB_VR_CPU0_VCCIO_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VCCIO_CURR:
      sprintf(name, "MB_VR_CPU0_VCCIO_CURR");
      break;
    case MB_SENSOR_VR_CPU0_VCCIO_VOLT:
      sprintf(name, "MB_VR_CPU0_VCCIO_VOLT");
      break;
    case MB_SENSOR_VR_CPU0_VCCIO_POWER:
      sprintf(name, "MB_VR_CPU0_VCCIO_POWER");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_TEMP:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPA_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_CURR:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPA_CURR");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_VOLT:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPA_VOLT");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_POWER:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPA_POWER");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_TEMP:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPB_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_CURR:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPB_CURR");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_VOLT:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPB_VOLT");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_POWER:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPB_POWER");
      break;
    case MB_SENSOR_VR_CPU1_VCCIN_TEMP:
      sprintf(name, "MB_VR_CPU1_VCCIN_TEMP");
      break;
    case MB_SENSOR_VR_CPU1_VCCIN_CURR:
      sprintf(name, "MB_VR_CPU1_VCCIN_CURR");
      break;
    case MB_SENSOR_VR_CPU1_VCCIN_VOLT:
      sprintf(name, "MB_VR_CPU1_VCCIN_VOLT");
      break;
    case MB_SENSOR_VR_CPU1_VCCIN_POWER:
      sprintf(name, "MB_VR_CPU1_VCCIN_POWER");
      break;
    case MB_SENSOR_VR_CPU1_VSA_TEMP:
      sprintf(name, "MB_VR_CPU1_VSA_TEMP");
      break;
    case MB_SENSOR_VR_CPU1_VSA_CURR:
      sprintf(name, "MB_VR_CPU1_VSA_CURR");
      break;
    case MB_SENSOR_VR_CPU1_VSA_VOLT:
      sprintf(name, "MB_VR_CPU1_VSA_VOLT");
      break;
    case MB_SENSOR_VR_CPU1_VSA_POWER:
      sprintf(name, "MB_VR_CPU1_VSA_POWER");
      break;
    case MB_SENSOR_VR_CPU1_VCCIO_TEMP:
      sprintf(name, "MB_VR_CPU1_VCCIO_TEMP");
      break;
    case MB_SENSOR_VR_CPU1_VCCIO_CURR:
      sprintf(name, "MB_VR_CPU1_VCCIO_CURR");
      break;
    case MB_SENSOR_VR_CPU1_VCCIO_VOLT:
      sprintf(name, "MB_VR_CPU1_VCCIO_VOLT");
      break;
    case MB_SENSOR_VR_CPU1_VCCIO_POWER:
      sprintf(name, "MB_VR_CPU1_VCCIO_POWER");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_TEMP:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPC_TEMP");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_CURR:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPC_CURR");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_VOLT:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPC_VOLT");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_POWER:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPC_POWER");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_TEMP:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPD_TEMP");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_CURR:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPD_CURR");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_VOLT:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPD_VOLT");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_POWER:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPD_POWER");
      break;
    case MB_SENSOR_VR_PCH_PVNN_TEMP:
      sprintf(name, "MB_VR_PCH_PVNN_TEMP");
      break;
    case MB_SENSOR_VR_PCH_PVNN_CURR:
      sprintf(name, "MB_VR_PCH_PVNN_CURR");
      break;
    case MB_SENSOR_VR_PCH_PVNN_VOLT:
      sprintf(name, "MB_VR_PCH_PVNN_VOLT");
      break;
    case MB_SENSOR_VR_PCH_PVNN_POWER:
      sprintf(name, "MB_VR_PCH_PVNN_POWER");
      break;
    case MB_SENSOR_VR_PCH_P1V05_TEMP:
      sprintf(name, "MB_VR_PCH_P1V05_TEMP");
      break;
    case MB_SENSOR_VR_PCH_P1V05_CURR:
      sprintf(name, "MB_VR_PCH_P1V05_CURR");
      break;
    case MB_SENSOR_VR_PCH_P1V05_VOLT:
      sprintf(name, "MB_VR_PCH_P1V05_VOLT");
      break;
    case MB_SENSOR_VR_PCH_P1V05_POWER:
      sprintf(name, "MB_VR_PCH_P1V05_POWER");
      break;
    case MB_SENSOR_HOST_BOOT_TEMP:
      sprintf(name, "MB_HOST_BOOT_TEMP");
      break;
    case MB_SENSOR_C2_NVME_CTEMP:
      sprintf(name, "MB_C2_NVME_CTEMP");
      break;
    case MB_SENSOR_C3_NVME_CTEMP:
      sprintf(name, "MB_C3_NVME_CTEMP");
      break;
    case MB_SENSOR_C4_NVME_CTEMP:
      sprintf(name, "MB_C4_NVME_CTEMP");
      break;
    case MB_SENSOR_C2_AVA_FTEMP:
      sprintf(name, "MB_C2_AVA_FTEMP");
      break;
    case MB_SENSOR_C2_AVA_RTEMP:
      sprintf(name, "MB_C2_AVA_RTEMP");
      break;
    case MB_SENSOR_C2_1_NVME_CTEMP:
      sprintf(name, "MB_C2_0_NVME_CTEMP");
      break;
    case MB_SENSOR_C2_2_NVME_CTEMP:
      sprintf(name, "MB_C2_1_NVME_CTEMP");
      break;
    case MB_SENSOR_C2_3_NVME_CTEMP:
      sprintf(name, "MB_C2_2_NVME_CTEMP");
      break;
    case MB_SENSOR_C2_4_NVME_CTEMP:
      sprintf(name, "MB_C2_3_NVME_CTEMP");
      break;
    case MB_SENSOR_C3_AVA_FTEMP:
      sprintf(name, "MB_C3_AVA_FTEMP");
      break;
    case MB_SENSOR_C3_AVA_RTEMP:
      sprintf(name, "MB_C3_AVA_RTEMP");
      break;
    case MB_SENSOR_C3_1_NVME_CTEMP:
      sprintf(name, "MB_C3_0_NVME_CTEMP");
      break;
    case MB_SENSOR_C3_2_NVME_CTEMP:
      sprintf(name, "MB_C3_1_NVME_CTEMP");
      break;
    case MB_SENSOR_C3_3_NVME_CTEMP:
      sprintf(name, "MB_C3_2_NVME_CTEMP");
      break;
    case MB_SENSOR_C3_4_NVME_CTEMP:
      sprintf(name, "MB_C3_3_NVME_CTEMP");
      break;
    case MB_SENSOR_C4_AVA_FTEMP:
      sprintf(name, "MB_C4_AVA_FTEMP");
      break;
    case MB_SENSOR_C4_AVA_RTEMP:
      sprintf(name, "MB_C4_AVA_RTEMP");
      break;
    case MB_SENSOR_C4_1_NVME_CTEMP:
      sprintf(name, "MB_C4_0_NVME_CTEMP");
      break;
    case MB_SENSOR_C4_2_NVME_CTEMP:
      sprintf(name, "MB_C4_1_NVME_CTEMP");
      break;
    case MB_SENSOR_C4_3_NVME_CTEMP:
      sprintf(name, "MB_C4_2_NVME_CTEMP");
      break;
    case MB_SENSOR_C4_4_NVME_CTEMP:
      sprintf(name, "MB_C4_3_NVME_CTEMP");
      break;
    case MB_SENSOR_C2_P12V_INA230_VOL:
      sprintf(name, "MB_C2_P12V_INA230_VOL");
      break;
    case MB_SENSOR_C2_P12V_INA230_CURR:
      sprintf(name, "MB_C2_P12V_INA230_CURR");
      break;
    case MB_SENSOR_C2_P12V_INA230_PWR:
      sprintf(name, "MB_C2_P12V_INA230_PWR");
      break;
    case MB_SENSOR_C3_P12V_INA230_VOL:
      sprintf(name, "MB_C3_P12V_INA230_VOL");
      break;
    case MB_SENSOR_C3_P12V_INA230_CURR:
      sprintf(name, "MB_C3_P12V_INA230_CURR");
      break;
    case MB_SENSOR_C3_P12V_INA230_PWR:
      sprintf(name, "MB_C3_P12V_INA230_PWR");
      break;
    case MB_SENSOR_C4_P12V_INA230_VOL:
      sprintf(name, "MB_C4_P12V_INA230_VOL");
      break;
    case MB_SENSOR_C4_P12V_INA230_CURR:
      sprintf(name, "MB_C4_P12V_INA230_CURR");
      break;
    case MB_SENSOR_C4_P12V_INA230_PWR:
      sprintf(name, "MB_C4_P12V_INA230_PWR");
      break;
    case MB_SENSOR_CONN_P12V_INA230_VOL:
      sprintf(name, "MB_CONN_P12V_INA230_VOL");
      break;
    case MB_SENSOR_CONN_P12V_INA230_CURR:
      sprintf(name, "MB_CONN_P12V_INA230_CURR");
      break;
    case MB_SENSOR_CONN_P12V_INA230_PWR:
      sprintf(name, "MB_CONN_P12V_INA230_PWR");
      break;
    case MB_SENSOR_POWER_FAIL:
      sprintf(name, "MB_POWER_FAIL");
      break;
    case MB_SENSOR_MEMORY_LOOP_FAIL:
      sprintf(name, "MB_MEMORY_LOOP_FAIL");
      break;
    case MB_SENSOR_PROCESSOR_FAIL:
      sprintf(name, "MB_PROCESSOR_FAIL");
      break;

    default:
      return -1;
    }
    break;
  case FRU_NIC:
    switch(sensor_num) {
    case MEZZ_SENSOR_TEMP:
      sprintf(name, "MEZZ_SENSOR_TEMP");
      break;
    default:
      return -1;
    }
    break;
  default:
    return -1;
  }
  return 0;
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  switch(fru) {
  case FRU_MB:
  case FRU_RISER_SLOT2:
  case FRU_RISER_SLOT3:
  case FRU_RISER_SLOT4:
    switch(sensor_num) {
    case MB_SENSOR_INLET_TEMP:
    case MB_SENSOR_OUTLET_TEMP:
    case MB_SENSOR_INLET_REMOTE_TEMP:
    case MB_SENSOR_OUTLET_REMOTE_TEMP:
    case MB_SENSOR_CPU0_TEMP:
    case MB_SENSOR_CPU0_TJMAX:
    case MB_SENSOR_CPU0_THERM_MARGIN:
    case MB_SENSOR_CPU1_TEMP:
    case MB_SENSOR_CPU1_TJMAX:
    case MB_SENSOR_CPU1_THERM_MARGIN:
    case MB_SENSOR_PCH_TEMP:
    case MB_SENSOR_CPU0_DIMM_GRPA_TEMP:
    case MB_SENSOR_CPU0_DIMM_GRPB_TEMP:
    case MB_SENSOR_CPU1_DIMM_GRPC_TEMP:
    case MB_SENSOR_CPU1_DIMM_GRPD_TEMP:
    case MB_SENSOR_VR_CPU0_VCCIN_TEMP:
    case MB_SENSOR_VR_CPU0_VSA_TEMP:
    case MB_SENSOR_VR_CPU0_VCCIO_TEMP:
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_TEMP:
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_TEMP:
    case MB_SENSOR_VR_CPU1_VCCIN_TEMP:
    case MB_SENSOR_VR_CPU1_VSA_TEMP:
    case MB_SENSOR_VR_CPU1_VCCIO_TEMP:
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_TEMP:
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_TEMP:
    case MB_SENSOR_VR_PCH_PVNN_TEMP:
    case MB_SENSOR_VR_PCH_P1V05_TEMP:
    case MB_SENSOR_HOST_BOOT_TEMP:
    case MB_SENSOR_C2_NVME_CTEMP:
    case MB_SENSOR_C3_NVME_CTEMP:
    case MB_SENSOR_C4_NVME_CTEMP:
    case MB_SENSOR_C2_AVA_FTEMP:
    case MB_SENSOR_C2_AVA_RTEMP:
    case MB_SENSOR_C2_1_NVME_CTEMP:
    case MB_SENSOR_C2_2_NVME_CTEMP:
    case MB_SENSOR_C2_3_NVME_CTEMP:
    case MB_SENSOR_C2_4_NVME_CTEMP:
    case MB_SENSOR_C3_AVA_FTEMP:
    case MB_SENSOR_C3_AVA_RTEMP:
    case MB_SENSOR_C3_1_NVME_CTEMP:
    case MB_SENSOR_C3_2_NVME_CTEMP:
    case MB_SENSOR_C3_3_NVME_CTEMP:
    case MB_SENSOR_C3_4_NVME_CTEMP:
    case MB_SENSOR_C4_AVA_FTEMP:
    case MB_SENSOR_C4_AVA_RTEMP:
    case MB_SENSOR_C4_1_NVME_CTEMP:
    case MB_SENSOR_C4_2_NVME_CTEMP:
    case MB_SENSOR_C4_3_NVME_CTEMP:
    case MB_SENSOR_C4_4_NVME_CTEMP:
    case MB_SENSOR_HSC_TEMP:
      sprintf(units, "C");
      break;
    case MB_SENSOR_FAN0_TACH:
    case MB_SENSOR_FAN1_TACH:
      sprintf(units, "RPM");
      break;
    case MB_SENSOR_P3V3:
    case MB_SENSOR_P5V:
    case MB_SENSOR_P12V:
    case MB_SENSOR_P1V05:
    case MB_SENSOR_PVNN_PCH_STBY:
    case MB_SENSOR_P3V3_STBY:
    case MB_SENSOR_P5V_STBY:
    case MB_SENSOR_P3V_BAT:
    case MB_SENSOR_HSC_IN_VOLT:
    case MB_SENSOR_VR_CPU0_VCCIN_VOLT:
    case MB_SENSOR_VR_CPU0_VSA_VOLT:
    case MB_SENSOR_VR_CPU0_VCCIO_VOLT:
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_VOLT:
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_VOLT:
    case MB_SENSOR_VR_CPU1_VCCIN_VOLT:
    case MB_SENSOR_VR_CPU1_VSA_VOLT:
    case MB_SENSOR_VR_CPU1_VCCIO_VOLT:
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_VOLT:
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_VOLT:
    case MB_SENSOR_VR_PCH_PVNN_VOLT:
    case MB_SENSOR_VR_PCH_P1V05_VOLT:
    case MB_SENSOR_C2_P12V_INA230_VOL:
    case MB_SENSOR_C3_P12V_INA230_VOL:
    case MB_SENSOR_C4_P12V_INA230_VOL:
    case MB_SENSOR_CONN_P12V_INA230_VOL:
      sprintf(units, "Volts");
      break;
    case MB_SENSOR_HSC_OUT_CURR:
    case MB_SENSOR_VR_CPU0_VCCIN_CURR:
    case MB_SENSOR_VR_CPU0_VSA_CURR:
    case MB_SENSOR_VR_CPU0_VCCIO_CURR:
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_CURR:
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_CURR:
    case MB_SENSOR_VR_CPU1_VCCIN_CURR:
    case MB_SENSOR_VR_CPU1_VSA_CURR:
    case MB_SENSOR_VR_CPU1_VCCIO_CURR:
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_CURR:
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_CURR:
    case MB_SENSOR_VR_PCH_PVNN_CURR:
    case MB_SENSOR_VR_PCH_P1V05_CURR:
    case MB_SENSOR_C2_P12V_INA230_CURR:
    case MB_SENSOR_C3_P12V_INA230_CURR:
    case MB_SENSOR_C4_P12V_INA230_CURR:
    case MB_SENSOR_CONN_P12V_INA230_CURR:
      sprintf(units, "Amps");
      break;
    case MB_SENSOR_HSC_IN_POWER:
    case MB_SENSOR_VR_CPU0_VCCIN_POWER:
    case MB_SENSOR_VR_CPU0_VSA_POWER:
    case MB_SENSOR_VR_CPU0_VCCIO_POWER:
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_POWER:
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_POWER:
    case MB_SENSOR_VR_CPU1_VCCIN_POWER:
    case MB_SENSOR_VR_CPU1_VSA_POWER:
    case MB_SENSOR_VR_CPU1_VCCIO_POWER:
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_POWER:
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_POWER:
    case MB_SENSOR_VR_PCH_PVNN_POWER:
    case MB_SENSOR_VR_PCH_P1V05_POWER:
    case MB_SENSOR_CPU0_PKG_POWER:
    case MB_SENSOR_CPU1_PKG_POWER:
    case MB_SENSOR_C2_P12V_INA230_PWR:
    case MB_SENSOR_C3_P12V_INA230_PWR:
    case MB_SENSOR_C4_P12V_INA230_PWR:
    case MB_SENSOR_CONN_P12V_INA230_PWR:
      sprintf(units, "Watts");
      break;
    default:
      return -1;
    }
    break;
  case FRU_NIC:
    switch(sensor_num) {
    case MEZZ_SENSOR_TEMP:
      sprintf(units, "C");
      break;
    default:
      return -1;
    }
    break;
  default:
    return -1;
  }
  return 0;
}

int
pal_get_fruid_path(uint8_t fru, char *path) {
  char fname[16] = {0};

  switch(fru) {
  case FRU_MB:
    sprintf(fname, "mb");
    break;
  case FRU_NIC:
    sprintf(fname, "nic");
    break;
  case FRU_RISER_SLOT2:
    sprintf(fname, "riser_slot2");
    break;
  case FRU_RISER_SLOT3:
    sprintf(fname, "riser_slot3");
    break;
  case FRU_RISER_SLOT4:
    sprintf(fname, "riser_slot4");
    break;
  default:
    return -1;
  }

  sprintf(path, "/tmp/fruid_%s.bin", fname);

  return 0;
}

int
pal_get_fruid_eeprom_path(uint8_t fru, char *path) {
  switch(fru) {
  case FRU_MB:
    sprintf(path, FRU_EEPROM);
    break;
  default:
    return -1;
  }

  return 0;
}

int
pal_get_fruid_name(uint8_t fru, char *name) {
  switch(fru) {
  case FRU_MB:
    sprintf(name, "Mother Board");
    break;
  case FRU_NIC:
    sprintf(name, "NIC Mezzanine");
    break;
  case FRU_RISER_SLOT2:
    sprintf(name, "FRU content on the riser slot 2");
    break;
  case FRU_RISER_SLOT3:
    sprintf(name, "FRU content on the riser slot 3");
    break;
  case FRU_RISER_SLOT4:
    sprintf(name, "FRU content on the riser slot 4");
    break;
  default:
    return -1;
  }
  return 0;
}

int
pal_set_def_key_value() {

  int i;
  char key[MAX_KEY_LEN] = {0};

  for(i = 0; strcmp(key_cfg[i].name, LAST_KEY) != 0; i++) {
    if (kv_set(key_cfg[i].name, key_cfg[i].def_val, 0, KV_FCREATE | KV_FPERSIST)) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pal_set_def_key_value: kv_set failed.");
#endif
    }
    if (key_cfg[i].function) {
      key_cfg[i].function(KEY_AFTER_INI, key_cfg[i].name);
    }
  }

  /* Actions to be taken on Power On Reset */
  if (pal_is_bmc_por()) {
    /* Clear all the SEL errors */
    memset(key, 0, MAX_KEY_LEN);
    strcpy(key, "server_sel_error");

    /* Write the value "1" which means FRU_STATUS_GOOD */
    pal_set_key_value(key, "1");

    /* Clear all the sensor health files*/
    memset(key, 0, MAX_KEY_LEN);
    strcpy(key, "server_sensor_health");

    /* Write the value "1" which means FRU_STATUS_GOOD */
    pal_set_key_value(key, "1");
  }

  return 0;
}

int
pal_get_fru_devtty(uint8_t fru, char *devtty) {
  sprintf(devtty, "/dev/ttyS1");
  return 0;
}

void
pal_dump_key_value(void) {
  int ret;
  int i = 0;
  char value[MAX_VALUE_LEN] = {0x0};

  while (strcmp(key_cfg[i].name, LAST_KEY)) {
    printf("%s:", key_cfg[i].name);
    if ((ret = kv_get(key_cfg[i].name, value, NULL, KV_FPERSIST)) < 0) {
      printf("\n");
    } else {
      printf("%s\n",  value);
    }
    i++;
    memset(value, 0, MAX_VALUE_LEN);
  }
}

int
pal_set_last_pwr_state(uint8_t fru, char *state) {

  int ret;
  char key[MAX_KEY_LEN] = {0};

  sprintf(key, "%s", "pwr_server_last_state");

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

  sprintf(key, "%s", "pwr_server_last_state");

  ret = pal_get_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_get_last_pwr_state: pal_get_key_value failed for "
        "fru %u", fru);
#endif
  }

  return ret;
}

// GUID for System and Device
static int
pal_get_guid(uint16_t offset, char *guid) {
  int fd = 0;
  ssize_t bytes_rd;

  errno = 0;

  // Check if file is present
  if (access(FRU_EEPROM, F_OK) == -1) {
      syslog(LOG_ERR, "pal_get_guid: unable to access the %s file: %s",
          FRU_EEPROM, strerror(errno));
      return errno;
  }

  // Open the file
  fd = open(FRU_EEPROM, O_RDONLY);
  if (fd == -1) {
    syslog(LOG_ERR, "pal_get_guid: unable to open the %s file: %s",
        FRU_EEPROM, strerror(errno));
    return errno;
  }

  // seek to the offset
  lseek(fd, offset, SEEK_SET);

  // Read bytes from location
  bytes_rd = read(fd, guid, GUID_SIZE);
  if (bytes_rd != GUID_SIZE) {
    syslog(LOG_ERR, "pal_get_guid: read to %s file failed: %s",
        FRU_EEPROM, strerror(errno));
    goto err_exit;
  }

err_exit:
  close(fd);
  return errno;
}

static int
pal_set_guid(uint16_t offset, char *guid) {
  int fd = 0;
  ssize_t bytes_wr;

  errno = 0;

  // Check for file presence
  if (access(FRU_EEPROM, F_OK) == -1) {
      syslog(LOG_ERR, "pal_set_guid: unable to access the %s file: %s",
          FRU_EEPROM, strerror(errno));
      return errno;
  }

  // Open file
  fd = open(FRU_EEPROM, O_WRONLY);
  if (fd == -1) {
    syslog(LOG_ERR, "pal_set_guid: unable to open the %s file: %s",
        FRU_EEPROM, strerror(errno));
    return errno;
  }

  // Seek the offset
  lseek(fd, offset, SEEK_SET);

  // Write GUID data
  bytes_wr = write(fd, guid, GUID_SIZE);
  if (bytes_wr != GUID_SIZE) {
    syslog(LOG_ERR, "pal_set_guid: write to %s file failed: %s",
        FRU_EEPROM, strerror(errno));
    goto err_exit;
  }

err_exit:
  close(fd);
  return errno;
}

// GUID based on RFC4122 format @ https://tools.ietf.org/html/rfc4122
static void
pal_populate_guid(char *guid, char *str) {
  unsigned int secs;
  unsigned int usecs;
  struct timeval tv;
  uint8_t count;
  uint8_t lsb, msb;
  int i, r;

  // Populate time
  gettimeofday(&tv, NULL);

  secs = tv.tv_sec;
  usecs = tv.tv_usec;
  guid[0] = usecs & 0xFF;
  guid[1] = (usecs >> 8) & 0xFF;
  guid[2] = (usecs >> 16) & 0xFF;
  guid[3] = (usecs >> 24) & 0xFF;
  guid[4] = secs & 0xFF;
  guid[5] = (secs >> 8) & 0xFF;
  guid[6] = (secs >> 16) & 0xFF;
  guid[7] = (secs >> 24) & 0x0F;

  // Populate version
  guid[7] |= 0x10;

  // Populate clock seq with randmom number
  //getrandom(&guid[8], 2, 0);
  srand(time(NULL));
  //memcpy(&guid[8], rand(), 2);
  r = rand();
  guid[8] = r & 0xFF;
  guid[9] = (r>>8) & 0xFF;

  // Use string to populate 6 bytes unique
  // e.g. LSP62100035 => 'S' 'P' 0x62 0x10 0x00 0x35
  count = 0;
  for (i = strlen(str)-1; i >= 0; i--) {
    if (count == 6) {
      break;
    }

    // If alphabet use the character as is
    if (isalpha(str[i])) {
      guid[15-count] = str[i];
      count++;
      continue;
    }

    // If it is 0-9, use two numbers as BCD
    lsb = str[i] - '0';
    if (i > 0) {
      i--;
      if (isalpha(str[i])) {
        i++;
        msb = 0;
      } else {
        msb = str[i] - '0';
      }
    } else {
      msb = 0;
    }
    guid[15-count] = (msb << 4) | lsb;
    count++;
  }

  // zero the remaining bytes, if any
  if (count != 6) {
    memset(&guid[10], 0, 6-count);
  }

}

int
pal_set_sys_guid(uint8_t fru, char *str) {
  pal_populate_guid(g_sys_guid, str);

  return pal_set_guid(OFFSET_SYS_GUID, g_sys_guid);
}

int
pal_set_dev_guid(uint8_t fru, char *str) {
  pal_populate_guid(g_dev_guid, str);

  return pal_set_guid(OFFSET_DEV_GUID, g_dev_guid);
}

int
pal_get_sys_guid(uint8_t fru, char *guid) {
  pal_get_guid(OFFSET_SYS_GUID, g_sys_guid);
  memcpy(guid, g_sys_guid, GUID_SIZE);

  return 0;
}

int
pal_get_dev_guid(uint8_t fru, char *guid) {

  pal_get_guid(OFFSET_DEV_GUID, g_dev_guid);

  memcpy(guid, g_dev_guid, GUID_SIZE);

  return 0;
}

void
pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len) {

   char str_server_por_cfg[64];
   char buff[MAX_VALUE_LEN];
   int policy = 3;
   unsigned char *data = res_data;

   // Platform Power Policy
   memset(str_server_por_cfg, 0 , sizeof(char) * 64);
   sprintf(str_server_por_cfg, "%s", "server_por_cfg");

   if (pal_get_key_value(str_server_por_cfg, buff) == 0)
   {
     if (!memcmp(buff, "off", strlen("off")))
       policy = 0;
     else if (!memcmp(buff, "lps", strlen("lps")))
       policy = 1;
     else if (!memcmp(buff, "on", strlen("on")))
       policy = 2;
     else
       policy = 3;
   }
   *data++ = ((is_server_off())?0x00:0x01) | (policy << 5);
   *data++ = 0x00;   // Last Power Event
   *data++ = 0x40;   // Misc. Chassis Status
   *data++ = 0x00;   // Front Panel Button Disable
   *res_len = data - res_data;
}

int
pal_set_sysfw_ver(uint8_t fru, uint8_t *ver) {
  int i;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10] = {0};

  sprintf(key, "sysfw_ver_server");

  for (i = 0; i < SIZE_SYSFW_VER; i++) {
    sprintf(tstr, "%02x", ver[i]);
    strcat(str, tstr);
  }

  return pal_set_key_value(key, str);
}

int
pal_get_sysfw_ver(uint8_t fru, uint8_t *ver) {
  int i;
  int j = 0;
  int ret;
  int msb, lsb;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4] = {0};

  sprintf(key, "sysfw_ver_server");

  ret = pal_get_key_value(key, str);
  if (ret) {
    return ret;
  }

  for (i = 0; i < 2*SIZE_SYSFW_VER; i += 2) {
    sprintf(tstr, "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    sprintf(tstr, "%c\n", str[i+1]);
    lsb = strtol(tstr, NULL, 16);
    ver[j++] = (msb << 4) | lsb;
  }

  return 0;
}

int
pal_set_boot_order(uint8_t slot, uint8_t *boot, uint8_t *res_data, uint8_t *res_len) {
  int i, j, network_dev = 0;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10] = {0};
  *res_len = 0;
  sprintf(key, "server_boot_order");

  for (i = 0; i < SIZE_BOOT_ORDER; i++) {
    //Byte 0 is boot mode, Byte 1~5 is boot order
    if ((i > 0) && (boot[i] != 0xFF)) {
      for (j = i+1; j < SIZE_BOOT_ORDER; j++) {
        if ( boot[i] == boot[j])
          return CC_INVALID_PARAM;
      }

      //If Bit 2:0 is 001b (Network), Bit3 is IPv4/IPv6 order
      //Bit3=0b: IPv4 first
      //Bit3=1b: IPv6 first
      if ( boot[i] == BOOT_DEVICE_IPV4 || boot[i] == BOOT_DEVICE_IPV6)
        network_dev++;
    }

    snprintf(tstr, 3, "%02x", boot[i]);
    strncat(str, tstr, 3);
  }

  //Not allow having more than 1 network boot device in the boot order.
  if (network_dev > 1)
    return CC_INVALID_PARAM;

  return pal_set_key_value(key, str);
}

int
pal_get_boot_order(uint8_t slot, uint8_t *req_data, uint8_t *boot, uint8_t *res_len) {
  int i;
  int j = 0;
  int ret;
  int msb, lsb;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4] = {0};

  sprintf(key, "server_boot_order");

  ret = pal_get_key_value(key, str);
  if (ret) {
    *res_len = 0;
    return ret;
  }

  for (i = 0; i < 2*SIZE_BOOT_ORDER; i += 2) {
    sprintf(tstr, "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    sprintf(tstr, "%c\n", str[i+1]);
    lsb = strtol(tstr, NULL, 16);
    boot[j++] = (msb << 4) | lsb;
  }
  *res_len = SIZE_BOOT_ORDER;
  return 0;
}

int
pal_is_bmc_por(void) {
  FILE *fp;
  int por = 0;

  fp = fopen("/tmp/ast_por", "r");
  if (fp != NULL) {
    if (fscanf(fp, "%d", &por) != 1) {
      return 0;
    }
    fclose(fp);
  }

  return (por)?1:0;
}

int
pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  switch(fru) {
  case FRU_MB:
    *sensor_list = (uint8_t *) mb_discrete_sensor_list;
    *cnt = mb_discrete_sensor_cnt;
    break;
  default:
    if (fru > MAX_NUM_FRUS)
      return -1;
    // Nothing to read yet.
    *sensor_list = NULL;
    *cnt = 0;
  }
    return 0;
}

static void
_print_sensor_discrete_log(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t val, char *event) {
  if (val) {
    syslog(LOG_CRIT, "ASSERT: %s discrete - raised - FRU: %d, num: 0x%X,"
        " snr: %-16s val: 0x%X", event, fru, snr_num, snr_name, val);
  } else {
    syslog(LOG_CRIT, "DEASSERT: %s discrete - settled - FRU: %d, num: 0x%X,"
        " snr: %-16s val: 0x%X", event, fru, snr_num, snr_name, val);
  }
  pal_update_ts_sled();
}

int
pal_sensor_discrete_check(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t o_val, uint8_t n_val) {

  return 0;
}

#define MAX_DUMP_TIME 10800 /* 3 hours */
static pthread_t tid_dwr = -1;
static void *dwr_handler(void *arg) {
#if 0
  int len;
  ipmb_req_t *req;
  ipmb_res_t *res;

  //delay 30s for system reset
  sleep(30);

  // Get biosscratchpad7[26]: DWR assert check
  req = ipmb_txb();
  res = ipmb_rxb();
  req->res_slave_addr = 0x2C; //ME's Slave Address
  req->netfn_lun = NETFN_NM_REQ<<2;
  req->cmd = CMD_NM_SEND_RAW_PECI;
  req->data[0] = 0x57;
  req->data[1] = 0x01;
  req->data[2] = 0x00;
  req->data[3] = 0x30; // CPU0
  req->data[4] = 0x06;
  req->data[5] = 0x05;
  req->data[6] = 0x61; // RdPCIConfig
  req->data[7] = 0x00; // Index
  // Bus 0 Device 8 Fun 2, offset BCh
  req->data[8] = 0xbc;
  req->data[9] = 0x20;
  req->data[10] = 0x04;
  req->data[11] = 0x00;
  // Invoke IPMB library handler
  len = ipmb_send_buf(0x4, 12+MIN_IPMB_REQ_LEN);

  if (len >= (8+MIN_IPMB_RES_LEN) && // Data len >= 8
    res->cc == 0 && // IPMB Success
    res->data[3] == 0x40 && // PECI Success
    (res->data[7] & 0x04) == 0x04) { // DWR mode
    // System is in DWR mode
    syslog(LOG_WARNING, "Start DWR Autodump");
    if (system("/usr/local/bin/autodump.sh --dwr &") != 0) {
      syslog(LOG_ERR, "DWR autodump failed!\n");
    }
  } else {
    syslog(LOG_WARNING, "Start Second Autodump");
    if (system("/usr/local/bin/autodump.sh --second &") != 0) {
      syslog(LOG_ERR, "Second autodump failed!\n");
    }
  }
#endif
  syslog(LOG_WARNING, "Start Second/DWR Autodump");
  if (system("/usr/local/bin/autodump.sh --second &") != 0) {
    syslog(LOG_ERR, "Autodump.sh --second failed!\n");
  }

  tid_dwr = -1;
  pthread_exit(NULL);
}

void
pal_second_crashdump_chk(void) {
    int fd;
    size_t len;

    if (tid_dwr != -1)
      pthread_cancel(tid_dwr);

    if (pthread_create(&tid_dwr, NULL, dwr_handler, NULL) == 0) {
      memset(postcodes_last, 0, sizeof(postcodes_last));
      pal_get_80port_record(FRU_MB, postcodes_last, sizeof(postcodes_last), &len);

      fd =  creat("/tmp/DWR", 0644);
      if (fd)
        close(fd);
      pthread_detach(tid_dwr);
    }
    else
      tid_dwr = -1;
}

int
pal_sel_handler(uint8_t fru, uint8_t snr_num, uint8_t *event_data) {

  return 0;
}

static int
pal_get_sensor_health_key(uint8_t fru, char *key)
{
  switch (fru) {
    case FRU_MB:
      sprintf(key, "server_sensor_health");
      break;
    case FRU_NIC:
      sprintf(key, "nic_sensor_health");
      break;
    default:
      return -1;
  }
  return 0;
}

int
pal_set_sensor_health(uint8_t fru, uint8_t value) {

  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};

  if (pal_get_sensor_health_key(fru, key))
    return -1;

  sprintf(cvalue, (value > 0) ? "1": "0");

  return pal_set_key_value(key, cvalue);
}

int
pal_get_fru_health(uint8_t fru, uint8_t *value) {

  char cvalue[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  int ret;

  if (pal_get_sensor_health_key(fru, key)) {
    return ERR_NOT_READY;
  }

  ret = pal_get_key_value(key, cvalue);
  if (ret) {
    syslog(LOG_INFO, "pal_get_fru_health(%d): getting value for %s failed\n", fru, key);
    return ret;
  }

  *value = atoi(cvalue);

  if (fru != FRU_MB)
    return 0;

  // If MB, get SEL error status.
  sprintf(key, "server_sel_error");
  memset(cvalue, 0, MAX_VALUE_LEN);

  ret = pal_get_key_value(key, cvalue);
  if (ret) {
    syslog(LOG_INFO, "pal_get_fru_health(%d): getting value for %s failed\n", fru, key);
    return ret;
  }

  *value = *value & atoi(cvalue);
  return 0;
}

void
pal_inform_bic_mode(uint8_t fru, uint8_t mode) {
}

int
pal_get_fan_name(uint8_t num, char *name) {

  switch(num) {

    case FAN_0:
      sprintf(name, "Fan 0");
      break;

    case FAN_1:
      sprintf(name, "Fan 1");
      break;

    default:
      return -1;
  }

  return 0;
}


int
pal_set_fan_speed(uint8_t fan, uint8_t pwm) {
  int ret = -1;

  if (fan >= pal_pwm_cnt) {
    syslog(LOG_INFO, "pal_set_fan_speed: fan number is invalid - %d", fan);
    return -1;
  }

  // Do not allow setting fan when server is off.
  if (is_server_off()) {
    return PAL_ENOTREADY;
  }

  if (fan == 0) {
    ret = sensors_write_fan("pwm1", (float)pwm);
  } else if (fan == 1) {
    ret = sensors_write_fan("pwm2", (float)pwm);
  }
  return ret;
}

int
pal_get_fan_speed(uint8_t fan, int *rpm) {
  int ret;
  float value = 0.0;
  
  if (fan == 0) {
    ret = sensors_read_fan("fan1", &value);
  } else if (fan == 1) {
    ret = sensors_read_fan("fan3", &value);
  } else {
    syslog(LOG_INFO, "get_fan_speed: invalid fan#:%d", fan);
    return -1;
  }
  *rpm = (int)value;
  return ret;
}

void
pal_update_ts_sled()
{
  char key[MAX_KEY_LEN] = {0};
  char tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts);
  sprintf(tstr, "%ld", ts.tv_sec);

  sprintf(key, "timestamp_sled");

  pal_set_key_value(key, tstr);
}

int
pal_handle_dcmi(uint8_t fru, uint8_t *request, uint8_t req_len, uint8_t *response, uint8_t *rlen) {
  return me_xmit(request, req_len, response, rlen);
}

int
pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  int ret;
  uint8_t platform_id  = 0x00;
  uint8_t board_rev_id = 0x00;
  uint8_t mb_slot_id = 0x00;
  uint8_t raiser_card_slot_id = 0x00;
  int completion_code=CC_UNSPECIFIED_ERROR;

  ret = pal_get_platform_id(&platform_id);
  if (ret) {
    *res_len = 0x00;
    return completion_code;
  }

  ret = pal_get_board_rev_id(&board_rev_id);
  if (ret) {
    *res_len = 0x00;
    return completion_code;
  }

  ret = pal_get_mb_slot_id(&mb_slot_id);
  if (ret) {
    *res_len = 0x00;
    return completion_code;
  }

  ret = pal_get_slot_cfg_id(&raiser_card_slot_id);
  if (ret) {
    *res_len = 0x00;
    return completion_code;
  }

  // Prepare response buffer
  completion_code = CC_SUCCESS;
  res_data[0] = platform_id;
  res_data[1] = board_rev_id;
  res_data[2] = mb_slot_id;
  res_data[3] = raiser_card_slot_id;
  *res_len = 0x04;

  return completion_code;
}

static int
get_gpio_shadow_array(const char **shadows, int num, uint8_t *mask)
{
  int i;
  *mask = 0;
  for (i = 0; i < num; i++) {
    int ret;
    gpio_value_t value;
    gpio_desc_t *gpio = gpio_open_by_shadow(shadows[i]);
    if (!gpio) {
      return -1;
    }
    ret = gpio_get_value(gpio, &value);
    gpio_close(gpio);
    if (ret != 0) {
      return -1;
    }
    *mask |= (value == GPIO_VALUE_HIGH ? 1 : 0) << i;
  }
  return 0;
}

int
pal_get_platform_id(uint8_t *id) {
  static bool cached = false;
  static uint8_t cached_id = 0;

  if (!cached) {
    const char *shadows[] = {
      "FM_BOARD_SKU_ID0",
      "FM_BOARD_SKU_ID1",
      "FM_BOARD_SKU_ID2",
      "FM_BOARD_SKU_ID3",
      "FM_BOARD_SKU_ID4"
    };
    if (get_gpio_shadow_array(shadows, ARRAY_SIZE(shadows), &cached_id)) {
      return -1;
    }
    cached = true;
  }
  *id = cached_id;
  return 0;
}

int
pal_get_board_rev_id(uint8_t *id) {
  static bool cached = false;
  static uint8_t cached_id = 0;

  if (!cached) {
    const char *shadows[] = {
      "FM_BOARD_REV_ID0",
      "FM_BOARD_REV_ID1",
      "FM_BOARD_REV_ID2"
    };
    if (get_gpio_shadow_array(shadows, ARRAY_SIZE(shadows), &cached_id)) {
      return -1;
    }
    cached = true;
  }
  *id = cached_id;
  return 0;
}

int
pal_get_mb_slot_id(uint8_t *id) {
  int fd = 0, ret = -1;
  char fn[32];
  uint8_t retry = 3, tcount, rcount, addr;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", CPLD_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    goto err_exit;
  }

  //PCA9554 slave address
  addr = 0x42;
  //0x00: input register
  tbuf[0] = 0x00;
  tcount = 1;
  rcount = 1;

  while (ret < 0 && retry-- > 0 ) {
    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tcount, rbuf, rcount);
  }
  if (ret < 0) {
    goto err_exit;
  }

  *id = rbuf[0] & 0x07;

  err_exit:
    if (fd > 0)
      close(fd);

    return ret;
}


int
pal_get_slot_cfg_id(uint8_t *id) {
  static bool cached = false;
  static uint8_t cached_id = 0;

  if (!cached) {
    const char *shadows[] = {
      "FM_BOARD_SKU_ID5",
      "FM_BOARD_SKU_ID6"
    };
    if (get_gpio_shadow_array(shadows, ARRAY_SIZE(shadows), &cached_id)) {
      return -1;
    }
    cached = true;
  }
  *id = cached_id;
  return 0;
}

void
pal_log_clear(char *fru) {
  if (!strcmp(fru, "mb")) {
    pal_set_key_value("server_sensor_health", "1");
    pal_set_key_value("server_sel_error", "1");
  } else if (!strcmp(fru, "nic")) {
    pal_set_key_value("nic_sensor_health", "1");
  } else if (!strcmp(fru, "all")) {
    pal_set_key_value("server_sensor_health", "1");
    pal_set_key_value("server_sel_error", "1");
    pal_set_key_value("nic_sensor_health", "1");
  }
}

//For OEM command "CMD_OEM_GET_PLAT_INFO" 0x7e
int
pal_get_plat_sku_id(void) {

  return 0;
}

int
pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {

  return 0;
}

int
pal_get_pwm_value(uint8_t fan_num, uint8_t *value) {
  int ret;
  float val;
  if (fan_num == 0) {
    ret = sensors_read_fan("pwm1", &val);
  } else if (fan_num == 1) {
    ret = sensors_read_fan("pwm2", &val);
  } else {
    return -1;
  }
  if (!ret)
    *value = (uint8_t)val;
  return ret;
}

int
pal_fan_dead_handle(int fan_num) {

  // TODO: Add action in case of fan dead
  return 0;
}

int
pal_fan_recovered_handle(int fan_num) {

  // TODO: Add action in case of fan recovered
  return 0;
}

static bool
is_cpu_socket_occupy(unsigned int cpu_idx) {
  static bool cached = false;
  static uint8_t cached_id = 0;

  if (!cached) {
    const char *shadows[] = {
      "FM_CPU0_SKTOCC_LVT3_N",
      "FM_CPU1_SKTOCC_LVT3_N"
    };
    if (get_gpio_shadow_array(shadows, ARRAY_SIZE(shadows), &cached_id)) {
      return false;
    }
    cached = true;
  }

  // bit == 1 implies CPU is absent.
  if (cached_id & (1 << cpu_idx)) {
    return false;
  }
  return true;
}

void
pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh) {
  char cmd[128];
  char thresh_name[10];

  switch (thresh) {
    case UNR_THRESH:
        sprintf(thresh_name, "UNR");
      break;
    case UCR_THRESH:
        sprintf(thresh_name, "UCR");
      break;
    case UNC_THRESH:
        sprintf(thresh_name, "UNCR");
      break;
    case LNR_THRESH:
        sprintf(thresh_name, "LNR");
      break;
    case LCR_THRESH:
        sprintf(thresh_name, "LCR");
      break;
    case LNC_THRESH:
        sprintf(thresh_name, "LNCR");
      break;
    default:
      syslog(LOG_WARNING, "pal_sensor_assert_handle: wrong thresh enum value");
      exit(-1);
  }

  switch(snr_num) {
    case MB_SENSOR_FAN0_TACH:
      sprintf(cmd, "Fan0 %s %.0fRPM - Assert", thresh_name, val);
      break;
    case MB_SENSOR_FAN1_TACH:
      sprintf(cmd, "Fan1 %s %.0fRPM - Assert", thresh_name, val);
      break;
    case MB_SENSOR_CPU0_TEMP:
      sprintf(cmd, "P0 Temp %s %.0fC - Assert", thresh_name, val);
      break;
    case MB_SENSOR_CPU1_TEMP:
      sprintf(cmd, "P1 Temp %s %.0fC - Assert", thresh_name, val);
      break;
    case MB_SENSOR_P3V_BAT:
      sprintf(cmd, "P3V_BAT %s %.2fV - Assert", thresh_name, val);
      break;
    case MB_SENSOR_P3V3:
      sprintf(cmd, "P3V3 %s %.2fV - Assert", thresh_name, val);
      break;
    case MB_SENSOR_P5V:
      sprintf(cmd, "P5V %s %.2fV - Assert", thresh_name, val);
      break;
    case MB_SENSOR_P12V:
      sprintf(cmd, "P12V %s %.2fV - Assert", thresh_name, val);
      break;
    case MB_SENSOR_P1V05:
      sprintf(cmd, "P1V05 %s %.2fV - Assert", thresh_name, val);
      break;
    case MB_SENSOR_PVNN_PCH_STBY:
      sprintf(cmd, "PVNN_PCH_STBY %s %.2fV - Assert", thresh_name, val);
      break;
    case MB_SENSOR_P3V3_STBY:
      sprintf(cmd, "P3V3_STBY %s %.2fV - Assert", thresh_name, val);
      break;
    case MB_SENSOR_P5V_STBY:
      sprintf(cmd, "P5V_STBY %s %.2fV - Assert", thresh_name, val);
      break;
    default:
      return;
  }
  pal_add_cri_sel(cmd);

}

void
pal_sensor_deassert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh) {
  char cmd[128];
  char thresh_name[10];

  switch (thresh) {
    case UNR_THRESH:
        sprintf(thresh_name, "UNR");
      break;
    case UCR_THRESH:
        sprintf(thresh_name, "UCR");
      break;
    case UNC_THRESH:
        sprintf(thresh_name, "UNCR");
      break;
    case LNR_THRESH:
        sprintf(thresh_name, "LNR");
      break;
    case LCR_THRESH:
        sprintf(thresh_name, "LCR");
      break;
    case LNC_THRESH:
        sprintf(thresh_name, "LNCR");
      break;
    default:
      syslog(LOG_WARNING, "pal_sensor_assert_handle: wrong thresh enum value");
      exit(-1);
  }

  switch(snr_num) {
    case MB_SENSOR_FAN0_TACH:
      sprintf(cmd, "Fan0 %s %3.0fRPM - Deassert", thresh_name, val);
      break;
    case MB_SENSOR_FAN1_TACH:
      sprintf(cmd, "Fan1 %s %3.0fRPM - Deassert", thresh_name, val);
      break;
    case MB_SENSOR_CPU0_TEMP:
      sprintf(cmd, "P0 Temp %s %3.0fC - Deassert", thresh_name, val);
      break;
    case MB_SENSOR_CPU1_TEMP:
      sprintf(cmd, "P1 Temp %s %3.0fC - Deassert", thresh_name, val);
      break;
    case MB_SENSOR_P3V_BAT:
      sprintf(cmd, "P3V_BAT %s %3.0fV - Deassert", thresh_name, val);
      break;
    case MB_SENSOR_P3V3:
      sprintf(cmd, "P3V3 %s %3.0fV - Deassert", thresh_name, val);
      break;
    case MB_SENSOR_P5V:
      sprintf(cmd, "P5V %s %3.0fV - Deassert", thresh_name, val);
      break;
    case MB_SENSOR_P12V:
      sprintf(cmd, "P12V %s %3.0fV - Deassert", thresh_name, val);
      break;
    case MB_SENSOR_P1V05:
      sprintf(cmd, "P1V05 %s %3.0fV - Deassert", thresh_name, val);
      break;
    case MB_SENSOR_PVNN_PCH_STBY:
      sprintf(cmd, "PVNN_PCH_STBY %s %3.0fV - Deassert", thresh_name, val);
      break;
    case MB_SENSOR_P3V3_STBY:
      sprintf(cmd, "P3V3_STBY %s %3.0fV - Deassert", thresh_name, val);
      break;
    case MB_SENSOR_P5V_STBY:
      sprintf(cmd, "P5V_STBY %s %3.0fV - Deassert", thresh_name, val);
      break;

    default:
      return;
  }
  pal_add_cri_sel(cmd);

}

void
pal_post_end_chk(uint8_t *post_end_chk) {
  static uint8_t post_end = 1;

  if (*post_end_chk == 1) {
    post_end = 1;
  } else if (*post_end_chk == 0) {
    *post_end_chk = post_end;
    post_end = 0;
  }
}

void
pal_set_post_end(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len)
{
  uint8_t post_end = 1;

  *res_len = 0;

  //Set post end chk flag to update LCD info page
  pal_post_end_chk(&post_end);

  // log the post end event
  syslog (LOG_INFO, "POST End Event for Payload#%d\n", slot);

  // Sync time with system
  if (system("/usr/local/bin/sync_date.sh &") != 0) {
    syslog(LOG_ERR, "Sync date failed!\n");
  }
}

int
pal_get_fw_info(uint8_t fru, unsigned char target, unsigned char* res, unsigned char* res_len)
{
  return -1;
}

int
pal_get_last_boot_time(uint8_t slot, uint8_t *last_boot_time) {
  return 0;
}

uint8_t
pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy, uint8_t *res_data) {

  uint8_t completion_code;
  completion_code = CC_SUCCESS;  // Fill response with default values
  unsigned char policy = *pwr_policy & 0x07;  // Power restore policy

  switch (policy)
  {
      case 0:
        if (pal_set_key_value("server_por_cfg", "off") != 0)
          completion_code = CC_UNSPECIFIED_ERROR;
        break;
      case 1:
        if (pal_set_key_value("server_por_cfg", "lps") != 0)
          completion_code = CC_UNSPECIFIED_ERROR;
        break;
      case 2:
        if (pal_set_key_value("server_por_cfg", "on") != 0)
          completion_code = CC_UNSPECIFIED_ERROR;
        break;
      case 3:
        // no change (just get present policy support)
        break;
      default:
        completion_code = CC_PARAM_OUT_OF_RANGE;
        break;
  }
  return completion_code;
}

uint8_t
pal_get_status(void) {
  char str_server_por_cfg[64];
  char buff[MAX_VALUE_LEN];
  int policy = 3;
  uint8_t data;

  // Platform Power Policy
  memset(str_server_por_cfg, 0 , sizeof(char) * 64);
  sprintf(str_server_por_cfg, "%s", "server_por_cfg");

  if (pal_get_key_value(str_server_por_cfg, buff) == 0)
  {
    if (!memcmp(buff, "off", strlen("off")))
      policy = 0;
    else if (!memcmp(buff, "lps", strlen("lps")))
      policy = 1;
    else if (!memcmp(buff, "on", strlen("on")))
      policy = 2;
    else
      policy = 3;
  }

  data = 0x01 | (policy << 5);

  return data;
}

unsigned char option_offset[] = {0,1,2,3,4,6,11,20,37,164};
unsigned char option_size[]   = {1,1,1,1,2,5,9,17,127};

void
pal_set_boot_option(unsigned char para,unsigned char* pbuff)
{
  return;
}

int
pal_get_boot_option(unsigned char para,unsigned char* pbuff)
{
  unsigned char size = option_size[para];
  memset(pbuff, 0, size);
  return size;
}

int
pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t snr_num = sel[11];
  uint8_t *event_data = &sel[10];
  uint8_t *ed = &event_data[3];
  char temp_log[512] = {0};

  if( ( MEMORY_ECC_ERR == snr_num ) || ( MEMORY_ERR_LOG_DIS == snr_num ) )
    {
    strcpy(error_log, "");
    if ( MEMORY_ECC_ERR == snr_num )
      {
      switch( ed[0] & 0x0F )
        {
        case 0x00:
          {
          strcat(error_log, "Correctable");
          sprintf(temp_log, "DIMM%02X ECC err", ed[2]);
          pal_add_cri_sel(temp_log);
          }
          break;
        case 0x01:
          {
          strcat(error_log, "Uncorrectable");
          sprintf(temp_log, "DIMM%02X UECC err", ed[2]);
          pal_add_cri_sel( temp_log );
          }
          break;
        case 0x02:
          strcat(error_log,"Parity");
          break;
        case 0x05:
          strcat(error_log, "Correctable ECC error Logging Limit Reached");
          break;
        default:
          strcat(error_log, "Unknown");
          break;
        }
      }
    else if ( MEMORY_ERR_LOG_DIS == snr_num )
      {
      if ( 0x00 == ( ed[0] & 0x0F ) ) {
          strcat(error_log, "Correctable Memory Error Logging Disabled");
        } else {
          strcat(error_log, "Unknown");
        }
      }
      // Common routine for both MEM_ECC_ERR and MEMORY_ERR_LOG_DIS
      sprintf(temp_log, " (DIMM %02X)", ed[2]);
      strcat(error_log, temp_log);

      sprintf(temp_log, " Logical Rank %d", ed[1] & 0x03);
      strcat(error_log, temp_log);

      switch((ed[1] & 0x0C) >> 2 ) {
         case 0x00:
            //Ignore when " All info available"
            break;
         case 0x01:
            strcat(error_log, " DIMM info not valid");
            break;
         case 0x02:
            strcat(error_log, " CHN info not valid");
            break;
         case 0x03:
            strcat(error_log, " CPU info not valid");
            break;
         default:
            strcat(error_log, " Unknown");
            break;
      }

      if ( ( ( event_data[2] & 0x80 ) >> 7 ) == 0) {
        sprintf(temp_log, " Assertion");
        strcat(error_log, temp_log);
      } else {
        sprintf(temp_log, " Deassertion");
        strcat(error_log, temp_log);
      }
    return 0;
    }
  pal_parse_sel_helper(fru, sel, error_log);
  return 0;
}

int
pal_parse_oem_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  char str[128];
  uint16_t bank, col;
  uint8_t record_type = (uint8_t) sel[2];
  uint32_t mfg_id;
  error_log[0] = '\0';

  /* Manufacturer ID (byte 9:7) */
  mfg_id = (*(uint32_t*)&sel[7]) & 0xFFFFFF;

  if (record_type == 0xc0 && mfg_id == 0x1c4c) {
    snprintf(str, sizeof(str), "Slot %d PCIe err", sel[14]);
    pal_add_cri_sel(str);
    sprintf(error_log, "VID:0x%02x%2x DID:0x%02x%2x Slot:0x%x Error ID:0x%x",
                        sel[11], sel[10], sel[13], sel[12], sel[14], sel[15]);
  }
  else if (record_type == 0xc2 && mfg_id == 0x1c4c) {
    sprintf(error_log, "Extra info:0x%x MSCOD:0x%02x%02x MCACOD:0x%02x%02x",
                        sel[11], sel[13], sel[12], sel[15], sel[14]);
  }
  else if (record_type == 0xc3 && mfg_id == 0x1c4c) {
    bank= (sel[11] & 0xf0) >> 4;
    col= ((sel[11] & 0x0f) << 8) | sel[12];
    sprintf(error_log, "Fail Device:0x%x Bank:0x%x Column:0x%x Failed Row:0x%02x%02x%02x",
                       sel[10], bank, col, sel[13], sel[14], sel[15]);
  }
  else
    return 0;

  return 0;
}

void
pal_sensor_sts_check(uint8_t snr_num, float val, uint8_t *thresh) {
  int ret;
  int fru = 1;
  float ucr_thresh_val,lcr_thresh_val;

  ret = pal_get_sensor_threshold(fru, snr_num, UCR_THRESH, &ucr_thresh_val);
  if (ret) {
    syslog(LOG_WARNING, "get ucr fail:%f",lcr_thresh_val);
  }

  ret = pal_get_sensor_threshold(fru, snr_num, LCR_THRESH, &lcr_thresh_val);
  if (ret) {
    syslog(LOG_WARNING, "get lcr fail:%f",lcr_thresh_val);
  }

  if(val >= ucr_thresh_val)
    *thresh = UCR_THRESH;
  else if(val <= lcr_thresh_val)
    *thresh = LCR_THRESH;
  else
    *thresh = 0;
}

int
pal_set_ppin_info(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10] = {0};
  int i;
  int completion_code = CC_UNSPECIFIED_ERROR;
  *res_len = 0;
  sprintf(key, "mb_cpu_ppin");

  if (req_len > SIZE_CPU_PPIN*2)
    req_len = SIZE_CPU_PPIN*2;

  for (i = 0; i < req_len; i++) {
    sprintf(tstr, "%02x", req_data[i]);
    strcat(str, tstr);
  }

  if (kv_set(key, str, 0, KV_FPERSIST) != 0)
    return completion_code;

  completion_code = CC_SUCCESS;

  return completion_code;
}

int
pal_get_syscfg_text (char *text) {
  char key[MAX_KEY_LEN], value[MAX_VALUE_LEN], entry[MAX_VALUE_LEN];
  char *key_prefix = "sys_config/";
  int num_cpu=2, num_dimm, num_drive=14;
  int index, surface, bubble;
  size_t ret;
  unsigned char board_id, revision_id;
  char **dimm_labels;
  struct dimm_map map[24], temp_map;

  if (text == NULL)
    return -1;

  // Clear string buffer
  text[0] = '\0';

  // CPU information
  for (index = 0; index < num_cpu; index++) {
    if (!is_cpu_socket_occupy((unsigned int)index))
      continue;
    sprintf(entry, "CPU%d:", index);

    // Processor#
    snprintf(key, MAX_KEY_LEN, "%sfru1_cpu%d_product_name",
      key_prefix, index);
    if (kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 26) {
      // Read 4 bytes Processor#
      if (snprintf(&entry[strlen(entry)], 5, "%s", &value[22]) > 5) {
        syslog(LOG_ERR, "%s: CPU processor ID truncation detected!\n", __func__);
      }
    }

    // Frequency & Core Number
    snprintf(key, MAX_KEY_LEN, "%sfru1_cpu%d_basic_info",
      key_prefix, index);
    if(kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 5) {
      sprintf(&entry[strlen(entry)], "/%.1fG/%dc",
        (float) (value[4] << 8 | value[3])/1000, value[0]);
    }

    sprintf(&entry[strlen(entry)], "\n");
    strcat(text, entry);
  }

  // prepare DIMM map
  pal_get_platform_id(&board_id);
  pal_get_board_rev_id(&revision_id);
  if (board_id & PLAT_ID_SKU_MASK) {
    // Double Side
    num_dimm = 24;
    if (revision_id < BOARD_REV_PVT)
      dimm_labels = dimm_label_DS_DVT;
    else
      dimm_labels = dimm_label_DS_PVT;
  } else {
    // Single Side
    num_dimm = 12;
    if (revision_id < BOARD_REV_PVT)
      dimm_labels = dimm_label_SS_DVT;
    else
      dimm_labels = dimm_label_SS_PVT;
  }
  // Initialize map
  for (index = 0; index < num_dimm; index++) {
    map[index].index = index;
    map[index].label = dimm_labels[index];
  }
  // Bubble Sort the map according label string
  surface = num_dimm;
  for (surface = num_dimm; surface > 1;) {
    bubble = 0;
    for(index = 0; index < surface - 1; index++) {
      if (strcmp(map[index].label, map[index+1].label) > 0) {
        // Swap
        temp_map = map[index+1];
        map[index+1] = map[index];
        map[index] = temp_map;

        bubble = index + 1;
      }
    }
    surface = bubble;
  }
  // DIMM information
  for (index = 0; index < num_dimm; index++) {
    sprintf(entry, "MEM%s:", map[index].label);

    // Check Present
    snprintf(key, MAX_KEY_LEN, "%sfru1_dimm%d_location",
      key_prefix, map[index].index);
    if(kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 1) {
      // Skip if not present
      if (value[0] != 0x01)
        continue;
    }

    // Module Manufacturer ID
    snprintf(key, MAX_KEY_LEN, "%sfru1_dimm%d_manufacturer_id",
      key_prefix, map[index].index);
    if(kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 2) {
      switch (value[1]) {
        case 0xce:
          sprintf(&entry[strlen(entry)], "Samsung");
          break;
        case 0xad:
          sprintf(&entry[strlen(entry)], "Hynix");
          break;
        case 0x2c:
          sprintf(&entry[strlen(entry)], "Micron");
          break;
        default:
          sprintf(&entry[strlen(entry)], "unknown");
          break;
      }
    }

    // Speed
    snprintf(key, MAX_KEY_LEN, "%sfru1_dimm%d_speed",
      key_prefix, map[index].index);
    if(kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 6) {
      sprintf(&entry[strlen(entry)], "/%dMhz/%dGB",
        value[1]<<8 | value[0],
        (value[5]<<24 | value[4]<<16 | value[3]<<8 | value[2])/1024 );
    }

    sprintf(&entry[strlen(entry)], "\n");
    strcat(text, entry);
  }

  // Drive information
  for (index = 0; index < num_drive; index++) {
    sprintf(entry, "HDD%d:", index);

    // Check Present
    snprintf(key, MAX_KEY_LEN, "%sfru1_B_drive%d_location",
      key_prefix, index);
    if(kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 3) {
      // Skip if not present
      if (value[2] == 0xff)
        continue;
    }

    // Model name
    snprintf(key, MAX_KEY_LEN, "%sfru1_B_drive%d_model_name",
      key_prefix, index);
    if(kv_get(key, value, &ret, KV_FPERSIST) == 0 && ret >= 1) {
      snprintf(&entry[strlen(entry)], ret+1, "%s", value);
    }

    sprintf(&entry[strlen(entry)], "\n");
    strcat(text, entry);
  }

  return 0;
}

int
pal_set_adr_trigger(uint8_t slot, bool trigger) {
  if (slot != FRU_MB) {
    return -1;
  }
  if (trigger) {
    FORCE_ADR();
  }
  return 0;
}

int
pal_get_nm_selftest_result(uint8_t fruid, uint8_t *data)
{
  uint8_t bus_id = 0x4;
  int rlen = 0;
  int ret = PAL_EOK;

  rlen = ipmb_send(
    bus_id,
    0x2c,
    NETFN_APP_REQ << 2,
    CMD_APP_GET_SELFTEST_RESULTS);

  if ( rlen < 2 )
  {
    ret = PAL_ENOTSUP;
  }
  else
  {
    //get the response data
    memcpy(data, ipmb_rxb()->data, 2);
  }
  return ret;
}

int pal_add_i2c_device(uint8_t bus, char *device_name, uint8_t slave_addr)
{
  char cmd[64] = {0};
  int ret = 0;
  const char *template_path="echo %s 0x%x > /sys/bus/i2c/devices/i2c-%d/new_device";

  sprintf(cmd, template_path, device_name, slave_addr, bus);

#if DEBUG
  syslog(LOG_WARNING, "[%s] Cmd: %s", __func__, cmd);
#endif

  ret = system(cmd);
  if (ret != 0) {
    syslog(LOG_ERR, "Adding I2C device %d:%d:%s failed\n", bus, slave_addr, device_name);
  }

  return ret;
}

int pal_del_i2c_device(uint8_t bus, uint8_t slave_addr)
{
  char cmd[64] = {0};
  int ret = 0;
  const char *template_path="echo 0x%x > /sys/bus/i2c/devices/i2c-%d/delete_device";

  sprintf(cmd, template_path, slave_addr, bus);

#if DEBUG
  syslog(LOG_WARNING, "[%s] Cmd: %s", __func__, cmd);
#endif

  ret = system(cmd);
  if (ret != 0) {
    syslog(LOG_ERR, "Deleting I2C device %d:%d failed\n", bus, slave_addr);
  }

  return ret;
}

bool
pal_is_ava_card(uint8_t riser_slot)
{
  int fd = 0;
  char fn[32];
  bool ret;
  uint8_t ava_fruid_addr = 0xa0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tcount, rcount;
  int  val;

  // control I2C multiplexer to target channel.
  val = mux_lock(&riser_mux, riser_slot, 2);
  if ( val < 0 ) {
    syslog(LOG_WARNING, "[%s]Cannot switch the riser card channel", __func__);
    ret = false;
    goto error_exit;
  }

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", RISER_BUS_ID);

  fd = open(fn, O_RDWR);
  if ( fd < 0 ) {
    ret = false;
    goto release_mux_and_exit;
  }

  //Send I2C to AVA for FRU present check
  rcount = 1;
  tcount = 0;
  val = i2c_rdwr_msg_transfer(fd, ava_fruid_addr, tbuf, tcount, rbuf, rcount);
  if( val < 0 ) {
    ret = false;
      goto release_mux_and_exit;
  }
  ret = true;

release_mux_and_exit:

  mux_release(&riser_mux);

error_exit:

  if (fd > 0)
  {
    close(fd);
  }

  return ret;
}

bool pal_is_retimer_card ( uint8_t riser_slot )
{
  int fd = 0;
  char fn[32];
  bool ret;
  uint8_t re_timer_present_chk_addr = 0x82;
  uint8_t tbuf = 0x0;
  uint8_t rbuf = 0x0;
  uint8_t tcount, rcount;
  int  val;

  // control I2C multiplexer to target channel.
  val = mux_lock(&riser_mux, riser_slot, 2);
  if ( val < 0 )
  {
    syslog(LOG_WARNING, "[%s]Cannot switch the riser card channel", __func__);
    ret = false;
    goto error_exit;
  }

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", RISER_BUS_ID);
  fd = open(fn, O_RDWR);
  if ( fd < 0 )
  {
    ret = false;
    goto release_mux_and_exit;
  }

  //Send I2C to re-timer
  tcount = 1;
  rcount = 1;
  val = i2c_rdwr_msg_transfer(fd, re_timer_present_chk_addr, &tbuf, tcount, &rbuf, rcount);
  if( val < 0 )
  {
    ret = false;
    goto release_mux_and_exit;
  }

  ret = true;

release_mux_and_exit:
  mux_release(&riser_mux);

error_exit:
  if (fd > 0)
  {
    close(fd);
  }

  return ret;
}

bool pal_is_pcie_ssd_card( uint8_t riser_slot )
{
  bool ret = false;
  int fd = 0;
  char fn[32];
  uint8_t pcie_ssd_addr = 0xd4;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tcount, rcount;
  int  val;

  if( !(pal_is_retimer_card(riser_slot) || pal_is_ava_card(riser_slot) ) )
    {
    // control I2C multiplexer to target channel.
    val = mux_lock(&riser_mux, riser_slot, 2);
    if ( val < 0 ) {
      syslog(LOG_WARNING, "[%s]Cannot switch the riser card channel", __func__);
      ret = false;
      goto error_exit;
    }

    snprintf(fn, sizeof(fn), "/dev/i2c-%d", RISER_BUS_ID);
    fd = open(fn, O_RDWR);
    if ( fd < 0 ) {
      ret = false;
      goto release_mux_and_exit;
    }

    //Read to check card present
    tbuf[0] = 0x00;
    tcount = 1;
    rcount = 8;
    val = i2c_rdwr_msg_transfer(fd, pcie_ssd_addr, tbuf, tcount, rbuf, rcount);
    if( val < 0 ) {
      ret = false;
      goto release_mux_and_exit;
    }
    ret = true;

    release_mux_and_exit:

    mux_release(&riser_mux);

    error_exit:

    if (fd > 0)
      {
      close(fd);
      }
    }
  return ret;
}
int pal_riser_mux_switch (uint8_t riser_slot)
{
  return mux_lock(&riser_mux, riser_slot, 2);
}

int pal_riser_mux_release(void)
{
  return mux_release( &riser_mux);
}

int
pal_is_fru_on_riser_card(uint8_t riser_slot, uint8_t *device_type)
{
  int ret = PAL_ENOTSUP;

  if ( true == pal_is_ava_card(riser_slot) )
  {
    *device_type = FOUND_AVA_DEVICE;
    ret = PAL_EOK;
  }
  else if ( true == pal_is_retimer_card(riser_slot) )
  {
    *device_type = FOUND_RETIMER_DEVICE;
    ret = PAL_EOK;
  }
  else
  {
    //riser_slot start from 2
    syslog(LOG_WARNING, "Unknown or no device on the riser slot %d", riser_slot+2);
  }

  return ret;
}

bool
pal_is_BBV_prsnt()
{
  int fd = 0;
  char fn[32];
  bool ret;
  uint8_t BBV_present_chk_addr = 0x92;
  uint8_t re_timer_present_chk_addr = 0x82;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tcount=0, rcount;
  int  val;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", RISER_BUS_ID);

  fd = open(fn, O_RDWR);
  if ( fd < 0 ) {
    ret = false;
    goto error_exit;
  }

  //Send I2C to Bridge card on BBV
  rcount = 1;
  val = i2c_rdwr_msg_transfer(fd, BBV_present_chk_addr, tbuf, tcount, rbuf, rcount);
  if( val < 0 ) {
    ret = false;
      goto error_exit;
  }

  //Send I2C to re-timer
  rcount = 1;
  val = i2c_rdwr_msg_transfer(fd, re_timer_present_chk_addr, tbuf, tcount, rbuf, rcount);
  if( val < 0 ) {
    ret = false;
      goto error_exit;
  }
  ret = true;

error_exit:
  if (fd > 0)
  {
    close(fd);
  }

  return ret;
}

int
pal_CPU_error_num_chk(bool is_caterr)
{
  int len;
  int cpu_num = -1;
  ipmb_req_t *req;
  ipmb_res_t *res;

  // Get biosscratchpad7[26]: DWR
  req = ipmb_txb();
  res = ipmb_rxb();
  req->res_slave_addr = 0x2C; //ME's Slave Address
  req->netfn_lun = NETFN_NM_REQ<<2;
  req->cmd = CMD_NM_SEND_RAW_PECI;
  req->data[0] = 0x57;
  req->data[1] = 0x01;
  req->data[2] = 0x00;
  req->data[3] = 0x30;
  req->data[4] = 0x05;
  req->data[5] = 0x05;
  req->data[6] = 0xa1;
  req->data[7] = 0x00;
  req->data[8] = 0x00;
  req->data[9] = 0x05;
  req->data[10] = 0x00;
  // Invoke IPMB library handler
  len = ipmb_send_buf(0x4, 11+MIN_IPMB_REQ_LEN);
  // Data len >= 4 and  IPMB Success
  if ( ( len >= (4+MIN_IPMB_RES_LEN) ) && ( res->cc == 0 ) && ( res->data[3] == 0x40 ) ) {
    if( is_caterr ) {
      if(((res->data[7] & 0xE0) > 0) && ((res->data[7] & 0x1C) > 0))
        cpu_num = 2; //Both
      else if((res->data[7] & 0xE0) > 0)
        cpu_num = 1; //CPU1
      else if((res->data[7] & 0x1C) > 0)
        cpu_num = 0; // CPU0
      } else {
      if(((res->data[6] & 0xE0) > 0) && ((res->data[6] & 0x1C) > 0))
        cpu_num = 2; //Both
      else if((res->data[6] & 0xE0) > 0)
        cpu_num = 1; //CPU1
      else if((res->data[6] & 0x1C) > 0)
        cpu_num = 0; // CPU0
      }
  }
  return cpu_num;
}

int
pal_mmap (uint32_t base, uint8_t offset, int option, uint32_t para)
{
  uint32_t mmap_fd;
  uint32_t ctrl;
  void *reg_base;
  void *reg_offset;

  mmap_fd = open("/dev/mem", O_RDWR | O_SYNC );
  if (mmap_fd < 0) {
    return -1;
  }

  reg_base = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mmap_fd, base);
  reg_offset = (char*) reg_base + offset;
  ctrl = *(volatile uint32_t*) reg_offset;

  switch(option) {
    case UARTSW_BY_BMC:                //UART Switch control by bmc
      ctrl &= 0x00ffffff;
      break;
    case UARTSW_BY_DEBUG:           //UART Switch control by debug card
      ctrl |= 0x01000000;
      break;
    case SET_SEVEN_SEGMENT:      //set channel on the seven segment display
      ctrl &= 0x00ffffff;
      ctrl |= para;
      break;
    default:
      syslog(LOG_WARNING, "pal_mmap: unknown option");
      break;
  }
  *(volatile uint32_t*) reg_offset = ctrl;

  munmap(reg_base, PAGE_SIZE);
  close(mmap_fd);

  return 0;
}

int
pal_uart_switch_for_led_ctrl (void)
{
  static uint32_t pre_channel = 0xffffffff;
  uint8_t vals;
  uint32_t channel = 0;
  const char *shadows[] = {
    "FM_UARTSW_LSB_N",
    "FM_UARTSW_MSB_N"
  };

  //UART Switch control by bmc
  pal_mmap (AST_GPIO_BASE, UARTSW_OFFSET, UARTSW_BY_BMC, 0);

  if (get_gpio_shadow_array(shadows, ARRAY_SIZE(shadows), &vals)) {
    return -1;
  }
  // The GPIOs are active-low. So, invert it.
  channel = (uint32_t)(~vals & 0x3);
  // Shift to get to the bit position of the led.
  channel = channel << 24;

  // If the requested channel is the same as the previous, do nothing.
  if (channel == pre_channel) {
    return -1;
  }
  pre_channel = channel;

  //show channel on 7-segment display
  pal_mmap (AST_GPIO_BASE, SEVEN_SEGMENT_OFFSET, SET_SEVEN_SEGMENT, channel);

  return 0;
}

void
pal_set_def_restart_cause(uint8_t slot)
{
  char pwr_policy[MAX_VALUE_LEN] = {0};
  char last_pwr_st[MAX_VALUE_LEN] = {0};
  if ( FRU_MB == slot )
  {
    kv_get("pwr_server_last_state", last_pwr_st, NULL, KV_FPERSIST);
    kv_get("server_por_cfg", pwr_policy, NULL, KV_FPERSIST);
    if( pal_is_bmc_por() )
    {
      if( !strcmp( pwr_policy, "on") )
      {
        pal_set_restart_cause(FRU_MB, RESTART_CAUSE_AUTOMATIC_PWR_UP);
      }
      else if( !strcmp( pwr_policy, "lps") && !strcmp( last_pwr_st, "on") )
      {
        pal_set_restart_cause(FRU_MB, RESTART_CAUSE_AUTOMATIC_PWR_UP_LPR);
      }
    }
  }
}

struct fsc_monitor
{
  uint8_t sensor_num;
  char *sensor_name;
  bool (*check_sensor_sts)(uint8_t);
  bool is_alive;
  uint8_t init_count;
  uint8_t retry;
};

static struct fsc_monitor fsc_monitor_riser_list[] =
{
  {MB_SENSOR_C2_NVME_CTEMP  , "mb_c2_nvme_ctemp"  , pal_is_pcie_ssd_card , false, 5, 5},
  {MB_SENSOR_C2_1_NVME_CTEMP, "mb_c2_0_nvme_ctemp", pal_is_ava_card      , false, 5, 5},
  {MB_SENSOR_C2_2_NVME_CTEMP, "mb_c2_1_nvme_ctemp", pal_is_ava_card      , false, 5, 5},
  {MB_SENSOR_C2_3_NVME_CTEMP, "mb_c2_2_nvme_ctemp", pal_is_ava_card      , false, 5, 5},
  {MB_SENSOR_C2_4_NVME_CTEMP, "mb_c2_3_nvme_ctemp", pal_is_ava_card      , false, 5, 5},
  {MB_SENSOR_C3_NVME_CTEMP  , "mb_c3_nvme_ctemp"  , pal_is_pcie_ssd_card , false, 5, 5},
  {MB_SENSOR_C3_1_NVME_CTEMP, "mb_c3_0_nvme_ctemp", pal_is_ava_card      , false, 5, 5},
  {MB_SENSOR_C3_2_NVME_CTEMP, "mb_c3_1_nvme_ctemp", pal_is_ava_card      , false, 5, 5},
  {MB_SENSOR_C3_3_NVME_CTEMP, "mb_c3_2_nvme_ctemp", pal_is_ava_card      , false, 5, 5},
  {MB_SENSOR_C3_4_NVME_CTEMP, "mb_c3_3_nvme_ctemp", pal_is_ava_card      , false, 5, 5},
  {MB_SENSOR_C4_NVME_CTEMP  , "mb_c4_nvme_ctemp"  , pal_is_pcie_ssd_card , false, 5, 5},
  {MB_SENSOR_C4_1_NVME_CTEMP, "mb_c4_0_nvme_ctemp", pal_is_ava_card      , false, 5, 5},
  {MB_SENSOR_C4_2_NVME_CTEMP, "mb_c4_1_nvme_ctemp", pal_is_ava_card      , false, 5, 5},
  {MB_SENSOR_C4_3_NVME_CTEMP, "mb_c4_2_nvme_ctemp", pal_is_ava_card      , false, 5, 5},
  {MB_SENSOR_C4_4_NVME_CTEMP, "mb_c4_3_nvme_ctemp", pal_is_ava_card      , false, 5, 5},
};

static int fsc_monitor_riser_list_size = sizeof(fsc_monitor_riser_list) / sizeof(struct fsc_monitor);

static struct fsc_monitor fsc_monitor_basic_snr_list[] =
{
  {MB_SENSOR_INLET_REMOTE_TEMP  , "mb_inlet_remote_temp"  , NULL, false, 5, 5},
  {MB_SENSOR_CPU0_PKG_POWER     , "mb_cpu0_pkg_power"     , NULL, false, 5, 5},
  {MB_SENSOR_CPU1_PKG_POWER     , "mb_cpu1_pkg_power"     , NULL, false, 5, 5},
  {MB_SENSOR_CPU0_THERM_MARGIN  , "mb_cpu0_therm_margin"  , NULL, false, 5, 5},
  {MB_SENSOR_CPU1_THERM_MARGIN  , "mb_cpu1_therm_margin"  , NULL, false, 5, 5},
  //dimm sensors wait for 240s. 240=80*3(fsc monitor interval)
  {MB_SENSOR_CPU0_DIMM_GRPA_TEMP, "mb_cpu0_dimm_grpa_temp", pal_is_dimm_present, false, 80, 5},
  {MB_SENSOR_CPU0_DIMM_GRPB_TEMP, "mb_cpu0_dimm_grpb_temp", pal_is_dimm_present, false, 80, 5},
  {MB_SENSOR_CPU1_DIMM_GRPC_TEMP, "mb_cpu1_dimm_grpc_temp", pal_is_dimm_present, false, 80, 5},
  {MB_SENSOR_CPU1_DIMM_GRPD_TEMP, "mb_cpu1_dimm_grpd_temp", pal_is_dimm_present, false, 80, 5},
};

static int fsc_monitor_basic_snr_list_size = sizeof(fsc_monitor_basic_snr_list) / sizeof(struct fsc_monitor);

int pal_fsc_get_target_snr(char *sname, struct fsc_monitor *fsc_fru_list, int fsc_fru_list_size)
{
  int i;
  for ( i=0;  i<fsc_fru_list_size; i++)
  {
    if ( 0 == strcmp(sname, fsc_fru_list[i].sensor_name) )
    {
#ifdef FSC_DEBUG
      syslog(LOG_WARNING,"[%s]sensor is found:%s, idx:%d", __func__, sname, i);
#endif
      return i;
    }
  }

  syslog(LOG_WARNING,"[%s]Unknown sensor name:%s", __func__, sname);
  return PAL_ENOTSUP;
}

int
pal_init_fsc_snr_sts(uint8_t fru_id, struct fsc_monitor *curr_snr)
{
  int ret = PAL_EOK;
  int actual_fru_id;
  float value;
  bool is_snr_prsnt = false;
  bool is_check_snr_num = false;

  //if the sensor is exist, return.
  if ( true == curr_snr->is_alive )
  {
    curr_snr->init_count = 0;
    return ret;
  }

  if ( fru_id >= FRU_RISER_SLOT2 )
  {
    actual_fru_id = fru_id - FRU_RISER_SLOT2;
  }
  else
  {
    actual_fru_id = fru_id;

    if ( (MB_SENSOR_CPU0_DIMM_GRPA_TEMP == curr_snr->sensor_num) || (MB_SENSOR_CPU0_DIMM_GRPB_TEMP == curr_snr->sensor_num) ||
         (MB_SENSOR_CPU1_DIMM_GRPC_TEMP == curr_snr->sensor_num) || (MB_SENSOR_CPU1_DIMM_GRPD_TEMP == curr_snr->sensor_num) )
    {
      is_check_snr_num = true;
    }
  }

  //some sensors need to be judged with preidentify function
  if ( NULL != curr_snr->check_sensor_sts )
  {
    if ( true == is_check_snr_num )
    {
      //check sensor present based on the sensor name
      is_snr_prsnt = curr_snr->check_sensor_sts(curr_snr->sensor_num);
    }
    else
    {
      //check sensor present based on its fru
      is_snr_prsnt = curr_snr->check_sensor_sts(actual_fru_id);
    }
#ifdef FSC_DEBUG
    syslog(LOG_WARNING,"[%s]Check snr status. is_snr_prsnt:%d, fru_id:%d, actual_fru_id:%d", __func__, is_snr_prsnt, fru_id, actual_fru_id);
#endif

    if ( (true == is_snr_prsnt) && (false == curr_snr->is_alive) )
    {
      //if the fru is exist, check the reading
      //if the reading is N/A, we assume the sensor is not present
      //if the reading is the numerical value, we assume the sensor is present
      ret = sensor_cache_read(fru_id, curr_snr->sensor_num, &value);

#ifdef FSC_DEBUG
      syslog(LOG_WARNING,"[%s]Check snr reading. fru_id:%d, snum:%x, ret=%d", __func__, fru_id, curr_snr->sensor_num, ret);
#endif

      if ( PAL_EOK == ret )
      {
#ifdef FSC_DEBUG
        syslog(LOG_WARNING,"[%s] snr num %x is found. ret=%d", __func__, curr_snr->sensor_num, ret);
#endif
        curr_snr->is_alive = true;
      }
    }
  }
  else
  {
    //no preidentify function. sensors should exist anyway
#ifdef FSC_DEBUG
    syslog(LOG_WARNING,"[%s] snr num %x is found. ret=%d", __func__, curr_snr->sensor_num, ret);
#endif
    ret = PAL_EOK;
    curr_snr->is_alive = true;
  }

  curr_snr->init_count--;

  return ret;
}

void
pal_reinit_fsc_monitor_list()
{
  int i;

  //dimm sensors need to be re-init when the system do reset
  //we only re-init the basic snr list since the dimm sensors is included in it
  for ( i=0; i<fsc_monitor_basic_snr_list_size; i++ )
  {
    fsc_monitor_basic_snr_list[i].is_alive = false;
    fsc_monitor_basic_snr_list[i].retry = 5;

    if ( (MB_SENSOR_CPU0_DIMM_GRPA_TEMP == fsc_monitor_basic_snr_list[i].sensor_num) ||
         (MB_SENSOR_CPU0_DIMM_GRPB_TEMP == fsc_monitor_basic_snr_list[i].sensor_num) ||
         (MB_SENSOR_CPU1_DIMM_GRPC_TEMP == fsc_monitor_basic_snr_list[i].sensor_num) ||
         (MB_SENSOR_CPU1_DIMM_GRPD_TEMP == fsc_monitor_basic_snr_list[i].sensor_num) )
    {
      fsc_monitor_basic_snr_list[i].init_count = 80;
    }
    else
    {
      fsc_monitor_basic_snr_list[i].init_count = 5;
    }
  }
}

bool pal_sensor_is_valid(char *fru_name, char *sensor_name)
{
  const uint8_t init_done = 0;
  uint8_t fru_id;
  struct fsc_monitor *fsc_fru_list;
  int fsc_fru_list_size;
  int index;
  int ret;
  static time_t rst_time = 0;
  struct stat file_stat;

  //check the fru name is valid or not
  ret = pal_get_fru_id(fru_name, &fru_id);
  if ( ret < 0 )
  {
    syslog(LOG_WARNING,"[%s] Wrong fru#%s", __func__, fru_name);
    return false;
  }

  //if power reset is executed, re-init the list
  if ( (stat("/tmp/rst_touch", &file_stat) == 0) && (file_stat.st_mtime > rst_time) )
  {
    rst_time = file_stat.st_mtime;
    //in order to record rst_time, the function will be executed at first time
    pal_reinit_fsc_monitor_list();
  }

  //check the fru is riser or not
  if ( fru_id >= FRU_RISER_SLOT2 )
  {
    fsc_fru_list = fsc_monitor_riser_list;
    fsc_fru_list_size = fsc_monitor_riser_list_size;
  }
  else
  {
    fsc_fru_list = fsc_monitor_basic_snr_list;
    fsc_fru_list_size = fsc_monitor_basic_snr_list_size;
  }

  //get the target sensor
  ret = pal_fsc_get_target_snr(sensor_name, fsc_fru_list, fsc_fru_list_size);
  if ( ret < 0 )
  {
    syslog(LOG_WARNING,"[%s] undefined sensor: %s", __func__, sensor_name);
    return false;
  }

  index = ret;

  //init the snr list before checking snr fail
  if ( init_done != fsc_fru_list[index].init_count )
  {
    //if we get a sensor reading, it will be checked below
    //return false if the sensor is not ready
    ret = pal_init_fsc_snr_sts(fru_id, &fsc_fru_list[index]);
    if ( PAL_EOK != ret )
    {
      return false;
    }
  }


  //after a sensor is init done, we check its is_alive flag
  //If the flag is true, return true to make fsc check it
  //if the flag is false, return false to make fsc skip to check it
  if ( false == fsc_fru_list[index].is_alive )
  {
    return false;
  }

  //to avoid getting the different reading between here and fscd
  //wait for two loop
  if ( fsc_fru_list[index].retry > 3 )
  {
    fsc_fru_list[index].retry--;
    return false;
  }

  return true;
}

bool
pal_sensor_is_source_host(uint8_t fru, uint8_t sensor_id)
{
  if (fru == FRU_MB && sensor_id == MB_SENSOR_HOST_BOOT_TEMP) {
    return true;
  }
  return false;
}

int
pal_get_nic_fru_id(void)
{
  return FRU_NIC;
}

int
pal_fw_update_prepare(uint8_t fru, const char *comp) {
  int ret = 0, retry = 3;
  uint8_t status;
  gpio_desc_t *desc;

  if ((fru == FRU_MB) && !strcmp(comp, "bios")) {
    pal_set_server_power(FRU_MB, SERVER_POWER_OFF);
    while (retry > 0) {
      if (!pal_get_server_power(FRU_MB, &status) && (status == SERVER_POWER_OFF)) {
        break;
      }
      if ((--retry) > 0) {
        sleep(1);
      }
    }
    if (retry <= 0) {
      printf("Failed to Power Off Server. Stopping the update!\n");
      return -1;
    }

    sleep(10);
    if (system("/usr/local/bin/me-util 0xB8 0xDF 0x57 0x01 0x00 0x01 > /dev/null") != 0) {
      printf("Warning: Could not put ME in recovery mode");
    }
    sleep(1);

    ret = -1;
    do {
      if (gpio_export_by_name(GPIO_CHIP_ASPEED, "GPION5", "BMC_BIOS_FLASH_CTRL")) {
        printf("ERROR: Export of SPI-Switch GPIO failed\n");
        break;
      }

      desc = gpio_open_by_shadow("BMC_BIOS_FLASH_CTRL");
      if (!desc) {
        printf("ERROR: Opening SPI-Switch GPIO failed\n");
        break;
      }

      if (!gpio_set_direction(desc, GPIO_DIRECTION_OUT) && !gpio_set_value(desc, GPIO_VALUE_HIGH)) {
        ret = system("echo -n spi1.0 > /sys/bus/spi/drivers/m25p80/bind");
      } else {
        printf("ERROR: Switching BIOS to BMC failed\n");
      }
      gpio_close(desc);
    } while (0);
  }

  return ret;
}

int
pal_fw_update_finished(uint8_t fru, const char *comp, int status) {
  int ret = 0;
  gpio_desc_t *desc;

  if ((fru == FRU_MB) && !strcmp(comp, "bios")) {
    if (system("echo -n spi1.0 > /sys/bus/spi/drivers/m25p80/unbind") != 0) {
      printf("WARNING: Could not unbind BIOS SPI device\n");
    }

    desc = gpio_open_by_shadow("BMC_BIOS_FLASH_CTRL");
    if (desc) {
      gpio_set_value(desc, GPIO_VALUE_LOW);
      gpio_set_direction(desc, GPIO_DIRECTION_IN);
      gpio_close(desc);
      gpio_unexport("BMC_BIOS_FLASH_CTRL");
    }

    ret = status;
    if (status == 0) {
      sleep(1);
      pal_power_button_override(fru);
      sleep(10);
      pal_set_server_power(FRU_MB, SERVER_POWER_ON);
    }
  }

  return ret;
}
