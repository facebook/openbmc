/*
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
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

 /*
  * Wedge100 specific Sensor routine - There is no HW sensor related
  * code here - Everything initalized here is S/W sensors such as
  * BMC itself as a sensor and S/W watchdog.
  */

#include "sensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>

/*
 * Wedge100 doesn't support any OEM sensor nor any threshold,
 * but we leave it as 1, because we cannot define an array with size 0.
 */
#define SENSOR_THRESH_MAX 1
#define SENSOR_OEM_MAX 1
#define SENSOR_MGMT_MAX 1
#define SENSOR_DISC_MAX 2
#define BMC_SLAVE_ADDR 0x20

// Type definitions
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

// Global sensor arrays
static sensor_mgmt_info_t g_sensor_mgmt = {0};
static sensor_disc_info_t g_sensor_disc = {0};
static sensor_thresh_info_t g_sensor_thresh = {0};
static sensor_oem_info_t g_sensor_oem = {0};

// Access functions to the global arrays
void plat_sensor_mgmt_info(int *p_num, sensor_mgmt_t **p_sensor) {
  *p_num = g_sensor_mgmt.num;
  *p_sensor = g_sensor_mgmt.sensor;
}
void plat_sensor_disc_info(int *p_num, sensor_disc_t **p_sensor) {
  *p_num = g_sensor_disc.num;
  *p_sensor = g_sensor_disc.sensor;
}
void plat_sensor_thresh_info(int *p_num, sensor_thresh_t **p_sensor) {
  *p_num = g_sensor_thresh.num;
  *p_sensor = g_sensor_thresh.sensor;
}
void plat_sensor_oem_info(int *p_num, sensor_oem_t **p_sensor) {
  *p_num = g_sensor_oem.num;
  *p_sensor = g_sensor_oem.sensor;
}


// A function to add BMC itself as a IPMI management interface
static void add_bmc_mgmt() {
  sensor_mgmt_t sensor = {0};
  // Add record for the AST2400 BMC
  sensor.slave_addr = BMC_SLAVE_ADDR;
  sensor.chan_no = 0x0;
  sensor.pwr_state_init = 0x00;
  // FRUID = true, SEL = true, SDR = true, Sensor = true
  sensor.dev_caps = 0x0F;
  // Type - 0xC0: ASCII, Length - 0x09
  sensor.str_type_len = 0xC0 + 0x09;
  strncpy(sensor.str, "WEDGE100-BMC ", 0x09);
  // Add this sensor to the global table
  if (g_sensor_mgmt.num >= SENSOR_MGMT_MAX) {
    syslog(LOG_WARNING, "populate_mgmt_sensors: num exceeded\n");
    return;
  }
  memcpy(&g_sensor_mgmt.sensor[g_sensor_mgmt.num], &sensor,
         sizeof(sensor_mgmt_t));
  g_sensor_mgmt.num++;

  return;
}

// Function to add SEL repository as a sensor
static void add_sel_sensor() {
  sensor_disc_t sensor = {0};
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
    syslog(LOG_WARNING, "populate_disc_sensors: num exceeded\n");
    return;
  }
  memcpy(&g_sensor_disc.sensor[g_sensor_disc.num], &sensor,
        sizeof(sensor_disc_t));
  g_sensor_disc.num++;
  return;
}

// Function to add BMC's software watchdog times as a sensor
static void add_wdt_sensor() {
  sensor_disc_t sensor = {0};
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
  sensor.str_type_len = 0xC0 + 3;
  strncpy(sensor.str, "WDT", 3);
  // Add this sensor to the global table
  if (g_sensor_disc.num >= SENSOR_DISC_MAX) {
    syslog(LOG_WARNING, "populate_disc_sensors: num exceeded\n");
    return;
  }
  memcpy(&g_sensor_disc.sensor[g_sensor_disc.num], &sensor,
         sizeof(sensor_disc_t));
  g_sensor_disc.num++;
  return;
}

/*
 * In Galaxy100, we initially have SEL sensor and
 * WDT (software watchdog) only
 */
static void populate_disc_sensors(void) {
  add_sel_sensor();
  add_wdt_sensor();
  return;
}
static void populate_mgmt_sensors(void) {
  add_bmc_mgmt();
}

// Initialize Sensor Table
int plat_sensor_init(void) {
  // Populate all Sensors
  populate_mgmt_sensors();
  populate_disc_sensors();

  return 0;
}
