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
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openbmc/pal.h>
#include <openbmc/sdr.h>
#include <openbmc/kv.h>
#include <openbmc/pal_sensors.h>
#include <openbmc/fruid.h>
#include <arpa/inet.h>
#include "usb-dbg-conf.h"

#define ESCAPE "\x1B"
#define ESC_BAT ESCAPE"B"
#define ESC_MCU_BL_VER ESCAPE"U"
#define ESC_MCU_RUN_VER ESCAPE"R"
#define ESC_ALT ESCAPE"[5;7m"
#define ESC_RST ESCAPE"[m"
#define ESC_NOR ESCAPE"[0m"

#define LINE_DELIMITER '\x1F'

#ifdef CONFIG_FBY3
#define FRAME_BUFF_SIZE 5120
#else
#define FRAME_BUFF_SIZE 4096
#endif

#define FRAME_PAGE_BUF_SIZE 256
#define MAX_UART_SEL_NAME_SIZE 16
#define MRC_DESC_SIZE 64

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

static const char *find_esc(const char *buf, int blen, const char *esc, size_t elen) {
  const char *ptr;

  // to find the last CSI-codes
  if (!(ptr = memrchr(buf, esc[0], blen)))
    return NULL;

  if ((blen - (ptr - buf) >= elen) && !memcmp(ptr, esc, elen))
    return ptr;

  return NULL;
}

// return 0 on seccuess
static int frame_append (struct frame *self, char *string, int indent)
{
  const size_t buf_size = 128;
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

    if (*ptr == LINE_DELIMITER) {
      self->lines++;
      // workaround when Blink CSI-codes spread 2 pages,
      // add ESC_RST to the end of previous page,
      // add ESC_ALT to the start of next page
      if (!(self->lines % self->line_per_page)) {
        const char *esc = find_esc(buf, ptr - buf, ESC_ALT, strlen(ESC_ALT));
        if (esc != NULL) {
          esc += strlen(ESC_ALT);
          if (!find_esc(esc, ptr - esc, ESC_RST, strlen(ESC_RST))) {
            char tmp[16], *ptmp;
            snprintf(tmp, sizeof(tmp), ESC_RST"%c"ESC_ALT, LINE_DELIMITER);
            for (ptmp = tmp; *ptmp; ptmp++) {
              if (self->isFull(self)) {
                if (!self->overwrite) {
                  return -1;
                }
                if (self->buf[self->idx_head] == LINE_DELIMITER)
                  self->lines--;
                self->idx_head = (self->idx_head + 1) % self->max_size;
              }
              self->buf[self->idx_tail] = *ptmp;
              self->idx_tail = (self->idx_tail + 1) % self->max_size;
            }
            continue;
          }
        }
      }
    }
    self->buf[self->idx_tail] = *ptr;
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
  const size_t buf_size = 128;
  char buf[buf_size];
  char *ptr;
  int ret;
  int i;

  ret = self->parse(self, buf, buf_size, string, indent);

  if (ret < 0)
    return ret;

  for (i = strlen(buf) - 1; i >= 0; i--) {
    ptr = &buf[i];
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

  if (page_buf == NULL || page_buf_size < self->line_width)
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
  uint8_t curr_sts;

  if (self == NULL)
    return -1;

  curr_sts = self->esc_sts;
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
static FRAME_DECLARE(frame_postcode);
static FRAME_DECLARE(frame_mrc);

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

static int
chk_cri_sel_update(uint8_t *cri_sel_up) {
  FILE *fp;
  struct stat file_stat;
  uint8_t pos = plat_get_fru_sel();
  static uint8_t pre_pos = 0xff;

  fp = fopen("/mnt/data/cri_sel", "r");
  if (fp) {
    if ((stat("/mnt/data/cri_sel", &file_stat) == 0) &&
        (file_stat.st_mtime != frame_sel.mtime || pre_pos != pos)) {
      *cri_sel_up = 1;
    } else {
      *cri_sel_up = 0;
    }
    fclose(fp);
  } else {
    if (frame_sel.buf == NULL || frame_sel.lines != 0 || pre_pos != pos) {
      *cri_sel_up = 1;
    } else {
      *cri_sel_up = 0;
    }
  }
  pre_pos = pos;
  return 0;
}

int
plat_udbg_get_frame_info()
{
  if (!plat_supported()) {
    return -1;
  }

  return pal_udbg_get_frame_total_num();
}

int
plat_udbg_get_updated_frames(uint8_t *count, uint8_t *buffer) {
  uint8_t cri_sel_up = 0;
  uint8_t num = 0;

  if (!plat_supported()) {
    return -1;
  }

  *count = 0;

  // info page update
  buffer[*count] = 1;
  *count += 1;

  // cri sel update
  chk_cri_sel_update(&cri_sel_up);
  if (cri_sel_up == 1) {
    buffer[*count] = 2;
    *count += 1;
  }

  // cri sensor update
  buffer[*count] = 3;
  *count += 1;

  int rc = pal_udbg_get_frame_total_num();
  if (rc < 0) {
      return -1;
  }
  num = (uint8_t) rc;

  if (num >= 4) {
    buffer[*count] = 4;
    *count += 1;
  }

  if (num >= 5) {
    buffer[*count] = 5;
    *count += 1;
  }

  return 0;
}

int
plat_udbg_get_post_desc(uint8_t index, uint8_t *next, uint8_t phase,  uint8_t *end, uint8_t *length, uint8_t *buffer) {
  int target, pdesc_size = 0;
  post_phase_desc_t *post_phase;
  size_t post_phase_desc_cnt, i;
  post_desc_t *ptr = NULL;
  post_desc_t *next_phase = NULL;
  uint8_t pos = plat_get_fru_sel();

  if (!plat_supported() ||
      plat_get_post_phase(pos, &post_phase, &post_phase_desc_cnt)) {
    return -1;
  }
  for (i = 0; i < post_phase_desc_cnt; i++) {
    if (post_phase[i].phase == PHASE_ANY) {
      ptr = post_phase[i].post_tbl;
      pdesc_size = post_phase[i].post_tbl_cnt;
      next_phase = NULL;
      break;
    }
    if (post_phase[i].phase == phase) {
      ptr = post_phase[i].post_tbl;
      pdesc_size = post_phase[i].post_tbl_cnt;
    } else if (post_phase[i].phase == phase + 1) {
      next_phase = post_phase[i].post_tbl;
    }
  }
  if (!ptr) {
    return -1;
  }

  for (target = 0; target < pdesc_size; target++, ptr++) {
    if (index == ptr->code) {
      *length = strlen(ptr->desc);
      memcpy(buffer, ptr->desc, *length);
      buffer[*length] = '\0';

      /* This is the end of this phase's post code */
      if (target == pdesc_size - 1) {
        /* Point to next phase if exists, else terminate */
        if (next_phase != NULL) {
          *next = next_phase[0].code;
          *end = 0x00;
        } else {
          *next = 0xFF;
          *end = 0x01;
        }
      } else {
        *next = (ptr + 1)->code;
        *end  = 0x00;
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
  dbg_gpio_desc_t *gdesc = NULL;
  size_t gdesc_count = 0;
  uint8_t pos = plat_get_fru_sel();

  if (!plat_supported() ||
      plat_get_gdesc(pos, &gdesc, &gdesc_count)) {
    return -1;
  }

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

static int
udbg_get_cri_sel(uint8_t frame, uint8_t page, uint8_t *next, uint8_t *count, uint8_t *buffer) {
  int len;
  int ret;
  char line_buff[FRAME_PAGE_BUF_SIZE], *ptr;
  char *fptr;
  FILE *fp;
  struct stat file_stat;
  uint8_t pos = plat_get_fru_sel();
  static uint8_t pre_pos = FRU_ALL;
  bool pos_changed = pre_pos != pos;

  pre_pos = pos;

  fp = fopen("/mnt/data/cri_sel", "r");
  if (fp) {
    if ((stat("/mnt/data/cri_sel", &file_stat) == 0) &&
				(file_stat.st_mtime != frame_sel.mtime || pos_changed)) {
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
        if (ptr == NULL) {
          continue;
        }
        // Check if FRU specific information
        fptr = ptr;
        fptr = strstr(fptr, ",FRU:");
        if (fptr) {
            if ((fptr[5]-'0') != pos) {
                continue;
            }
            // Remove ',FRU:X' from the string.
            *fptr = '\0';
        }

        if ((ptr = strrchr(ptr, ':')) == NULL) {
            continue;
        }
        len = strlen(ptr);
        if (len > 2) {
            // to skip log string ": "
            ptr += 2;
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

  ret = frame_sel.getPage(&frame_sel, page, (char *)buffer, FRAME_PAGE_BUF_SIZE);
  if (ret < 0) {
    *count = 0;
    return -1;
  }
  *count = (uint8_t)ret;

  if (page < frame_sel.pages)
    *next = page+1;
  else
    *next = 0xFF; // Set the value of next to 0xFF to indicate this is the last page

  return 0;
}

static int
udbg_get_cri_sensor (uint8_t frame, uint8_t page, uint8_t *next, uint8_t *count, uint8_t *buffer) {
  char str[64], temp_val[16], temp_thresh[8], print_format[32];
  int i, ret;
  float fvalue;
  thresh_sensor_t thresh;
  sensor_desc_t *cri_sensor = NULL;
  size_t sensor_count = 0;
  uint8_t pos = plat_get_fru_sel();
  uint8_t fru;

  if (plat_get_sensor_desc(pos, &cri_sensor, &sensor_count)) {
    return -1;
  }

  if (page == 1) {
    // Only update frame data while getting page 1

    // initialize and clear frame
    frame_snr.init(&frame_snr, FRAME_BUFF_SIZE);
    snprintf(frame_snr.title, 32, "CriSensor");

    for (i = 0; i < sensor_count; i++) {
      /* Pos implies BMC (FRU_ALL) and configuration for this sensor
       * wants us to use the FRU info from the pos. Skip this sensor */
      if (cri_sensor[i].fru == FRU_ALL && pos == FRU_ALL) {
        continue;
      }
      fru = cri_sensor[i].fru == FRU_ALL ? pos : cri_sensor[i].fru;

      temp_thresh[0] = 0;
      ret = sensor_cache_read(fru, cri_sensor[i].sensor_num, &fvalue);
      if (ret < 0) {
        strcpy(temp_val, "NA");
      } else {
        ret = sdr_get_snr_thresh(fru, cri_sensor[i].sensor_num, &thresh);
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
        snprintf(print_format, sizeof(print_format), "%%.%df%%s", (int)cri_sensor[i].disp_prec);
        snprintf(temp_val, sizeof(temp_val), (const char *)print_format, fvalue, cri_sensor[i].unit);
      }
      if (temp_thresh[0] != 0)
        snprintf(str, sizeof(str), ESC_ALT"%s%s%s"ESC_RST, cri_sensor[i].name, temp_val, temp_thresh);
      else
        snprintf(str, sizeof(str), ESC_NOR"%s%s"ESC_RST, cri_sensor[i].name, temp_val);
      frame_snr.append(&frame_snr, str, 0);
    }
  }  // End of update frame

  if (page > frame_snr.pages) {
    return -1;
  }

  ret = frame_snr.getPage(&frame_snr, page, (char *)buffer, FRAME_PAGE_BUF_SIZE);
  if (ret < 0) {
    *count = 0;
    return -1;
  }
  *count = (uint8_t)ret;

  if (page < frame_snr.pages)
    *next = page + 1;
  else
    *next = 0xFF;  // Set the value of next to 0xFF to indicate this is the last page

  return 0;
}

int __attribute__((weak))
plat_udbg_get_uart_sel_num(uint8_t *uart_sel_num) {
  return -1;
}

int __attribute__((weak))
plat_udbg_get_uart_sel_name(uint8_t uart_sel_num, char *uart_sel_name) {
  return -1;
}

static int
udbg_get_info_page (uint8_t frame, uint8_t page, uint8_t *next, uint8_t *count, uint8_t *buffer) {
  int ret;
  char line_buff[2048], *pres_dev = line_buff, *delim = "\n";
  FILE *fp;
  fruid_info_t fruid;
  lan_config_t lan_config = { 0 };
  unsigned char zero_ip_addr[SIZE_IP_ADDR] = { 0 };
  unsigned char zero_ip6_addr[SIZE_IP6_ADDR] = { 0 };
  char fruid_path[256];
  uint8_t pos = plat_get_fru_sel();
  char uart_sel_name[MAX_UART_SEL_NAME_SIZE] = {0};
  uint8_t uart_sel_num = 0;

  if (page == 1) {
    // Only update frame data while getting page 1

    // initialize and clear frame
    frame_info.init(&frame_info, FRAME_BUFF_SIZE);
    snprintf(frame_info.title, 32, "SYS_Info");

    pres_dev = line_buff;
    if (plat_get_extra_sysinfo(pos, line_buff) == 0) {
      pres_dev = strtok(pres_dev, delim);
      if (pres_dev) {
        do {
          frame_info.append(&frame_info, pres_dev, 0);
        } while ((pres_dev = strtok(NULL, delim)) != NULL);
      }
    }

    // FRU
    if (pos != FRU_ALL && pal_get_fruid_path(pos, fruid_path) == 0 &&
      fruid_parse(fruid_path, &fruid) == 0) {
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
        char fw_ver[64];
        if ((ret = sscanf(line_buff, "%*s %*s %63s", fw_ver)) == 1) {
          frame_info.append(&frame_info, "BMC_FW_ver:", 0);
          frame_info.append(&frame_info, fw_ver, 1);
        }
      }
      fclose(fp);
    }

    // BIOS ver
    if (pos != FRU_ALL && !pal_get_sysfw_ver(pos, (uint8_t *)line_buff)) {
      // BIOS version response contains the length at offset 2 followed by ascii string
      line_buff[3+line_buff[2]] = '\0';
      frame_info.append(&frame_info, "BIOS_FW_ver:", 0);
      frame_info.append(&frame_info, &line_buff[3], 1);
    }

    // ME status
    if (pos != FRU_ALL && !plat_get_me_status(pos, line_buff)) {
      frame_info.append(&frame_info, "ME_status:", 0);
      frame_info.append(&frame_info, line_buff, 1);
    }

    // Board ID
    if (!plat_get_board_id(line_buff)) {
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

    // Extra FW Version
    pres_dev = line_buff;
    if (plat_get_etra_fw_version(pos, pres_dev) == 0) {
      pres_dev = strtok(pres_dev, delim);
      if (pres_dev) {
        do {
          frame_info.append(&frame_info, pres_dev, 0);
          if ((pres_dev = strtok(NULL, delim)) != NULL) {
            frame_info.append(&frame_info, pres_dev, 1);
          } else {
            break;
          }
        } while ((pres_dev = strtok(NULL, delim)) != NULL);
      }
    }

    // Sys config present device
    pres_dev = line_buff;
    if (plat_get_syscfg_text(pos, pres_dev) == 0) {
      pres_dev = strtok(pres_dev, delim);
      if (pres_dev) {
        frame_info.append(&frame_info, "Sys Conf. info:", 0);
        do {
          frame_info.append(&frame_info, pres_dev, 1);
        } while ((pres_dev = strtok(NULL, delim)) != NULL);
      }
    }

    // Uart selection
    if (plat_udbg_get_uart_sel_num(&uart_sel_num) == 0) {
      if (plat_udbg_get_uart_sel_name(uart_sel_num, uart_sel_name) == 0) {
        snprintf(line_buff, sizeof(line_buff), "%u: %s", uart_sel_num, uart_sel_name);
        frame_info.append(&frame_info, "UART_SEL:", 0);
        frame_info.append(&frame_info, line_buff, 1);
      }
    }

  } // End of update frame

  if (page > frame_info.pages) {
    return -1;
  }

  ret = frame_info.getPage(&frame_info, page, (char *)buffer, FRAME_PAGE_BUF_SIZE);
  if (ret < 0) {
    *count = 0;
    return -1;
  }
  *count = (uint8_t)ret;

  if (page < frame_info.pages)
    *next = page + 1;
  else
    *next = 0xFF; // Set the value of next to 0xFF to indicate this is the last page

  return 0;
}

int __attribute__((weak))
plat_dword_postcode_buf(uint8_t fru, char *status) {
  return -1;
}

static int
udbg_get_postcode (uint8_t frame, uint8_t page, uint8_t *next, uint8_t *count, uint8_t *buffer) {
  int ret;
  char line_buff[2048] = {0}, *pres_dev = line_buff, *delim = "\n";

  if ((count == NULL) || (buffer == NULL)) {
    return -1;
  }

  uint8_t pos = plat_get_fru_sel();

  if (page == 1) {
    // Only update frame data while getting page 1

    // initialize and clear frame
    frame_postcode.init(&frame_postcode, FRAME_BUFF_SIZE);
    frame_postcode.max_page = 10;
    snprintf(frame_postcode.title, 32, "POST CODE");

    pres_dev = line_buff;
    if (pos != FRU_ALL && plat_dword_postcode_buf(pos, line_buff) == 0) {
      pres_dev = strtok(pres_dev, delim);
      if (pres_dev) {
        do {
          frame_postcode.append(&frame_postcode, pres_dev, 0);
        } while ((pres_dev = strtok(NULL, delim)) != NULL);
      }
    }

  } // End of update frame

  if (page > frame_postcode.pages) {
    return -1;
  }

  ret = frame_postcode.getPage(&frame_postcode, page, (char *)buffer, FRAME_PAGE_BUF_SIZE);
  if (ret < 0) {
    *count = 0;
    return -1;
  }
  *count = (uint8_t)ret;

  if (page < frame_postcode.pages)
    *next = page + 1;
  else
    *next = 0xFF; // Set the value of next to 0xFF to indicate this is the last page

  return 0;
}

int __attribute__((weak))
plat_get_amd_mrc_desc(uint16_t major, uint16_t minor, char *desc) {
#if defined(CONFIG_POSTCODE_AMD)
  if (desc == NULL) {
    return -1;
  }

  switch (minor) {
    case 0x4001:
      snprintf(desc, MRC_DESC_SIZE, "ABL_MEM_PMU_TRAIN_ERROR");
      break;
    case 0x4003:
      snprintf(desc, MRC_DESC_SIZE, "ABL_MEM_AGESA_MEMORY_TEST_ERROR");
      break;
    case 0x4020:
      snprintf(desc, MRC_DESC_SIZE, "ABL_MEM_ERROR_MIXED_ECC_AND_NON_ECC_DIMM_IN_SYSTEM");
      break;
    case 0x4021:
      snprintf(desc, MRC_DESC_SIZE, "ABL_MEM_ERROR_MIXED_3DS_AND_NON_3DS _DIMM_IN_CHANNEL");
      break;
    case 0x4022:
      snprintf(desc, MRC_DESC_SIZE, "ABL_MEM_ERROR_MIXED_X4_AND_X8_DIMM_IN_CHANNEL");
      break;
    case 0x4028:
      snprintf(desc, MRC_DESC_SIZE, "ABL_MEM_ERROR_MIXED_DIFFERENT_ECC_SIZE_DIMM_IN_CHANNEL");
      break;
    case 0x4029:
      snprintf(desc, MRC_DESC_SIZE, "ABL_MEM_WARNING_MEM_INSTALLED_ON_DISCONNECTED_CHANNEL");
      break;
    case 0x402A:
      snprintf(desc, MRC_DESC_SIZE, "ABL_MEM_RRW_ERROR");
      break;
    case 0x4030:
      snprintf(desc, MRC_DESC_SIZE, "ABL_MEM_ERROR_MBIST_RESULTS_ERROR");
      break;
    case 0x4033:
      snprintf(desc, MRC_DESC_SIZE, "ABL_MEM_ERROR_LRDIMM_MIXMFG");
      break;
    case 0x4065:
      snprintf(desc, MRC_DESC_SIZE, "ABL_CCD_BIST_FAILURE");
      break;
    case 0x4067:
      snprintf(desc, MRC_DESC_SIZE, "ABL_MEM_MEMORY_HEALING_BIST_ERROR");
      break;
    case 0x406A:
      snprintf(desc, MRC_DESC_SIZE, "ABL_MEM_ERROR_MODULE_POPULATION_ORDER");
      break;
    case 0x406B:
      snprintf(desc, MRC_DESC_SIZE, "ABL_MEM_ERROR_PMIC_ERROR");
      break;
    case 0x406C:
      snprintf(desc, MRC_DESC_SIZE, "ABL_MEM_CHANNEL_POPULATION_ORDER");
      break;
    case 0x406D:
      snprintf(desc, MRC_DESC_SIZE, "ABL_MEM_SPD_VERIFY_CRC_ERROR");
      break;
    case 0x406E:
      snprintf(desc, MRC_DESC_SIZE, "ABL_MEM_ERROR_PMIC_REAL_TIME_ERROR");
      break;
    default:
      snprintf(desc, MRC_DESC_SIZE, "UNKNOW_MINOR_DDEE%04X", minor);
      break;
  }
#endif
  return 0;
}

int __attribute__((weak))
plat_get_mrc_desc(uint8_t fru, uint16_t major, uint16_t minor, char *desc) {
  return -1;
}

static int
udbg_get_memory_loop_pattern (uint8_t frame, uint8_t page, uint8_t *next, uint8_t *count, uint8_t *buffer) {
  char dimm_loca[64] = "";
  char mrc_desc[MRC_DESC_SIZE] = {0};
  uint8_t mrc_warning_count = 0;
  int ret = 0;
  uint8_t pos = plat_get_fru_sel();
  DIMM_PATTERN dimm_loop_pattern = {0, "", 0, 0};

  if (!next) {
    syslog(LOG_ERR, "%s() Variable: next NULL pointer ERROR", __func__);
    return -1;
  }

  if (!count) {
    syslog(LOG_ERR, "%s() Variable: count NULL pointer ERROR", __func__);
    return -1;
  }

  if (!buffer) {
    syslog(LOG_ERR, "%s() Variable: buffer NULL pointer ERROR", __func__);
    return -1;
  }

  if (page == 1) {
    // Only update frame data while getting page 1
    // initialize and clear frame
    frame_mrc.init(&frame_mrc, FRAME_BUFF_SIZE);
    frame_mrc.max_page = 10;
    snprintf(frame_mrc.title, 32, "DIMM loop");

    pal_get_mrc_warning_count(pos, &mrc_warning_count);
    for (size_t i = 0; i < mrc_warning_count; i++) {
      ret = pal_get_dimm_loop_pattern(pos, i, &dimm_loop_pattern);
      if (ret < 0) {
        syslog(LOG_ERR, "%s() Fail to get DIMM loop pattern at index %u", __func__, i);
        continue;
      }

      ret = plat_get_mrc_desc(pos, dimm_loop_pattern.major_code, dimm_loop_pattern.minor_code, mrc_desc);
      if (ret < 0) {
        syslog(LOG_ERR, "%s() Fail to get MRC description", __func__);
        continue;
      }

      snprintf(dimm_loca, sizeof(dimm_loca), "DIMM %s", (dimm_loop_pattern.dimm_location));
      frame_mrc.append(&frame_mrc, dimm_loca, 0);
      frame_mrc.append(&frame_mrc, mrc_desc, 1);
    }
  }

  if (page > frame_mrc.pages) {
    return -1;
  }

  ret = frame_mrc.getPage(&frame_mrc, page, (char *)buffer, FRAME_PAGE_BUF_SIZE);
  if (ret < 0) {
    *count = 0;
    return -1;
  }
  *count = (uint8_t)ret;

  if (page < frame_mrc.pages)
    *next = page + 1;
  else
    *next = 0xFF;  // Set the value of next to 0xFF to indicate this is the last page

  return 0;
}

int
plat_udbg_get_frame_data(uint8_t frame, uint8_t page, uint8_t *next, uint8_t *count, uint8_t *buffer)
{
  if (!plat_supported()) {
    return -1;
  }
  switch (frame) {
    case 1: //info_page
      return udbg_get_info_page(frame, page, next, count, buffer);
    case 2: //critical SEL
      return udbg_get_cri_sel(frame, page, next, count, buffer);
    case 3: //critical Sensor
      return udbg_get_cri_sensor(frame, page, next, count, buffer);
    case 4: //dimm loop pattern
      return udbg_get_memory_loop_pattern(frame, page, next, count, buffer);
    case 5:
      return udbg_get_postcode(frame, page, next, count, buffer);
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
  uint8_t pos = plat_get_fru_sel();

  if (pos != FRU_ALL && pal_get_boot_order(pos, buff, buff, &len) == 0) {
    if (item > 0 && item < SIZE_BOOT_ORDER) {
      pickup = buff[item];
      while (item > 1) {
        buff[item] = buff[item -1];
        item--;
      }
      buff[item] = pickup;
      buff[0] |= 0x80;
      pal_set_boot_order(pos, buff, buff, &len);

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
  } else {
    panels[PANEL_BOOT_ORDER].item_num = 0;
  }
  return PANEL_BOOT_ORDER;
}

static uint8_t panel_power_policy (uint8_t item) {
  uint8_t buff[32] = {0};
  uint8_t res_len;
  uint8_t pos = plat_get_fru_sel();
  uint8_t policy;
  uint8_t pwr_policy_item_map[3] = {POWER_CFG_ON, POWER_CFG_LPS, POWER_CFG_OFF};

  if (pos != FRU_ALL) {
    if (item > 0 && item <= sizeof(pwr_policy_item_map)) {
      policy = pwr_policy_item_map[item - 1];
      pal_set_power_restore_policy(pos, &policy, NULL);
    }
    pal_get_chassis_status(pos, NULL, buff, &res_len);
    policy = (((uint8_t)buff[0]) >> 5) & 0x7;
    snprintf(panels[PANEL_POWER_POLICY].item_str[1], 32,
        "%cPower On", policy == POWER_CFG_ON ? '*' : ' ');
    snprintf(panels[PANEL_POWER_POLICY].item_str[2], 32,
        "%cLast State", policy == POWER_CFG_LPS ? '*' : ' ');
    snprintf(panels[PANEL_POWER_POLICY].item_str[3], 32,
        "%cPower Off", policy == POWER_CFG_OFF ? '*' : ' ');
    panels[PANEL_POWER_POLICY].item_num = 3;
  } else {
    panels[PANEL_POWER_POLICY].item_num = 0;
  }
  return PANEL_POWER_POLICY;
}

int
plat_udbg_control_panel(uint8_t panel, uint8_t operation, uint8_t item, uint8_t *count, uint8_t *buffer)
{
  if (!plat_supported()) {
    return -1;
  }

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
