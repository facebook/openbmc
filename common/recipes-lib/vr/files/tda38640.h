#ifndef __TDA38640_H__
#define __TDA38640_H__

#include "vr.h"

#define TDA38640_CNFG_BYTE_NUM 4
#define TDA38640_MAX_SECT_NUM 48
#define TDA38640_SECT_COLUMN_NUM 64

enum {
  CRC_LOW_REG = 0xAE,
  CRC_HIGH_REG = 0xB0,
  CNFG_REG = 0xB2,
  USER_1_REG = 0xB4,
  USER_2_REG = 0xB6,
  USER_3_REG = 0xB8,
  UNLOCK_REGS_REG = 0xD4,
  PROG_CMD_REG = 0xD6,
  PAGE_REG = 0xff,
};

enum {
  CNFG_WR = 0x12,
  USER_RD = 0x41,
  USER_WR = 0x42,
};

struct tda38640_config_sect {
  uint16_t offset[TDA38640_SECT_COLUMN_NUM];
  uint8_t offset_count;
  uint8_t data[TDA38640_SECT_COLUMN_NUM * 16];
};

struct tda38640_config {
  uint32_t checksum;
  uint8_t cnfg_data[TDA38640_CNFG_BYTE_NUM];
  uint8_t sect_count;
  struct tda38640_config_sect section[TDA38640_MAX_SECT_NUM];
};

int get_tda38640_ver(struct vr_info*, char*);
void * tda38640_parse_file(struct vr_info*, const char*);
int tda38640_fw_update(struct vr_info*, void*);

#endif
