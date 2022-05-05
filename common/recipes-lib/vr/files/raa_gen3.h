#ifndef __RAAGEN3_H__
#define __RAAGEN3_H__

// Support ISL69260, RAA228XXX, RAA229XXX

#include "vr.h"

// RAA GEN3_
#define VR_RAA_REG_MCU_VER              0x30
#define VR_RAA_REG_REMAIN_WR            0x35
#define VR_RAA_REG_DMA_ADDR             0xC7
#define VR_RAA_REG_DMA_DATA             0xC5
#define VR_RAA_REG_PROG_STATUS          0x7E
#define VR_RAA_REG_CRC                  0x94
#define VR_RAA_REG_DEVID                0xAD
#define VR_RAA_REG_DEVREV               0xAE
#define VR_RAA_REG_RESTORE_CFG          0xF2

#define VR_RAA_REG_HEX_MODE_CFG0        0x87
#define VR_RAA_REG_HEX_MODE_CFG1        0xBD

#define VR_RAA_REV_MIN                  0x06
#define VR_RAA_DEV_ID_LEN               4
#define VR_RAA_DEV_REV_LEN              4
#define VR_RAA_CHECKSUM_LEN             4

#define VR_RAA_CFG_ID                   (7)
#define VR_RAA_LEGACY_CRC               (271)
#define VR_RAA_PRODUCTION_CRC           (285)

struct raa_data {
  union {
    uint8_t raw[32];
    struct {
      uint8_t addr;
      uint8_t cmd;
      uint8_t data[];
    } __attribute__((packed));
  };
  uint8_t len;  // [cmd] ~ data[N]
  uint8_t pec;
};

struct raa_config {
  uint8_t addr;
  uint8_t mode;
  uint8_t cfg_id;
  uint16_t wr_cnt;
  uint32_t devid_exp;
  uint32_t rev_exp;
  uint32_t crc_exp;
  struct raa_data pdata[512];
};

enum {
  RAA_LEGACY,
  RAA_PRODUCTION,
};

int get_raa_ver(struct vr_info*, char*);
void * raa_parse_file(struct vr_info*, const char*);
int raa_fw_update(struct vr_info*, void*);
int raa_fw_verify(struct vr_info*, void*);

#endif
