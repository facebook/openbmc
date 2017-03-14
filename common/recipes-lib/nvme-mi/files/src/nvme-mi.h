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
  uint8_t pdlu;		        //Percentage Drive Life Used
  uint16_t vendor;        //Vendor ID
  uint8_t serial_num[20];   //Serial Number
} ssd_data;

typedef struct {
  char key[32];
  char value[28];
} t_key_value_pair;

// For Byte 1 - Status Flag. 
typedef struct {
  t_key_value_pair self; //"Status Flag" and it's raw data
  t_key_value_pair read_complete;
  t_key_value_pair ready;
  t_key_value_pair functional;
  t_key_value_pair reset_required;
  t_key_value_pair port0_link;
  t_key_value_pair port1_link;
} t_status_flags;

// For Byte 2 - SMART critical warning 
typedef struct{
t_key_value_pair self; // SMART Critical Warning and it's raw data
t_key_value_pair spare_space;
t_key_value_pair temp_warning;
t_key_value_pair reliability;
t_key_value_pair media_status;
t_key_value_pair backup_device;
} t_smart_warning; 

int nvme_read_byte(const char *i2c_bus, uint8_t item, uint8_t *value);
int nvme_read_word(const char *i2c_bus, uint8_t item, uint16_t *value);
int nvme_sflgs_read(const char *i2c_bus, uint8_t *value);
int nvme_smart_warning_read(const char *i2c_bus, uint8_t *value);
int nvme_temp_read(const char *i2c_bus, uint8_t *value);
int nvme_pdlu_read(const char *i2c_bus, uint8_t *value);
int nvme_vendor_read(const char *i2c_bus, uint16_t *value);
int nvme_serial_num_read(const char *i2c_bus, uint8_t *value, int size);

int nvme_sflgs_read_decode(const char *i2c_bus, uint8_t *value, t_status_flags *status_flag_decoding);
int nvme_smart_warning_read_decode(const char *i2c_bus, uint8_t *value, t_smart_warning *smart_warning_decoding);
int nvme_temp_read_decode(const char *i2c_bus, uint8_t *value, t_key_value_pair *temp_decoding);
int nvme_pdlu_read_decode(const char *i2c_bus, uint8_t *value, t_key_value_pair *pdlu_decoding);
int nvme_vendor_read_decode(const char *i2c_bus, uint16_t *value, t_key_value_pair *vendor_decoding);
int nvme_serial_num_read_decode(const char *i2c_bus, uint8_t *value, int size, t_key_value_pair *sn_decoding);

#endif
