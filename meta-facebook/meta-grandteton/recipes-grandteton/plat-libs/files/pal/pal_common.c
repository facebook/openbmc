#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/libgpio.h>
#include <openbmc/kv.h>
#include <syslog.h>
#include "pal_common.h"
#include "pal_def.h"
#include "pal_gpio.h"
#include <libpldm/pldm.h>
#include <libpldm/platform.h>
#include <libpldm-oem/pldm.h>

//#define DEBUG

char *mb_source_data[] = {
  "mb_hsc_source",
  "mb_vr_source",
  "mb_adc_source",
};

char *vpdb_source_data[] = {
  "vpdb_hsc_source",
  "vpdb_brick_source",
};

char *hpdb_source_data[] = {
  "hpdb_hsc_source",
};

char *bp0_source_data[] = {
  "bp0_fan_chip_source",
};

char *bp1_source_data[] = {
  "bp1_fan_chip_source",
};

struct source_info {
  uint8_t fru;
  char** source;
};


struct source_info comp_source_data[] = {
  {FRU_ALL,   NULL},
  {FRU_MB,    mb_source_data},
  {FRU_SWB,   NULL},
  {FRU_HMC,   NULL},
  {FRU_NIC0,  NULL},
  {FRU_NIC1,  NULL},
  {FRU_DBG,   NULL},
  {FRU_BMC,   NULL},
  {FRU_SCM,   NULL},
  {FRU_PDBV,  vpdb_source_data},
  {FRU_PDBH,  hpdb_source_data},
  {FRU_BP0,   bp0_source_data},
  {FRU_BP1,   bp1_source_data},
  {FRU_FIO,   NULL},
  {FRU_HSC,   NULL},
  {FRU_SHSC,  NULL},

};


bool
is_cpu_socket_occupy(uint8_t cpu_idx) {
  static bool cached = false;
  static unsigned int cached_id = 0;

  if (!cached) {
    const char *shadows[] = {
      FM_CPU0_SKTOCC,
      FM_CPU1_SKTOCC,
    };
    if (gpio_get_value_by_shadow_list(shadows, ARRAY_SIZE(shadows), &cached_id)) {
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

bool
pal_skip_access_me(void) {
  if (!access("/tmp/fin_bios_upd", F_OK)) {
    return true;
  }

  return false;
}

int
read_device(const char *device, int *value) {
  FILE *fp = NULL;

  if (device == NULL || value == NULL) {
    syslog(LOG_ERR, "%s: Invalid parameter", __func__);
    return -1;
  }

  fp = fopen(device, "r");
  if (fp == NULL) {
    syslog(LOG_INFO, "%s failed to open device %s error: %s", __func__, device, strerror(errno));
    return -1;
  }

  if (fscanf(fp, "%d", value) != 1) {
    syslog(LOG_INFO, "%s failed to read device %s error: %s", __func__, device, strerror(errno));
    fclose(fp);
    return -1;
  }
  fclose(fp);

  return 0;
}


bool
fru_presence(void) {
  return true;
}

bool
hgx_presence(void) {
  gpio_value_t value;

  value = gpio_get_value_by_shadow(HMC_PRESENCE);
  if(value != GPIO_VALUE_INVALID)
    return value ? false : true;

  return false;
}

bool
swb_presence(void) {
  gpio_value_t value;

  value = gpio_get_value_by_shadow(BIC_READY);
  if(value != GPIO_VALUE_INVALID)
    return value ? false : true;

  return false;
}


bool
check_pwron_time(int time) {
  char str[MAX_VALUE_LEN] = {0};
  struct timespec ts;
  long pwron_time;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (!kv_get("snr_pwron_flag", str, NULL, 0)) {
    pwron_time = strtoul(str, NULL, 10);
   // syslog(LOG_WARNING, "power on time =%ld\n", pwron_time);
    if ( (ts.tv_sec - pwron_time > time ) && ( pwron_time != 0 ) ) {
      return true;
    }
  } else {
     sprintf(str, "%ld", ts.tv_sec);
     kv_set("snr_pwron_flag", str, 0, 0);
  }

  return false;
}

bool
pal_bios_completed(uint8_t fru)
{
  gpio_value_t value;

  if ( FRU_MB != fru )
  {
    syslog(LOG_WARNING, "[%s]incorrect fru id: %d", __func__, fru);
    return false;
  }

//BIOS COMPLT need time to inital when platform reset.
  if( !check_pwron_time(10) ) {
    return false;
  }

  value = gpio_get_value_by_shadow(FM_BIOS_POST_CMPLT);
  if(value != GPIO_VALUE_INVALID)
    return value ? false : true;

  return false;
}

static void
get_dimm_present_info(uint8_t fru, bool *dimm_sts_list) {
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  int i;
  size_t ret;

  //check dimm info from /mnt/data/sys_config/
  for (i=0; i<MAX_DIMM_NUM; i++) {
    sprintf(key, "sys_config/fru%d_dimm%d_location", fru, i);
    if(kv_get(key, value, &ret, KV_FPERSIST) != 0 || ret < 4) {
      syslog(LOG_WARNING,"[%s]Cannot get dimm_slot%d present info", __func__, i);
      return;
    }

    if ( 0xff == value[0] ) {
      dimm_sts_list[i] = false;
    } else {
      dimm_sts_list[i] = true;
    }
  }
}

bool
is_dimm_present(uint8_t dimm_id) {
  static bool is_check = false;
  static bool dimm_sts_list[MAX_DIMM_NUM] = {0};
  uint8_t fru = FRU_MB;

  if (!pal_bios_completed(fru) ) {
    return false;
  }

  if ( is_check == false ) {
    is_check = true;
    get_dimm_present_info(fru, dimm_sts_list);
  }

#ifdef DEBUG
  syslog(LOG_WARNING, "dimm id=%d, presnet=%d\n", dimm_id, dimm_sts_list[dimm_id]);
#endif
  if( dimm_sts_list[dimm_id] == true) {
    return true;
  }
  return false;
}

int
get_comp_source(uint8_t fru, uint8_t comp_id, uint8_t* source) {
  char value[MAX_VALUE_LEN] = {0};

  if(kv_get(comp_source_data[fru].source[comp_id], value, 0, 0)) {
    syslog(LOG_WARNING,"[%s] get source fail fru=%d comp=%d", __func__, fru, comp_id);
    return -1;
  }

  *source = (uint8_t)atoi(value);
  return 0;
}

bool
is_mb_hsc_module(void) {
  static bool cached = false;
  static bool val = false;
  uint8_t id;

  if (!cached) {
    get_comp_source(FRU_MB, MB_HSC_SOURCE, &id); //Fail:Main Source
    if (id == SECOND_SOURCE || id == THIRD_SOURCE)
      val = true;
    cached = true;
  }
  return val;
}

bool
is_swb_hsc_module(void) {
  static bool cached = false;
  static bool val = false;
  uint8_t inf_devid[3] = { 0x02, 0x79, 0x02 };
  uint8_t tbuf[16], rbuf[16];
  uint8_t tlen=0;
  int ret;

  if(!cached) {
    kv_set("swb_hsc_module", 0, 0, 0);
    if(swb_presence()) {
      tbuf[tlen++] = VR0_COMP;
      tbuf[tlen++] = 3;
      tbuf[tlen++] = 0xAD;

      size_t rlen = 0;
      ret = pldm_oem_ipmi_send_recv(SWB_BUS_ID, SWB_BIC_EID,
                                 NETFN_OEM_1S_REQ, CMD_OEM_1S_BIC_BRIDGE,
                                 tbuf, tlen,
                                 rbuf, &rlen);

      if(!ret && !memcmp(rbuf, inf_devid, 3)) {
        val = true;
        kv_set("swb_hsc_module", "1", 0, 0);
      }
    }
    cached = true;
  }
  return val;
}

