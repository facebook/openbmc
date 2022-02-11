#ifndef __XDPE12284C_H__
#define __XDPE12284C_H__

#include "vr.h"

// XDPE12284C
#define VR_XDPE_PAGE_20 0x20
#define VR_XDPE_PAGE_32 0x32
#define VR_XDPE_PAGE_50 0x50
#define VR_XDPE_PAGE_60 0x60
#define VR_XDPE_PAGE_62 0x62

#define VR_XDPE_REG_REMAIN_WR 0x82  // page 0x50

#define VR_XDPE_REG_CRC_L 0x42     // page 0x62
#define VR_XDPE_REG_CRC_H 0x43     // page 0x62
#define VR_XDPE_REG_NEXT_MEM 0x65  // page 0x62

#define VR_XDPE_TOTAL_RW_SIZE 1080

struct xdpe_config {
  uint8_t addr;
  uint32_t crc_exp;
  uint8_t data[1200];
};

int get_xdpe_ver(struct vr_info*, char*);
void * xdpe_parse_file(struct vr_info*, const char*);
int xdpe_fw_update(struct vr_info*, void*);
int xdpe_fw_verify(struct vr_info*, void*);

#endif
