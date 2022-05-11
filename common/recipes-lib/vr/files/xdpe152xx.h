#ifndef __XDPE152XX_H__
#define __XDPE152XX_H__

#include "vr.h"

#define MAX_SECT_DATA_NUM 200
#define MAX_SECT_NUM      16

struct config_sect {
  uint8_t type;
  uint16_t data_cnt;
  uint32_t data[MAX_SECT_DATA_NUM];
};

struct xdpe152xx_config {
  uint8_t addr;
  uint8_t sect_cnt;
  uint16_t total_cnt;
  uint32_t sum_exp;
  struct config_sect section[MAX_SECT_NUM];
};

int get_xdpe152xx_ver(struct vr_info*, char*);
void * xdpe152xx_parse_file(struct vr_info*, const char*);
int xdpe152xx_fw_update(struct vr_info*, void*);
int xdpe152xx_fw_verify(struct vr_info*, void*);

#endif
