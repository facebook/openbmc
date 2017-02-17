/* Copyright 2017-present Facebook. All Rights Reserved.
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

#ifndef __NVME_MI_H__
#define __NVME_MI_H__

typedef struct {
  uint8_t sflgs;            //Status Flags
  uint8_t warning;          //SMART Warnings
  uint8_t temp;             //Composite Temperature
  uint8_t pdlu;		    //Percentage Drive Life Used
  uint8_t serial_num[20]    //Serial Number
} ssd_data;

int nvme_read_byte(const char *device, uint8_t item, uint8_t *value);
int nvme_sflgs_read(const char *device, uint8_t *value);
int nvme_smart_warning_read(const char *device, uint8_t *value);
int nvme_temp_read(const char *device, uint8_t *value);
int nvme_pdlu_read(const char *device, uint8_t *value);
int nvme_serial_num_read(const char *device, uint8_t *value, int size);

#endif
