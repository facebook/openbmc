/*
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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
#ifndef _PLDM_PMC_H_
#define _PLDM_PMC_H_

#include "pldm_base.h"
// defines data structure specified in DSP0248 v1.1.1 PLDM Platform Monitoring
//  and Control Spec

// fw update cmds completion codes
#define CC_PMC_BASE          0x80


// PLDM Firmware update commands
typedef enum pldm_pmc_cmds {
  // terminus commands
  CMD_PMC_SET_TID          = 0x01,
  CMD_PMC_GET_TID          = 0x02,
  CMD_PMC_GET_TERMINUS_UUID = 0x03,
  // numeric sensor commands
  CMD_GET_SENSOR_READING = 0x11,
  CMD_GET_SENSOR_THRESHOLDS = 0x12,
  CMD_SET_SENSOR_THRESHOLDS = 0x13,
  CMD_RESTORE_THRESHOLDS    = 0x14,
  CMD_GET_SENSOR_HYSTERESIS = 0x15,
  CMD_SET_SENSOR_HYSTERESIS = 0x16,

  // state sensor commands
  CMD_SET_STATE_SENSOR_ENABLES = 0x20,
  CMD_GET_STATE_SENSOR_READINGS = 0x21,

  // PDR Repository commands
  CMD_GET_PDR_REPO_INFO       = 0x50,
  CMD_GET_PDR                 = 0x51,
  CMD_FIND_PDR                = 0x52,
} PldmPMCCmds;


typedef enum pldm_sensor_id_mlx {
  MLX_PRIMARY_TEMP_SENSOR_ID             = 0x1,
  MLX_AMBIENT_TEMP_SENSOR_ID             = 0x2,  // not supported
  MLX_HEALTH_STATE_SENSOR_ID             = 0x3,
  MLX_PORT_0_LINK_STATE_SENSOR_ID        = 0x4,
  MLX_PORT_1_LINK_STATE_SENSOR_ID        = 0x5,
  MLX_PORT_0_TEMP_SENSOR_ID              = 0x6,   // need cable to support
  MLX_PORT_1_TEMP_SENSOR_ID              = 0x7,   // need cable to support
  MLX_CABLE_1_COMPOSITE_STATE_SENSOR_ID  = 0x8,   // not supported
  MLX_CABLE_2_COMPOSITE_STATE_SENSOR_ID  = 0x9,   // not supported
  MLX_PORT_0_LINK_SPEED_SENSOR_ID        = 0xA,
  MLX_PORT_1_LINK_SPEED_SENSOR_ID        = 0xB,
} pldm_sensor_id_mlx;


typedef enum pldm_sensor_id_brcm {
  /* Numeric Sensors */
  BRCM_THERMAL_SENSOR_ONCHIP_ID    = 3,
  BRCM_THERMAL_SENSOR_PORT_0_ID    = 4,
  BRCM_THERMAL_SENSOR_PORT_1_ID    = 5,
  BRCM_LINK_SPEED_SENSOR_PORT_0_ID = 6,
  BRCM_LINK_SPEED_SENSOR_PORT_1_ID = 7,

  /* State Sensors */
  BRCM_DEVICE_STATE_SENSORS_ID     = 8,
  BRCM_LINK_STATE_SENSOR_PORT_0_ID = 9,
  BRCM_LINK_STATE_SENSOR_PORT_1_ID = 0xA,
} pldm_sensor_id_brcm;



// cdb for pldm cmd CMD_GET_SENSOR_READING
typedef struct {
	uint16_t sensorId;
  uint8_t  rearmEventState;
} __attribute__((packed)) PLDM_Get_Sensor_Reading_t;

// response for CMD_GET_SENSOR_READING
typedef struct {
  uint8_t completionCode;
  uint8_t dataSize;
  uint8_t opState;
  uint8_t eventMsgEnable;
  uint8_t presentState;
  uint8_t prevState;
  uint8_t eventState;
  uint32_t reading;
} __attribute__((packed)) PLDM_SensorReading_Response_t;


// cdb for pldm cmd CMD_GET_SENSOR_THRESHOLD
typedef struct {
	uint16_t sensorId;
} __attribute__((packed)) PLDM_Get_Sensor_Thresh_t;

// response for CMD_GET_SENSOR_THRESHOLD
#define MAX_SENSOR_THRESHOLD_LENGTH 24
typedef struct {
  uint8_t completionCode;
  uint8_t dataSize;
  uint8_t rawdata[MAX_SENSOR_THRESHOLD_LENGTH];
} __attribute__((packed)) PLDM_SensorThresh_Response_t;

typedef struct {
  uint8_t upperWarning;
  uint8_t upperCrit;
  uint8_t upperFatal;
  uint8_t lowerWarning;
  uint8_t lowerCrit;
  uint8_t lowerFatal;
} __attribute__((packed)) PLDM_SensorThresh_int8_t;

typedef struct {
  uint16_t upperWarning;
  uint16_t upperCrit;
  uint16_t upperFatal;
  uint16_t lowerWarning;
  uint16_t lowerCrit;
  uint16_t lowerFatal;
} __attribute__((packed)) PLDM_SensorThresh_int16_t;

typedef struct {
  uint32_t upperWarning;
  uint32_t upperCrit;
  uint32_t upperFatal;
  uint32_t lowerWarning;
  uint32_t lowerCrit;
  uint32_t lowerFatal;
} __attribute__((packed)) PLDM_SensorThresh_int32_t;


// cdb for pldm cmd CMD_SET_STATE_SENSOR_ENABLES
typedef struct {
	uint16_t sensorId;
  uint8_t  sensorRearm;
  uint8_t  rsv;
} __attribute__((packed)) PLDM_Get_StateSensor_Reading_t;

typedef struct {
  uint8_t opState;
  uint8_t presentState;
  uint8_t prevState;
  uint8_t eventState;
} __attribute__((packed)) PLDM_StateField_t;

#define MAX_COMPOSITE_SENSOR 8
typedef struct {
  uint8_t completionCode;
  uint8_t compositeSensorCount;
  PLDM_StateField_t sensors[MAX_COMPOSITE_SENSOR];
} __attribute__((packed)) PLDM_StateSensorReading_Response_t;


void pldmCreateGetSensorReadingCmd(pldm_cmd_req *pPldmCdb, uint16_t sensor);
void pldmCreateGetSensorThreshCmd(pldm_cmd_req *pPldmCdb, uint16_t sensor);
void pldmCreateGetStateSensorReadingCmd(pldm_cmd_req *pPldmCdb, uint16_t sensor);
int pldm_get_num_snr_val(PLDM_SensorReading_Response_t *pData);
int pldmHandleGetSensorReadingResp(PLDM_SensorReading_Response_t *pPldmResp);
int pldm_get_state_snr_val(PLDM_StateSensorReading_Response_t *pData);
int pldmHandleGetStateSensorReadingResp(PLDM_StateSensorReading_Response_t *pPldmResp);
int pldmHandlePmcResp(pldm_response *pResp);
#endif
