/*
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
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

#ifndef __VR_H__
#define __VR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <openbmc/pmbus.h>
#include <stdint.h>
#include <stdbool.h>

#define VR_WARN_REMAIN_WR 3

#define MAX_VER_STR_LEN 80
#define VR_REG_PAGE 0x00

enum {
  VR_STATUS_SUCCESS = 0,
  VR_STATUS_FAILURE = -1,
  VR_STATUS_SKIP    = -2,
};

struct vr_info;

struct vr_ops {
  /*
   * get_fw_ver: (Required)
   * 	This function shall fill string with version number of configuration.
   * 	Return -1 if failed, otherwise 0.
   */
  int (*get_fw_ver)(struct vr_info*, char*);

  /* parse_file: (Required)
   * 	This function shall parse input file and save as configuration.
   * 	Return the pointer to the data structure , or NULL if failed.
   */
  void* (*parse_file)(struct vr_info*, const char*);

  /*
   * validate_file: (Optional)
   * 	This function shall validate the configuration file.
   * 	Retrun -1 if failed, otherwise 0.
   */
  int (*validate_file)(struct vr_info*, const char*);

  /*
   * fw_update: (Required)
   * 	This function performs the update operation with configuration.
   * 	Return -1 if failed, otherwise 0.
   */
  int (*fw_update)(struct vr_info*, void*);

  /*
   * fw_verify: (Optional)
   *	This function shall check if the data of VR match with configuration.
   * 	Retrun -1 if failed, otherwise 0.
   */
  int (*fw_verify)(struct vr_info*, void*);
};

struct vr_info {
  uint8_t bus;
  uint8_t addr;
  uint8_t slot_id;
  uint8_t padding[2];
  uint64_t dev_id;
  char dev_name[64];
  bool force;
  struct vr_ops *ops;
  void *private_data;
  int (*xfer)(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t);
  int (*sensor_polling_ctrl)(bool);
};

extern void *plat_configs;

int vr_device_register(struct vr_info*, int);
void vr_device_unregister(void);
int vr_probe(void);
void vr_remove(void);
int vr_fw_version(int, const char*, char*);
int vr_fw_full_version(int, const char*, char*, char*);
void vr_get_fw_avtive_key(struct vr_info*, char*);
int vr_fw_update(const char*, const char*, bool);

extern int plat_vr_init(void);
extern void plat_vr_exit(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
