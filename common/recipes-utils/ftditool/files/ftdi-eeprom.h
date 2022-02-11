/*
 * ftditool
 *
 * Copyright 2021-present Facebook. All Rights Reserved.
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

#include <stdint.h>

#define FTDI_EEPROM_SIZE    256

union FTDI_EEPROM {
    unsigned char data[FTDI_EEPROM_SIZE];
    struct {
        uint8_t header:1;             //0      Read value and write the same value back to the device
        uint8_t useExtOsc:1;          //1      Use External Oscillator = ‘1’
        uint8_t highDriveIO:1;        //2      High Drive I/Os = '1'
        uint8_t loadDriver:1;         //3      '1' = load D2XX driver
        uint8_t reserve_01_b7_4:4;    //7-4    reserve
        uint16_t endpoint_size:7;     //14-8   endpoint_size
        uint8_t reserve_01_b15:1;     //15     reserve

        uint16_t VendorID;
        uint16_t ProductID;
        uint16_t release_number;
        uint8_t config_des;
        uint8_t max_power;

        uint8_t reserve_0a_b1_0:2;
        uint8_t pulldown_en:1;
        uint8_t do_serialnumber:1;
        uint8_t reserve_0a_b7_4:4;
        uint8_t Invert_TXD:1;  // non-zero if invert TXD
        uint8_t Invert_RXD:1;  // non-zero if invert RXD
        uint8_t Invert_RTS:1;  // non-zero if invert RTS
        uint8_t Invert_CTS:1;  // non-zero if invert CTS
        uint8_t Invert_DTR:1;  // non-zero if invert DTR
        uint8_t Invert_DSR:1;  // non-zero if invert DSR
        uint8_t Invert_DCD:1;  // non-zero if invert DCD
        uint8_t Invert_RI:1;   // non-zero if invert RI

        uint16_t usb_version;

        uint8_t ManString_ptr;
        uint8_t ManString_len;
        uint8_t PrdString_ptr;
        uint8_t PrdString_len;
        uint8_t SerString_ptr;
        uint8_t SerString_len;

        uint8_t cbus0:4;  // Cbus Mux control
        uint8_t cbus1:4;  // Cbus Mux control
        uint8_t cbus2:4;  // Cbus Mux control
        uint8_t cbus3:4;  // Cbus Mux control
        uint8_t cbus4:4;  // Cbus Mux control
        uint8_t reserve_16_b7_4:4;
        uint8_t reserve_17;

        uint64_t reserve_18_1F;
        uint64_t reserve_20_27;
        uint64_t reserve_28_2F;
        uint64_t reserve_30_37;
        uint64_t reserve_38_3F;
        uint64_t reserve_40_47;
        uint64_t reserve_48_4F;
        uint64_t reserve_50_57;
        uint64_t reserve_58_5F;
        uint64_t reserve_60_67;
        uint64_t reserve_68_6F;
        uint64_t reserve_70_77;
        uint32_t reserve_78_7B;
        uint16_t reserve_7C_7D;
        uint16_t checksum;
    } info;
};
