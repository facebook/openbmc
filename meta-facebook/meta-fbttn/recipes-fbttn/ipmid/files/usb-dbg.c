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
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openbmc/pal.h>
#include <openbmc/fruid.h>
#include <openbmc/sdr.h>
#include <openbmc/obmc-sensor.h>
#include <facebook/bic.h>

#define ESCAPE "\x1B"
#define ESC_BAT ESCAPE"B"
#define ESC_MCU_BL_VER ESCAPE"U"
#define ESC_MCU_RUN_VER ESCAPE"R"
#define ESC_ALT ESCAPE"[5;7m"
#define ESC_RST ESCAPE"[m"

#define LINE_DELIMITER '\x1F'

#define FRAME_BUFF_SIZE 4096
#define FRAME_PAGE_BUF_SIZE 256

#define GPIO_UART_SEL 145

struct frame {
  char title[32];
  size_t max_size;
  size_t max_page;
  char *buf;
  uint16_t idx_head, idx_tail;
  uint8_t line_per_page;
  uint8_t line_width;
  uint16_t lines, pages;
  uint8_t esc_sts;
  uint8_t overwrite;
  time_t mtime;
  int (*init)(struct frame *self, size_t size);
  int (*append)(struct frame *self, char *string, int indent);
  int (*insert)(struct frame *self, char *string, int indent);
  int (*getPage)(struct frame *self, int page, char *page_buf, size_t page_buf_size);
  int (*isFull)(struct frame *self);
  int (*isEscSeq)(struct frame *self, char chr);
  int (*parse)(struct frame *self, char *buf, size_t buf_size, char *input, int indent);
};

// return 0 on seccuess
static int frame_init (struct frame *self, size_t size) {
  // Reset status
  self->idx_head = self->idx_tail = 0;
  self->lines = 0;
  self->esc_sts = 0;
  self->pages = 1;

  if (self->buf != NULL && self->max_size == size) {
    // reinit
    return 0;
  }

  if (self->buf != NULL && self->max_size != size){
    free(self->buf);
  }
  // Initialize Configuration
  self->title[0] = '\0';
  self->buf = malloc(size);
  self->max_size = size;
  self->max_page = size;
  self->line_per_page = 7;
  self->line_width = 16;
  self->overwrite = 0;

  if (self->buf)
    return 0;
  else
    return -1;
}

// return 0 on seccuess
static int frame_append (struct frame *self, char *string, int indent)
{
  const size_t buf_size = 64;
  char buf[buf_size];
  char *ptr;
  int ret;

  ret = self->parse(self, buf, buf_size, string, indent);

  if (ret < 0)
    return ret;

  for (ptr = buf; *ptr != '\0'; ptr++) {
    if (self->isFull(self)) {
      if (self->overwrite) {
        if (self->buf[self->idx_head] == LINE_DELIMITER)
          self->lines--;
        self->idx_head = (self->idx_head + 1) % self->max_size;
      } else
        return -1;
    }

    self->buf[self->idx_tail] = *ptr;
    if (*ptr == LINE_DELIMITER)
      self->lines++;

    self->idx_tail = (self->idx_tail + 1) % self->max_size;
  }

  self->pages = (self->lines / self->line_per_page) +
    ((self->lines % self->line_per_page)?1:0);

  if (self->pages > self->max_page)
    self->pages = self->max_page;

  return 0;
}

// return 0 on seccuess
static int frame_insert (struct frame *self, char *string, int indent)
{
  const size_t buf_size = 64;
  char buf[buf_size];
  char *ptr;
  int ret;

  ret = self->parse(self, buf, buf_size, string, indent);

  if (ret < 0)
    return ret;

  for (ptr = &buf[strlen(buf)-1]; ptr != (buf - 1); ptr--) {
    if (self->isFull(self)) {
      if (self->overwrite) {
        self->idx_tail = (self->idx_tail + self->max_size - 1) % self->max_size;
        if (self->buf[self->idx_tail] == LINE_DELIMITER)
          self->lines--;
      } else
        return -1;
    }

    self->idx_head = (self->idx_head + self->max_size - 1) % self->max_size;

    self->buf[self->idx_head] = *ptr;
    if (*ptr == LINE_DELIMITER)
      self->lines++;
  }

  self->pages = (self->lines / self->line_per_page) +
    ((self->lines % self->line_per_page)?1:0);

  if (self->pages > self->max_page)
    self->pages = self->max_page;

  return 0;
}

// return page size
static int frame_getPage (struct frame *self, int page, char *page_buf, size_t page_buf_size)
{
  int ret;
  uint16_t line = 0;
  uint16_t idx, len;

  if (self == NULL || self->buf == NULL)
    return -1;

  // 1-based page
  if (page > self->pages || page < 1)
    return -1;

  if (page_buf == NULL || page_buf_size < 0)
    return -1;

  ret = snprintf(page_buf, 17, "%-10s %02d/%02d", self->title, page, self->pages);
  len = strlen(page_buf);
  if (ret < 0)
    return -1;

  line = 0;
  idx = self->idx_head;
  while (line < ((page-1) * self->line_per_page) && idx != self->idx_tail) {
    if (self->buf[idx] == LINE_DELIMITER)
      line++;
    idx = (idx + 1) % self->max_size;
  }

  while (line < ((page) * self->line_per_page) && idx != self->idx_tail) {
    if (self->buf[idx] == LINE_DELIMITER) {
      line++;
    } else {
      page_buf[len++] = self->buf[idx];
      if (len == (page_buf_size - 1)) {
        break;
      }
    }
    idx = (idx + 1) % self->max_size;
  }

  return len;
}

// return 1 for frame buffer full
static int frame_isFull (struct frame *self)
{
  if (self == NULL || self->buf == NULL)
    return -1;

  if ((self->idx_tail + 1) % self->max_size == self->idx_head)
    return 1;
  else
    return 0;
}

// return 1 for Escape Sequence
static int frame_isEscSeq(struct frame *self, char chr) {
  uint8_t curr_sts = self->esc_sts;

  if (self == NULL)
    return -1;

  if (self->esc_sts == 0 && (chr == 0x1b))
    self->esc_sts = 1; // Escape Sequence
  else if (self->esc_sts == 1 && (chr == 0x5b))
    self->esc_sts = 2; // Control Sequence Introducer(CSI)
  else if (self->esc_sts == 1 && (chr != 0x5b))
    self->esc_sts = 0;
  else if (self->esc_sts == 2 && (chr>=0x40 && chr <=0x7e))
    self->esc_sts = 0;

  if (curr_sts || self->esc_sts)
    return 1;
  else
    return 0;
}

// return 0 on seccuess
static int frame_parse (struct frame *self, char *buf, size_t buf_size, char *input, int indent)
{
  uint8_t pos, esc;
  int i;
  char *in, *end;

  if (self == NULL || self->buf == NULL || input == NULL)
    return -1;

  if (indent >= self->line_width || indent < 0)
    return -1;

  in = input;
  end = in + strlen(input);
  pos = 0;  // line position
  esc = 0; // escape state
  i = 0; // buf index
  while (in != end) {
    if (i >= buf_size)
      break;

    if (pos < indent) {
      // fill indent
      buf[i++] = ' ';
      pos++;
      continue;
    }

    esc = self->isEscSeq(self, *in);

    if (!esc && pos == self->line_width) {
      buf[i++] = LINE_DELIMITER;
      pos = 0;
      continue;
    }

    if (!esc)
      pos++;

    // fill input data
    buf[i++] = *(in++);
  }

  // padding
  while (pos <= self->line_width) {
    if (i >= buf_size)
      break;
    if (pos < self->line_width)
      buf[i++] = ' ';
    else
      buf[i++] = LINE_DELIMITER;
    pos++;
  }

  // full
  if (i >= buf_size)
    return -1;

  buf[i++] = '\0';

  return 0;
}

#define FRAME_DECLARE(NAME) \
struct frame NAME = {\
  .buf = NULL,\
  .pages = 0,\
  .mtime = 0,\
  .init = frame_init,\
  .append = frame_append,\
  .insert = frame_insert,\
  .getPage = frame_getPage,\
  .isFull = frame_isFull,\
  .isEscSeq = frame_isEscSeq,\
  .parse = frame_parse,\
};

static FRAME_DECLARE(frame_info);
static FRAME_DECLARE(frame_sel);
static FRAME_DECLARE(frame_snr);

enum ENUM_PANEL {
  PANEL_MAIN = 1,
  PANEL_BOOT_ORDER = 2,
  PANEL_POWER_POLICY = 3,
};

struct ctrl_panel {
  uint8_t parent;
  uint8_t item_num;
  char item_str[8][32];
  uint8_t (*select)(uint8_t item);
};

static uint8_t panel_main (uint8_t item);
static uint8_t panel_boot_order (uint8_t item);
static uint8_t panel_power_policy (uint8_t item);

static struct ctrl_panel panels[] = {
  { /* dummy entry for making other to 1-based */ },
  {
    .parent = PANEL_MAIN,
    .item_num = 2,
    .item_str = {
      "User Setting",
      ">Boot Order",
      ">Power Policy",
    },
    .select = panel_main,
  },
  {
    .parent = PANEL_MAIN,
    .item_num = 0,
    .item_str = {
      "Boot Order",
    },
    .select = panel_boot_order,
  },
  {
    .parent = PANEL_MAIN,
    .item_num = 0,
    .item_str = {
      "Power Policy",
    },
    .select = panel_power_policy,
  },
};
static int panelNum = (sizeof(panels)/sizeof(struct ctrl_panel)) - 1;

extern void plat_lan_init(lan_config_t *lan);

typedef struct _post_desc {
  uint8_t code;
  char    desc[32];
} post_desc_t;

typedef struct _gpio_desc {
  uint8_t pin;
  uint8_t level;
  uint8_t def;
  char    desc[32];
} gpio_desc_t;

typedef struct _sensor_desc {
  char    name[16];
  uint8_t sensor_num;
  char    unit[5];
  uint8_t fru;
} sensor_desc_c;

//These postcodes are defined in document "F08 BIOS Specification" Revision: 2A
static post_desc_t pdesc_phase1[] = {
  { 0x00, "Not used" },
  { 0x01, "POWER_ON" },
  { 0x02, "MICROCODE" },
  { 0x03, "CACHE_ENABLED" },
  { 0x04, "SECSB_INIT" },
  { 0x05, "OEM_INIT_ENTRY" },
  { 0x06, "CPU_EARLY_INIT" },
  { 0x1D, "OEM_pMEM_INIT" },
  { 0x19, "PM_SB_INITS" },
  { 0xA1, "STS_COLLECT_INFO" },
  { 0xA3, "STS_SETUP_PATH" },
  { 0xA7, "STS_TOPOLOGY_DISC" },
  { 0xA8, "STS_FINAL_ROUTE" },
  { 0xA9, "STS_FINAL_IO_SAD" },
  { 0xAA, "STS_PROTOCOL_SET" },
  { 0xAE, "STS_COHERNCY_SETUP" },
  { 0xAF, "STS_KTI_COMPLETE" },
  { 0xE0, "S3_RESUME_START" },
  { 0xE1, "S3_BOOT_SCRIPT_EXE" },
  { 0xE4, "AMI_PROG_CODE" },
  { 0xE3, "S3_OS_WAKE" },
  { 0xE5, "AMI_PROG_CODE" },
  { 0xB0, "STS_DIMM_DETECT" },
  { 0xB1, "STS_CHECK_INIT" },
  { 0xB4, "STS_RAKE_DETECT" },
  { 0xB2, "STS_SPD_DATA" },
  { 0xB3, "STS_GLOBAL_EARILY" },
  { 0xB6, "STS_DDRIO_INIT" },
  { 0xB7, "STS_TRAIN_DRR" },
  { 0xBE, "STS_GET_MARGINS" },
  { 0xB8, "STS_INIT_THROTTLING" },
  { 0xB9, "STS_MEMBIST" },
  { 0xBA, "STS_MEMINIT" },
  { 0xBB, "STS_DDR_M_INIT" },
  { 0xBC, "STS_RAS_MEMMAP" },
  { 0xBF, "STS_MRC_DONE" },
  { 0xE6, "AMI_PROG_CODE" },
  { 0xE7, "AMI_PROG_CODE" },
  { 0xE8, "S3_RESUME_FAIL" },
  { 0xE9, "S3_PPI_NOT_FOUND" },
  { 0xEB, "S3_OS_WAKE_ERR" },
  { 0xEC, "AMI_ERR_CODE" },
  { 0xED, "AMI_ERR_CODE" },
  { 0xEE, "AMI_ERR_CODE" },

  /*--------------------- UPI Phase - Start--------------------- */
  { 0xA0, "STS_DATA_INIT" },
  { 0xA6, "STS_PBSP_SYNC" },
  { 0xAB, "STS_FULL_SPEED" },
  /*--------------------- UPI Phase - End--------------------- */

  /*--------------------- SEC Phase - Start--------------------- */
  { 0x07, "AP_INIT" },
  { 0x08, "NB_INIT" },
  { 0x09, "SB_INIT" },
  { 0x0A, "OEM_INIT_END" },
  { 0x0B, "CASHE_INIT" },
  { 0x0C, "SEC_ERR" },
  { 0x0D, "SEC_ERR" },
  { 0x0E, "MICROC_N_FOUND" },
  { 0x0F, "MICROC_N_LOAD" },
  /*--------------------- SEC Phase - End----------------------- */

  /*--------------------- PEI Phase - Start--------------------- */
  { 0x10, "PEI_CORE_START" },
  { 0x11, "PM_CPU_INITS" },
  { 0x12, "PM_CPU_INIT1" },
  { 0x13, "PM_CPU_INIT2" },
  { 0x14, "PM_CPU_INIT3" },
  { 0x15, "PM_NB_INITS" },
  { 0x16, "PM_NB_INIT1" },
  { 0x17, "PM_NB_INIT2" },
  { 0x18, "PM_NB_INIT3" },
  { 0x1A, "PM_SB_INIT1" },
  { 0x1B, "PM_SB_INIT2" },
  { 0x1C, "PM_SB_INIT3" },
  { 0x1E, "OEM_pMEM_INIT" },
  { 0x1F, "OEM_pMEM_INIT" },

  { 0x20, "OEM_pMEM_INIT" },
  { 0x21, "OEM_pMEM_INIT" },
  { 0x22, "OEM_pMEM_INIT" },
  { 0x23, "OEM_pMEM_INIT" },
  { 0x24, "OEM_pMEM_INIT" },
  { 0x25, "OEM_pMEM_INIT" },
  { 0x26, "OEM_pMEM_INIT" },
  { 0x27, "OEM_pMEM_INIT" },
  { 0x28, "OEM_pMEM_INIT" },
  { 0x29, "OEM_pMEM_INIT" },
  { 0x2A, "OEM_pMEM_INIT" },
  { 0x2B, "MEM_INIT_SPD" },
  { 0x2C, "MEM_INIT_PRS" },
  { 0x2D, "MEM_INIT_PROG" },
  { 0x2E, "MEM_INIT_CFG" },
  { 0x2F, "MEM_INIT" },

  { 0x30, "ASL_STATUS" },
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
  { 0x3F, "OEM_pMEM_INIT" },

  { 0x41, "OEM_pMEM_INIT" },
  { 0x42, "OEM_pMEM_INIT" },
  { 0x43, "OEM_pMEM_INIT" },
  { 0x44, "OEM_pMEM_INIT" },
  { 0x45, "OEM_pMEM_INIT" },
  { 0x46, "OEM_pMEM_INIT" },
  { 0x47, "OEM_pMEM_INIT" },
  { 0x48, "OEM_pMEM_INIT" },
  { 0x49, "OEM_pMEM_INIT" },
  { 0x4A, "OEM_pMEM_INIT" },
  { 0x4B, "OEM_pMEM_INIT" },
  { 0x4C, "OEM_pMEM_INIT" },
  { 0x4D, "OEM_pMEM_INIT" },
  { 0x4E, "OEM_pMEM_INIT" },

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
  { 0x5A, "CPU_INT_ERR" },
  { 0x5B, "RESET_PPI_ERR" },
  { 0x5C, "BMC_SELF_TEST_F" },
  { 0x5D, "AMI_ERR_CODE" },
  { 0x5E, "AMI_ERR_CODE" },
  { 0x5F, "AMI_ERR_CODE" },

  // S3 Resume Progress Code
  { 0xE2, "S3_VIDEO_REPOST" },

  // S3 Resume Error Code
  { 0xEA, "S3_BOOT_SCRIPT_ERR" },
  { 0xEF, "AMI_ERR_CODE" },

  // Recovery Progress Code
  { 0xF0, "REC_BY_FW" },
  { 0xF1, "REC_BY_USER" },
  { 0xF2, "REC_STARTED" },
  { 0xF3, "REC_FW_FOUNDED" },
  { 0xF4, "REC_FW_LOADED" },
  { 0xF5, "AMI_PROG_CODE" },
  { 0xF6, "AMI_PROG_CODE" },
  { 0xF7, "AMI_PROG_CODE" },

  // Recovery Error code
  { 0xF8, "RECOVERY_PPI_FAIL" },
  { 0xFA, "RECOVERY_CAP_ERR" },
  { 0xFB, "AMI_ERR_CODE" },
  { 0xFC, "AMI_ERR_CODE" },
  { 0xFD, "AMI_ERR_CODE" },
  { 0xFE, "AMI_ERR_CODE" },
  { 0xFF, "AMI_ERR_CODE" },
  /*--------------------- PEI Phase - End----------------------- */

  /*--------------------- MRC Phase - Start--------------------- */
  { 0xB5, "STS_CHAN_EARILY" },
  { 0xBD, "STS_RAS_CONF" },
  /*--------------------- MRC Phase - End----------------------- */

  { 0x4F, "DXE_IPL_START" }
};

static post_desc_t pdesc_phase2[] = {
  { 0x61, "NVRAM_INIT" },
  { 0x9A, "USB_INIT" },
  { 0x78, "ACPI_INIT" },
  { 0x68, "PCI_BRIDEGE_INIT" },
  { 0x70, "SB_DXE_START" },
  { 0x79, "CSM_INIT" },
  { 0xD1, "NB_INIT_ERR" },
  { 0xD2, "SB_INIT_ERR" },
  { 0xD4, "PCI_ALLOC_ERR" },
  { 0x92, "PCIB_INIT" },
  { 0x94, "PCIB_ENUMERATION" },
  { 0x95, "PCIB_REQ_RESOURCE" },
  { 0x96, "PCIB_ASS_RESOURCE" },
  { 0xEF, "PCIB_INIT" },
  { 0x99, "SUPER_IO_INIT" },
  { 0x91, "DRIVER_CONN_START" },
  { 0xD5, "NO_SPACE_ROM" },
  { 0x97, "CONSOLE_INPUT_CONN" },
  { 0xB2, "LEGACY_ROM_INIT" },
  { 0xAA, "ACPI_ACPI_MODE" },
  { 0xC0, "OEM_BDS_INIT" },
  { 0xBB, "AMI_CODE" },
  { 0xC1, "OEM_BDS_INIT" },
  { 0x98, "CONSOLE_OUTPUT_CONN" },
  { 0x9D, "USB_ENABLE" },
  { 0x9C, "USB_DETECT" },
  { 0xB4, "USB_HOT_PLUG" },
  { 0xA0, "IDE_INIT" },
  { 0xA2, "IDE_DETECT" },
  { 0xA9, "START_OF_SETUP" },
  { 0xAB, "SETUP_INIT_WAIT" },

  /*--------------------- ACPI/ASL Phase - Start--------------------- */
  { 0x01, "S1_SLEEP_STATE" },
  { 0x02, "S2_SLEEP_STATE" },
  { 0x03, "S3_SLEEP_STATE" },
  { 0x04, "S4_SLEEP_STATE" },
  { 0x05, "S5_SLEEP_STATE" },
  { 0x06, "S6_SLEEP_STATE" },
  { 0x10, "WEAK_FROM_S1" },
  { 0x20, "WEAK_FROM_S2" },
  { 0x30, "WEAK_FROM_S3" },
  { 0x40, "WEAK_FROM_S4" },
  { 0xAC, "ACPI_PIC_MODE" },
  /*--------------------- ACPI/ASL Phase - Start--------------------- */

  /*--------------------- DXE Phase - Start--------------------- */
  { 0x60, "DXE_CORE_START" },
  { 0x62, "INSTALL_SB_SERVICE" },
  { 0x63, "CPU_DXE_STARTED" },
  { 0x64, "CPU_DXE_INIT" },
  { 0x65, "CPU_DXE_INIT" },
  { 0x66, "CPU_DXE_INIT" },
  { 0x67, "CPU_DXE_INIT" },
  { 0x69, "NB_DEX_INIT" },
  { 0x6A, "NB_DEX_SMM_INIT" },
  { 0x6B, "NB_DEX_BRI_START" },
  { 0x6C, "NB_DEX_BRI_START" },
  { 0x6D, "NB_DEX_BRI_START" },
  { 0x6E, "NB_DEX_BRI_START" },
  { 0x6F, "NB_DEX_BRI_START" },

  { 0x71, "SB_DEX_SMM_INIT" },
  { 0x72, "SB_DEX_DEV_START" },
  { 0x73, "SB_DEX_BRI_START" },
  { 0x74, "SB_DEX_BRI_START" },
  { 0x75, "SB_DEX_BRI_START" },
  { 0x76, "SB_DEX_BRI_START" },
  { 0x77, "SB_DEX_BRI_START" },
  { 0x7A, "AMI_DXE_CODE" },
  { 0x7B, "AMI_DXE_CODE" },
  { 0x7C, "AMI_DXE_CODE" },
  { 0x7D, "AMI_DXE_CODE" },
  { 0x7E, "AMI_DXE_CODE" },
  { 0x7F, "AMI_DXE_CODE" },

  //OEM DXE INIT CODE
  { 0x80, "OEM_DEX_INIT" },
  { 0x81, "OEM_DEX_INIT" },
  { 0x82, "OEM_DEX_INIT" },
  { 0x83, "OEM_DEX_INIT" },
  { 0x84, "OEM_DEX_INIT" },
  { 0x85, "OEM_DEX_INIT" },
  { 0x86, "OEM_DEX_INIT" },
  { 0x87, "OEM_DEX_INIT" },
  { 0x88, "OEM_DEX_INIT" },
  { 0x89, "OEM_DEX_INIT" },
  { 0x8A, "OEM_DEX_INIT" },
  { 0x8B, "OEM_DEX_INIT" },
  { 0x8C, "OEM_DEX_INIT" },
  { 0x8D, "OEM_DEX_INIT" },
  { 0x8E, "OEM_DEX_INIT" },
  { 0x8F, "OEM_DEX_INIT" },

  //BDS EXECUTION
  { 0x90, "BDS_START" },
  { 0x93, "PCIB_HOT_PLUG_INIT" },
  { 0x9B, "USB_RESET" },
  { 0x9E, "AMI_CODE" },
  { 0x9F, "AMI_CODE" },

  { 0xA1, "IDE_RESET" },
  { 0xA3, "IDE_ENABLE" },
  { 0xA4, "SCSI_INIT" },
  { 0xA5, "SCSI_RESET" },
  { 0xA6, "SCSI_DETECT" },
  { 0xA7, "SCSI_ENABLE" },
  { 0xA8, "SETUP_VERIFY_PW" },
  { 0xAD, "READY_TO_BOOT" },
  { 0xAE, "LEGACY_BOOT_EVE" },
  { 0xAF, "EXIT_BOOT_EVE" },

  { 0xB0, "SET_VIR_ADDR_START" },
  { 0xB1, "SET_VIR_ADDR_END" },
  { 0xB3, "SYS_RESET" },
  { 0xB5, "PCIB_HOT_PLUG" },
  { 0xB6, "CLEAN_NVRAM" },
  { 0xB7, "CONFIG_RESET" },
  { 0xB8, "AMI_CODE" },
  { 0xB9, "AMI_CODE" },
  { 0xBA, "ASL_CODE" },
  { 0xBC, "AMI_CODE" },
  { 0xBD, "AMI_CODE" },
  { 0xBE, "AMI_CODE" },
  { 0xBF, "AMI_CODE" },

  //OEM BDS Initialization Code
  { 0xC2, "OEM_BDS_INIT" },
  { 0xC3, "OEM_BDS_INIT" },
  { 0xC4, "OEM_BDS_INIT" },
  { 0xC5, "OEM_BDS_INIT" },
  { 0xC6, "OEM_BDS_INIT" },
  { 0xC7, "OEM_BDS_INIT" },
  { 0xC8, "OEM_BDS_INIT" },
  { 0xC9, "OEM_BDS_INIT" },
  { 0xCA, "OEM_BDS_INIT" },
  { 0xCB, "OEM_BDS_INIT" },
  { 0xCC, "OEM_BDS_INIT" },
  { 0xCD, "OEM_BDS_INIT" },
  { 0xCE, "OEM_BDS_INIT" },
  { 0xCF, "OEM_BDS_INIT" },

  //DXE Phase
  { 0xD0, "CPU_INIT_ERR" },
  { 0xD3, "ARCH_PROT_ERR" },
  { 0xD6, "NO_CONSOLE_OUT" },
  { 0xD7, "NO_CONSOLE_IN" },
  { 0xD8, "INVALID_PW" },
  { 0xD9, "BOOT_OPT_ERR" },
  { 0xDA, "BOOT_OPT_FAIL" },
  { 0xDB, "FLASH_UPD_FAIL" },
  { 0xDC, "RST_PROT_NA" },
  { 0xDD, "DEX_SELTEST_FAILs" }
  /*--------------------- DXE Phase - End--------------------- */

};

static post_desc_t pdesc_error[] = {
  {0, "No error"}, //Error Code 0
  {1, "Expander I2C bus 0 crash"},
  {2, "Expander I2C bus 1 crash"},
  {3, "Expander I2C bus 2 crash"},
  {4, "Expander I2C bus 3 crash"},
  {5, "Expander I2C bus 4 crash"},
  {6, "Expander I2C bus 5 crash"},
  {7, "Expander I2C bus 6 crash"},
  {8, "Expander I2C bus 7 crash"},
  {9, "Expander I2C bus 8 crash"},
  {10, "Expander I2C bus 9 crash"},
  {11, "Expander I2C bus 10 crash"},
  {12, "Expander I2C bus 11 crash"},
  {13, "Fan 1 front fault"},
  {14, "Fan 1 rear fault"},
  {15, "Fan 2 front fault"},
  {16, "Fan 2 rear fault"},
  {17, "Fan 3 front fault"},
  {18, "Fan 3 rear fault"},
  {19, "Fan 4 front fault"},
  {20, "Fan 4 rear fault"},
  {21, "SCC voltage warning"},
  {22, "DPB voltage warning"},
  {25, "SCC current warning"},
  {26, "DPB current warning"},
  {29, "DPB_Temp1"},
  {30, "DPB_Temp2"},
  {31, "SCC_Expander_Temp"},
  {32, "SCC_IOC_Temp"},
  {33, "HDD X SMART temp. warning"},
  {36, "HDD0 fault"},
  {37, "HDD1 fault"},
  {38, "HDD2 fault"},
  {39, "HDD3 fault"},
  {40, "HDD4 fault"},
  {41, "HDD5 fault"},
  {42, "HDD6 fault"},
  {43, "HDD7 fault"},
  {44, "HDD8 fault"},
  {45, "HDD9 fault"},
  {46, "HDD10 fault"},
  {47, "HDD11 fault"},
  {48, "HDD12 fault"},
  {49, "HDD13 fault"},
  {50, "HDD14 fault"},
  {51, "HDD15 fault"},
  {52, "HDD16 fault"},
  {53, "HDD17 fault"},
  {54, "HDD18 fault"},
  {55, "HDD19 fault"},
  {56, "HDD20 fault"},
  {57, "HDD21 fault"},
  {58, "HDD22 fault"},
  {59, "HDD23 fault"},
  {60, "HDD24 fault"},
  {61, "HDD25 fault"},
  {62, "HDD26 fault"},
  {63, "HDD27 fault"},
  {64, "HDD28 fault"},
  {65, "HDD29 fault"},
  {66, "HDD30 fault"},
  {67, "HDD31 fault"},
  {68, "HDD32 fault"},
  {69, "HDD33 fault"},
  {70, "HDD34 fault"},
  {71, "HDD35 fault"},
  {90, "Internal Mini-SAS link error"},
  {91, "Internal Mini-SAS link error"},
  {92, "Drawer be pulled out"},
  {93, "Peer SCC be plug out"},
  {94, "IOMA be plug out"},
  {95, "IOMB be plug out"},
  {99, "H/W Configuration/Type Not Match"}, //Error Code 99
  {0xE0, "BMC CPU utilization exceeded"},// BMC error code
  {0xE1, "BMC Memory utilization exceeded"},
  {0xE2, "ECC Recoverable Error"},
  {0xE3, "ECC Un-recoverable Error"},
  {0xE4, "Server board Missing"},
  {0xE7, "SCC Missing"},
  {0xE8, "NIC is plugged out"},
  {0xE9, "I2C bus 0 hang"},
  {0xEA, "I2C bus 1 hang"},
  {0xEB, "I2C bus 2 hang"},
  {0xEC, "I2C bus 3 hang"},
  {0xED, "I2C bus 4 hang"},
  {0xEE, "I2C bus 5 hang"},
  {0xEF, "I2C bus 6 hang"},
  {0xF0, "I2C bus 7 hang"},
  {0xF1, "I2C bus 8 hang"},
  {0xF2, "I2C bus 9 hang"},
  {0xF3, "I2C bus 10 hang"},
  {0xF4, "I2C bus 11 hang"},
  {0xF5, "I2C bus 12 hang"},
  {0xF6, "I2C bus 13 hang"},
  {0xF7, "Server health ERR"},
  {0xF8, "IOM health ERR"},
  {0xF9, "DPB health ERR"},
  {0xFA, "SCC health ERR"},
  {0xFB, "NIC health ERR"},
  {0xFC, "Remote BMC health ERR"},
  {0xFD, "Local SCC health ERR"},
  {0xFE, "Remote SCC health ERR"},
};

static gpio_desc_t gdesc[] = {
  { 0x10, 0, 2, "DBG_RST_BTN_N" },
  { 0x11, 0, 1, "DBG_PWR_BTN_N" },
  { 0x12, 0, 0, "DBG_GPIO_BMC2" },
  { 0x13, 0, 0, "DBG_GPIO_BMC3" },
  { 0x14, 0, 0, "DBG_GPIO_BMC4" },
  { 0x15, 0, 0, "DBG_GPIO_BMC5" },
  { 0x16, 0, 0, "DBG_GPIO_BMC6" },
  { 0x17, 0, 3, "DBG_HDR_UART_SEL" },
};
static int gdesc_count = sizeof(gdesc) / sizeof (gpio_desc_t);

static sensor_desc_c cri_sensor[] =
{
    {"SOC_TEMP:"    , BIC_SENSOR_SOC_TEMP        , "C"   , FRU_SLOT1},
    {"DIMMA0_TEMP:" , BIC_SENSOR_SOC_DIMMA0_TEMP , "C"   , FRU_SLOT1},
    {"DIMMA1_TEMP:" , BIC_SENSOR_SOC_DIMMA1_TEMP , "C"   , FRU_SLOT1},
    {"DIMMB0_TEMP:" , BIC_SENSOR_SOC_DIMMB0_TEMP , "C"   , FRU_SLOT1},
    {"DIMMB1_TEMP:" , BIC_SENSOR_SOC_DIMMB1_TEMP , "C"   , FRU_SLOT1},
    {"HSC_PWR:"     , DPB_SENSOR_HSC_POWER       , "W"   , FRU_DPB},
    {"HSC_VOL:"     , DPB_SENSOR_HSC_VOLT        , "V"   , FRU_DPB},
    {"HSC_CUR:"     , DPB_SENSOR_HSC_CURR        , "A"   , FRU_DPB},
    {"FAN1_F:"      , DPB_SENSOR_FAN1_FRONT      , "RPM" , FRU_DPB},
    {"FAN1_R:"      , DPB_SENSOR_FAN1_REAR       , "RPM" , FRU_DPB},
    {"FAN2_F:"      , DPB_SENSOR_FAN2_FRONT      , "RPM" , FRU_DPB},
    {"FAN2_R:"      , DPB_SENSOR_FAN2_REAR       , "RPM" , FRU_DPB},
    {"FAN3_F:"      , DPB_SENSOR_FAN3_FRONT      , "RPM" , FRU_DPB},
    {"FAN3_R:"      , DPB_SENSOR_FAN3_REAR       , "RPM" , FRU_DPB},
    {"FAN4_F:"      , DPB_SENSOR_FAN4_FRONT      , "RPM" , FRU_DPB},
    {"FAN4_R:"      , DPB_SENSOR_FAN4_REAR       , "RPM" , FRU_DPB},
    {"NIC Temp:"    , MEZZ_SENSOR_TEMP           , "C"   , FRU_NIC},
};
static int sensor_count = sizeof(cri_sensor) / sizeof(sensor_desc_c);

static int
plat_chk_cri_sel_update(uint8_t *cri_sel_up) {
  FILE *fp;
  struct stat file_stat;

  fp = fopen("/mnt/data/cri_sel", "r");
  if (fp) {
    if ((stat("/mnt/data/cri_sel", &file_stat) == 0) && (file_stat.st_mtime != frame_sel.mtime)) {
      *cri_sel_up = 1;
    } else {
      *cri_sel_up = 0;
    }
    fclose(fp);
  } else {
    if (frame_sel.buf == NULL || frame_sel.lines != 0) {
      *cri_sel_up = 1;
    } else {
      *cri_sel_up = 0;
    }
  }

  return 0;
}

int
plat_udbg_get_frame_info(uint8_t *num) {
  *num = 3;
  return 0;
}

int
plat_udbg_get_updated_frames(uint8_t *count, uint8_t *buffer) {
  uint8_t cri_sel_up = 0;
  uint8_t info_page_up = 1;

  *count = 0;

  // info page update
  if (info_page_up == 1) {
    buffer[*count] = 1;
    *count += 1;
  }

  // cri sel update
  plat_chk_cri_sel_update(&cri_sel_up);
  if (cri_sel_up == 1) {
    buffer[*count] = 2;
    *count += 1;
  }

  // cri sensor update
  buffer[*count] = 3;
  *count += 1;

  return 0;
}

int
plat_udbg_get_post_desc(uint8_t index, uint8_t *next, uint8_t phase,  uint8_t *end, uint8_t *length, uint8_t *buffer) {
  int target, pdesc_size;
  post_desc_t *ptr;
  uint8_t val = 0;
  
  get_gpio_value(GPIO_UART_SEL, &val);

  if (val == 0) {
    switch (phase) {
      case 1:
        ptr = pdesc_phase1;
        pdesc_size = sizeof(pdesc_phase1) / sizeof(post_desc_t);
        break;
      case 2:
        ptr = pdesc_phase2;
        pdesc_size = sizeof(pdesc_phase2) / sizeof(post_desc_t);
        break;
      default:
        return -1;
        break;
    }
  } else {
    ptr = pdesc_error;
    pdesc_size  =  sizeof(pdesc_error) / sizeof(post_desc_t);
    //Expander Error Code Conversion
    //BMC send Hexadecimal Error Code to Debug Card
    //Debug Card will send the same value to BMC to query String
    //Expander Error String is in Decimal 0~99, so needs the conversion
    //Expmale: 0x99 Convert to 99 literally
    if(index < 0x9A)
      index = ( ((index/16)*10) + (index%16) );
  }

  for (target = 0; target < pdesc_size; target++, ptr++) {
    if (index == ptr->code) {
      *length = strlen(ptr->desc);
      memcpy(buffer, ptr->desc, *length);
      buffer[*length] = '\0';

      if (index == 0x4F) {
        *next = pdesc_phase2[0].code;
        *end = 0x00;
      }
      else if (target == pdesc_size - 1) {
        *next = 0xFF;
        *end = 0x01;
      }
      else {
        *next = (ptr+1)->code;
        *end = 0x00;
      }
      return 0;
    }
  }

  return -1;
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
  if (i == gdesc_count) {  // Check for not found
    return -1;
  }

  // Populate the description and next index
  *level = gdesc[i].level;
  *def = gdesc[i].def;
  *count = strlen(gdesc[i].desc);
  memcpy(buffer, gdesc[i].desc, *count);
  buffer[*count] = '\0';

  // Populate the next index
  if (i == gdesc_count-1) {  // last entry
    *next = 0xFF;
  } else {
    *next = gdesc[i+1].pin;
  }

  return 0;
}

static int
plat_udbg_get_cri_sel(uint8_t frame, uint8_t page, uint8_t *next, uint8_t *count, uint8_t *buffer) {
  int len;
  char line_buff[FRAME_PAGE_BUF_SIZE], *ptr;
  FILE *fp;
  struct stat file_stat;

  fp = fopen("/mnt/data/cri_sel", "r");
  if (fp) {
    if ((stat("/mnt/data/cri_sel", &file_stat) == 0) && (file_stat.st_mtime != frame_sel.mtime)) {
      // initialize and clear frame
      frame_sel.init(&frame_sel, FRAME_BUFF_SIZE);
      frame_sel.overwrite = 1;
      frame_sel.max_page = 20;
      frame_sel.mtime = file_stat.st_mtime;
      snprintf(frame_sel.title, 32, "Cri SEL");

      while (fgets(line_buff, FRAME_PAGE_BUF_SIZE, fp)) {
        // Remove newline
        line_buff[strlen(line_buff)-1] = '\0';
        ptr = line_buff;
        // Find message
        ptr = strstr(ptr, "local0.err");
        if(ptr != 0) {
          ptr = strstr(ptr, ":");
        }
        len = (ptr) ? strlen(ptr) : 0;
        if (len > 2) {
          ptr += 2;
        } else {
          continue;
        }

        // Write new message
        frame_sel.insert(&frame_sel, ptr, 0);
      }
    }
    fclose(fp);
  } else {
    // Title only
    frame_sel.init(&frame_sel, FRAME_BUFF_SIZE);
    snprintf(frame_sel.title, 32, "Cri SEL");
    frame_sel.mtime = 0;
  }

  if (page > frame_sel.pages) {
    return -1;
  }

  *count = frame_sel.getPage(&frame_sel, page, (char *)buffer, FRAME_PAGE_BUF_SIZE);
  if (*count < 0) {
    *count = 0;
    return -1;
  }

  if (page < frame_sel.pages)
    *next = page+1;
  else
    *next = 0xFF;  // Set the value of next to 0xFF to indicate this is the last page

  return 0;
}

static int
plat_udbg_get_cri_sensor (uint8_t frame, uint8_t page, uint8_t *next, uint8_t *count, uint8_t *buffer) {
  char str[32], temp_val[16], temp_thresh[8];
  int i, ret;
  float fvalue;
  thresh_sensor_t thresh;

  if (page == 1) {
    // Only update frame data while getting page 1

    // initialize and clear frame
    frame_snr.init(&frame_snr, FRAME_BUFF_SIZE);
    snprintf(frame_snr.title, 32, "CriSensor");

    for (i = 0; i < sensor_count; i++) {
      temp_thresh[0] = 0;
      ret = sensor_cache_read(cri_sensor[i].fru, cri_sensor[i].sensor_num, &fvalue);
      if (ret < 0) {
        strcpy(temp_val, "NA");
      } else {
        ret = sdr_get_snr_thresh(cri_sensor[i].fru, cri_sensor[i].sensor_num, &thresh);
        if (ret == 0) {
          if ((GETBIT(thresh.flag, UNR_THRESH) == 1) && (fvalue > thresh.unr_thresh)) {
            strcpy(temp_thresh, "/UNR");
          } else if (((GETBIT(thresh.flag, UCR_THRESH) == 1)) && (fvalue > thresh.ucr_thresh)) {
            strcpy(temp_thresh, "/UCR");
          } else if (((GETBIT(thresh.flag, UNC_THRESH) == 1)) && (fvalue > thresh.unc_thresh)) {
            strcpy(temp_thresh, "/UNC");
          } else if (((GETBIT(thresh.flag, LNR_THRESH) == 1)) && (fvalue < thresh.lnr_thresh)) {
            strcpy(temp_thresh, "/LNR");
          } else if (((GETBIT(thresh.flag, LCR_THRESH) == 1)) && (fvalue < thresh.lcr_thresh)) {
            strcpy(temp_thresh, "/LCR");
          } else if (((GETBIT(thresh.flag, LNC_THRESH) == 1)) && (fvalue < thresh.lnc_thresh)) {
            strcpy(temp_thresh, "/LNC");
          }
        }

        switch (cri_sensor[i].sensor_num) {
          case DPB_SENSOR_HSC_VOLT:
          case DPB_SENSOR_HSC_CURR:
            snprintf(temp_val, sizeof(temp_val), "%.2f%s", fvalue, cri_sensor[i].unit);
            break;
          case DPB_SENSOR_HSC_POWER:
            snprintf(temp_val, sizeof(temp_val), "%.1f%s", fvalue, cri_sensor[i].unit);
            break;
          default:
            snprintf(temp_val, sizeof(temp_val), "%.0f%s", fvalue, cri_sensor[i].unit);
            break;
        }
      }
      if (temp_thresh[0] != 0)
        snprintf(str, sizeof(str), ESC_ALT"%s%s%s"ESC_RST, cri_sensor[i].name, temp_val, temp_thresh);
      else
        snprintf(str, sizeof(str), "%s%s", cri_sensor[i].name, temp_val);
      frame_snr.append(&frame_snr, str, 0);
    }
  }  // End of update frame

  if (page > frame_snr.pages) {
    return -1;
  }

  *count = frame_snr.getPage(&frame_snr, page, (char *)buffer, FRAME_PAGE_BUF_SIZE);
  if (*count < 0) {
    *count = 0;
    return -1;
  }

  if (page < frame_snr.pages)
    *next = page + 1;
  else
    *next = 0xFF;  // Set the value of next to 0xFF to indicate this is the last page

  return 0;
}

static int
plat_udbg_get_info_page (uint8_t frame, uint8_t page, uint8_t *next, uint8_t *count, uint8_t *buffer) {
  int ret, boardid;
  char line_buff[FRAME_PAGE_BUF_SIZE];
  FILE *fp;
  fruid_info_t fruid;
  lan_config_t lan_config = {0};
  uint8_t rlen;
  uint8_t zero_ip_addr[SIZE_IP_ADDR] = {0};
  uint8_t zero_ip6_addr[SIZE_IP6_ADDR] = {0};

  if (page == 1) {
    // Only update frame data while getting page 1

    // initialize and clear frame
    frame_info.init(&frame_info, FRAME_BUFF_SIZE);
    snprintf(frame_info.title, 32, "SYS_Info");

    // FRU
    ret = fruid_parse("/tmp/fruid_server.bin", &fruid);
    if (ret == 0) {
      frame_info.append(&frame_info, "SN:", 0);
      frame_info.append(&frame_info, fruid.board.serial, 1);
      frame_info.append(&frame_info, "PN:", 0);
      frame_info.append(&frame_info, fruid.board.part, 1);
      free_fruid_info(&fruid);
    }

    // LAN
    plat_lan_init(&lan_config);
    if (memcmp(lan_config.ip_addr, zero_ip_addr, SIZE_IP_ADDR)) {
      inet_ntop(AF_INET, lan_config.ip_addr, line_buff, FRAME_PAGE_BUF_SIZE);
      frame_info.append(&frame_info, "BMC_IP:", 0);
      frame_info.append(&frame_info, line_buff, 1);
    }
    if (memcmp(lan_config.ip6_addr, zero_ip6_addr, SIZE_IP6_ADDR)) {
      inet_ntop(AF_INET6, lan_config.ip6_addr, line_buff, FRAME_PAGE_BUF_SIZE);
      frame_info.append(&frame_info, "BMC_IPv6:", 0);
      frame_info.append(&frame_info, line_buff, 1);
    }

    // BMC ver
    fp = fopen("/etc/issue", "r");
    if (fp != NULL) {
      if (fgets(line_buff, sizeof(line_buff), fp)) {
        if ((ret = sscanf(line_buff, "%*s %*s %s", line_buff)) == 1) {
          frame_info.append(&frame_info, "BMC_FW_ver:", 0);
          frame_info.append(&frame_info, line_buff, 1);
        }
      }
      fclose(fp);
    }

    // BIOS ver
    if (pal_get_sysfw_ver(FRU_SLOT1, (uint8_t *)line_buff) == 0) {
      // BIOS version response contains the length at offset 2 followed by ascii string
      line_buff[3+line_buff[2]] = '\0';
      frame_info.append(&frame_info, "BIOS_FW_ver:", 0);
      frame_info.append(&frame_info, &line_buff[3], 1);

      // ME status
      line_buff[0] = NETFN_APP_REQ << 2;
      line_buff[1] = CMD_APP_GET_DEVICE_ID;
      ret = bic_me_xmit(FRU_SLOT1, (uint8_t *)line_buff, 2, (uint8_t *)line_buff, &rlen);
      if (ret == 0) {
        strcpy(line_buff, ((line_buff[2] & 0x80) != 0) ? "recovery mode" : "operation mode");
        frame_info.append(&frame_info, "ME_status:", 0);
        frame_info.append(&frame_info, line_buff, 1);
      }
    }

    // Board ID
    boardid = pal_get_iom_board_id();
    if (boardid >= 0) {
      sprintf(line_buff, "%d%d%d",
        (boardid & (1 << 2)) ? 1 : 0,
        (boardid & (1 << 1)) ? 1 : 0,
        (boardid & (1 << 0)) ? 1 : 0);
      frame_info.append(&frame_info, "Board_ID:", 0);
      frame_info.append(&frame_info, line_buff, 1);
    }

    // Battery - Use Escape sequence
    frame_info.append(&frame_info, "Battery:", 0);
    frame_info.append(&frame_info, ESC_BAT"     ", 1);

    // MCU Version - Use Escape sequence
    frame_info.append(&frame_info, "MCUbl_ver:", 0);
    frame_info.append(&frame_info, ESC_MCU_BL_VER, 1);
    frame_info.append(&frame_info, "MCU_ver:", 0);
    frame_info.append(&frame_info, ESC_MCU_RUN_VER, 1);

  } // End of update frame

  if (page > frame_info.pages) {
    return -1;
  }

  *count = frame_info.getPage(&frame_info, page, (char *)buffer, FRAME_PAGE_BUF_SIZE);
  if (*count < 0) {
    *count = 0;
    return -1;
  }

  if (page < frame_info.pages)
    *next = page + 1;
  else
    *next = 0xFF;  // Set the value of next to 0xFF to indicate this is the last page

  return 0;
}

int
plat_udbg_get_frame_data(uint8_t frame, uint8_t page, uint8_t *next, uint8_t *count, uint8_t *buffer) {

  switch (frame) {
    case 1:  // info_page
      return plat_udbg_get_info_page(frame, page, next, count, buffer);
    case 2:  // critical SEL
      return plat_udbg_get_cri_sel(frame, page, next, count, buffer);
    case 3:  // critical Sensor
      return plat_udbg_get_cri_sensor(frame, page, next, count, buffer);
    default:
      return -1;
  }
}

static uint8_t panel_main (uint8_t item) {
  // Update item list when select item 0
  switch (item) {
    case 1:
      return panels[PANEL_BOOT_ORDER].select(0);
    case 2:
      return panels[PANEL_POWER_POLICY].select(0);
    default:
      return PANEL_MAIN;
  }
}

static uint8_t panel_boot_order (uint8_t item) {
  int i;
  unsigned char buff[MAX_VALUE_LEN], pickup, len;

  if (pal_get_boot_order(FRU_SLOT1, buff, buff, &len) == 0) {
    if (item > 0 && item < SIZE_BOOT_ORDER) {
      pickup = buff[item];
      while (item > 1) {
        buff[item] = buff[item - 1];
        item--;
      }
      buff[item] = pickup;
      buff[0] |= 0x80;
      pal_set_boot_order(FRU_SLOT1, buff, buff, &len);

      // refresh items
      return panels[PANEL_BOOT_ORDER].select(0);
    }

    // '*': boot flags valid, BIOS has not yet read
    snprintf(panels[PANEL_BOOT_ORDER].item_str[0], 32,
      "Boot Order%c", (buff[0] & 0x80)?'*':'\0');

    for (i = 1; i < SIZE_BOOT_ORDER; i++) {
      switch (buff[i]) {
        case 0x0:
          snprintf(panels[PANEL_BOOT_ORDER].item_str[i], 32,
            " USB device");
          break;
        case 0x1:
          snprintf(panels[PANEL_BOOT_ORDER].item_str[i], 32,
            " Network v4");
          break;
        case (0x1 | 0x8):
          snprintf(panels[PANEL_BOOT_ORDER].item_str[i], 32,
            " Network v6");
          break;
        case 0x2:
          snprintf(panels[PANEL_BOOT_ORDER].item_str[i], 32,
            " SATA HDD");
          break;
        case 0x3:
          snprintf(panels[PANEL_BOOT_ORDER].item_str[i], 32,
            " SATA-CDROM");
          break;
        case 0x4:
          snprintf(panels[PANEL_BOOT_ORDER].item_str[i], 32,
            " Other");
          break;
        default:
          panels[PANEL_BOOT_ORDER].item_str[i][0] = '\0';
          break;
      }
    }

    // remove empty items
    for (i--; (strlen(panels[PANEL_BOOT_ORDER].item_str[i]) == 0) && (i > 0); i--) ;

    panels[PANEL_BOOT_ORDER].item_num = i;
  }
  return PANEL_BOOT_ORDER;
}

static uint8_t panel_power_policy (uint8_t item) {
  char buff[MAX_VALUE_LEN];

  switch (item) {
    case 1:
      pal_set_key_value("slot1_por_cfg", "on");
      break;
    case 2:
      pal_set_key_value("slot1_por_cfg", "lps");
      break;
    case 3:
      pal_set_key_value("slot1_por_cfg", "off");
      break;
    default:
      break;
  }

  if (pal_get_key_value("slot1_por_cfg", buff) == 0) {
    snprintf(panels[PANEL_POWER_POLICY].item_str[1], 32,
      "%cPower On", (strcmp(buff, "on") == 0)?'*':' ');
    snprintf(panels[PANEL_POWER_POLICY].item_str[2], 32,
      "%cLast State", (strcmp(buff, "lps") == 0)?'*':' ');
    snprintf(panels[PANEL_POWER_POLICY].item_str[3], 32,
      "%cPower Off", (strcmp(buff, "off") == 0)?'*':' ');
    panels[PANEL_POWER_POLICY].item_num = 3;
  }

  return PANEL_POWER_POLICY;
}

int
plat_udbg_control_panel(uint8_t panel, uint8_t operation, uint8_t item, uint8_t *count, uint8_t *buffer) {

  if (panel > panelNum || panel < PANEL_MAIN)
    return CC_PARAM_OUT_OF_RANGE;

  // No more item; End of item list
  if (item > panels[panel].item_num)
    return CC_PARAM_OUT_OF_RANGE;

  switch (operation) {
    case 0: // Get Description
      break;
    case 1: // Select item
      panel = panels[panel].select(item);
      item = 0;
      break;
    case 2: // Back
      panel = panels[panel].parent;
      item = 0;
      break;
    default:
      return CC_PARAM_OUT_OF_RANGE;
  }

  buffer[0] = panel;
  buffer[1] = item;
  buffer[2] = strlen(panels[panel].item_str[item]);
  if (buffer[2] > 0 && (buffer[2] + 3) < FRAME_PAGE_BUF_SIZE) {
    memcpy(&buffer[3], panels[panel].item_str[item], buffer[2]);
  }
  *count = buffer[2] + 3;

  return CC_SUCCESS;
}
