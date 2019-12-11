#ifndef __MPQ8645P_H__
#define __MPQ8645P_H__

#include "vr.h"

#define MPQ8645P_ID                      0x53503D
#define MPQ8645P_REG_OPERATION           0x01
#define MPQ8645P_REG_ON_OFF_CONFIG       0x02
#define MPQ8645P_REG_CLEAR_FAULTS        0x03
#define MPQ8645P_REG_WRITE_PROTECT       0x10
#define MPQ8645P_REG_STORE_USER_ALL      0x15
#define MPQ8645P_REG_RESTORE_USER_ALL    0x16
#define MPQ8645P_REG_CAPABILITY          0x19
#define MPQ8645P_REG_VOUT_MODE           0x20
#define MPQ8645P_REG_VOUT_COMMAND        0x21
#define MPQ8645P_REG_VOUT_MAX            0x24
#define MPQ8645P_REG_VOUT_MARGIN_HIGH    0x25
#define MPQ8645P_REG_VOUT_MARGIN_LOW     0x26
#define MPQ8645P_REG_VOUT_SCALE_LOOP     0x29
#define MPQ8645P_REG_MIN                 0x2B
#define MPQ8645P_REG_VIN_ON              0x35
#define MPQ8645P_REG_VIN_OFF             0x36
#define MPQ8645P_REG_IOUT_OC_FAULT_LIMIT 0x46
#define MPQ8645P_REG_IOUT_OC_WARN_LIMIT  0x4A
#define MPQ8645P_REG_OT_FAULT_LIMIT      0x4F
#define MPQ8645P_REG_OT_WARN_LIMIT       0x51
#define MPQ8645P_REG_VIN_OV_FAULT_LIMIT  0x55
#define MPQ8645P_REG_VIN_OV_WARN_LIMIT   0x57
#define MPQ8645P_REG_VIN_UV_WARN_LIMIT   0x58
#define MPQ8645P_REG_TON_DELAY           0x60
#define MPQ8645P_REG_TON_RISE            0x61
#define MPQ8645P_REG_STATUS_BYTE         0x78
#define MPQ8645P_REG_STATUS_WORD         0x79
#define MPQ8645P_REG_STATUS_VOUT         0x7A
#define MPQ8645P_REG_STATUS_IOUT         0x7B
#define MPQ8645P_REG_STATUS_INPUT        0x7C
#define MPQ8645P_REG_STATUS_TEMPERATURE  0x7D
#define MPQ8645P_REG_STATUS_CML          0x7E
#define MPQ8645P_REG_READ_VIN            0x88
#define MPQ8645P_REG_READ_VOUT           0x8B
#define MPQ8645P_REG_READ_IOUT           0x8C
#define MPQ8645P_REG_TEMPERATURE_1       0x8D
#define MPQ8645P_REG_PMBUS_REVISION      0x98
#define MPQ8645P_REG_MFR_ID              0x99
#define MPQ8645P_REG_MFR_MODEL           0x9A
#define MPQ8645P_REG_MFR_REVISION        0x9B
#define MPQ8645P_REG_MFR_4_DIGIT         0x9D
#define MPQ8645P_REG_MFR_CTRL_COMP       0xD0
#define MPQ8645P_REG_MFR_CTRL_VOUT       0xD1
#define MPQ8645P_REG_MFR_CTRL_OPS        0xD2
#define MPQ8645P_REG_MFR_ADD_PMBUS       0xD3
#define MPQ8645P_REG_MFR_VOUT_OVP_FALUT_LIMIT 0xD4
#define MPQ8645P_REG_MFR_OVP_NOCP_SET    0xD5
#define MPQ8645P_REG_MFR_OT_OC_SET       0xD6
#define MPQ8645P_REG_MFR_OC_PHASE_LIMIT  0xD7
#define MPQ8645P_REG_MFR_HICCUP_ITV_SET  0xD8
#define MPQ8645P_REG_MFR_PGOOD_ON_OFF    0xD9
#define MPQ8645P_REG_MFR_VOUT_STEP       0xDA
#define MPQ8645P_REG_MFR_LOW_POWER       0xE5
#define MPQ8645P_REG_MFR_CTRL            0xEA

struct config_data {
  char name[64];
  uint8_t value[8];
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
int mpq8645p_fw_update(struct vr_info*, void*);
int mpq8645p_fw_verify(struct vr_info*, void*);

void mpq8645p_free_configs(struct mpq8645p_config*);

#endif
