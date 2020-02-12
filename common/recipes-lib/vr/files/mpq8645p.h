#ifndef __MPQ8645P_H__
#define __MPQ8645P_H__

#include "vr.h"

#define MPQ8645P_ID                      0x53503D
#define MPQ8645P_REG_MFR_CTRL_COMP       0xD0
#define MPQ8645P_REG_MFR_CTRL_VOUT       0xD1
#define MPQ8645P_REG_MFR_CTRL_OPS        0xD2
#define MPQ8645P_REG_MFR_OC_PHASE_LIMIT  0xD7

#define MPQ8645P_DEBUGFS \
  "/sys/kernel/debug/pmbus/hwmon%d/mpq8645p/%s"

#define MPQ8645P_RESTORE_USER_ALL    "restore"
#define MPQ8645P_VOUT_COMMAND        "vout_command"
#define MPQ8645P_VOUT_SCALE_LOOP     "vout_scale_loop"
#define MPQ8645P_IOUT_OC_FAULT_LIMIT "oc_fault_limit"
#define MPQ8645P_IOUT_OC_WARN_LIMIT  "oc_warn_limit"
#define MPQ8645P_MFR_REVISION        "fw_version"
#define MPQ8645P_MFR_CTRL_COMP       "ctrl_comp"
#define MPQ8645P_MFR_CTRL_VOUT       "ctrl_vout"
#define MPQ8645P_MFR_CTRL_OPS        "ctrl_ops"
#define MPQ8645P_MFR_OC_PHASE_LIMIT  "oc_phase_limit"

struct config_data {
  char name[64];
  unsigned int value;
  uint8_t addr;
  uint8_t reg;
  uint8_t writable;
  uint8_t bytes;
};

struct mpq8645p_config {
  struct config_data *data;
  struct mpq8645p_config *next;
};

int mpq8645p_get_fw_ver(struct vr_info*, char*);
void* mpq8645p_parse_file(struct vr_info*, const char*);
int mpq8645p_validate_file(struct vr_info*, const char*);
int mpq8645p_fw_update(struct vr_info*, void*);
int mpq8645p_fw_verify(struct vr_info*, void*);

void mpq8645p_free_configs(struct mpq8645p_config*);

#endif
