/* Copyright 2015-present Facebook. All Rights Reserved.
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

#ifndef __LIGHTNING_FLASH_H__
#define __LIGHTNING_FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

enum i2c_mux_list {
  I2C_MUX_FLASH1,
  I2C_MUX_FLASH2,
};

enum i2c_mux_addr {
  I2C_MUX_FLASH1_ADDR = 0x71,
  I2C_MUX_FLASH2_ADDR = 0x72,
};

enum m2_mux_channel {
  M2_MUX_CHANNEL_0 = 0x01,
  M2_MUX_CHANNEL_1 = 0x02,
};

/*
 * Map the flash i2c bus to i2c mux channels
 * If the assigned value is XY,
 * X represents the i2c mux
 * Y represents the channel of that mux
 */
enum i2c_flash_map {
  I2C_MAP_FLASH0 = 12,
  I2C_MAP_FLASH1 = 11,
  I2C_MAP_FLASH2 = 10,
  I2C_MAP_FLASH3 = 00,
  I2C_MAP_FLASH4 = 01,
  I2C_MAP_FLASH5 = 13,
  I2C_MAP_FLASH6 = 15,
  I2C_MAP_FLASH7 = 17,
  I2C_MAP_FLASH8 = 03,
  I2C_MAP_FLASH9 = 05,
  I2C_MAP_FLASH10 = 14,
  I2C_MAP_FLASH11 = 16,
  I2C_MAP_FLASH12 = 02,
  I2C_MAP_FLASH13 = 04,
  I2C_MAP_FLASH14 = 06,
};


extern const uint8_t lightning_flash_list[];

extern size_t lightning_flash_cnt;

int lightning_flash_mux_sel_chan(uint8_t mux, uint8_t channel);
int lightning_flash_sec_mux_sel_chan(uint8_t mux, uint8_t channel);
int lightning_flash_temp_read(uint8_t i2c_map, float *temp);
int lightning_m2_amb_temp_read(uint8_t i2c_map, float *temp);
int lightning_m2_flash_temp_read(uint8_t i2c_map, float *temp);
int lightning_m2_temp_read(uint8_t i2c_map, uint8_t sec_chan, float *temp);
int lightning_u2_flash_temp_read(uint8_t i2c_map, float *temp);
int lightning_nvme_temp_read(uint8_t mux, float *temp);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __LIGHTNING_FLASH_H__ */
