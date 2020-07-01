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
#ifndef NIC_H
#define NIC_H

#include <stdint.h>

#include "nic_defs.h"

typedef struct oob_nic_t oob_nic;

oob_nic* oob_nic_open(int bus, uint8_t addr);
void oob_nic_close(oob_nic* dev);

/* MAC */
int oob_nic_get_mac(oob_nic *dev, uint8_t mac[6]);

/* Status */
typedef struct oob_nic_status_t oob_nic_status;
int oob_nic_get_status(oob_nic *dev, oob_nic_status *status);

int oob_nic_start(oob_nic *dev, const uint8_t mac[6]);
int oob_nic_stop(oob_nic *dev);

int oob_nic_send(oob_nic *dev, const uint8_t *data, int len);

int oob_nic_receive(oob_nic *dev, uint8_t *buf, int len);

#endif
