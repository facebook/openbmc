#ifndef __RAAGEN3P5_H__
#define __RAAGEN3P5_H__

#include "vr.h"

static uint32_t raa_gen3p5_supported_dev_ids[] = {
    0x49D2BA00, 0x49D2BB00, 0x49D2BC00, 0x49D2D600, 0x49D2DA00, 0x49D2BD00,
    0x49D2BE00, 0x49D2D500, 0x49D2C000, 0x49D2D700, 0x49D2C100, 0x49D2D800,
    0x49D2DB00, 0x49D2C400, 0x49D2C500, 0x49D2C600, 0x49D2C700, 0x49D2C800,
    0x49D2C900, 0x49D2DE00, 0x49D2F100, 0x49D2DF00, 0x49D2CA00};

// RAA GEN3.5
#define RAA_GEN3P5_CMD_DMAADDR 0xC7
#define RAA_GEN3P5_CMD_DMAFIX 0xC5

#define RAA_GEN3P5_DMAADDR_DISABLE_PKT_CAP 0x0102
#define RAA_GEN3P5_DMAADDR_REMAIN_WR 0x0035
#define RAA_GEN3P5_DMAADDR_PROGRAMMER_STATUS 0x0083
#define RAA_GEN3P5_DMAADDR_BANK_STATUS 0x0084
#define RAA_GEN3P5_DMAADDR_CRC 0x00F8

#define RAA_GEN3P5_CMD_IC_DEVICE_ID 0xAD
#define RAA_GEN3P5_IC_DEVICE_ID_LEN 4

#define RAA_GEN3P5_CMD_IC_DEVICE_REV 0xAE
#define RAA_GEN3P5_IC_DEVICE_REV_LEN 4

#define RAA_GEN3P5_CRC_LEN 4

#define RAA_GEN3P5_CONFIG_PDATA_LEN 1024

#define RAA_GEN3P5_FW_CRC_LINE (336)

struct raa_gen3p5_data {
  union {
    uint8_t raw[32];
    struct {
      uint8_t addr;
      uint8_t cmd;
      uint8_t data[];
    } __attribute__((packed));
  };
  uint8_t len; // [cmd] ~ data[N]
  uint8_t pec;
};

struct raa_gen3p5_config {
  uint8_t addr;
  uint16_t wr_cnt;
  uint32_t devid_exp;
  uint32_t rev_exp;
  uint32_t crc_exp;
  struct raa_gen3p5_data pdata[RAA_GEN3P5_CONFIG_PDATA_LEN];
};

int get_raa_gen3p5_ver(struct vr_info*, char*);
void* raa_gen3p5_parse_file(struct vr_info*, const char*);
int raa_gen3p5_fw_update(struct vr_info*, void*);

#endif
