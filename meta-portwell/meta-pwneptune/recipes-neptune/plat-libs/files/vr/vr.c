/*
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/kv.h>
#include "vr.h"

#define VR_BUS_ID 0x5

#define VR_FW_PAGE 0x2f
#define VR_FW_PAGE_2 0x6F
#define VR_FW_PAGE_3 0x4F

#define VR_FW_REG1 0xC
#define VR_FW_REG2 0xD
#define VR_FW_REG3 0x3D
#define VR_FW_REG4 0x3E
#define VR_FW_REG5 0x32

#define VR_TELEMETRY_VOLT 0x1A
#define VR_TELEMETRY_CURR 0x15
#define VR_TELEMETRY_POWER 0x2D
#define VR_TELEMETRY_TEMP 0x29

#define DATA_START_ADDR 18
#define VR_UPDATE_IN_PROGRESS "/tmp/stop_monitor_vr"
#define MAX_VR_CHIPS 9
#define VR_TIMEOUT 500
#define MAX_READ_RETRY 10
#define MAX_NEGATIVE_RETRY 3
#define READING_SKIP       1
#define BIT(value, index) ((value >> index) & 1)

//Used identify VR Chip info. there are 4 vr fw code in EVT3 and after
enum
{
    SS_Fairchild = 0x0,
    SS_IFX = 0x1,
    DS_Fairchild = 0x2,
    DS_IFX = 0x3,
    UNKNOWN_TYPE = 0xff,
};

//VR Chip info
typedef struct
{
  uint8_t SlaveAddr;

  uint8_t CRC[4];

  int  DataLength;

  int IndexStartAddr;

  int IndexEndAddr;

} VRBasicInfo;

#define VR_BIT0(Din, Dout) ((Din >> 0) ^ (Din >> 6) ^ (Din >> 9) ^ (Din >> 10) ^ (Din >> 12) ^ (Din >> 16) ^ (Din >> 24) ^ (Din >> 25) ^ (Din >> 26) ^ (Din >> 28) ^ (Din >> 29) ^ (Din >> 30) ^ (Din >> 31) ^ (Dout >> 0) ^ (Dout >> 6) ^ (Dout >> 9) ^ (Dout >> 10) ^ (Dout >> 12) ^ (Dout >> 16) ^ (Dout >> 24) ^ (Dout >> 25) ^ (Dout >> 26) ^ (Dout >> 28) ^ (Dout >> 29) ^ (Dout >> 30) ^ (Dout >> 31))
#define VR_BIT1(Din, Dout) ((Din >> 0) ^ (Din >> 1) ^ (Din >> 6) ^ (Din >> 7) ^ (Din >> 9) ^ (Din >> 11) ^ (Din >> 12) ^ (Din >> 13) ^ (Din >> 16) ^ (Din >> 17) ^ (Din >> 24) ^ (Din >> 27) ^ (Din >> 28) ^ (Dout >> 0) ^ (Dout >> 1) ^ (Dout >> 6) ^ (Dout >> 7) ^ (Dout >> 9) ^ (Dout >> 11) ^ (Dout >> 12) ^ (Dout >> 13) ^ (Dout >> 16) ^ (Dout >> 17) ^ (Dout >> 24) ^ (Dout >> 27) ^ (Dout >> 28))
#define VR_BIT2(Din, Dout) ((Din >> 0) ^ (Din >> 1) ^ (Din >> 2) ^ (Din >> 6) ^ (Din >> 7) ^ (Din >> 8) ^ (Din >> 9) ^ (Din >> 13) ^ (Din >> 14) ^ (Din >> 16) ^ (Din >> 17) ^ (Din >> 18) ^ (Din >> 24) ^ (Din >> 26) ^ (Din >> 30) ^ (Din >> 31) ^ (Dout >> 0) ^ (Dout >> 1) ^ (Dout >> 2) ^ (Dout >> 6) ^ (Dout >> 7) ^ (Dout >> 8) ^ (Dout >> 9) ^ (Dout >> 13) ^ (Dout >> 14) ^ (Dout >> 16) ^ (Dout >> 17) ^ (Dout >> 18) ^ (Dout >> 24) ^ (Dout >> 26) ^ (Dout >> 30) ^ (Dout >> 31))
#define VR_BIT3(Din, Dout) ((Din >> 1) ^ (Din >> 2) ^ (Din >> 3) ^ (Din >> 7) ^ (Din >> 8) ^ (Din >> 9) ^ (Din >> 10) ^ (Din >> 14) ^ (Din >> 15) ^ (Din >> 17) ^ (Din >> 18) ^ (Din >> 19) ^ (Din >> 25) ^ (Din >> 27) ^ (Din >> 31) ^ (Dout >> 1) ^ (Dout >> 2) ^ (Dout >> 3) ^ (Dout >> 7) ^ (Dout >> 8) ^ (Dout >> 9) ^ (Dout >> 10) ^ (Dout >> 14) ^ (Dout >> 15) ^ (Dout >> 17) ^ (Dout >> 18) ^ (Dout >> 19) ^ (Dout >> 25) ^ (Dout >> 27) ^ (Dout >> 31))
#define VR_BIT4(Din, Dout) ((Din >> 0) ^ (Din >> 2) ^ (Din >> 3) ^ (Din >> 4) ^ (Din >> 6) ^ (Din >> 8) ^ (Din >> 11) ^ (Din >> 12) ^ (Din >> 15) ^ (Din >> 18) ^ (Din >> 19) ^ (Din >> 20) ^ (Din >> 24) ^ (Din >> 25) ^ (Din >> 29) ^ (Din >> 30) ^ (Din >> 31) ^ (Dout >> 0) ^ (Dout >> 2) ^ (Dout >> 3) ^ (Dout >> 4) ^ (Dout >> 6) ^ (Dout >> 8) ^ (Dout >> 11) ^ (Dout >> 12) ^ (Dout >> 15) ^ (Dout >> 18) ^ (Dout >> 19) ^ (Dout >> 20) ^ (Dout >> 24) ^ (Dout >> 25) ^ (Dout >> 29) ^ (Dout >> 30) ^ (Dout >> 31))
#define VR_BIT5(Din, Dout) ((Din >> 0) ^ (Din >> 1) ^ (Din >> 3) ^ (Din >> 4) ^ (Din >> 5) ^ (Din >> 6) ^ (Din >> 7) ^ (Din >> 10) ^ (Din >> 13) ^ (Din >> 19) ^ (Din >> 20) ^ (Din >> 21) ^ (Din >> 24) ^ (Din >> 28) ^ (Din >> 29) ^ (Dout >> 0) ^ (Dout >> 1) ^ (Dout >> 3) ^ (Dout >> 4) ^ (Dout >> 5) ^ (Dout >> 6) ^ (Dout >> 7) ^ (Dout >> 10) ^ (Dout >> 13) ^ (Dout >> 19) ^ (Dout >> 20) ^ (Dout >> 21) ^ (Dout >> 24) ^ (Dout >> 28) ^ (Dout >> 29))
#define VR_BIT6(Din, Dout) ((Din >> 1) ^ (Din >> 2) ^ (Din >> 4) ^ (Din >> 5) ^ (Din >> 6) ^ (Din >> 7) ^ (Din >> 8) ^ (Din >> 11) ^ (Din >> 14) ^ (Din >> 20) ^ (Din >> 21) ^ (Din >> 22) ^ (Din >> 25) ^ (Din >> 29) ^ (Din >> 30) ^ (Dout >> 1) ^ (Dout >> 2) ^ (Dout >> 4) ^ (Dout >> 5) ^ (Dout >> 6) ^ (Dout >> 7) ^ (Dout >> 8) ^ (Dout >> 11) ^ (Dout >> 14) ^ (Dout >> 20) ^ (Dout >> 21) ^ (Dout >> 22) ^ (Dout >> 25) ^ (Dout >> 29) ^ (Dout >> 30))
#define VR_BIT7(Din, Dout) ((Din >> 0) ^ (Din >> 2) ^ (Din >> 3) ^ (Din >> 5) ^ (Din >> 7) ^ (Din >> 8) ^ (Din >> 10) ^ (Din >> 15) ^ (Din >> 16) ^ (Din >> 21) ^ (Din >> 22) ^ (Din >> 23) ^ (Din >> 24) ^ (Din >> 25) ^ (Din >> 28) ^ (Din >> 29) ^ (Dout >> 0) ^ (Dout >> 2) ^ (Dout >> 3) ^ (Dout >> 5) ^ (Dout >> 7) ^ (Dout >> 8) ^ (Dout >> 10) ^ (Dout >> 15) ^ (Dout >> 16) ^ (Dout >> 21) ^ (Dout >> 22) ^ (Dout >> 23) ^ (Dout >> 24) ^ (Dout >> 25) ^ (Dout >> 28) ^ (Dout >> 29))
#define VR_BIT8(Din, Dout) ((Din >> 0) ^ (Din >> 1) ^ (Din >> 3) ^ (Din >> 4) ^ (Din >> 8) ^ (Din >> 10) ^ (Din >> 11) ^ (Din >> 12) ^ (Din >> 17) ^ (Din >> 22) ^ (Din >> 23) ^ (Din >> 28) ^ (Din >> 31) ^ (Dout >> 0) ^ (Dout >> 1) ^ (Dout >> 3) ^ (Dout >> 4) ^ (Dout >> 8) ^ (Dout >> 10) ^ (Dout >> 11) ^ (Dout >> 12) ^ (Dout >> 17) ^ (Dout >> 22) ^ (Dout >> 23) ^ (Dout >> 28) ^ (Dout >> 31))
#define VR_BIT9(Din, Dout) ((Din >> 1) ^ (Din >> 2) ^ (Din >> 4) ^ (Din >> 5) ^ (Din >> 9) ^ (Din >> 11) ^ (Din >> 12) ^ (Din >> 13) ^ (Din >> 18) ^ (Din >> 23) ^ (Din >> 24) ^ (Din >> 29) ^ (Dout >> 1) ^ (Dout >> 2) ^ (Dout >> 4) ^ (Dout >> 5) ^ (Dout >> 9) ^ (Dout >> 11) ^ (Dout >> 12) ^ (Dout >> 13) ^ (Dout >> 18) ^ (Dout >> 23) ^ (Dout >> 24) ^ (Dout >> 29))
#define VR_BIT10(Din, Dout) ((Din >> 0) ^ (Din >> 2) ^ (Din >> 3) ^ (Din >> 5) ^ (Din >> 9) ^ (Din >> 13) ^ (Din >> 14) ^ (Din >> 16) ^ (Din >> 19) ^ (Din >> 26) ^ (Din >> 28) ^ (Din >> 29) ^ (Din >> 31) ^ (Dout >> 0) ^ (Dout >> 2) ^ (Dout >> 3) ^ (Dout >> 5) ^ (Dout >> 9) ^ (Dout >> 13) ^ (Dout >> 14) ^ (Dout >> 16) ^ (Dout >> 19) ^ (Dout >> 26) ^ (Dout >> 28) ^ (Dout >> 29) ^ (Dout >> 31))
#define VR_BIT11(Din, Dout) ((Din >> 0) ^ (Din >> 1) ^ (Din >> 3) ^ (Din >> 4) ^ (Din >> 9) ^ (Din >> 12) ^ (Din >> 14) ^ (Din >> 15) ^ (Din >> 16) ^ (Din >> 17) ^ (Din >> 20) ^ (Din >> 24) ^ (Din >> 25) ^ (Din >> 26) ^ (Din >> 27) ^ (Din >> 28) ^ (Din >> 31) ^ (Dout >> 0) ^ (Dout >> 1) ^ (Dout >> 3) ^ (Dout >> 4) ^ (Dout >> 9) ^ (Dout >> 12) ^ (Dout >> 14) ^ (Dout >> 15) ^ (Dout >> 16) ^ (Dout >> 17) ^ (Dout >> 20) ^ (Dout >> 24) ^ (Dout >> 25) ^ (Dout >> 26) ^ (Dout >> 27) ^ (Dout >> 28) ^ (Dout >> 31))
#define VR_BIT12(Din, Dout) ((Din >> 0) ^ (Din >> 1) ^ (Din >> 2) ^ (Din >> 4) ^ (Din >> 5) ^ (Din >> 6) ^ (Din >> 9) ^ (Din >> 12) ^ (Din >> 13) ^ (Din >> 15) ^ (Din >> 17) ^ (Din >> 18) ^ (Din >> 21) ^ (Din >> 24) ^ (Din >> 27) ^ (Din >> 30) ^ (Din >> 31) ^ (Dout >> 0) ^ (Dout >> 1) ^ (Dout >> 2) ^ (Dout >> 4) ^ (Dout >> 5) ^ (Dout >> 6) ^ (Dout >> 9) ^ (Dout >> 12) ^ (Dout >> 13) ^ (Dout >> 15) ^ (Dout >> 17) ^ (Dout >> 18) ^ (Dout >> 21) ^ (Dout >> 24) ^ (Dout >> 27) ^ (Dout >> 30) ^ (Dout >> 31))
#define VR_BIT13(Din, Dout) ((Din >> 1) ^ (Din >> 2) ^ (Din >> 3) ^ (Din >> 5) ^ (Din >> 6) ^ (Din >> 7) ^ (Din >> 10) ^ (Din >> 13) ^ (Din >> 14) ^ (Din >> 16) ^ (Din >> 18) ^ (Din >> 19) ^ (Din >> 22) ^ (Din >> 25) ^ (Din >> 28) ^ (Din >> 31) ^ (Dout >> 1) ^ (Dout >> 2) ^ (Dout >> 3) ^ (Dout >> 5) ^ (Dout >> 6) ^ (Dout >> 7) ^ (Dout >> 10) ^ (Dout >> 13) ^ (Dout >> 14) ^ (Dout >> 16) ^ (Dout >> 18) ^ (Dout >> 19) ^ (Dout >> 22) ^ (Dout >> 25) ^ (Dout >> 28) ^ (Dout >> 31))
#define VR_BIT14(Din, Dout) ((Din >> 2) ^ (Din >> 3) ^ (Din >> 4) ^ (Din >> 6) ^ (Din >> 7) ^ (Din >> 8) ^ (Din >> 11) ^ (Din >> 14) ^ (Din >> 15) ^ (Din >> 17) ^ (Din >> 19) ^ (Din >> 20) ^ (Din >> 23) ^ (Din >> 26) ^ (Din >> 29) ^ (Dout >> 2) ^ (Dout >> 3) ^ (Dout >> 4) ^ (Dout >> 6) ^ (Dout >> 7) ^ (Dout >> 8) ^ (Dout >> 11) ^ (Dout >> 14) ^ (Dout >> 15) ^ (Dout >> 17) ^ (Dout >> 19) ^ (Dout >> 20) ^ (Dout >> 23) ^ (Dout >> 26) ^ (Dout >> 29))
#define VR_BIT15(Din, Dout) ((Din >> 3) ^ (Din >> 4) ^ (Din >> 5) ^ (Din >> 7) ^ (Din >> 8) ^ (Din >> 9) ^ (Din >> 12) ^ (Din >> 15) ^ (Din >> 16) ^ (Din >> 18) ^ (Din >> 20) ^ (Din >> 21) ^ (Din >> 24) ^ (Din >> 27) ^ (Din >> 30) ^ (Dout >> 3) ^ (Dout >> 4) ^ (Dout >> 5) ^ (Dout >> 7) ^ (Dout >> 8) ^ (Dout >> 9) ^ (Dout >> 12) ^ (Dout >> 15) ^ (Dout >> 16) ^ (Dout >> 18) ^ (Dout >> 20) ^ (Dout >> 21) ^ (Dout >> 24) ^ (Dout >> 27) ^ (Dout >> 30))
#define VR_BIT16(Din, Dout) ((Din >> 0) ^ (Din >> 4) ^ (Din >> 5) ^ (Din >> 8) ^ (Din >> 12) ^ (Din >> 13) ^ (Din >> 17) ^ (Din >> 19) ^ (Din >> 21) ^ (Din >> 22) ^ (Din >> 24) ^ (Din >> 26) ^ (Din >> 29) ^ (Din >> 30) ^ (Dout >> 0) ^ (Dout >> 4) ^ (Dout >> 5) ^ (Dout >> 8) ^ (Dout >> 12) ^ (Dout >> 13) ^ (Dout >> 17) ^ (Dout >> 19) ^ (Dout >> 21) ^ (Dout >> 22) ^ (Dout >> 24) ^ (Dout >> 26) ^ (Dout >> 29) ^ (Dout >> 30))
#define VR_BIT17(Din, Dout) ((Din >> 1) ^ (Din >> 5) ^ (Din >> 6) ^ (Din >> 9) ^ (Din >> 13) ^ (Din >> 14) ^ (Din >> 18) ^ (Din >> 20) ^ (Din >> 22) ^ (Din >> 23) ^ (Din >> 25) ^ (Din >> 27) ^ (Din >> 30) ^ (Din >> 31) ^ (Dout >> 1) ^ (Dout >> 5) ^ (Dout >> 6) ^ (Dout >> 9) ^ (Dout >> 13) ^ (Dout >> 14) ^ (Dout >> 18) ^ (Dout >> 20) ^ (Dout >> 22) ^ (Dout >> 23) ^ (Dout >> 25) ^ (Dout >> 27) ^ (Dout >> 30) ^ (Dout >> 31))
#define VR_BIT18(Din, Dout) ((Din >> 2) ^ (Din >> 6) ^ (Din >> 7) ^ (Din >> 10) ^ (Din >> 14) ^ (Din >> 15) ^ (Din >> 19) ^ (Din >> 21) ^ (Din >> 23) ^ (Din >> 24) ^ (Din >> 26) ^ (Din >> 28) ^ (Din >> 31) ^ (Dout >> 2) ^ (Dout >> 6) ^ (Dout >> 7) ^ (Dout >> 10) ^ (Dout >> 14) ^ (Dout >> 15) ^ (Dout >> 19) ^ (Dout >> 21) ^ (Dout >> 23) ^ (Dout >> 24) ^ (Dout >> 26) ^ (Dout >> 28) ^ (Dout >> 31))
#define VR_BIT19(Din, Dout) ((Din >> 3) ^ (Din >> 7) ^ (Din >> 8) ^ (Din >> 11) ^ (Din >> 15) ^ (Din >> 16) ^ (Din >> 20) ^ (Din >> 22) ^ (Din >> 24) ^ (Din >> 25) ^ (Din >> 27) ^ (Din >> 29) ^ (Dout >> 3) ^ (Dout >> 7) ^ (Dout >> 8) ^ (Dout >> 11) ^ (Dout >> 15) ^ (Dout >> 16) ^ (Dout >> 20) ^ (Dout >> 22) ^ (Dout >> 24) ^ (Dout >> 25) ^ (Dout >> 27) ^ (Dout >> 29))
#define VR_BIT20(Din, Dout) ((Din >> 4) ^ (Din >> 8) ^ (Din >> 9) ^ (Din >> 12) ^ (Din >> 16) ^ (Din >> 17) ^ (Din >> 21) ^ (Din >> 23) ^ (Din >> 25) ^ (Din >> 26) ^ (Din >> 28) ^ (Din >> 30) ^ (Dout >> 4) ^ (Dout >> 8) ^ (Dout >> 9) ^ (Dout >> 12) ^ (Dout >> 16) ^ (Dout >> 17) ^ (Dout >> 21) ^ (Dout >> 23) ^ (Dout >> 25) ^ (Dout >> 26) ^ (Dout >> 28) ^ (Dout >> 30))
#define VR_BIT21(Din, Dout) ((Din >> 5) ^ (Din >> 9) ^ (Din >> 10) ^ (Din >> 13) ^ (Din >> 17) ^ (Din >> 18) ^ (Din >> 22) ^ (Din >> 24) ^ (Din >> 26) ^ (Din >> 27) ^ (Din >> 29) ^ (Din >> 31) ^ (Dout >> 5) ^ (Dout >> 9) ^ (Dout >> 10) ^ (Dout >> 13) ^ (Dout >> 17) ^ (Dout >> 18) ^ (Dout >> 22) ^ (Dout >> 24) ^ (Dout >> 26) ^ (Dout >> 27) ^ (Dout >> 29) ^ (Dout >> 31))
#define VR_BIT22(Din, Dout) ((Din >> 0) ^ (Din >> 9) ^ (Din >> 11) ^ (Din >> 12) ^ (Din >> 14) ^ (Din >> 16) ^ (Din >> 18) ^ (Din >> 19) ^ (Din >> 23) ^ (Din >> 24) ^ (Din >> 26) ^ (Din >> 27) ^ (Din >> 29) ^ (Din >> 31) ^ (Dout >> 0) ^ (Dout >> 9) ^ (Dout >> 11) ^ (Dout >> 12) ^ (Dout >> 14) ^ (Dout >> 16) ^ (Dout >> 18) ^ (Dout >> 19) ^ (Dout >> 23) ^ (Dout >> 24) ^ (Dout >> 26) ^ (Dout >> 27) ^ (Dout >> 29) ^ (Dout >> 31))
#define VR_BIT23(Din, Dout) ((Din >> 0) ^ (Din >> 1) ^ (Din >> 6) ^ (Din >> 9) ^ (Din >> 13) ^ (Din >> 15) ^ (Din >> 16) ^ (Din >> 17) ^ (Din >> 19) ^ (Din >> 20) ^ (Din >> 26) ^ (Din >> 27) ^ (Din >> 29) ^ (Din >> 31) ^ (Dout >> 0) ^ (Dout >> 1) ^ (Dout >> 6) ^ (Dout >> 9) ^ (Dout >> 13) ^ (Dout >> 15) ^ (Dout >> 16) ^ (Dout >> 17) ^ (Dout >> 19) ^ (Dout >> 20) ^ (Dout >> 26) ^ (Dout >> 27) ^ (Dout >> 29) ^ (Dout >> 31))
#define VR_BIT24(Din, Dout) ((Din >> 1) ^ (Din >> 2) ^ (Din >> 7) ^ (Din >> 10) ^ (Din >> 14) ^ (Din >> 16) ^ (Din >> 17) ^ (Din >> 18) ^ (Din >> 20) ^ (Din >> 21) ^ (Din >> 27) ^ (Din >> 28) ^ (Din >> 30) ^ (Dout >> 1) ^ (Dout >> 2) ^ (Dout >> 7) ^ (Dout >> 10) ^ (Dout >> 14) ^ (Dout >> 16) ^ (Dout >> 17) ^ (Dout >> 18) ^ (Dout >> 20) ^ (Dout >> 21) ^ (Dout >> 27) ^ (Dout >> 28) ^ (Dout >> 30))
#define VR_BIT25(Din, Dout) ((Din >> 2) ^ (Din >> 3) ^ (Din >> 8) ^ (Din >> 11) ^ (Din >> 15) ^ (Din >> 17) ^ (Din >> 18) ^ (Din >> 19) ^ (Din >> 21) ^ (Din >> 22) ^ (Din >> 28) ^ (Din >> 29) ^ (Din >> 31) ^ (Dout >> 2) ^ (Dout >> 3) ^ (Dout >> 8) ^ (Dout >> 11) ^ (Dout >> 15) ^ (Dout >> 17) ^ (Dout >> 18) ^ (Dout >> 19) ^ (Dout >> 21) ^ (Dout >> 22) ^ (Dout >> 28) ^ (Dout >> 29) ^ (Dout >> 31))
#define VR_BIT26(Din, Dout) ((Din >> 0) ^ (Din >> 3) ^ (Din >> 4) ^ (Din >> 6) ^ (Din >> 10) ^ (Din >> 18) ^ (Din >> 19) ^ (Din >> 20) ^ (Din >> 22) ^ (Din >> 23) ^ (Din >> 24) ^ (Din >> 25) ^ (Din >> 26) ^ (Din >> 28) ^ (Din >> 31) ^ (Dout >> 0) ^ (Dout >> 3) ^ (Dout >> 4) ^ (Dout >> 6) ^ (Dout >> 10) ^ (Dout >> 18) ^ (Dout >> 19) ^ (Dout >> 20) ^ (Dout >> 22) ^ (Dout >> 23) ^ (Dout >> 24) ^ (Dout >> 25) ^ (Dout >> 26) ^ (Dout >> 28) ^ (Dout >> 31))
#define VR_BIT27(Din, Dout) ((Din >> 1) ^ (Din >> 4) ^ (Din >> 5) ^ (Din >> 7) ^ (Din >> 11) ^ (Din >> 19) ^ (Din >> 20) ^ (Din >> 21) ^ (Din >> 23) ^ (Din >> 24) ^ (Din >> 25) ^ (Din >> 26) ^ (Din >> 27) ^ (Din >> 29) ^ (Dout >> 1) ^ (Dout >> 4) ^ (Dout >> 5) ^ (Dout >> 7) ^ (Dout >> 11) ^ (Dout >> 19) ^ (Dout >> 20) ^ (Dout >> 21) ^ (Dout >> 23) ^ (Dout >> 24) ^ (Dout >> 25) ^ (Dout >> 26) ^ (Dout >> 27) ^ (Dout >> 29))
#define VR_BIT28(Din, Dout) ((Din >> 2) ^ (Din >> 5) ^ (Din >> 6) ^ (Din >> 8) ^ (Din >> 12) ^ (Din >> 20) ^ (Din >> 21) ^ (Din >> 22) ^ (Din >> 24) ^ (Din >> 25) ^ (Din >> 26) ^ (Din >> 27) ^ (Din >> 28) ^ (Din >> 30) ^ (Dout >> 2) ^ (Dout >> 5) ^ (Dout >> 6) ^ (Dout >> 8) ^ (Dout >> 12) ^ (Dout >> 20) ^ (Dout >> 21) ^ (Dout >> 22) ^ (Dout >> 24) ^ (Dout >> 25) ^ (Dout >> 26) ^ (Dout >> 27) ^ (Dout >> 28) ^ (Dout >> 30))
#define VR_BIT29(Din, Dout) ((Din >> 3) ^ (Din >> 6) ^ (Din >> 7) ^ (Din >> 9) ^ (Din >> 13) ^ (Din >> 21) ^ (Din >> 22) ^ (Din >> 23) ^ (Din >> 25) ^ (Din >> 26) ^ (Din >> 27) ^ (Din >> 28) ^ (Din >> 29) ^ (Din >> 31) ^ (Dout >> 3) ^ (Dout >> 6) ^ (Dout >> 7) ^ (Dout >> 9) ^ (Dout >> 13) ^ (Dout >> 21) ^ (Dout >> 22) ^ (Dout >> 23) ^ (Dout >> 25) ^ (Dout >> 26) ^ (Dout >> 27) ^ (Dout >> 28) ^ (Dout >> 29) ^ (Dout >> 31))
#define VR_BIT30(Din, Dout) ((Din >> 4) ^ (Din >> 7) ^ (Din >> 8) ^ (Din >> 10) ^ (Din >> 14) ^ (Din >> 22) ^ (Din >> 23) ^ (Din >> 24) ^ (Din >> 26) ^ (Din >> 27) ^ (Din >> 28) ^ (Din >> 29) ^ (Din >> 30) ^ (Dout >> 4) ^ (Dout >> 7) ^ (Dout >> 8) ^ (Dout >> 10) ^ (Dout >> 14) ^ (Dout >> 22) ^ (Dout >> 23) ^ (Dout >> 24) ^ (Dout >> 26) ^ (Dout >> 27) ^ (Dout >> 28) ^ (Dout >> 29) ^ (Dout >> 30))
#define VR_BIT31(Din, Dout) ((Din >> 5) ^ (Din >> 8) ^ (Din >> 9) ^ (Din >> 11) ^ (Din >> 15) ^ (Din >> 23) ^ (Din >> 24) ^ (Din >> 25) ^ (Din >> 27) ^ (Din >> 28) ^ (Din >> 29) ^ (Din >> 30) ^ (Din >> 31) ^ (Dout >> 5) ^ (Dout >> 8) ^ (Dout >> 9) ^ (Dout >> 11) ^ (Dout >> 15) ^ (Dout >> 23) ^ (Dout >> 24) ^ (Dout >> 25) ^ (Dout >> 27) ^ (Dout >> 28) ^ (Dout >> 29) ^ (Dout >> 30) ^ (Dout >> 31))

static void
msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

int
vr_read_volt(uint8_t vr, uint8_t loop, float *value) {
  int fd;
  char fn[32];
  static int count = 0;
  int ret = -1;
  unsigned int retry = MAX_READ_RETRY;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  // The following block for detecting vr_update is in progress or not
  if ( access(VR_UPDATE_IN_PROGRESS, F_OK) == 0 )
  {
    //Avoid sensord unmonitoring vr sensors due to unexpected condition happen during vr_update
    if ( count > VR_TIMEOUT )
    {
      remove(VR_UPDATE_IN_PROGRESS);
      count = 0;
    }

    syslog(LOG_WARNING, "[%d]Stop Monitor VR Volt due to VR update is in progress\n", count++);

    return VR_STATUS_NOT_AVAILABLE;
  }

  count = 0;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  while (retry) {
    fd = open(fn, O_RDWR);
    if (fd < 0) {
      syslog(LOG_WARNING, "read_vr_volt: i2c_open failed for bus#%x\n", VR_BUS_ID);
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Set the page to read Voltage
  tbuf[0] = 0x00;
  tbuf[1] = loop;

  tcount = 2;
  rcount = 0;

  retry = MAX_READ_RETRY;
  while (retry) {
    ret = i2c_rdwr_msg_transfer(fd, vr, tbuf, tcount, rbuf, rcount);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "read_vr_volt: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Read 2 bytes from Voltage register
  tbuf[0] = VR_TELEMETRY_VOLT;

  tcount = 1;
  rcount = 2;

  retry = MAX_READ_RETRY;
  while(retry) {
    ret = i2c_rdwr_msg_transfer(fd, vr, tbuf, tcount, rbuf, rcount);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "read_vr_volt: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if (retry == 0)
      goto error_exit;
  }

  // Calculate Voltage
  *value = ((rbuf[1] & 0x0F) * 256 + rbuf[0] ) * 1.25;
  *value /= 1000;

error_exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

int
vr_read_curr(uint8_t vr, uint8_t loop, float *value) {
  int fd;
  char fn[32];
  static int count = 0;
  int ret = -1;
  unsigned int retry = MAX_READ_RETRY;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  // The following block for detecting vr_update is in progress or not
  if ( access(VR_UPDATE_IN_PROGRESS, F_OK) == 0 )
  {
    // Avoid sensord unmonitoring vr sensors due to unexpected condition happen during vr_update
    if ( count > VR_TIMEOUT )
    {
      remove(VR_UPDATE_IN_PROGRESS);
      count = 0;
    }

    syslog(LOG_WARNING, "[%d]Stop Monitor VR Curr due to VR update is in progress\n", count++);

    return VR_STATUS_NOT_AVAILABLE;
  }

  count = 0;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  while (retry) {
    fd = open(fn, O_RDWR);
    if (fd < 0) {
      syslog(LOG_WARNING, "read_vr_curr: i2c_open failed for bus#%x\n", VR_BUS_ID);
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Set the page to read Voltage
  tbuf[0] = 0x00;
  tbuf[1] = loop;

  tcount = 2;
  rcount = 0;

  retry = MAX_READ_RETRY;
  while(retry) {
    ret = i2c_rdwr_msg_transfer(fd, vr, tbuf, tcount, rbuf, rcount);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "read_vr_curr: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if (retry == 0)
      goto error_exit;
  }

  // Read 2 bytes from Voltage register
  tbuf[0] = VR_TELEMETRY_CURR;

  tcount = 1;
  rcount = 2;

  retry = MAX_READ_RETRY;
  while(retry) {
    ret = i2c_rdwr_msg_transfer(fd, vr, tbuf, tcount, rbuf, rcount);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "read_vr_curr: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if (retry == 0)
      goto error_exit;
  }

  // Calculate Current
  if (rbuf[1] < 0x40) {
    // Positive value (sign at bit6)
    *value = ((rbuf[1] & 0x7F) * 256 + rbuf[0] ) * 62.5;
    *value /= 1000;
  } else {
    // Negative value 2's complement
    uint16_t temp = ((rbuf[1] & 0x7F) << 8) | rbuf[0];
    temp = 0x7fff - temp + 1;

    *value = (((temp >> 8) & 0x7F) * 256 + (temp & 0xFF) ) * -62.5;
    *value /= 1000;
  }

  // Handle illegal values observed
  if ((*value < 0) && (*value >= -1.5)) {
    *value = 0;
  }

error_exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

int
vr_read_power(uint8_t vr, uint8_t loop, float *value) {
  int fd;
  char fn[32];
  static int count = 0;
  int ret = -1;
  unsigned int retry = MAX_READ_RETRY;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  // The following block for detecting vr_update is in progress or not
  if ( access(VR_UPDATE_IN_PROGRESS, F_OK) == 0 )
  {
    //avoid sensord unmonitoring vr sensors due to unexpected condition happen during vr_update
    if ( count > VR_TIMEOUT )
    {
      remove(VR_UPDATE_IN_PROGRESS);
      count = 0;
    }

    syslog(LOG_WARNING, "[%d]Stop Monitor VR Power due to VR update is in progress\n", count++);

    return VR_STATUS_NOT_AVAILABLE;
  }

  count = 0;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  while (retry) {
    fd = open(fn, O_RDWR);
    if (fd < 0) {
      syslog(LOG_WARNING, "read_vr_power: i2c_open failed for bus#%x\n", VR_BUS_ID);
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Set the page to read Power
  tbuf[0] = 0x00;
  tbuf[1] = loop;

  tcount = 2;
  rcount = 0;

  retry = MAX_READ_RETRY;
  while(retry) {
    ret = i2c_rdwr_msg_transfer(fd, vr, tbuf, tcount, rbuf, rcount);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "read_vr_power: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if (retry == 0)
      goto error_exit;
  }

  // Read 2 bytes from Power register
  tbuf[0] = VR_TELEMETRY_POWER;

  tcount = 1;
  rcount = 2;

  retry = MAX_READ_RETRY;
  while(retry) {
    ret = i2c_rdwr_msg_transfer(fd, vr, tbuf, tcount, rbuf, rcount);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "read_vr_power: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if (retry == 0)
      goto error_exit;
  }

  // Calculate Power
  *value = ((rbuf[1] & 0x3F) * 256 + rbuf[0] ) * 0.04;

error_exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

int
vr_read_temp(uint8_t vr, uint8_t loop, float *value) {
  int fd;
  char fn[32];
  static int count = 0;
  int ret = -1;
  unsigned int retry = MAX_READ_RETRY;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  int16_t temp;
  static unsigned int max_negative_retry = MAX_NEGATIVE_RETRY;

  // The following block for detecting vr_update is in progress or not
  if ( access(VR_UPDATE_IN_PROGRESS, F_OK) == 0 )
  {
    // Avoid sensord unmonitoring vr sensors due to unexpected condition happen during vr_update
    if ( count > VR_TIMEOUT )
    {
      remove(VR_UPDATE_IN_PROGRESS);
      count = 0;
    }

    syslog(LOG_WARNING, "[%d]Stop Monitor VR Temp due to VR update is in progress\n", count++);

    return VR_STATUS_NOT_AVAILABLE;
  }

  count = 0;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  while (retry) {
    fd = open(fn, O_RDWR);
    if (fd < 0) {
      syslog(LOG_WARNING, "read_vr_temp: i2c_open failed for bus#%x\n", VR_BUS_ID);
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Set the page to read Temperature
  tbuf[0] = 0x00;
  tbuf[1] = loop;

  tcount = 2;
  rcount = 0;

  retry = MAX_READ_RETRY;
  while (retry) {
  ret = i2c_rdwr_msg_transfer(fd, vr, tbuf, tcount, rbuf, rcount);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "read_vr_temp: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // Read 2 bytes from Power register
  tbuf[0] = VR_TELEMETRY_TEMP;

  tcount = 1;
  rcount = 2;

  retry = MAX_READ_RETRY;
  while(retry) {
    ret = i2c_rdwr_msg_transfer(fd, vr, tbuf, tcount, rbuf, rcount);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "read_vr_temp: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  // AN-E1610B-034B: temp[11:0]
  // Calculate Temp
  temp = (rbuf[1] << 8) | rbuf[0];
  if ((rbuf[1] & 0x08))
    temp |= 0xF000; // If negative, sign extend temp.
  *value = (float)temp * 0.125;

  //handle negative temperature value
  if( *value < 0 ) {
    if ( max_negative_retry > 0 ) {
      max_negative_retry--;
      ret = READING_SKIP;
    }
  } else {
    max_negative_retry = MAX_NEGATIVE_RETRY;
  }

error_exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

static int
fetch_vr_info(uint8_t vr, char *key, uint8_t page,
  uint8_t reg1, uint8_t reg2, uint8_t *info) {
  int fd;
  char fn[32];
  int ret = -1;
  unsigned int retry = MAX_READ_RETRY, retry_page = MAX_READ_RETRY;
  uint8_t tcount, rcount;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  char value[MAX_VALUE_LEN] = {0};

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", VR_BUS_ID);
  while (retry) {
    fd = open(fn, O_RDWR);
    if (fd < 0) {
      syslog(LOG_WARNING, "fetch_vr_info: i2c_open failed for bus#%x\n", VR_BUS_ID);
      retry--;
      msleep(100);
    } else {
      break;
    }

    if(retry == 0)
      goto error_exit;
  }

  retry_page = MAX_READ_RETRY;
  while(retry_page) {
    // Set the page to read FW Info
    tbuf[0] = 0x00;
    tbuf[1] = page;

    tcount = 2;
    rcount = 0;

    retry = MAX_READ_RETRY;
    while(retry) {
      ret = i2c_rdwr_msg_transfer(fd, vr, tbuf, tcount, rbuf, rcount);
      if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "fetch_vr_info: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
        retry--;
        msleep(100);
      } else {
        break;
      }

      if(retry == 0)
        goto error_exit;
    }

    // Read 2 bytes from first register
    tbuf[0] = reg1;

    tcount = 1;
    rcount = 2;

    retry = MAX_READ_RETRY;
    while(retry) {
      ret = i2c_rdwr_msg_transfer(fd, vr, tbuf, tcount, rbuf, rcount);
      // Treat data 0xFF as failed and return -1
    if (ret || rbuf[0] == 0xff || rbuf[1] == 0xff) {
      ret = (ret)?ret:-1;
#ifdef DEBUG
      syslog(LOG_WARNING, "fetch_vr_info: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
        retry--;
        msleep(100);
      } else {
        break;
      }

      if(retry == 0)
        goto error_exit;
    }

    info[0] = rbuf[1];
    info[1] = rbuf[0];

    if (reg2) {
      tbuf[0] = reg2;

      tcount = 1;
      rcount = 2;

      retry = MAX_READ_RETRY;
      while(retry) {
        ret = i2c_rdwr_msg_transfer(fd, vr, tbuf, tcount, rbuf, rcount);
        // Treat data 0xFF as failed and return -1
      if (ret || rbuf[0] == 0xff || rbuf[1] == 0xff) {
        ret = (ret)?ret:-1;
#ifdef DEBUG
        syslog(LOG_WARNING, "fetch_vr_info: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
          retry--;
          msleep(100);
        } else {
          break;
        }

        if(retry == 0)
          goto error_exit;
      }

      info[2] = rbuf[1];
      info[3] = rbuf[0];
    }

    // Get the page to confirm if page is switched by other program
    tbuf[0] = 0x00;

    tcount = 1;
    rcount = 1;

    retry = MAX_READ_RETRY;
    while(retry) {
      ret = i2c_rdwr_msg_transfer(fd, vr, tbuf, tcount, rbuf, rcount);
      if (ret) {
#ifdef DEBUG
        syslog(LOG_WARNING, "fetch_vr_info: i2c_io failed for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
        retry--;
        msleep(100);
      } else {
        break;
      }

      if(retry == 0)
        goto error_exit;
    }

    // page is incorrect, previous data might be wrong
    if (rbuf[0] != page) {
#ifdef DEBUG
      syslog(LOG_WARNING, "fetch_vr_info: incorrect page for bus#%x, dev#%x\n", VR_BUS_ID, vr);
#endif
      retry_page--;
    } else {
      break;
    }

    if (retry_page == 0)
      goto error_exit;
  }

  if (reg2)
    sprintf(value, "%08X", *(unsigned int*)info);
  else
    sprintf(value, "%04X", *(unsigned int*)info);
  kv_set(key, value, 0, 0);

error_exit:
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

static int
get_vr_ver(uint8_t vr, uint8_t *ver) {
  char key[MAX_KEY_LEN] = {0}, value[MAX_VALUE_LEN] = {0};
  sprintf(key, "vr_%02Xh_ver", vr);
  if (kv_get(key, value, NULL, 0) < 0)
    return fetch_vr_info(vr, key, VR_FW_PAGE, VR_FW_REG1, VR_FW_REG2, ver);
  *(unsigned int*)ver = (unsigned int)strtoul(value, NULL, 16);
  return 0;
}

static int
get_vr_checksum(uint8_t vr, uint8_t *checksum) {
  char key[MAX_KEY_LEN] = {0}, value[MAX_VALUE_LEN] = {0};
  sprintf(key, "vr_%02Xh_checksum", vr);
  if (kv_get(key, value, NULL, 0) < 0)
    return fetch_vr_info(vr, key, VR_FW_PAGE_2, VR_FW_REG4, VR_FW_REG3, checksum);
  *(unsigned int*)checksum = (unsigned int)strtoul(value, NULL, 16);
  return 0;
}

static int
get_vr_deviceId(uint8_t vr, uint8_t *deviceId) {
  char key[MAX_KEY_LEN] = {0}, value[MAX_VALUE_LEN] = {0};
  sprintf(key, "vr_%02Xh_deviceId", vr);
  if (kv_get(key, value, NULL, 0) < 0)
    return fetch_vr_info(vr, key, VR_FW_PAGE_3, VR_FW_REG5, 0, deviceId);
  *(unsigned short*)deviceId = (unsigned short)strtoul(value, NULL, 16);
  return 0;
}

int vr_fw_version(uint8_t vr, char *outvers_str)
{
  uint8_t ver[32];
  char tmp_str[32];
  *outvers_str = '\0';
  if (get_vr_ver(vr, ver)) {
    strcat(outvers_str, "Version: NA");
  } else {
    sprintf(tmp_str, "Version: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
    strcat(outvers_str, tmp_str);
  }

  if (get_vr_checksum(vr, ver)) {
    strcat(outvers_str, ", Checksum: NA");
  } else {
    sprintf(tmp_str, ", Checksum: %02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
    strcat(outvers_str, tmp_str);
  }

  if (get_vr_deviceId(vr, ver)) {
    strcat(outvers_str, ", DeviceID: NA");
  } else {
    sprintf(tmp_str, ", DeviceID: %02X%02X", ver[0], ver[1]);
    strcat(outvers_str, tmp_str);
  }
  return 0;
}

static int
i2c_io_master_write(char *BusName, uint8_t addr, uint8_t *buf, uint8_t count) {
  int fd;
  int Retry = MAX_READ_RETRY;
  int RetVal = -1;

  //open device
  while ( Retry )
  {
    fd = open(BusName, O_RDWR);
    if ( fd < 0 ) {
      syslog(LOG_WARNING, "[%s] i2c_open failed for bus#%x\n", __func__, VR_BUS_ID);
      Retry--;
      msleep(100);
#ifdef VR_DEBUG
      printf("[%s] i2c_open failed for bus#%x\n", __func__, VR_BUS_ID);
#endif
    } else {
      break;
    }

    if ( 0 == Retry ) {
      syslog(LOG_WARNING, "[%s] i2c_open failed for bus#%x\n", __func__, VR_BUS_ID);
#ifdef VR_DEBUG
      printf("[%s] i2c_open failed for bus#%x\n", __func__, VR_BUS_ID);
#endif
      goto error_exit;
    }
  }

  Retry = MAX_READ_RETRY;

  //Retry transmission
  while ( Retry )
  {
    RetVal = i2c_rdwr_msg_transfer(fd, addr, buf, count, NULL, 0);
    if ( RetVal != 0 )
    {
      syslog(LOG_WARNING, "[%s] Wrtie Bus#%x Slave address: %x failed.\n", __func__, VR_BUS_ID, addr);
      msleep(500);
      Retry--;
#ifdef VR_DEBUG
      printf("[%s] Wrtie Bus#%x Slave address: %x failed.\n", __func__, VR_BUS_ID, addr);
#endif
    } else {
      break;
    }
  }

  if ( RetVal != 0  )
  {
    syslog(LOG_WARNING, "[%s] Wrtie Bus#%x Slave address: %x failed.\n", __func__, VR_BUS_ID, addr);
#ifdef VR_DEBUG
    printf("[%s] Wrtie Bus#%x Slave address: %x failed.\n", __func__, VR_BUS_ID, addr);
#endif
    goto error_exit;
  }

error_exit:

  close(fd);

  return RetVal;
}

static int
i2c_io_rw(char *BusName, uint8_t addr, uint8_t *txbuf, uint8_t txcount, uint8_t *rxbuf,
              uint8_t rxcount)
{
  int fd;
  int Retry = MAX_READ_RETRY;
  int RetVal = -1;

  //open device
  while ( Retry )
  {
    fd = open(BusName, O_RDWR);
    if (fd < 0)
    {
      syslog(LOG_WARNING, "[%s] i2c_open failed for bus#%x\n", __func__, VR_BUS_ID);
      Retry--;
      msleep(100);
#ifdef VR_DEBUG
      printf("[%s] i2c_open failed for bus#%x\n", __func__, VR_BUS_ID);
#endif
    } else {
        break;
    }

    if ( 0 == Retry )
    {
      syslog(LOG_WARNING, "[%s] i2c_open failed for bus#%x\n", __func__, VR_BUS_ID);
#ifdef VR_DEBUG
      printf("[%s] i2c_open failed for bus#%x\n", __func__, VR_BUS_ID);
#endif
        goto error_exit;
    }
  }

  Retry = MAX_READ_RETRY;

  //Retry transmission
  while ( Retry )
  {
      RetVal = i2c_rdwr_msg_transfer(fd, addr, txbuf, txcount, rxbuf, rxcount);
      if ( RetVal != 0 )
      {
        Retry--;
        syslog(LOG_WARNING, "[%s] Read/Wrtie Bus#%x Slave address: %x failed.\n", __func__, VR_BUS_ID, addr);
#ifdef VR_DEBUG
        printf("[%s] Read/Wrtie Bus#%x Slave address: %x failed.\n", __func__, VR_BUS_ID, addr);
#endif
        msleep(500);
      } else {
        break;
      }
  }

  if ( RetVal != 0  )
  {
    syslog(LOG_WARNING, "[%s] Read/Wrtie Bus#%x Slave address: %x failed.\n", __func__, VR_BUS_ID, addr);
#ifdef VR_DEBUG
    printf("[%s] Read/Wrtie Bus#%x Slave address: %x failed.\n", __func__, VR_BUS_ID, addr);
#endif
  }

error_exit:

  close(fd);

  return RetVal;
}

/*
 * Board specific version match verfication
 *
*/
static int
check_vr_fw_code_match_MB(int startindex, int endindex, uint8_t *BinData, uint8_t BoardInfo)
{
  int RetVal = -1;
  int j;
  uint8_t BOARD_SKU_ID;
  uint8_t VR_SKUID; //identify the version which is suitable for MB or not
  uint8_t VR_Type = UNKNOWN_TYPE;

  //the mapper is defined as below and it is defined by power team
  // 0x1 - DS & IFX
  // 0x2 - SS & IFX
  // 0x3 - DS & Fairchild
  // 0x4 - SS & Fairchild
  uint8_t DevStageMapper[]=
  {
    DS_IFX,
    SS_IFX,
    DS_Fairchild,
    SS_Fairchild,
  };

  //get the type of MB by GPIO
  //0x0 - SS & Fairchild
  //0x1 - SS & IFX
  //0x2 - DS & Fairchild
  //0x3 - DS & IFX
  BOARD_SKU_ID = BoardInfo >> 3;

#ifdef VR_DEBUG
  printf("[%s] BoardInfo:%x, BOARD_SKU_ID:%x\n", __func__, BoardInfo, BOARD_SKU_ID);
#endif
  for ( j = startindex; j < endindex; j = j + 4 )// 4 bytes
  {
#ifdef VR_DEBUG
    printf("[%s] %x %x %x %x\n", __func__, BinData[j], BinData[j+1], BinData[j+2], BinData[j+3]);
#endif
    //user data0 - 5e0c address. It represents that the vr fw code version
    //we need to check the vr fw code is match MB or not
    if ( ( 0x5e == BinData[j] ) && ( 0x0c == BinData[j+1] ) )
    {
      //it need to map to DevStageMapper or its unknown type!
      VR_Type = BinData[j+3];

#ifdef VR_DEBUG
      printf("[%s] VR_Type: %x \n", __func__, VR_Type);
#endif
      break;
    }
  }

  if (VR_Type == UNKNOWN_TYPE) {
    goto error_exit;
  }

  VR_SKUID = DevStageMapper[VR_Type];

#ifdef VR_DEBUG
  printf("[%s] DevStageMapper[%d]\n", __func__, VR_SKUID);
#endif

  if ( VR_SKUID == BOARD_SKU_ID )
  {
      return 0;
  }
  else
  {
    syslog(LOG_WARNING, "[%s] VR fw code does not match MB!\n", __func__);

    RetVal = -1;

    goto error_exit;
  }

error_exit:
  return RetVal;
}

static int
check_integrity_of_vr_data(uint8_t SlaveAddr, uint8_t *ExpectedCRC, int StartIndex, int EndIndex, uint8_t *Data)
{
  int i = StartIndex;
  uint32_t CurrentData = 0x0;
  uint32_t CRC = 0x0;
  uint32_t Dout = 0xffffffff;

  for ( ; i < EndIndex; i = i + 4 )
  {
    //printf("[%d] %x %x %x %x\n", i, Data[i], Data[i+1], Data[i+2], Data[i+3]);

    CurrentData = (CurrentData & 0x0) | Data[i+2];//low byte

    CRC =  (VR_BIT0(Dout, CurrentData) & 0x1 ) + ((VR_BIT1(Dout, CurrentData) & 0x1 ) << 1 ) + ((VR_BIT2(Dout, CurrentData) & 0x1 ) << 2 ) + ((VR_BIT3(Dout, CurrentData) & 0x1 ) << 3 ) + ((VR_BIT4(Dout, CurrentData) & 0x1 ) << 4 ) + ((VR_BIT5(Dout, CurrentData) & 0x1 ) << 5 ) + ((VR_BIT6(Dout, CurrentData) & 0x1 ) << 6 ) + ((VR_BIT7(Dout, CurrentData) & 0x1 ) << 7 ) +
    ((VR_BIT8(Dout, CurrentData) & 0x1 ) << 8 ) + ((VR_BIT9(Dout, CurrentData) & 0x1 ) << 9 ) + ((VR_BIT10(Dout, CurrentData) & 0x1 ) << 10 ) + ((VR_BIT11(Dout, CurrentData) & 0x1 ) << 11 ) + ((VR_BIT12(Dout, CurrentData) & 0x1 ) << 12 ) + ((VR_BIT13(Dout, CurrentData) & 0x1 ) << 13 ) + ((VR_BIT14(Dout, CurrentData) & 0x1 ) << 14 ) + ((VR_BIT15(Dout, CurrentData) & 0x1 ) << 15 ) +
    ((VR_BIT16(Dout, CurrentData) & 0x1 ) << 16 ) + ((VR_BIT17(Dout, CurrentData) & 0x1 ) << 17 ) + ((VR_BIT18(Dout, CurrentData) & 0x1 ) << 18 ) + ((VR_BIT19(Dout, CurrentData) & 0x1 ) << 19 ) + ((VR_BIT20(Dout, CurrentData) & 0x1 ) << 20 ) + ((VR_BIT21(Dout, CurrentData) & 0x1 ) << 21 ) + ((VR_BIT22(Dout, CurrentData) & 0x1 ) << 22 ) + ((VR_BIT23(Dout, CurrentData) & 0x1 ) << 23 ) +
    ((VR_BIT24(Dout, CurrentData) & 0x1 ) << 24 ) + ((VR_BIT25(Dout, CurrentData) & 0x1 ) << 25 ) + ((VR_BIT26(Dout, CurrentData) & 0x1 ) << 26 ) + ((VR_BIT27(Dout, CurrentData) & 0x1 ) << 27 ) + ((VR_BIT28(Dout, CurrentData) & 0x1 ) << 28 ) + ((VR_BIT29(Dout, CurrentData) & 0x1 ) << 29 ) + ((VR_BIT30(Dout, CurrentData) & 0x1 ) << 30 ) + ((VR_BIT31(Dout, CurrentData) & 0x1 ) << 31 );

    Dout = CRC;

    CurrentData = (CurrentData & 0x0) | Data[i+3];//high byte

    CRC =  (VR_BIT0(Dout, CurrentData) & 0x1 ) + ((VR_BIT1(Dout, CurrentData) & 0x1 ) << 1 ) + ((VR_BIT2(Dout, CurrentData) & 0x1 ) << 2 ) + ((VR_BIT3(Dout, CurrentData) & 0x1 ) << 3 ) + ((VR_BIT4(Dout, CurrentData) & 0x1 ) << 4 ) + ((VR_BIT5(Dout, CurrentData) & 0x1 ) << 5 ) + ((VR_BIT6(Dout, CurrentData) & 0x1 ) << 6 ) + ((VR_BIT7(Dout, CurrentData) & 0x1 ) << 7 ) +
    ((VR_BIT8(Dout, CurrentData) & 0x1 ) << 8 ) + ((VR_BIT9(Dout, CurrentData) & 0x1 ) << 9 ) + ((VR_BIT10(Dout, CurrentData) & 0x1 ) << 10 ) + ((VR_BIT11(Dout, CurrentData) & 0x1 ) << 11 ) + ((VR_BIT12(Dout, CurrentData) & 0x1 ) << 12 ) + ((VR_BIT13(Dout, CurrentData) & 0x1 ) << 13 ) + ((VR_BIT14(Dout, CurrentData) & 0x1 ) << 14 ) + ((VR_BIT15(Dout, CurrentData) & 0x1 ) << 15 ) +
    ((VR_BIT16(Dout, CurrentData) & 0x1 ) << 16 ) + ((VR_BIT17(Dout, CurrentData) & 0x1 ) << 17 ) + ((VR_BIT18(Dout, CurrentData) & 0x1 ) << 18 ) + ((VR_BIT19(Dout, CurrentData) & 0x1 ) << 19 ) + ((VR_BIT20(Dout, CurrentData) & 0x1 ) << 20 ) + ((VR_BIT21(Dout, CurrentData) & 0x1 ) << 21 ) + ((VR_BIT22(Dout, CurrentData) & 0x1 ) << 22 ) + ((VR_BIT23(Dout, CurrentData) & 0x1 ) << 23 ) +
    ((VR_BIT24(Dout, CurrentData) & 0x1 ) << 24 ) + ((VR_BIT25(Dout, CurrentData) & 0x1 ) << 25 ) + ((VR_BIT26(Dout, CurrentData) & 0x1 ) << 26 ) + ((VR_BIT27(Dout, CurrentData) & 0x1 ) << 27 ) + ((VR_BIT28(Dout, CurrentData) & 0x1 ) << 28 ) + ((VR_BIT29(Dout, CurrentData) & 0x1 ) << 29 ) + ((VR_BIT30(Dout, CurrentData) & 0x1 ) << 30 ) + ((VR_BIT31(Dout, CurrentData) & 0x1 ) << 31 );

    Dout = CRC;
  }

  uint8_t ActualCRC32[4] = {0};

  ActualCRC32[0] = ( CRC & 0xff );
  ActualCRC32[1] = ( ( CRC >> 8 ) & 0xff );
  ActualCRC32[2] = ( ( CRC >> 16 ) & 0xff );
  ActualCRC32[3] = ( ( CRC >> 24 ) & 0xff );

  if ( (ActualCRC32[3] != ExpectedCRC[0]) || (ActualCRC32[2] != ExpectedCRC[1]) || (ActualCRC32[1] != ExpectedCRC[2]) || (ActualCRC32[0] != ExpectedCRC[3]) )
  {
    syslog(LOG_WARNING, "[%s] Slave: %x CRC not match !\n", __func__, SlaveAddr);
    syslog(LOG_WARNING, "[%s] Slave: %x CRC: %x %x %x %x!\n", __func__, SlaveAddr, ActualCRC32[0], ActualCRC32[1], ActualCRC32[2], ActualCRC32[3]);
    syslog(LOG_WARNING, "[%s] Slave: %x Expected CRC: %x %x %x %x!\n", __func__, SlaveAddr, ExpectedCRC[0], ExpectedCRC[1], ExpectedCRC[2], ExpectedCRC[3]);

    return -1;
  }

#ifdef VR_DEBUG
  else
  {
    printf("[%s] Slave: %x CRC match !\n", __func__, SlaveAddr);
  }
#endif

  return 0;
}

static int
unlock_vr_register(char *BusName, uint8_t SlaveAddr)
{
  int RetVal;
  uint8_t TxBuf[3] = {0};

  /********* Set Page***********/
  TxBuf[0] = 0x00;
  TxBuf[1] = 0x3f;

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] UnlockRegister: Change Register Page failed: 0x00 0x3f \n", __func__);
    goto error_exit;
  }

  /*********Write unlock data to 0x27***********/
  TxBuf[0] = 0x27;
  TxBuf[1] = 0x7c;
  TxBuf[2] = 0xb3;

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 3);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Unlock register failed: 0x27 0x7c 0xb3 \n", __func__);
    goto error_exit;
  }


  /*********Write unlock data to 0x2a***********/
  TxBuf[0] = 0x2a;
  TxBuf[1] = 0xb2;
  TxBuf[2] = 0x8a;

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 3);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Unlock register failed: 0x27, 0x7c, 0xb3 \n", __func__);
    goto error_exit;
  }

error_exit:

  return RetVal;
}

static int
check_vr_chip_remaining_times(char *BusName, uint8_t SlaveAddr)
{
  int RetVal;
  uint16_t ChipRemainTimes = 0;
  uint8_t TxBuf[3] = {0};
  uint8_t RxBuf[3] = {0};

  //set page
  TxBuf[0] = 0x00;
  TxBuf[1] = 0x50;

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] set page to 0x00 0x50 failed\n", __func__);
    goto error_exit;
  }

  //read two bytes from address 0x82
  TxBuf[0] = 0x82;
  RxBuf[0] = 0;
  RxBuf[1] = 0;

  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] CheckChipRemainingTimes: Read data from address 0x82 failed\n", __func__);
    goto error_exit;
  }

  /*get bit 6 ~ bit 11*/
  ChipRemainTimes = (RxBuf[1] << 8) + RxBuf[0];
  ChipRemainTimes = (ChipRemainTimes >> 6 ) & 0x3f;

  //set to page 20
  TxBuf[0] = 0x00;
  TxBuf[1] = 0x20;

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Change Register Page failed[End]: 0x00 0x20 failed\n", __func__);
    goto error_exit;
  }

  //the remaining time of chip is 0, we cannot flash vr
  if ( ChipRemainTimes == 0 )
  {
    RetVal = -1;
  }

error_exit:

  return RetVal;
}

static int
write_data_to_vr(char *BusName, uint8_t SlaveAddr, uint8_t *Data)
{
  int RetVal;
  uint8_t RxBuf[3] = {0};
  uint8_t TxBuf[3] = {0};

#ifdef VR_DEBUG
  printf("[%s] %x %x %x %x \n", __func__, Data[0], Data[1], Data[2], Data[3]);
#endif
  /***set Page***/
  TxBuf[0] = 0x00;
  TxBuf[1] = (Data[0] >> 1) & 0x3f;

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2 );
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Change Register Page failed: 0x00, 0x3f \n", __func__);
    goto error_exit;
  }

#ifdef VR_DEBUG
  //Check Page is changed or not
  TxBuf[0] = 0x0;
  RxBuf[0] = 0x0;

  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Read page failed: 0x0\n", __func__);
    goto error_exit;
  }

    /***set data to buf***/
    //read data from the target address. just for debugging
  TxBuf[0] = Data[1];
  RxBuf[0] = 0;
  RxBuf[1] = 0;

  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Read failed: %x , %x, %x \n", __func__, Data[1], Data[2], Data[3]);
    goto error_exit;
  }

    printf("[%s] [Write]Page %x, Addr %x ,low byte %x, high byte %x\n", __func__, (Data[0] >> 1) & 0x3f, Data[1], Data[2], Data[3]);
#endif

    //write data to register
  TxBuf[0] = Data[1];//addr
  TxBuf[1] = Data[2];//low byte
  TxBuf[2] = Data[3];//high byte

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 3);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s]  Write failed: %x, %x, %x\n", __func__, TxBuf[0], TxBuf[1], TxBuf[2]);
    goto error_exit;
  }

  //double check -- read the address again
  TxBuf[0] = Data[1];
  RxBuf[0] = 0;
  RxBuf[1] = 0;

  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Read failed: %x , %x, %x \n",__func__ ,Data[1], Data[2], Data[3]);
    goto error_exit;
  }

  //printf("[Read2]Page %x, Addr %x ,low byte %x, high byte %x\n", page, TxBuf[0], RxBuf[0], RxBuf[1]);

  if ( (RxBuf[0] != Data[2]) || (RxBuf[1] != Data[3]) )
  {
    syslog(LOG_WARNING, "[%s] The result between Read and Write is DIFFERENT!!!!\n",__func__);
    syslog(LOG_WARNING, "[%s] Write: Page %x, Addr %x, LowByte %x, HighByte %x \n",__func__ ,(Data[0] >> 1) & 0x3f ,Data[0] ,Data[1], Data[2] );
    syslog(LOG_WARNING, "[%s] Read:  Page %x, Addr %x, LowByte %x, HighByte %x \n",__func__ ,(Data[0] >> 1) & 0x3f ,TxBuf[0] ,RxBuf[0], RxBuf[1]);
    RetVal = -1;

      goto error_exit;
  }

error_exit:

  return RetVal;
}

static int
write_vr_finish(char *BusName, uint8_t SlaveAddr)
{
  int RetVal;
  uint8_t TxBuf[3] = {0};
  uint8_t RxBuf[3] = {0};

  /*********Set Page***********/
  TxBuf[0] = 0x00;
  TxBuf[1] = 0x3f;

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Change Register Page failed: 0x00, 0x3f \n", __func__);
    goto error_exit;
  }

#ifdef VR_DEBUG
  //check page is changed or not
  TxBuf[0] = 0x00;
  RxBuf[0] = 0x00;

  //check page
  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check page failed: 0x3f \n",__func__);
    goto error_exit;
  }

  printf("[%s] Check Page: %x \n",__func__, RxBuf[0]);
#endif

  TxBuf[0] = 0x2a;
  TxBuf[1] = 0x00;
  TxBuf[2] = 0x00;

  //write word to the target address
  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf,3);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Lock register failed: 0x2a, 0x00, 0x00 \n",__func__);
    goto error_exit;
  }

  //check the writing data from the target address
  TxBuf[0] = 0x2a;
  RxBuf[0] = 0;
  RxBuf[1] = 0;

  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check data failed: 0x2a, 0x00, 0x00 \n",__func__);
    goto error_exit;
  }

error_exit:

  return RetVal;
}

static int
save_data_to_vr_NVM(char *BusName, uint8_t SlaveAddr)
{
  int RetVal;
  uint8_t TxBuf[3] = {0};
  uint8_t RxBuf[3] = {0};

  /*********Set Page***********/
  TxBuf[0] = 0x00;
  TxBuf[1] = 0x3f;

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Change Register Page failed: 0x00, 0x3f \n", __func__);
    goto error_exit;
  }

  TxBuf[0] = 0x00;
  RxBuf[0] = 0x00;

  /****Check Page is changed or not****/
  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check page failed: 0x3f \n", __func__);
    goto error_exit;
  }

  /****lock - fill buf****/
  TxBuf[0] = 0x29;
  TxBuf[1] = 0xd7;
  TxBuf[2] = 0xef;

  /****lock - write to the address****/
  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 3);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Lock register failed: 0x29, 0xd7, 0xef \n", __func__);
    goto error_exit;
  }

  /****lock - check the data again****/
  RxBuf[0] = 0;
  RxBuf[1] = 0;

  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check data failed: 0x29, 0xd7, 0xef \n", __func__);
    goto error_exit;
  }

  /*********clear fault flag***********/

  TxBuf[0] = 0x2c;

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Clear fault failed: 0x2c \n",__func__);
    goto error_exit;
  }

  /*********upload data to NVM***********/
  //initiates an upload from the register to NVM
  TxBuf[0] = 0x34;

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Lock register failed: 0x34 \n", __func__);
    goto error_exit;
  }

  /*********Set Page***********/
  sleep(3);//avoid page failure

  TxBuf[0] = 0x00;
  TxBuf[1] = 0x60;

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Change Register Page failed: 0x00, 0x60 \n", __func__);
    goto error_exit;
  }

#ifdef VR_DEBUG
  RxBuf[0] = 0;
  TxBuf[0] = 0;

  //check page
  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check page failed: 0x60 \n", __func__);

    goto error_exit;
  }
#endif

  /*********Read Data***********/
  //read word from reg 0x01 and 0x02
  //read data from reg1
  TxBuf[0] = 0x01;
  RxBuf[0] = 0;
  RxBuf[1] = 0;

  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Read data from page 0x60  addr 0x01 failed \n",__func__);
    goto error_exit;
  }

  //read data from reg2
  TxBuf[0] = 0x02;

  RxBuf[0] = 0;
  RxBuf[1] = 0;

  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Read data from page 0x60  addr 0x02 failed \n",__func__);
    goto error_exit;
  }

  /*********Page***********/
  sleep(1); //avoid page failure

  TxBuf[0] = 0x00;
  TxBuf[1] = 0x3f;

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Change Register Page failed: 0x00, 0x3f \n",__func__);
    goto error_exit;
  }

#ifdef VR_DEBUG
  //check page
  TxBuf[0] = 0x0;
  RxBuf[0] = 0x0;

  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check page failed: 0x3f \n",__func__);
    goto error_exit;
  }

  //printf("[%s] Check Page: %x \n",__func__, RxBuf[0]);
#endif

  /*********End***********/
  TxBuf[0] = 0x29;
  TxBuf[1] = 0x00;
  TxBuf[2] = 0x00;

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 3);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Failed to write 0x29 0x00 0x00 \n",__func__);
    goto error_exit;
  }

  sleep(2);

#ifdef VR_DEBUG
  TxBuf[0] = 0x29;

  RxBuf[0] = 0x0;
  RxBuf[1] = 0x0;

  //check write data
  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check data failed: 0x29 0x00 0x00 \n",__func__);
    goto error_exit;
  }
#endif

error_exit:

  return RetVal;
}

static int
check_vr_CRC(char *BusName, uint8_t SlaveAddr, uint8_t *ExpectedCRC)
{
  int RetVal=0;
  uint8_t TxBuf[3]={0};
  uint8_t RxBuf[4]={0};

  /****Set Page****/
  TxBuf[0] = 0x00;
  TxBuf[1] = 0x3f;

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Change Register Page failed: 0x00, 0x3f \n", __func__);
    goto error_exit;
  }

#ifdef VR_DEBUG
  /****Check Page changed or not****/
  TxBuf[0] = 0;
  RxBuf[0] = 0;

  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] read page failed: 0x3f \n", __func__);
    goto error_exit;
  }
#endif

  sleep(1);//force sleep

  /*********data***********/
  TxBuf[0] = 0x27;
  TxBuf[1] = 0x7c;
  TxBuf[2] = 0xb3;

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 3);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] set data failed: 0x27 0x7c 0xb3 \n", __func__);
    goto error_exit;
  }

#ifdef VR_DEBUG
  //check data
  TxBuf[0] = 0x27;

  RxBuf[0] = 0;
  RxBuf[1] = 0;

  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check data failed: 0x27 0x7c 0xb3 \n", __func__);
    goto error_exit;
  }
#endif

  /*********data***********/
  TxBuf[0] = 0x32;

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] set data failed: 0x32  \n", __func__);
    goto error_exit;
  }


  /*********Page***********/
  TxBuf[0] = 0x00;
  TxBuf[1] = 0x6f;

  RetVal = i2c_io_master_write(BusName, SlaveAddr, TxBuf, 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Change Register Page failed: 0x00, 0x6f \n", __func__);
    goto error_exit;
  }

#ifdef VR_DEBUG
  /*********Check page changed or not***********/
  TxBuf[0] = 0x00;
  RxBuf[0] = 0x00;

  //check page
  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, RxBuf, 1);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] Check page failed: 0x6f \n", __func__);
    goto error_exit;
  }
#endif

  /********read data*************/
  TxBuf[0] = 0x3d;

  RxBuf[0] = 0x00;
  RxBuf[1] = 0x00;

  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, &RxBuf[0], 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] read addr failed: 0x3d \n", __func__);
    goto error_exit;
  }

  TxBuf[0] = 0x3e;
  RxBuf[2] = 0x00;
  RxBuf[3] = 0x00;

  RetVal = i2c_io_rw(BusName, SlaveAddr, TxBuf, 1, &RxBuf[2], 2);
  if ( RetVal < 0 )
  {
    syslog(LOG_WARNING, "[%s] read addr failed: 0x3e \n",__func__);
    goto error_exit;
  }

  //check CRC match
  if ( (RxBuf[3] != ExpectedCRC[0]) || (RxBuf[2] != ExpectedCRC[1]) || (RxBuf[1] != ExpectedCRC[2]) || (RxBuf[0] != ExpectedCRC[3]))
  {
    syslog(LOG_WARNING, "[%s] CRC cannot MATCH!!\n",__func__);
    syslog(LOG_WARNING, "[%s] Expected CRC: %x %x %x %x\n",__func__, ExpectedCRC[0], ExpectedCRC[1], ExpectedCRC[2], ExpectedCRC[3]);
    syslog(LOG_WARNING, "[%s] Actual CRC: %x %x %x %x\n",__func__, RxBuf[3], RxBuf[2], RxBuf[1], RxBuf[0]);

    RetVal = -1;

    goto error_exit;
  }

error_exit:
  return RetVal;
}

static int
get_vr_update_data(const char *update_file, uint8_t **BinData)
{
    FILE *fp;
    int FileSize = 0;
    int RetVal = 0;
    fp = fopen( update_file, "rb" );
    if ( fp )
    {
        //get file size
        fseek(fp, 0L, SEEK_END);
        FileSize = ftell(fp);
        *BinData = (uint8_t*) malloc( FileSize * sizeof(uint8_t));
        //recovery the fp to the beginning
        rewind(fp);
        fread(*BinData, sizeof(uint8_t), FileSize, fp);
        fclose(fp);
#ifdef VR_DEBUG
        int i;
        for ( i = 0; i < FileSize; i++ )
        {
            if ( (i % 16 == 0) && (i != 0) )
            {
                printf("%x ", *BinData[i]);
                printf("\n");
            }
            else
            {
                printf("%x ", *BinData[i]);
            }
        }
#endif
        printf("Data Size: %d bytes!\n", FileSize);
    }
    else
    {
        RetVal = -1;
    }
    return RetVal;
}

static int vr_fw_update_binary(uint8_t *BinData, uint8_t BoardInfo)
{
  VRBasicInfo vr_data[MAX_VR_CHIPS];//there are nine vr chips
  int VRDataLength = 0;
  int VR_counts = MAX_VR_CHIPS;
  int i, j;
  int fd;
  int RetVal;
  int TotalProgress = 0;
  int CurrentProgress = 0;
  char BusName[64];
  uint8_t BOARD_SKU_ID0;

  //create a file to inform "vr_read function". do not send req to VR
  fd = open(VR_UPDATE_IN_PROGRESS, O_WRONLY | O_CREAT | O_TRUNC, 0700);
  if ( -1 == fd )
  {
    printf("[%s] Create %s file error",__func__, VR_UPDATE_IN_PROGRESS);
    RetVal = -1;
    goto error_exit;
  }

  close(fd);

  //wait for the sensord monitor cycle end
  sleep(3);

  snprintf(BusName, sizeof(BusName), "/dev/i2c-%d", VR_BUS_ID);

  //init vr array
  memset(vr_data, 0, sizeof(vr_data));

#ifdef VR_DEBUG
    printf("[%s] VR Counts:%d, Data_Start_Addr: %d\n", __func__, VR_counts, DATA_START_ADDR);
#endif

  //before updating vr, we parse data first.
  //DataLength = update data header + update data
  for ( i = 0 ; i < VR_counts; i++ )
  {
    vr_data[i].DataLength = (BinData[ (2*i) ] << 8 ) + BinData[ (2*i) + 1 ];
#ifdef VR_DEBUG
    printf("[%s] DataLength: %d \n", __func__, vr_data[i].DataLength);
#endif
  }

    //catch the basic info
  for ( i = 0 ; i < VR_counts; i++ )
  {
    /*need to identify the first one. and set the start address*/
    if ( 0 == i )
    {
      j = DATA_START_ADDR;

      VRDataLength = vr_data[ i ].DataLength + DATA_START_ADDR;

    }
    else
    {
      j = VRDataLength;

      VRDataLength = vr_data[ i ].DataLength + VRDataLength;
    }

    vr_data[i].SlaveAddr = BinData[ j ];
    vr_data[i].IndexStartAddr = j + 5;
    //copy crc
    memcpy(vr_data[i].CRC, &BinData[ j + 1 ], 4);
    vr_data[i].IndexEndAddr = VRDataLength;
#ifdef VR_DEBUG
    printf("[%s]: SlaveAddr: %x\n", __func__, vr_data[i].SlaveAddr );
    printf("[%s]: CRC %x %x %x %x\n", __func__, vr_data[i].CRC[0], vr_data[i].CRC[1],
          vr_data[i].CRC[2], vr_data[i].CRC[3]);
#endif

  }

  //calculate the reality data length
  for ( i = 0 ; i < VR_counts; i++ )
  {
    TotalProgress += (vr_data[i].IndexEndAddr - vr_data[i].IndexStartAddr);
  }

  /**Check Chip**/
  printf("Check Data...");

  //check the remaining times of flashing chip
  for ( i = 0 ; i < VR_counts; i++ )
  {
    RetVal = check_vr_chip_remaining_times( BusName, vr_data[i].SlaveAddr );
    if ( RetVal < 0 )
    {
      //The Remaining Times of Chip is 0!
      goto error_exit;
    }
  }

  BOARD_SKU_ID0 = BoardInfo & 0x1; //get FM_BOARD_SKU_ID0
  if ( BOARD_SKU_ID0 )
  {
    //get 0x5e0c-userdata(version) data to compare the
      RetVal = check_vr_fw_code_match_MB(vr_data[0].IndexStartAddr, vr_data[0].IndexEndAddr, BinData, BoardInfo);
      if ( RetVal < 0 )
      {
          syslog(LOG_WARNING, "[%s] Please Check vr fw code!\n", __func__);
          goto error_exit;
      }
  }

  //check CRC
  for ( i = 0 ; i < VR_counts; i++ )
  {
    RetVal = check_integrity_of_vr_data(vr_data[i].SlaveAddr, vr_data[i].CRC, vr_data[i].IndexStartAddr, vr_data[i].IndexEndAddr, BinData);
    if ( RetVal < 0 )
    {
      //the bin file is incomplete
      goto error_exit;
    }
  }

  /**Check Chip**/
  printf("Done\n");

  for ( i = 0; i < VR_counts; i++ )
  {

#ifdef VR_DEBUG
    printf("[%s][%d]vr_fw_update: UnlockRegister \n", __func__, i);
#endif

    RetVal = unlock_vr_register( BusName, vr_data[i].SlaveAddr );
    if ( RetVal < 0 )
    {
      //unlock register failed
      syslog(LOG_WARNING, "[%s] Failed Occurred at Unlocking Register\n", __func__);
      goto error_exit;
    }

    //the starting data index of the current vr
    j = vr_data[i].IndexStartAddr;

    //the end index of the current vr
    int EndAddr = vr_data[i].IndexEndAddr;

    for ( ; j < EndAddr; j = j + 4 )// 4 bytes
    {
      CurrentProgress += 4;

      printf("Writing Data: %d/%d (%.2f%%) \r",(CurrentProgress), TotalProgress, (((CurrentProgress)/(float)TotalProgress)*100));

      RetVal = write_data_to_vr( BusName, vr_data[i].SlaveAddr, &BinData[j] );
      if ( RetVal < 0 )
      {
        syslog(LOG_WARNING, "[%s] Failed Occurred at Writing Data\n", __func__);
        goto error_exit;
      }

    }

    //After writing data, we need to store it
    RetVal = write_vr_finish( BusName, vr_data[i].SlaveAddr );
    if ( RetVal < 0 )
    {
      syslog(LOG_WARNING, "[%s] Error at Locking register\n", __func__);
      goto error_exit;
    }

    RetVal = save_data_to_vr_NVM( BusName, vr_data[i].SlaveAddr );
    if ( RetVal < 0 )
    {
      syslog(LOG_WARNING, "[%s] Error at Saving Data\n", __func__);
      goto error_exit;
    }

    RetVal = check_vr_CRC ( BusName, vr_data[i].SlaveAddr, vr_data[i].CRC);
    if ( RetVal < 0 )
    {
      syslog(LOG_WARNING, "[%s] Error at CRC value from the register\n", __func__);
      goto error_exit;
    }

  }

  printf("\nUpdate VR Success!\n");

error_exit:
  if ( -1 == remove(VR_UPDATE_IN_PROGRESS) )
  {
    printf("[%s] Remove %s Error\n", __func__, VR_UPDATE_IN_PROGRESS);
    RetVal = -1;
  }
  return RetVal;
}

int vr_fw_update(uint8_t fru, uint8_t board_info, const char *file)
{
    uint8_t *BinData=NULL;
    int ret;

    ret = get_vr_update_data(file, &BinData);
    if ( ret < 0 )
    {
      printf("Cannot locate the file at: %s!\n", file);
      return ret;
    }

    ret = vr_fw_update_binary(BinData, board_info);
    if ( ret < 0 )
    {
      printf("Error Occur at updating VR FW!\n");
      return ret;
    }
    return 0;
}

