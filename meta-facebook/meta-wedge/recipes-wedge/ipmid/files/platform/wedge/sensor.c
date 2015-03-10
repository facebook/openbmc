/*
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
 * This file provides platform specific implementation of sensor information
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
#include "../sensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>

#define SENSOR_MGMT_MAX 1
#define SENSOR_DISC_MAX 8
#define SENSOR_THRESH_MAX 1
#define SENSOR_OEM_MAX 1

#define BMC_SLAVE_ADDR 0x20

typedef struct {
  unsigned char num;
  sensor_mgmt_t sensor[SENSOR_MGMT_MAX];
} sensor_mgmt_info_t;

typedef struct {
  unsigned char num;
  sensor_disc_t sensor[SENSOR_DISC_MAX];
} sensor_disc_info_t;

typedef struct {
  unsigned char num;
  sensor_thresh_t sensor[SENSOR_THRESH_MAX];
} sensor_thresh_info_t;

typedef struct {
  unsigned char num;
  sensor_oem_t sensor[SENSOR_OEM_MAX];
} sensor_oem_info_t;

// Global structures
static sensor_mgmt_info_t g_sensor_mgmt = {0};
static sensor_disc_info_t g_sensor_disc = {0};
static sensor_thresh_info_t g_sensor_thresh = {0};
static sensor_oem_info_t g_sensor_oem = {0};

static void
populate_mgmt_sensors(void) {
  sensor_mgmt_t sensor = {0};

  // Add record for the AST2100 BMC
  sensor.slave_addr = BMC_SLAVE_ADDR;
  sensor.chan_no = 0x0; // Primary BMC controller

  // Init Agent = false
  sensor.pwr_state_init = 0x00;

  // FRUID = true, SEL = true, SDR = true, Sensor = true
  sensor.dev_caps = 0x0F;

  // Device ID string
  // Type - 0xC0: ASCII, Length - 0x09
  sensor.str_type_len = 0xC0 + 0x09;
  strncpy(sensor.str, "Wedge-BMC", 0x09);

  // Add this sensor to the global table
  if (g_sensor_mgmt.num >= SENSOR_MGMT_MAX) {
    syslog(LOG_ALERT, "populate_mgmt_sensors: num exceeded\n");
    return;
  }

  memcpy(&g_sensor_mgmt.sensor[g_sensor_mgmt.num], &sensor, sizeof(sensor_mgmt_t));

  g_sensor_mgmt.num++;

  return;
}

static void
populate_disc_sensors(void) {

  sensor_disc_t sensor = {0};

  // Sensor uS Status
  // Sensor# 0x10
  // EntitiyId# 0xD0, EntityInst# 0x00
  // Sensor Type# Chassis 0x18
  // Event Read/Type# OEM 0x70
  sensor.owner= BMC_SLAVE_ADDR;
  sensor.lun = 0x00;
  sensor.sensor_num = 0x10;

  sensor.ent_id = 0xD0;
  sensor.ent_inst = 0x00;
  // Enable Scanning, Enable Events
  sensor.sensor_init = 0x63;
  // Supports Auto Re-Arm
  sensor.sensor_caps = 0x40;
  sensor.sensor_type = 0x18;
  sensor.evt_read_type = 0x70;
  // 1-bit for CPU0 Thermal Trip
  sensor.assert_evt_mask[0] = 0x04;
  sensor.deassert_evt_mask[0] = 0x00;
  sensor.read_evt_mask[0] = 0x04;

  // Device ID string
  // Type - 0xC0: ASCII, Length - 12
  sensor.str_type_len = 0xC0 + 9;
  strncpy(sensor.str, "uS-Status", 9);

  // Add this sensor to the global table
  if (g_sensor_disc.num >= SENSOR_DISC_MAX) {
    syslog(LOG_ALERT, "populate_disc_sensors: num exceeded\n");
    return;
  }

  memcpy(&g_sensor_disc.sensor[g_sensor_disc.num], &sensor, sizeof(sensor_disc_t));

  g_sensor_disc.num++;

  // Sensor SEL Status
  // Sensor# 0x5F
  // EntitiyId# 0xD0, EntityInst# 0x02
  // Sensor Type# OEM: 0xC0
  // Event Read/Type# Sensor Specific: 0x6F
  sensor.owner= BMC_SLAVE_ADDR;
  sensor.lun = 0x00;
  sensor.sensor_num = 0x5F;

  sensor.ent_id = 0xD0;
  sensor.ent_inst = 0x02;
  // Enable Scanning, Enable Events
  sensor.sensor_init = 0x63;
  // Supports Auto Re-Arm
  sensor.sensor_caps = 0x40;
  sensor.sensor_type = 0xC0;
  sensor.evt_read_type = 0x6F;
  // SEL Clear(bit1), SEL Rollover(bit8)
  sensor.assert_evt_mask[0] = 0x02;
  sensor.assert_evt_mask[1] = 0x01;
  sensor.deassert_evt_mask[0] = 0x00;
  sensor.deassert_evt_mask[1] = 0x00;
  sensor.read_evt_mask[0] = 0x02;
  sensor.read_evt_mask[1] = 0x01;

  // Device ID string
  // Type - 0xC0: ASCII, Length - 12
  sensor.str_type_len = 0xC0 + 10;
  strncpy(sensor.str, "SEL-Status", 10);

  // Add this sensor to the global table
  if (g_sensor_disc.num >= SENSOR_DISC_MAX) {
    syslog(LOG_ALERT, "populate_disc_sensors: num exceeded\n");
    return;
  }

  memcpy(&g_sensor_disc.sensor[g_sensor_disc.num], &sensor, sizeof(sensor_disc_t));

  g_sensor_disc.num++;


  // Sensor WDT
  // Sensor# 0x60
  // EntitiyId# 0xD0, EntityInst# 0x03
  // Sensor Type# WDT2: 0x23
  // Event Read/Type# Sensor Specific: 0x6F
  sensor.owner= BMC_SLAVE_ADDR;
  sensor.lun = 0x00;
  sensor.sensor_num = 0x60;

  sensor.ent_id = 0xD0;
  sensor.ent_inst = 0x03;
  // Enable Scanning, Enable Events
  sensor.sensor_init = 0x63;
  // Supports Auto Re-Arm
  sensor.sensor_caps = 0x40;
  sensor.sensor_type = 0x23;
  sensor.evt_read_type = 0x6F;
  // 5 bits for expiry, reset, pwrdown, pwrcycle, timer
  sensor.assert_evt_mask[0] = 0x0F;
  sensor.assert_evt_mask[1] = 0x01;
  sensor.deassert_evt_mask[0] = 0x00;
  sensor.deassert_evt_mask[1] = 0x00;
  sensor.read_evt_mask[0] = 0x0F;
  sensor.read_evt_mask[1] = 0x01;

  // Device ID string
  // Type - 0xC0: ASCII, Length - 12
  sensor.str_type_len = 0xC0 + 3;
  strncpy(sensor.str, "WDT", 3);

  // Add this sensor to the global table
  if (g_sensor_disc.num >= SENSOR_DISC_MAX) {
    syslog(LOG_ALERT, "populate_disc_sensors: num exceeded\n");
    return;
  }

  memcpy(&g_sensor_disc.sensor[g_sensor_disc.num], &sensor, sizeof(sensor_disc_t));

  g_sensor_disc.num++;


  // Sensor Chassis Pwr Sts
  // Sensor# 0x70
  // EntitiyId# 0x15, EntityInst# 0x00
  // Sensor Type# OEM: 0xC8
  // Event Read/Type# Sensor Specific: 0x6F
  sensor.owner= BMC_SLAVE_ADDR;
  sensor.lun = 0x00;
  sensor.sensor_num = 0x70;

  sensor.ent_id = 0x15;
  sensor.ent_inst = 0x00;
  // Enable Scanning, Enable Events
  sensor.sensor_init = 0x63;
  // Supports Auto Re-Arm
  sensor.sensor_caps = 0x40;
  sensor.sensor_type = 0xC8;
  sensor.evt_read_type = 0x6F;
  // 6 bits for pwroff, pwrcycle, pwron, softdown, ac-lost, hard-reset
  sensor.assert_evt_mask[0] = 0x3F;
  sensor.deassert_evt_mask[0] = 0x00;
  sensor.read_evt_mask[0] = 0x3F;

  // Device ID string
  // Type - 0xC0: ASCII, Length - 12
  sensor.str_type_len = 0xC0 + 13;
  strncpy(sensor.str, "CH-Pwr-Status", 13);

  // Add this sensor to the global table
  if (g_sensor_disc.num >= SENSOR_DISC_MAX) {
    syslog(LOG_ALERT, "populate_disc_sensors: num exceeded\n");
    return;
  }

  memcpy(&g_sensor_disc.sensor[g_sensor_disc.num], &sensor, sizeof(sensor_disc_t));

  g_sensor_disc.num++;

  // Sensor CPU DIMM Hot
  // Sensor# 0xB3
  // EntitiyId# 0xD0, EntityInst# 0x05
  // Sensor Type# OEM 0xC6
  // Event Read/Type# Sensor Specific 6Fh
  sensor.owner= BMC_SLAVE_ADDR;
  sensor.lun = 0x00;
  sensor.sensor_num = 0xB3;

  sensor.ent_id = 0xD0;
  sensor.ent_inst = 0x05;
  // Enable Scanning, Enable Events
  sensor.sensor_init = 0x63;
  // Supports Auto Re-Arm
  sensor.sensor_caps = 0x40;
  sensor.sensor_type = 0xC6;
  sensor.evt_read_type = 0x6F;
  // Two bits for CPU Hot, DIMM Hot
  sensor.assert_evt_mask[0] = 0x05;
  sensor.deassert_evt_mask[0] = 0x05;
  sensor.read_evt_mask[0] = 0x05;

  // Device ID string
  // Type - 0xC0: ASCII, Length - 12
  sensor.str_type_len = 0xC0 + 12;
  strncpy(sensor.str, "CPU_DIMM_HOT", 12);

  // Add this sensor to the global table
  if (g_sensor_disc.num >= SENSOR_DISC_MAX) {
    syslog(LOG_ALERT, "populate_disc_sensors: num exceeded\n");
    return;
  }

  memcpy(&g_sensor_disc.sensor[g_sensor_disc.num], &sensor, sizeof(sensor_disc_t));

  g_sensor_disc.num++;

  // Sensor PMBus Status Word Low
  // Sensor PMBus Status Word High
  // Sensor PMBus Status MFR
  // Sensor PMBus Status Input
  // Sensor NTP Status
  // Sensor# 0xED
  // EntitiyId# 0x35, EntityInst# 0x00
  // Sensor Type# OEM 0xC7
  // Event Read/Type# Sensor Specific 6Fh
  sensor.owner= BMC_SLAVE_ADDR;
  sensor.lun = 0x00;
  sensor.sensor_num = 0xED;

  sensor.ent_id = 0x35;
  sensor.ent_inst = 0x00;
  // Enable Scanning, Enable Events
  sensor.sensor_init = 0x63;
  // Supports Auto Re-Arm
  sensor.sensor_caps = 0x40;
  sensor.sensor_type = 0xC7;
  sensor.evt_read_type = 0x6F;
  // 1-bit for date/time sync failed
  sensor.assert_evt_mask[0] = 0x01;
  sensor.deassert_evt_mask[0] = 0x00;
  sensor.read_evt_mask[0] = 0x01;

  // Device ID string
  // Type - 0xC0: ASCII, Length - 10
  sensor.str_type_len = 0xC0 + 12;
  strncpy(sensor.str, "NTP-Status", 10);

  // Add this sensor to the global table
  if (g_sensor_disc.num >= SENSOR_DISC_MAX) {
    syslog(LOG_ALERT, "populate_disc_sensors: num exceeded\n");
    return;
  }

  memcpy(&g_sensor_disc.sensor[g_sensor_disc.num], &sensor, sizeof(sensor_disc_t));

  g_sensor_disc.num++;

  return;
}

// Access functions for Sensor Table
void
plat_sensor_mgmt_info(int *p_num, sensor_mgmt_t **p_sensor) {
  *p_num = g_sensor_mgmt.num;
  *p_sensor = g_sensor_mgmt.sensor;
}

void
plat_sensor_disc_info(int *p_num, sensor_disc_t **p_sensor) {
  *p_num = g_sensor_disc.num;
  *p_sensor = g_sensor_disc.sensor;
}

void
plat_sensor_thresh_info(int *p_num, sensor_thresh_t **p_sensor) {
  *p_num = g_sensor_thresh.num;
  *p_sensor = g_sensor_thresh.sensor;
}

void
plat_sensor_oem_info(int *p_num, sensor_oem_t **p_sensor) {
  *p_num = g_sensor_oem.num;
  *p_sensor = g_sensor_oem.sensor;
}

// Initialize Sensor Table
int
plat_sensor_init(void) {

  // Populate all Sensors
  populate_mgmt_sensors();
  populate_disc_sensors();

  return 0;
}
