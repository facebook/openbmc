/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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

#ifndef __BIC_FWUPDATE_H__
#define __BIC_FWUPDATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define AST_BIC_IPMB_WRITE_COUNT_MAX 224
#define PKT_SIZE (64*1024)

enum {
  UPDATE_BIOS = 0,
  UPDATE_CPLD,
  UPDATE_BIC,
  UPDATE_CXL,
};

enum {
  DUMP_BIOS = 0,
};

// Update from file.
int bic_update_fw(uint8_t slot_id, uint8_t comp, char *path, uint8_t force);

// Update from existing file descriptor. May be a pipe or other stream, don't assume it's seekable.
int bic_update_fw_fd(uint8_t slot_id, uint8_t comp, int fd, uint8_t force);
int get_board_revid_from_bbbic(uint8_t slot_id, uint8_t* rev_id);
int get_board_rev(uint8_t slot_id, uint8_t board_id, uint8_t* rev_id);
char* get_component_name(uint8_t comp);
int dump_bic_usb_bios(uint8_t slot_id, uint8_t comp, char *path);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __BIC_FWUPDATE_H__ */
