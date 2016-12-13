/*
 *
 * Copyright 2016-present Facebook. All Rights Reserved.
 *
 * This file represents platform specific implementation for storing
 * SDR record entries and acts as back-end for IPMI stack
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <sys/types.h>

typedef struct _post_desc {
  uint8_t code;
  uint8_t desc[32];
} post_desc_t;

typedef struct _gpio_desc {
  uint8_t pin;
  uint8_t level;
  uint8_t def;
  uint8_t desc[32];
} gpio_desc_t;

static post_desc_t pdesc[] = {
  // SEC Phase
  { 0x01, "POWER_ON" },
  { 0x02, "MICROCODE" },
  { 0x03, "CACHE_ENABLED" },
  { 0x04, "SECSB_INIT" },
  { 0x06, "CPU_EARLY_INIT" },
  { 0xCA, "GENUINE_CPU" },
  { 0xCB, "PNUI_VCU_FAIL" },
  { 0xCC, "CSR_FATAL" },
  { 0xCD, "PROC_MICROCODE" },
  { 0xCE, "BUS_NOSTACK" },
  { 0xD0, "CACHE_TEST_FAIL" },
  { 0xD1, "CFG_TEST_FAIL" },

  // PEI Phase
  { 0x10, "CORE_START" },
  { 0x11, "PM_CPU_INITS" },
  { 0x12, "PM_CPU_INIT1" },
  { 0x13, "PM_CPU_INIT2" },
  { 0x14, "PM_CPU_INIT3" },
  { 0x15, "PM_NB_INITS" },
  { 0x16, "PM_NB_INIT1" },
  { 0x17, "PM_NB_INIT2" },
  { 0x18, "PM_NB_INIT3" },
  { 0x19, "PM_SB_INITS" },
  { 0x1A, "PM_SB_INIT1" },
  { 0x1B, "PM_SB_INIT2" },
  { 0x1C, "PM_SB_INIT3" },
  { 0x2B, "MEM_INIT_SPD" },
  { 0x2C, "MEM_INIT_PRS" },
  { 0x2D, "MEM_INIT_PROG" },
  { 0x2E, "MEM_INIT_CFG" },
  { 0x2F, "MEM_INIT" },
  { 0x31, "MEM_INSTALLED" },
  { 0x32, "CPU_INITS" },
  { 0x33, "CPU_CACHE_INIT" },
  { 0x34, "CPU_AP_INIT" },
  { 0x35, "CPU_BSP_INIT" },
  { 0x36, "CPU_SMM_INIT" },
  { 0x37, "NB_INITS" },
  { 0x38, "NB_INIT1" },
  { 0x39, "NB_INIT2" },
  { 0x3A, "NB_INIT3" },
  { 0x3B, "SB_INITS" },
  { 0x3C, "SB_INIT1" },
  { 0x3D, "SB_INIT2" },
  { 0x3E, "SB_INIT3" },
  { 0x4F, "DXL_IPL" },
  { 0x50, "INVALID_MEM" },
  { 0x51, "SPD_READ_FAIL" },
  { 0x52, "INVALID_MEM_SIZE" },
  { 0x53, "NO_USABLE_MEMORY" },
  { 0x54, "MEM_INIT_ERROR" },
  { 0x55, "MEM_NOT_INSTALLED" },
  { 0x56, "INVALID_CPU" },
  { 0x57, "CPU_MISMATCH" },
  { 0x58, "CPU_SELFTEST_FAIL" },
  { 0x59, "MICROCODE_FAIL" },
  { 0x5A, "CPU_INT_ERROR" },
  { 0x5B, "RESET_PPI_ERROR" },

  // DXE Phase
  { 0xD0, "CPU_INIT_ERROR" },
  { 0xD1, "NB_INIT_ERROR" },
  { 0xD2, "SB_INIT_ERROR" },
  { 0xD3, "ARCH_PROT_ERROR" },
  { 0xD4, "PCI_ALLOC_ERROR" },
  { 0xD5, "NO_SPACE_ROM" },
  { 0xD6, "NO_CONSOLE_OUT" },
  { 0xD7, "NO_CONSOLE_IN" },
  { 0xD8, "INVALID_PASSWD" },
  { 0xD9, "BOOT_OPT_ERROR" },
  { 0xDA, "BOOT_OPT_FAIL" },
  { 0xDB, "FLASH_UPD_FAIL" },
  { 0xDC, "RST_PROT_NA" },
};

static gpio_desc_t gdesc[] = {
  { 10, 0, 2, "FM_DBG_RST_BTN" },
  { 11, 0, 1, "FM_PWR_BTN" },
  { 12, 0, 0, "SYS_PWROK" },
  { 13, 0, 0, "RST_PLTRST" },
  { 14, 0, 0, "DSW_PWROK" },
  { 15, 0, 0, "FM_CPU_CATERR" },
  { 16, 0, 0, "FM_SLPS3" },
  { 17, 0, 0, "FM_CPU_MSMI" },
};

static uint8_t test_page1[128] = {0};
static uint8_t test_page2[128] = {0};

static int pdesc_count = sizeof(pdesc) / sizeof (post_desc_t);
static int gdesc_count = sizeof(gdesc) / sizeof (gpio_desc_t);

int
plat_udbg_get_frame_info(uint8_t *num) {
  *num = 1;
  return 0;
}

int
plat_udbg_get_updated_frames(uint8_t *count, uint8_t *buffer) {
  *count = 1;
  buffer[0] = 1;
  return 0;
}

int
plat_udbg_get_post_desc(uint8_t index, uint8_t *next, uint8_t *count, uint8_t *buffer) {
  int i = 0;

  // If the index is 0x00: populate the next pointer with the first
  if (index == 0x00) {
    *next = pdesc[0].code;
    *count = 0;
    return 0;
  }

  // Look for the index
  for (i = 0; i < pdesc_count; i++) {
    if (index == pdesc[i].code) {
      break;
    }
  }

  // Check for not found
  if (i == pdesc_count) {
    return -1;
  }

  // Populate the description and next index
  *count = strlen(pdesc[i].desc);
  memcpy(buffer, pdesc[i].desc, *count);
  buffer[*count] = '\0';

  // Populate the next index
  if (i == pdesc_count-1) { // last entry
    *next = 0xFF;
  } else {
    *next = pdesc[i+1].code;
  }

  return 0;
}

int
plat_udbg_get_gpio_desc(uint8_t index, uint8_t *next, uint8_t *level, uint8_t *def,
                            uint8_t *count, uint8_t *buffer) {
  int i = 0;

  // If the index is 0x00: populate the next pointer with the first
  if (index == 0x00) {
    *next = gdesc[0].pin;
    *count = 0;
    return 0;
  }

  // Look for the index
  for (i = 0; i < gdesc_count; i++) {
    if (index == gdesc[i].pin) {
      break;
    }
  }

  // Check for not found
  if (i == gdesc_count) {
    return -1;
  }

  // Populate the description and next index
  *level = gdesc[i].level;
  *def = gdesc[i].def;
  *count = strlen(gdesc[i].desc);
  memcpy(buffer, gdesc[i].desc, *count);
  buffer[*count] = '\0';

  // Populate the next index
  if (i == gdesc_count-1) { // last entry
    *next = 0xFF;
  } else {
    *next = gdesc[i+1].pin;
  }

  return 0;
}

int
plat_udbg_get_frame_data(uint8_t frame, uint8_t page, uint8_t *next, uint8_t *count, uint8_t buffer) {
  static uint8_t once = 1;
  int i = 0;

  if (once) {
    // Initailize test page#1 (assuming 8 x 16 screen)
    memcpy(test_page1, "     Frame#1    ", 16); // First line

    for (i = 16; i < (127-16); i++) {
      test_page1[i] = 16+i;
    }

    memcpy(&test_page1[127-16], "   Page 1 of 2   ", 15); // Last line

    // Initailize test page#2 (assuming 8 x 16 screen)
    memcpy(test_page2, "     Frame#1    ", 16); // First line

    for (i = 127-16; i >= 16; i--) {
      test_page2[i] = 16+i;
    }

    memcpy(&test_page2[127-16], "   Page 2 of 2   ", 15); // Last line
  }

  // Test frame/page
  if (frame > 1 || page > 2) {
    return -1;
  }

  // Set next to 0xFF to indicate this is last page
  *next = 0xFF;

  *count = 128;
  if (page == 1) {
    memcpy(test_page1, buffer, 128);
  } else {
    memcpy(test_page2, buffer, 128);
  }

  return 0;
}
