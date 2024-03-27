#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openbmc/pal.h>
#include <openbmc/kv.h>
#include "raa_gen3.h"
#include "xdpe12284c.h"
#include "xdpe152xx.h"
#include "mp2856.h"
#include <sys/time.h>

#define MB_VR_BUS_ID   (28)

int
mb_vr_polling_ctrl(bool enable) {
  int rev;

  if (enable) {
      rev = system("systemctl start xyz.openbmc_project.psusensor.service");
  } else {
      rev = system("systemctl stop xyz.openbmc_project.psusensor.service");
  }
  return rev;
}

static int
plat_mp2856_fw_update(struct vr_info *info, void *args) {
  int ret;

  //restart time is 5S
  mb_vr_polling_ctrl(false);
  msleep(5000);
  ret = mp2856_fw_update(info, args);
  mb_vr_polling_ctrl(true);
  msleep(6000);

  return ret;
}

enum {
   MAIN_SOURCE = 0,
   SECOND_SOURCE = 1,
   THIRD_SOURCE = 2,
};

enum {
  VR_MB_CPU0_VCORE0 = 0,
  VR_MB_CPU0_VCORE1 = 1,
  VR_MB_CPU0_PVDD11 = 2,
  MB_VR_CNT = 3,
};

enum {
  VR_MB_CPU0_RT_P0V9   = 0,
  MB_RT_VR_CNT,
};

//MPS Address
enum {
  ADDR_CPU0_VCORE0 = 0x9E,
  ADDR_CPU0_VCORE1 = 0x9C,
  ADDR_CPU0_PVDD11 = 0x96,
};

//INF Address
enum {
  ADDR_INF_CPU0_VCORE0  = 0xC8,
  ADDR_INF_CPU0_VCORE1  = 0xCC,
  ADDR_INF_CPU0_PVDD11  = 0xD0,
};

//RAA Address
enum {
  ADDR_RAA_CPU0_VCORE0  = 0xC2,
  ADDR_RAA_CPU0_VCORE1  = 0xC4,
  ADDR_RAA_CPU0_PVDD11  = 0xC6,
};

enum {
  ADDR_CPU0_RT_P0V9 = 0xC0,
};

uint8_t mb_inf_vr_addr[] = {
  ADDR_INF_CPU0_VCORE0,
  ADDR_INF_CPU0_VCORE1,
  ADDR_INF_CPU0_PVDD11,
};

uint8_t mb_raa_vr_addr[] = {
  ADDR_RAA_CPU0_VCORE0,
  ADDR_RAA_CPU0_VCORE1,
  ADDR_RAA_CPU0_PVDD11,
};

//MPS VR
struct vr_ops mp2856_ops = {
  .get_fw_ver = get_mp2856_ver,
  .parse_file = mp2856_parse_file,
  .validate_file = NULL,
  .fw_update = plat_mp2856_fw_update,
  .fw_verify = NULL,
};


// MB VR Support list
//Renesas VR
struct vr_ops raa_gen2_3_ops = {
  .get_fw_ver = get_raa_ver,
  .parse_file = raa_parse_file,
  .validate_file = NULL,
  .fw_update = raa_fw_update,
  .fw_verify = NULL,
};

//INFINEON VR
struct vr_ops xdpe152xx_ops = {
  .get_fw_ver = get_xdpe152xx_ver,
  .parse_file = xdpe152xx_parse_file,
  .validate_file = NULL,
  .fw_update = xdpe152xx_fw_update,
  .fw_verify = NULL,
};

struct vr_info mb_vr_list[] = {
  [VR_MB_CPU0_VCORE0] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU0_VCORE0,
    .dev_name = "VR_CPU0_VCORE0/SOC",
    .ops = &mp2856_ops,
    .private_data = "mb",
    .xfer = NULL,
    .sensor_polling_ctrl = NULL,
  },
  [VR_MB_CPU0_VCORE1] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU0_VCORE1,
    .dev_name = "VR_CPU0_VCORE1/PVDDIO",
    .ops = &mp2856_ops,
    .private_data = "mb",
    .xfer = NULL,
    .sensor_polling_ctrl = NULL,
  },
  [VR_MB_CPU0_PVDD11] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU0_PVDD11,
    .dev_name = "VR_CPU0_PVDD11",
    .ops = &mp2856_ops,
    .private_data = "mb",
    .xfer = NULL,
    .sensor_polling_ctrl = NULL,
  },
};

struct vr_info mb_rt_vr_list[] = {
  [VR_MB_CPU0_RT_P0V9] = {
    .bus = MB_VR_BUS_ID,
    .addr = ADDR_CPU0_RT_P0V9,
    .dev_name = "VR_CPU0_RETIMER_P0V9",
    .ops = &mp2856_ops,
    .private_data = "mb",
    .xfer = NULL,
  },
};

#define MAX_VR_CNT (MB_VR_CNT + MB_RT_VR_CNT)
struct vr_info vr_list[MAX_VR_CNT] = {0};

int get_vr_comp_source(uint8_t* id) {
  char value[MAX_VALUE_LEN] = {0};

  if (kv_get("mb_vr_source", value, NULL, 0)) {
    return -1;
  }

  *id = (uint8_t)atoi(value);
  printf("id=%d\n", *id);
  return 0;
}

int plat_vr_init(void) {
  int i = 0;
  int ret = 0;
  uint8_t vr_cnt = 0;
  uint8_t id = 0;

  // Add MB VR
  memcpy(vr_list + vr_cnt, mb_vr_list, MB_VR_CNT*sizeof(struct vr_info));
  vr_cnt += MB_VR_CNT;

  // get MB VR source
  get_vr_comp_source(&id);
  if (id == SECOND_SOURCE) {
    for (i = 0; i < MB_VR_CNT; i++) {
      vr_list[i].ops =  &xdpe152xx_ops;
      vr_list[i].addr = mb_inf_vr_addr[i];
    }
  } else if (id == THIRD_SOURCE) {
    for (i = 0; i < MB_VR_CNT; i++) {
      vr_list[i].ops =  &raa_gen2_3_ops;
      vr_list[i].addr = mb_raa_vr_addr[i];
    }
  }

  //Add Retimer VR
  memcpy(vr_list + vr_cnt, mb_rt_vr_list, MB_RT_VR_CNT * sizeof(struct vr_info));
  vr_cnt += MB_RT_VR_CNT;

  ret = vr_device_register(vr_list, vr_cnt);
  if (ret < 0) {
    vr_device_unregister();
  }

  return ret;
}

void plat_vr_exit(void) {
  if (plat_configs) {
    free(plat_configs);
  }
  return;
}
