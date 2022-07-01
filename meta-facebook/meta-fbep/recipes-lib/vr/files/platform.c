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
  VR_P1V0_AVD0,
  VR_P1V0_AVD1,
  VR_P1V0_AVD2,
  VR_P1V0_AVD3
};

enum {
  VR_P0V9_VDD0 = 0,
  VR_P0V9_VDD1,
  VR_P0V9_VDD2,
  VR_P0V9_VDD3,
  VR_P1V8_AVD0,
  VR_P1V8_AVD1,
  VR_P1V8_AVD2,
  VR_P1V8_AVD3
};

enum {
  ADDR_P0V8_VDD0 = 0x30,
  ADDR_P0V8_VDD1 = 0x31,
  ADDR_P0V8_VDD2 = 0x32,
  ADDR_P0V8_VDD3 = 0x33,
  ADDR_P1V0_AVD0 = 0x34,
  ADDR_P1V0_AVD1 = 0x35,
  ADDR_P1V0_AVD2 = 0x36,
  ADDR_P1V0_AVD3 = 0x3b
};

enum {
  ADDR_P0V9_VDD0 = 0xC0,
  ADDR_P0V9_VDD1 = 0xC2,
  ADDR_P0V9_VDD2 = 0xE4,
  ADDR_P0V9_VDD3 = 0xEA,
  ADDR_P1V8_AVD0 = 0x34,
  ADDR_P1V8_AVD1 = 0x35,
  ADDR_P1V8_AVD2 = 0x36,
  ADDR_P1V8_AVD3 = 0x3b
};

struct vr_ops mpq8645p_ops = {
  .get_fw_ver = mpq8645p_get_fw_ver,
  .parse_file = mpq8645p_parse_file,
  .validate_file = mpq8645p_validate_file,
  .fw_update = mpq8645p_fw_update,
  .fw_verify = mpq8645p_fw_verify,
};

struct vr_ops raa_gen2_3_ops = {
  .get_fw_ver = get_raa_ver,
  .parse_file = raa_parse_file,
  .validate_file = NULL,
  .fw_update = raa_fw_update,
  .fw_verify = raa_fw_verify,
};

struct vr_info fbep_vr_list[] = {
  [VR_P0V8_VDD0] = {
    .bus = 5,
    .addr = ADDR_P0V8_VDD0,
    .dev_id = MPQ8645P_ID,
    .dev_name = "VR_P0V8_VDD0",
    .ops = &mpq8645p_ops,
  },
  [VR_P0V8_VDD1] = {
    .bus = 5,
    .addr = ADDR_P0V8_VDD1,
    .dev_id = MPQ8645P_ID,
    .dev_name = "VR_P0V8_VDD1",
    .ops = &mpq8645p_ops,
  },
  [VR_P0V8_VDD2] = {
    .bus = 5,
    .addr = ADDR_P0V8_VDD2,
    .dev_id = MPQ8645P_ID,
    .dev_name = "VR_P0V8_VDD2",
    .ops = &mpq8645p_ops,
  },
  [VR_P0V8_VDD3] = {
    .bus = 5,
    .addr = ADDR_P0V8_VDD3,
    .dev_id = MPQ8645P_ID,
    .dev_name = "VR_P0V8_VDD3",
    .ops = &mpq8645p_ops,
  },
  [VR_P1V0_AVD0] = {
    .bus = 5,
    .addr = ADDR_P1V0_AVD0,
    .dev_id = MPQ8645P_ID,
    .dev_name = "VR_P1V0_AVD0",
    .ops = &mpq8645p_ops,
  },
  [VR_P1V0_AVD1] = {
    .bus = 5,
    .addr = ADDR_P1V0_AVD1,
    .dev_id = MPQ8645P_ID,
    .dev_name = "VR_P1V0_AVD1",
    .ops = &mpq8645p_ops,
  },
  [VR_P1V0_AVD2] = {
    .bus = 5,
    .addr = ADDR_P1V0_AVD2,
    .dev_id = MPQ8645P_ID,
    .dev_name = "VR_P1V0_AVD2",
    .ops = &mpq8645p_ops,
  },
  [VR_P1V0_AVD3] = {
    .bus = 5,
    .addr = ADDR_P1V0_AVD3,
    .dev_id = MPQ8645P_ID,
    .dev_name = "VR_P1V0_AVD3",
    .ops = &mpq8645p_ops,
  }
};

struct vr_info fbep_brcm_vr_list[] = {
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
  [VR_P1V8_AVD0] = {
    .bus = 5,
    .addr = ADDR_P1V8_AVD0,
    .dev_name = "VR_P1V8_AVD0",
    .dev_id = MPQ8645P_ID,
    .ops = &mpq8645p_ops,
    .private_data = "vr",
  },
  [VR_P1V8_AVD1] = {
    .bus = 5,
    .addr = ADDR_P1V8_AVD1,
    .dev_name = "VR_P1V8_AVD1",
    .dev_id = MPQ8645P_ID,
    .ops = &mpq8645p_ops,
    .private_data = "vr",
  },
  [VR_P1V8_AVD2] = {
    .bus = 5,
    .addr = ADDR_P1V8_AVD2,
    .dev_name = "VR_P1V8_AVD2",
    .dev_id = MPQ8645P_ID,
    .ops = &mpq8645p_ops,
    .private_data = "vr",
  },
  [VR_P1V8_AVD3] = {
    .bus = 5,
    .addr = ADDR_P1V8_AVD3,
    .dev_name = "VR_P1V8_AVD3",
    .dev_id = MPQ8645P_ID,
    .ops = &mpq8645p_ops,
    .private_data = "vr",
  }
};

int plat_vr_init()
{
  int ret, i, index, *p;
  struct dirent *dp;
  DIR *dir;
  char path[128] = {0};

  if(pal_check_switch_config()) {
    int vr_cnt = sizeof(fbep_brcm_vr_list)/sizeof(fbep_brcm_vr_list[0]);

    for (i = VR_P1V8_AVD0; i < vr_cnt; i++) {
      snprintf(path, sizeof(path), MPQ8645P_HWMON_DIR, fbep_brcm_vr_list[i].addr);
      dir = opendir(path);
      if (dir == NULL)
        goto err;
  
      while ((dp = readdir(dir)) != NULL) {
        if (sscanf(dp->d_name, "hwmon%d", &index))
  	break;
      }
      if (dp == NULL) {
        closedir(dir);
        goto err;
      }
  
      p  = (int*)malloc(sizeof(int));
      if (p == NULL) {
        closedir(dir);
        goto err;
      }
  
      *p = index;
      fbep_brcm_vr_list[i].private_data = p;
      closedir(dir);
    }

    ret = vr_device_register(fbep_brcm_vr_list, vr_cnt );
    if (ret < 0) {
      vr_device_unregister();
    }

    return ret;
  err:
    for (i = i-1; i >= VR_P1V8_AVD0; i--)
      free(fbep_brcm_vr_list[i].private_data);
    return -1;
  } else {
    int list_cnt = sizeof(fbep_vr_list)/sizeof(fbep_vr_list[0]);
  
    for (i = 0; i < list_cnt; i++) {
      snprintf(path, sizeof(path), MPQ8645P_HWMON_DIR, fbep_vr_list[i].addr);
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
      fbep_vr_list[i].private_data = p;
      closedir(dir);
    }
  
    ret = vr_device_register(fbep_vr_list, sizeof(fbep_vr_list)/sizeof(fbep_vr_list[0]));
    if (ret < 0) {
      vr_device_unregister();
    }
  
    return ret;
  err_exit:
    for (i = i-1; i >= 0; i--)
      free(fbep_vr_list[i].private_data);
    return -1;
  }
}

void plat_vr_exit()
{
  int i;
  int list_cnt = sizeof(fbep_vr_list)/sizeof(fbep_vr_list[0]);

  for (i = 0; i < list_cnt; i++)
    free(fbep_vr_list[i].private_data);

  mpq8645p_free_configs(plat_configs);
}
