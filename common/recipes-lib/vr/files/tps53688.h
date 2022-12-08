#ifndef __TPS53688_H__
#define __TPS53688_H__

#include "vr.h"

// TPS53688
#define VR_TPS_REG_DEVID   0xAD
#define VR_TPS_REG_CRC     0xF0
#define VR_TPS_REG_NVM_IDX 0xF5
#define VR_TPS_REG_NVM_EXE 0xF6

// TPS53689
#define VR_TPS_REG_CRC2    0xF4
#define VR_TPS53689_DEVID  0x544953689000

#define VR_TPS_OFFS_CRC 10

#define VR_TPS_FIRST_BLK_LEN 21
#define VR_TPS_BLK_LEN       32
#define VR_TPS_BLK_WR_LEN    (VR_TPS_BLK_LEN+1)
#define VR_TPS_LAST_BLK_LEN  9
#define VR_TPS_NVM_IDX_NUM   9
#define VR_TPS_DEVID_LEN     6
#define VR_TPS_TOTAL_WR_SIZE (VR_TPS_BLK_WR_LEN * VR_TPS_NVM_IDX_NUM)

struct tps_config {
  uint8_t addr;
  uint16_t crc_exp;
  uint64_t devid_exp;
  uint8_t data[512];
};

typedef enum{
  GET_VR_CRC = 0,
  UPDATE_VR_CRC,
} VR_CRC_OPERATION;

int get_tps_ver(struct vr_info*, char*);
void * tps_parse_file(struct vr_info*, const char*);
int tps_fw_update(struct vr_info*, void*);
int tps_fw_verify(struct vr_info*, void*);

#endif
