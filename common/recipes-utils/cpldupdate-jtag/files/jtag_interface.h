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

#ifndef __JTAG_INTERFACE_H__
#define __JTAG_INTERFACE_H__

#include <linux/types.h>

#include "linux/jtag.h"

#ifdef __cplusplus
extern "C" {
#endif

int jtag_interface_open(const char *dev_name);
void jtag_interface_close(int jtag_fd);
int jtag_interface_set_freq(int jtag_fd, unsigned int freq);
unsigned int jtag_interface_get_freq(int jtag_fd);

int jtag_interface_run_test_idle(int jtag_fd, 
                                 unsigned char reset, 
                                 unsigned char endstate, 
                                 unsigned char tck);

int jtag_interface_set_xfer_mode(int jtag_fd, unsigned int mode);
int jtag_interface_get_status(int jtag_fd, enum jtag_endstate *endstate);

int jtag_interface_xfer(int jtag_fd, 
                        unsigned char type,
                        unsigned char direction,
                        unsigned char endstate,
                        unsigned int length,
                        unsigned int tdio);



#ifdef __cplusplus
}
#endif

#endif /* __JTAG_INTERFACE_H__ */
