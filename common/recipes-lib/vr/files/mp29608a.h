#ifndef __mp29608a_H__
#define __mp29608a_H__

// Support MP2869, MP29612, MP29608, MP2869A, MP29612A, MP29608A

#include "vr.h"

#define MAX_REG_DATA_LEN 4
#define MAX_CRC_LEN 2
#define MAX_ATE_DATA_NUM 1024

struct mp29608a_data {
	uint16_t cfg_id;
	uint8_t page;
	uint8_t reg_addr;
	uint8_t reg_data[MAX_REG_DATA_LEN];
	uint8_t reg_data_len;
};

struct mp29608a_config {
	uint8_t bus;
	uint8_t addr;
	uint16_t cfg_id;
	uint16_t wr_cnt;
	uint32_t product_id_exp;
	uint8_t crc_code[MAX_CRC_LEN];
	struct mp29608a_data pdata[MAX_ATE_DATA_NUM];
};

void *mp29608a_parse_file(struct vr_info *, const char *);
int mp29608a_fw_update(struct vr_info *, void *);
int get_mp29608a_ver(struct vr_info *, char *);

#endif
