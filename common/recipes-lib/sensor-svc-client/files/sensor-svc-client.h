/*
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
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

#ifndef __SENSOR_SVC_CLIENT_H__
#define __SENSOR_SVC_CLIENT_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SENSOR_SVC_DBUS_NAME "org.openbmc.SensorService"
#define SENSOR_SVC_BASE_PATH "/org/openbmc/SensorService"
#define SENSOR_SVC_SENSOR_TREE_INTERFACE "org.openbmc.SensorTree"
#define SENSOR_SVC_SENSOR_OBJECT_INTERFACE "org.openbmc.SensorObject"

extern int sensor_svc_raw_read(uint8_t fru, uint8_t sensor_num, float *value);
extern int sensor_svc_read(uint8_t fru, uint8_t sensor_num, float *value);

#ifdef __cplusplus
} // extern "C"
#endif
#endif /* __SENSOR_SVC_CLIENT_H__ */
