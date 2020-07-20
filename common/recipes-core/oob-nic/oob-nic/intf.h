/*
 * Copyright 2014-present Facebook. All Rights Reserved.
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
#ifndef INTF_H
#define INTF_H

#include <stdint.h>

typedef struct oob_intf_t oob_intf;

oob_intf* oob_intf_create(const char *name, const uint8_t mac[6]);
int oob_intf_get_fd(const oob_intf *intf);

int oob_intf_receive(const oob_intf *intf, char *buf, int len);
int oob_intf_send(const oob_intf *intf, const char *buf, int len);

#endif
