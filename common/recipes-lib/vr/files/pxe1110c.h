#ifndef __PXE1110C_H__
#define __PXE1110C_H__

#include "vr.h"

// PXE1110C
#define VR_PXE_PAGE_20 0x20
#define VR_PXE_PAGE_3F 0x3F
#define VR_PXE_PAGE_50 0x50
#define VR_PXE_PAGE_60 0x60
#define VR_PXE_PAGE_6F 0x6F

#define VR_PXE_REG_REMAIN_WR 0x82  // page 0x50

#define VR_PXE_REG_CRC_L 0x3D  // page 0x6F
#define VR_PXE_REG_CRC_H 0x3E  // page 0x6F

#define VR_PXE_TOTAL_RW_SIZE 2040

struct pxe_config {
  uint8_t addr;
  uint32_t crc_exp;
  uint8_t data[2048];
};

int get_pxe_ver(struct vr_info*, char*);
void * pxe_parse_file(struct vr_info*, const char*);
int pxe_fw_update(struct vr_info*, void*);
int pxe_fw_verify(struct vr_info*, void*);

#endif
