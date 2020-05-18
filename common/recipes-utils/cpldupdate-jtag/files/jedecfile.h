/* Jedecfile .jed file parser

Copyright (C) Uwe Bonnes 2009 bon@elektron.ikp.physik.tu-darmstadt.de

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/*
 * Using a slightly corrected version from IPAL libjedec
 * Copyright (c) 2000 Stephen Williams (steve@icarus.com)
 */ 

#ifndef JEDECFILE_H
#define JEDECFILE_H

#include <string.h>

#define MAX_ITEM   8
#define MAX_SIZE 256

#define JED_XC95X 0
#define JED_XC2C  1
#define JED_XC95  2

typedef unsigned char byte;

struct jedec_data {
    char device[MAX_SIZE];
    char version[MAX_SIZE];
    char date[MAX_SIZE];

    unsigned fuse_count;
    unsigned pin_count;
    unsigned cfg_count;
    unsigned checksum;
    unsigned char fuse_default;
    unsigned char *fuse_list;
};
typedef struct jedec_data *jedec_data_t;

#ifdef __cplusplus__
extern "C" {
#endif

void jedec_init();
void jedec_uninit(void);
int jedec_read_file(FILE *fp);
unsigned int jedec_get_length();
unsigned short jedec_get_checksum();
char *jedec_get_device();
char *jedec_get_version();
char *jedec_get_date();
unsigned short jedec_calc_checksum();
void jedec_set_length(unsigned int fuse_count);
int jedec_get_fuse(unsigned int idx);
void jedec_set_fuse(unsigned int idx, int blow);
void jedec_save_as_jed(const char *device, FILE *fp);
unsigned char jedec_get_byte(unsigned int idx);
unsigned int jedec_get_long(unsigned int idx);
int jedec_get_cfg_fuse();
// TODO: to be implemented to support more feature
int jedec_get_user_code(unsigned int *usercode);
int jedec_get_feature_row(unsigned char *buff, unsigned int *length);
int jedec_get_feature_bits(unsigned char *buff, unsigned int *length);

#ifdef __cplusplus__
}
#endif

#endif //JEDECFILE_H
