/*
 * Copyright 2020-present Facebook. All Rights Reserved.
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

/*
 * This file is intended to contain platform specific declarations, and
 * it's set to empty intentionally. Each platform needs to override the
 * file if platform specific settings are required.
 */
#ifndef _PEM_PLATFORM_H_
#define _PEM_PLATFORM_H_

#error "please implement the platform PEM define"

/*
* Sample:
* It needs to be configured under the corresponding platform
*/
#define PEM1_EEPROM  I2C_SYSFS_DEV_ENTRY(49-0050, eeprom)
#define PEM2_EEPROM  I2C_SYSFS_DEV_ENTRY(48-0050, eeprom)
#define PEM3_EEPROM  I2C_SYSFS_DEV_ENTRY(57-0050, eeprom)
#define PEM4_EEPROM  I2C_SYSFS_DEV_ENTRY(56-0050, eeprom)
i2c_info_t pem[] = {
  {-1, 48, {0x58, 0x18, 0x50}, {PEM_LTC4282_REG_CNT, PEM_MAX6615_REG_CNT, 0},
   {PEM_BLACKBOX(1, ltc4282), PEM_BLACKBOX(1, max6615), PEM1_EEPROM}},
  {-1, 49, {0x58, 0x18, 0x50}, {PEM_LTC4282_REG_CNT, PEM_MAX6615_REG_CNT, 0},
   {PEM_BLACKBOX(2, ltc4282), PEM_BLACKBOX(2, max6615), PEM2_EEPROM}},
  {-1, 56, {0x58, 0x18, 0x50}, {PEM_LTC4282_REG_CNT, PEM_MAX6615_REG_CNT, 0},
   {PEM_BLACKBOX(1, ltc4282), PEM_BLACKBOX(1, max6615), PEM3_EEPROM}},
  {-1, 57, {0x58, 0x18, 0x50}, {PEM_LTC4282_REG_CNT, PEM_MAX6615_REG_CNT, 0},
   {PEM_BLACKBOX(2, ltc4282), PEM_BLACKBOX(2, max6615), PEM4_EEPROM}},
};

#endif