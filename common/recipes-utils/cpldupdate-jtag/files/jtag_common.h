/*
 * Copyright 2019-present Facebook. All Rights Reserved.
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

#ifndef __JTAG_COMMON_H__
#define __JTAG_COMMON_H__

#include "jtag_interface.h"


typedef struct {
    int fd;
    int mode;           /* hardware mode or software mode */
    unsigned int freq;  /* working frequent */
}jtag_object_t;


#ifdef __cplusplus
extern "C" {
#endif

jtag_object_t *jtag_init(const char *if_name, unsigned char mode, unsigned int freq);

void jtag_uninit(jtag_object_t *pobj);

int jtag_write_data_register(jtag_object_t *pobj, 
                             unsigned int endstate,
                             unsigned int *buf,
                             int length);

int jtag_write_instruction_register(jtag_object_t *pobj, 
                                    unsigned int endstate,
                                    unsigned int *buf,
                                    int length);

int jtag_read_data_register(jtag_object_t *pobj, 
                            unsigned int endstate,
                            unsigned int *buf,
                            int length);

int jtag_get_status(jtag_object_t *pobj, enum jtag_endstate *endstate);

int jtag_run_test_idle(jtag_object_t *pobj, 
                       unsigned char reset,
                       unsigned char endstate,
                       unsigned char tck);


#ifdef __cplusplus
}
#endif
#endif /* __JTAG_COMMON_H__ */

