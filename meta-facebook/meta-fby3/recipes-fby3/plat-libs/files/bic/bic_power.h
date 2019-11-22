/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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

#ifndef __BIC_POWER_H__
#define __BIC_POWER_H__

#include "bic_xfer.h"

#ifdef __cplusplus
extern "C" {
#endif

int bic_server_power_on(uint8_t slot_id);
int bic_server_power_off(uint8_t slot_id);
int bic_server_graceful_power_off(uint8_t slot_id);
int bic_server_power_reset(uint8_t slot_id);
int bic_server_power_cycle(uint8_t slot_id);
int bic_get_server_power_status(uint8_t slot_id, uint8_t *power_status);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_POWER_H__ */
