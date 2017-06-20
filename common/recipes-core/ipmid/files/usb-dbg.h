/*
 *
 * Copyright 2016-present Facebook. All Rights Reserved.
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

#ifndef __USB-DBG_H__
#define __USB-DBG_H__

int plat_udbg_get_frame_info(uint8_t *num);
int plat_udbg_get_updated_frames(uint8_t *count, uint8_t *buffer);
int plat_udbg_get_post_desc(uint8_t index, uint8_t *next, uint8_t *end, uint8_t *count, uint8_t *buffer);
int plat_udbg_get_gpio_desc(uint8_t index, uint8_t *next, uint8_t *level, uint8_t *def,
                            uint8_t *count, uint8_t *buffer);
int plat_udbg_get_frame_data(uint8_t frame, uint8_t page, uint8_t *next, uint8_t *count, uint8_t *buffer);
int plat_udbg_control_panel(uint8_t panel, uint8_t operation, uint8_t item, uint8_t *count, uint8_t *buffer);

#endif /* __USB-DBG_H__ */
