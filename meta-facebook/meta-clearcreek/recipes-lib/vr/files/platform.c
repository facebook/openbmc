#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include "mpq8645p.h"
#include "raa_gen3.h"
#include <openbmc/pal.h>

#define MPQ8645P_HWMON_DIR "/sys/bus/i2c/drivers/mpq8645p/5-00%x/hwmon"

enum {
  VR_P0V8_VDD0 = 0,
  VR_P0V8_VDD1,
  VR_P0V8_VDD2,
  VR_P0V8_VDD3,
  VR_P0V8_AVD0,
  VR_P0V8_AVD1,
  VR_P0V8_AVD2,
  VR_P0V8_AVD3
};

enum {
  ADDR_P0V8_VDD0 = 0x30,
  ADDR_P0V8_VDD1 = 0x31,
  ADDR_P0V8_VDD2 = 0x32,
  ADDR_P0V8_VDD3 = 0x33,
  ADDR_P0V8_AVD0 = 0x34,
  ADDR_P0V8_AVD1 = 0x35,
  ADDR_P0V8_AVD2 = 0x36,
  ADDR_P0V8_AVD3 = 0x3b
};

struct vr_ops mpq8645p_ops = {
  .get_fw_ver = mpq8645p_get_fw_ver,
  .parse_file = mpq8645p_parse_file,
  .validate_file = mpq8645p_validate_file,
  .fw_update = mpq8645p_fw_update,
  .fw_verify = mpq8645p_fw_verify,
};

struct vr_info fbcc_vr_list[] = {
  [VR_P0V8_VDD0] = {
    .bus = 5,
    .addr = ADDR_P0V8_VDD0,
    .dev_id = MPQ8645P_ID,
    .dev_name = "P0V8_VDD0",
    .ops = &mpq8645p_ops,
  },
  [VR_P0V8_VDD1] = {
    .bus = 5,
    .addr = ADDR_P0V8_VDD1,
    .dev_id = MPQ8645P_ID,
    .dev_name = "P0V8_VDD1",
    .ops = &mpq8645p_ops,
  },
  [VR_P0V8_VDD2] = {
    .bus = 5,
    .addr = ADDR_P0V8_VDD2,
    .dev_id = MPQ8645P_ID,
    .dev_name = "P0V8_VDD2",
    .ops = &mpq8645p_ops,
  },
  [VR_P0V8_VDD3] = {
    .bus = 5,
    .addr = ADDR_P0V8_VDD3,
    .dev_id = MPQ8645P_ID,
    .dev_name = "P0V8_VDD3",
    .ops = &mpq8645p_ops,
  },
  [VR_P0V8_AVD0] = {
    .bus = 5,
    .addr = ADDR_P0V8_AVD0,
    .dev_id = MPQ8645P_ID,
    .dev_name = "P0V8_AVD_PCIE0",
    .ops = &mpq8645p_ops,
  },
  [VR_P0V8_AVD1] = {
    .bus = 5,
    .addr = ADDR_P0V8_AVD1,
    .dev_id = MPQ8645P_ID,
    .dev_name = "P0V8_AVD_PCIE1",
    .ops = &mpq8645p_ops,
  },
  [VR_P0V8_AVD2] = {
    .bus = 5,
    .addr = ADDR_P0V8_AVD2,
    .dev_id = MPQ8645P_ID,
    .dev_name = "P0V8_AVD_PCIE2",
    .ops = &mpq8645p_ops,
  },
  [VR_P0V8_AVD3] = {
    .bus = 5,
    .addr = ADDR_P0V8_AVD3,
    .dev_id = MPQ8645P_ID,
    .dev_name = "P0V8_AVD_PCIE3",
    .ops = &mpq8645p_ops,
  }
};

enum {
  VR_P0V9_VDD0 = 0,
  VR_P0V9_VDD1,
  VR_P0V9_VDD2,
  VR_P0V9_VDD3,
  VR_P1V8_AVD0_1,
  VR_P1V8_AVD2_3,
};

enum {
  ADDR_P0V9_VDD0 = 0xC0,
  ADDR_P0V9_VDD1 = 0xC2,
  ADDR_P0V9_VDD2 = 0xE4,
  ADDR_P0V9_VDD3 = 0xEA,
  ADDR_P1V8_AVD0_1 = 0x34,
  ADDR_P1V8_AVD2_3 = 0x36,
};

struct vr_ops raa_gen2_3_ops = {
  .get_fw_ver = get_raa_ver,
  .parse_file = raa_parse_file,
  .validate_file = NULL,
  .fw_update = raa_fw_update,
  .fw_verify = raa_fw_verify,
};

struct vr_info fbcc_brcm_vr_list[] = {
  [VR_P0V9_VDD0] = {
    .bus = 5,
    .addr = ADDR_P0V9_VDD0,
    .dev_name = "VR_P0V9_VDD0",
    .ops = &raa_gen2_3_ops,
    .private_data = "vr",
    .xfer = NULL,
  },
  [VR_P0V9_VDD1] = {
    .bus = 5,
    .addr = ADDR_P0V9_VDD1,
    .dev_name = "VR_P0V9_VDD1",
    .ops = &raa_gen2_3_ops,
    .private_data = "vr",
    .xfer = NULL,
  },
  [VR_P0V9_VDD2] = {
    .bus = 5,
    .addr = ADDR_P0V9_VDD2,
    .dev_name = "VR_P0V9_VDD2",
    .ops = &raa_gen2_3_ops,
    .private_data = "vr",
    .xfer = NULL,
  },
  [VR_P0V9_VDD3] = {
    .bus = 5,
    .addr = ADDR_P0V9_VDD3,
    .dev_name = "VR_P0V9_VDD3",
    .ops = &raa_gen2_3_ops,
    .private_data = "vr",
    .xfer = NULL,
  },
  [VR_P1V8_AVD0_1] = {
    .bus = 5,
    .addr = ADDR_P1V8_AVD0_1,
    .dev_name = "VR_P1V8_AVD0_1",
    .dev_id = MPQ8645P_ID,
    .ops = &mpq8645p_ops,
    .private_data = "vr",
  },
  [VR_P1V8_AVD2_3] = {
    .bus = 5,
    .addr = ADDR_P1V8_AVD2_3,
    .dev_name = "VR_P1V8_AVD2_3",
    .dev_id = MPQ8645P_ID,
    .ops = &mpq8645p_ops,
    .private_data = "vr",
  },
};

int plat_vr_init()
{
  int ret, i, index, *p;
  struct dirent *dp;
  DIR *dir;
  char path[128] = {0};
  int list_cnt, list_start;
  uint8_t board_id = 0xFF;

  pal_get_platform_id(&board_id);

  if((board_id & 0x7) == 0x3) {
    list_cnt = sizeof(fbcc_brcm_vr_list)/sizeof(fbcc_brcm_vr_list[0]);
    list_start = 4;
  }
  else {
    list_cnt = sizeof(fbcc_vr_list)/sizeof(fbcc_vr_list[0]);
    list_start = 0;
  }

  for (i = list_start; i < list_cnt; i++) {
    if((board_id & 0x7) == 0x3) {
      snprintf(path, sizeof(path), MPQ8645P_HWMON_DIR, fbcc_brcm_vr_list[i].addr);
    }
    else {
      snprintf(path, sizeof(path), MPQ8645P_HWMON_DIR, fbcc_vr_list[i].addr);
    }

    dir = opendir(path);
    if (dir == NULL)
      goto err_exit;

    while ((dp = readdir(dir)) != NULL) {
      if (sscanf(dp->d_name, "hwmon%d", &index))
	break;
    }
    if (dp == NULL) {
      closedir(dir);
      goto err_exit;
    }

    p  = (int*)malloc(sizeof(int));
    if (p == NULL) {
      closedir(dir);
      goto err_exit;
    }

    *p = index;
    if((board_id & 0x7) == 0x3) {
      fbcc_brcm_vr_list[i].private_data = p;
    }
    else {
      fbcc_vr_list[i].private_data = p;
    }

    closedir(dir);
  }

  if((board_id & 0x7) == 0x3) {
    ret = vr_device_register(fbcc_brcm_vr_list, sizeof(fbcc_brcm_vr_list)/sizeof(fbcc_brcm_vr_list[0]));
  }
  else {
    ret = vr_device_register(fbcc_vr_list, sizeof(fbcc_vr_list)/sizeof(fbcc_vr_list[0]));
  }

  if (ret < 0) {
    vr_device_unregister();
  }

  return ret;

err_exit:
  if((board_id & 0x7) == 0x3) {
    free(fbcc_brcm_vr_list[VR_P1V8_AVD0_1].private_data);
    free(fbcc_brcm_vr_list[VR_P1V8_AVD2_3].private_data);
  }

  for (i = i-1; i >= 0; i--)
    free(fbcc_vr_list[i].private_data);
  return -1;
}

void plat_vr_exit()
{
  int i;
  int list_cnt = sizeof(fbcc_vr_list)/sizeof(fbcc_vr_list[0]);

  for (i = 0; i < list_cnt; i++)
    free(fbcc_vr_list[i].private_data);

  mpq8645p_free_configs(plat_configs);
}
