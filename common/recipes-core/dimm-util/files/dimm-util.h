/*
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __DIMM_UTIL_H__
#define __DIMM_UTIL_H__

#define PLAT_EOK       0
//#define PLAT_ENOTREADY -1

#define MAX_CPU_NUM       2  // max number of CPUs
#define MAX_DIMM_PER_CPU 12 //  max number of dimms per CPU

#define OFFSET_TYPE         0x2
#define OFFSET_SPD5_DIMM_SN 0x205
#define OFFSET_SPD5_DIMM_PN 0x209
#define LEN_SPD5_DIMM_PN    30
#define LEN_SIZE_STRING     32
#define LEN_SPEED_STRING    16
#define LEN_MFG_STRING      32
#define SPD5_DUMP_LEN       1024

#define OFFSET_SERIAL 0x45
#define LEN_SERIAL    4
#define LEN_SERIAL_STRING ((LEN_SERIAL * 2) + 1) // 2 hex digit per byte + null
#define OFFSET_PART_NUMBER 0x49
#define LEN_PART_NUMBER    20
#define LEN_PN_STRING      (LEN_SPD5_DIMM_PN + 1)

#define DEFAULT_DUMP_OFFSET 0
#define DEFAULT_DUMP_LEN    0x100
#define MAX_RETRY 3
#define MAX_FAIL_CNT LEN_SERIAL
#define MAX_SPD_READ_LEN 32

#define ERR_INVALID_SYNTAX -2

#define INTEL_ID_LEN  3
#define MANU_INTEL_0  0x57
#define MANU_INTEL_1  0x01
#define MANU_INTEL_2  0x00

#define FACEBOOK_ID_LEN  3
#define MANU_FACEBOOK_0  0x15
#define MANU_FACEBOOK_1  0xa0
#define MANU_FACEBOOK_2  0x00

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof((_a)[0]))
#endif /* ARRAY_SIZE */

#define PKG_RANK(x) (x + 1)



extern char *vendor_name[];
extern const char * mfg_string(uint16_t id);

extern int num_frus;
extern int num_cpus;
extern int num_dimms_per_cpu;
extern int total_dimms;
extern char const **fru_name;
extern int fru_id_all;
extern int fru_id_min;
extern int fru_id_max;
extern bool read_block;

int get_die_capacity(uint8_t data);
int get_bus_width_bits(uint8_t data);
int get_device_width_bits(uint8_t data);

int get_spd5_dimm_vendor(uint8_t fru_id, uint8_t cpu, uint8_t dimm, char *mfg_str);
int get_spd5_reg_vendor(uint8_t fru_id, uint8_t cpu, uint8_t dimm, char *mfg_str);
int get_spd5_pmic_vendor(uint8_t fru_id, uint8_t cpu, uint8_t dimm, char *mfg_str);
int get_spd5_dimm_mfg_date(uint8_t fru_id, uint8_t cpu, uint8_t dimm, char *date_str);
int get_spd5_dimm_size(uint8_t fru_id, uint8_t cpu, uint8_t dimm, char *size_str);
int get_spd5_dimm_speed(uint8_t fru_id, uint8_t cpu, uint8_t dimm, char *speed_str);
int util_read_spd_with_retry(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint16_t offset, uint16_t len,
                             uint16_t early_exit_cnt, uint8_t *buf, uint8_t *present);

// util functions to be provided by each platform
int util_check_me_status(uint8_t fru_id);
int util_set_EE_page(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint8_t page_num);
int util_read_spd_byte(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint8_t offset);
int util_read_spd(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint16_t offset, uint8_t len, uint8_t *rxbuf);
int plat_init();
const char * get_dimm_label(uint8_t cpu, uint8_t dimm);
uint8_t get_dimm_cache_id(uint8_t cpu, uint8_t dimm);

#endif
