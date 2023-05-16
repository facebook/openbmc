#ifndef __MP2856_H__
#define __MP2856_H__

// Support MP2856,MP2857,MP2971,MP2973

#include "vr.h"

#define VR_MPS_PAGE_0    0x00
#define VR_MPS_PAGE_1    0x01
#define VR_MPS_PAGE_2    0x02
#define VR_MPS_PAGE_3    0x03
#define VR_MPS_PAGE_29   0x29
#define VR_MPS_PAGE_2A   0x2A



/*Page0 */
#define VR_MPS_REG_WRITE_PROTECT      0x10
#define VR_MPS_REG_PRODUCT_ID         0x9A
#define VR_MPS_REG_CONFIG_ID          0x9D
#define VR_MPS_REG_STATUS_CML         0x7E

#define VR_MPS_CMD_STORE_NORMAL_CODE  0xF1
#define VR_MPS_CMD_STORE_NORMAL_CODE_2993  0x15

/*Page1 */
#define VR_MPS_REG_MTP_PAGE_EN_2993   0x04
#define VR_MPS_REG_MFR_VR_CONFIG2     0x35
#define VR_MPS_REG_MFR_MTP_PMBUS_CTRL 0x4F
#define VR_MPS_REG_CRC_NORMAL_CODE_2856    0xAD
#define VR_MPS_REG_CRC_MULTI_CONFIG_2856   0xAF
#define VR_MPS_REG_CRC_NORMAL_CODE_297X    0xAB
#define VR_MPS_REG_CRC_MULTI_CONFIG_297X   0xAD
#define VR_MPS_REG_MTP_PAGE_EN 0x04

/*Page2 */
#define VR_MPS_CMD_STORE_MULTI_CODE   0xF3

/*Page29 */
#define VR_MPS_REG_CRC_USER           0xFF

/*Page2A */
#define VR_MPS_REG_MULTI_CONFIG       0xBF

/*Page3 */
#define VR_MPS_REG_PAGE_PLUS_WRITE    0x05
#define VR_MPS_REG_DISABLE_STORE_FAULT_TRIGGERING 0x51

/*CRC for MP2993 */
#define VR_MPS_REG_CRC_2993           0xB8

/*CRC for MP2985H */
#define VR_MPS_REG_CRC_2985H           0xB8

/* STATUS_CML bit[3] */
#define MASK_PWD_MATCH                0x08
/* MFR_VR_CONFIG2 bit[2] */
#define  MASK_WRITE_PROTECT_MODE      0x04
/* MFR_MTP_PMBUS_CTRL bit[5] */
#define  MASK_MTP_BYTE_RW_EN          0x20
#define  MASK_MTP_BYTE_RW_EN_2993     0x80
#define  MASK_MTP_BYTE_RW_LOCK_2993   0x7f
#define MASK_EN_TEST_MODE_BYTE 0x80
#define MASK_DIS_TEST_MODE_BYTE 0x7f

#define MP2856_DISABLE_WRITE_PROTECT 0x63
#define MP2993_DISABLE_WRITE_PROTECT 0x00

#define MP2985H_DISABLE_STORE_FAULT_TRIGGERING_BYTE0 0x10
#define MP2985H_DISABLE_STORE_FAULT_TRIGGERING_BYTE1 0x00

#define MP2856_PRODUCT_ID 0x2856
#define MP2857_PRODUCT_ID 0x2857
#define MP2993_PRODUCT_ID 0x2993
#define MP2985H_MODULE_ID 0x0185

#define MAX_MP2856_REMAIN_WR 1000
#define MAX_MP2985H_REMAIN_WR 100

#define MAX_REMAIN_WR_MSG 64

enum {
  BYTE_WRITE_READ = 0,
  WORD_WRITE_READ ,
  BLOCK_WRITE_2_BYTE,
};

enum {
  ATE_CONF_ID = 0,
  ATE_PAGE_NUM,
  ATE_REG_ADDR_HEX,
  ATE_REG_ADDR_DEC,
  ATE_REG_NAME,
  ATE_REG_DATA_HEX,
  ATE_REG_DATA_DEC,
  ATE_WRITE_TYPE,
  ATE_COL_MAX,
};

struct mp2856_data {
  uint16_t cfg_id;
  uint8_t page;
  uint8_t reg_addr;
  uint8_t reg_data[4];
  uint8_t reg_len;
};

// This config is for all MPS model
struct mp2856_config {
  uint8_t mode;
  uint8_t addr;
  uint16_t cfg_id;
  uint16_t wr_cnt;
  uint16_t product_id_exp;
  uint16_t crc_code[2];
  struct mp2856_data pdata[1024];
};

enum config_mode {
  MP285X,
  MP297X,
  MP2993,
  MP2985H,
};

void* mp2856_parse_file(struct vr_info*, const char*);
int mp2856_fw_update(struct vr_info*, void*);
int get_mp2856_ver(struct vr_info*, char*);
int get_mp2993_ver(struct vr_info*, char*);
int get_mp2985h_ver(struct vr_info*, char*);

#endif
