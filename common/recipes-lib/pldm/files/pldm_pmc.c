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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <openbmc/ncsi.h>
#include "pldm_base.h"
#include "pldm.h"
#include "pldm_pmc.h"


#define DEBUG_PLDM

#ifdef DEBUG_PLDM
  #define DBG_PRINT(fmt, args...) printf(fmt, ##args)
#else
  #define DBG_PRINT(fmt, args...)
#endif


/* PLDM numeric sensor decoding */
const char *sensor_data_size[] = {
    "uint8",
    "sint8",
    "uint16",
    "sint16",
    "uint32",
    "sint32",
};

const char *sensor_opState_str[] = {
    "enabled",
    "disabled",
    "unavailable",
    "statusUnknown",
    "failed",
    "initializing",
    "shuttingDown",
    "inTest",
};

const char *numeric_event_str[] = {
    "noEventGeneration",
    "eventsDisabled",
    "eventsEnabled",
    "opEventsOnlyEnabled",
    "stateEventsOnlyEnabled",
};

const char *sensor_state_str[] = {
    "Unknown",
    "Normal",
    "Warning",
    "Critical",
    "Fatal",
    "LowerWarning",
    "LowerCritical",
    "LowerFatal",
    "UpperWarning",
    "Critical",
    "UpperFatal",
};


// PLDM Platform monitoring & Control commands

// Cmd CMD_GET_SENSOR_READING
void pldmCreateGetSensorReadingCmd(pldm_cmd_req *pPldmCdb, uint16_t sensor)
{
  PLDM_Get_Sensor_Reading_t *pCmdPayload =
      (PLDM_Get_Sensor_Reading_t *)&(pPldmCdb->payload);

  memset(pPldmCdb, 0, sizeof(pldm_cmd_req));
  genReqCommonFields(PLDM_TYPE_PLATFORM_MONITORING_AND_CTRL, CMD_GET_SENSOR_READING,
                     &(pPldmCdb->common[0]));
  pCmdPayload->sensorId = sensor;
  pCmdPayload->rearmEventState = 0;

  pPldmCdb->payload_size = PLDM_COMMON_REQ_LEN + sizeof(PLDM_Get_Sensor_Reading_t);

  dbgPrintCdb(pPldmCdb);
  return;
}

// CMD_GET_SENSOR_THRESHOLDS
void pldmCreateGetSensorThreshCmd(pldm_cmd_req *pPldmCdb, uint16_t sensor)
{
  PLDM_Get_Sensor_Thresh_t *pCmdPayload =
      (PLDM_Get_Sensor_Thresh_t *)&(pPldmCdb->payload);

  memset(pPldmCdb, 0, sizeof(pldm_cmd_req));
  genReqCommonFields(PLDM_TYPE_PLATFORM_MONITORING_AND_CTRL, CMD_GET_SENSOR_THRESHOLDS,
                     &(pPldmCdb->common[0]));

  pCmdPayload->sensorId = sensor;
  pPldmCdb->payload_size = PLDM_COMMON_REQ_LEN + sizeof(PLDM_Get_Sensor_Thresh_t);
  dbgPrintCdb(pPldmCdb);
  return;
}



// Cmd CMD_GET_STATE_SENSOR_READINGS
void pldmCreateGetStateSensorReadingCmd(pldm_cmd_req *pPldmCdb, uint16_t sensor)
{
  PLDM_Get_StateSensor_Reading_t *pCmdPayload =
      (PLDM_Get_StateSensor_Reading_t *)&(pPldmCdb->payload);


  memset(pPldmCdb, 0, sizeof(pldm_cmd_req));
  genReqCommonFields(PLDM_TYPE_PLATFORM_MONITORING_AND_CTRL, CMD_GET_STATE_SENSOR_READINGS,
                     &(pPldmCdb->common[0]));
  pCmdPayload->sensorId = sensor;
  pCmdPayload->sensorRearm = 0;
  pPldmCdb->payload_size = PLDM_COMMON_REQ_LEN + sizeof(PLDM_Get_StateSensor_Reading_t);

  dbgPrintCdb(pPldmCdb);
  return;
}


const char *
numeric_state_to_name(unsigned int state, const char *name_str[], size_t n)
{
  if (state < 0 || state >= n  || name_str[state] == NULL) {
      return "unknown_str_type";
  }
  return name_str[state];
}

// PLDM numeric sensor can be different size, this helper function
// decodes it and return the current sensor reading
int pldm_get_num_snr_val(PLDM_SensorReading_Response_t *pData)
{
  int snr_val = 0;

  if (pData->dataSize <= 1) { // uint8/sint8
    snr_val = pData->reading & 0xff;
  } else if (pData->dataSize <= 3) {  // uint16/sint16
    snr_val = pData->reading & 0xffff;
  } else {  // uint32/sint32
    snr_val = pData->reading;
  }

  return snr_val;
}

void dbgPrintSensorData(PLDM_SensorReading_Response_t *pData)
{
  printf("Reading Numeric Sensor (PLDM type 2, Cmd 0x11)\n");
  printf("  data size:            %d (%s)\n", pData->dataSize,
          numeric_state_to_name(pData->dataSize, sensor_data_size,
               sizeof(sensor_data_size)/sizeof(sensor_data_size[0])));
  printf("  operational state:    %d (%s)\n", pData->opState,
          numeric_state_to_name(pData->opState, sensor_opState_str,
               sizeof(sensor_opState_str)/sizeof(sensor_opState_str[0])));
  printf("  event message enable: %d (%s)\n", pData->eventMsgEnable,
          numeric_state_to_name(pData->eventMsgEnable, numeric_event_str,
               sizeof(numeric_event_str)/sizeof(numeric_event_str[0])));
  printf("  present state:        %d (%s)\n", pData->presentState,
          numeric_state_to_name(pData->presentState, sensor_state_str,
               sizeof(sensor_state_str)/sizeof(sensor_state_str[0])));
  printf("  previous state:       %d (%s)\n", pData->prevState,
          numeric_state_to_name(pData->prevState, sensor_state_str,
               sizeof(sensor_state_str)/sizeof(sensor_state_str[0])));
  printf("  event state:          %d (%s)\n", pData->eventState,
          numeric_state_to_name(pData->eventState, sensor_state_str,
               sizeof(sensor_state_str)/sizeof(sensor_state_str[0])));

  // hack here, "sensor reading" may be variable size
  if (pData->dataSize <= 1) { // uint8/sint8
    uint8_t reading = pData->reading & 0xff;
    printf("  present reading: 0x%x (%d)\n", reading, reading);
  } else if (pData->dataSize <= 3) {  // uint16/sint16
    uint16_t reading = pData->reading & 0xffff;
    printf("  present reading: 0x%x (%d)\n", reading, reading);
  } else {  // uint32/sint32
    uint32_t reading = pData->reading;
    printf("  present reading: 0x%x (%d)\n", reading, reading);
  }
}
// handle Cmd 0x011 (Get numeric sensor reading) response
int pldmHandleGetSensorReadingResp(PLDM_SensorReading_Response_t *pPldmResp)
{
  // for now just print the sensor data, for debug purpose
  dbgPrintSensorData(pPldmResp);
  // TODO: save this to /tmp/cache_store for sensor-util

  return 0;
}


void dbgPrintSensorThresh(PLDM_SensorThresh_Response_t *pThresh) {
  printf("Reading Numeric Sensor Threshold (PLDM type 2, Cmd 0x12)\n");
  printf("  data size: %d (%s)\n", pThresh->dataSize,
         numeric_state_to_name(pThresh->dataSize, sensor_data_size,
                               sizeof(sensor_data_size) /
                                   sizeof(sensor_data_size[0])));
  // interpret sensor threshold data based on size
  if (pThresh->dataSize <= 1) { // uint8/sint8
    PLDM_SensorThresh_int8_t *thresh = (void *)pThresh->rawdata;
    printf("  upper warning:  %d\n", thresh->upperWarning);
    printf("  upper critical: %d\n", thresh->upperCrit);
    printf("  upper fatal:    %d\n", thresh->upperFatal);
    printf("  lower warning:  %d\n", thresh->lowerWarning);
    printf("  lower critical: %d\n", thresh->lowerCrit);
    printf("  lower fatal:    %d\n", thresh->lowerFatal);
  } else if (pThresh->dataSize <= 3) { // uint16/sint16
    PLDM_SensorThresh_int16_t *thresh = (void *)pThresh->rawdata;
    printf("  upper warning:  %d\n", thresh->upperWarning);
    printf("  upper critical: %d\n", thresh->upperCrit);
    printf("  upper fatal:    %d\n", thresh->upperFatal);
    printf("  lower warning:  %d\n", thresh->lowerWarning);
    printf("  lower critical: %d\n", thresh->lowerCrit);
    printf("  lower fatal:    %d\n", thresh->lowerFatal);
  } else { // uint32/sint32
    PLDM_SensorThresh_int32_t *thresh = (void *)pThresh->rawdata;
    printf("  upper warning:  %d\n", thresh->upperWarning);
    printf("  upper critical: %d\n", thresh->upperCrit);
    printf("  upper fatal:    %d\n", thresh->upperFatal);
    printf("  lower warning:  %d\n", thresh->lowerWarning);
    printf("  lower critical: %d\n", thresh->lowerCrit);
    printf("  lower fatal:    %d\n", thresh->lowerFatal);
  }
}


// handle Cmd 0x012 (Get numeric sensor reading) response
int pldmHandleGetSensorThreshResp(PLDM_SensorThresh_Response_t *pPldmResp)
{
  // for now just print the sensor data, for debug purpose
  dbgPrintSensorThresh(pPldmResp);
  // TODO: save this to /tmp/cache_store for sensor-util

  return 0;
}


// returns "Present State" reading of a state sensor
int pldm_get_state_snr_val(PLDM_StateSensorReading_Response_t *pData)
{
  return pData->sensors[0].presentState;
}

void dbgPrintStateSensorData(PLDM_StateSensorReading_Response_t *pData)
{
  printf("Reading State Sensor (PLDM type 2, Cmd 0x21)\n");
  printf("  Operational State: %d (%s)\n", pData->sensors[0].opState,
          numeric_state_to_name(pData->sensors[0].opState, sensor_opState_str,
               sizeof(sensor_opState_str)/sizeof(sensor_opState_str[0])));
  printf("  Present State:     %d (%s)\n", pData->sensors[0].presentState,
          numeric_state_to_name(pData->sensors[0].presentState, sensor_state_str,
               sizeof(sensor_state_str)/sizeof(sensor_state_str[0])));
  printf("  Previous State:    %d (%s)\n", pData->sensors[0].prevState,
          numeric_state_to_name(pData->sensors[0].prevState, sensor_state_str,
               sizeof(sensor_state_str)/sizeof(sensor_state_str[0])));
  printf("  Event State:       %d (%s)\n", pData->sensors[0].eventState,
          numeric_state_to_name(pData->sensors[0].eventState, sensor_state_str,
               sizeof(sensor_state_str)/sizeof(sensor_state_str[0])));
}


// handle Cmd 0x021 (Get state sensor reading) response
int pldmHandleGetStateSensorReadingResp(PLDM_StateSensorReading_Response_t *pPldmResp)
{
  // for now just print the sensor data, for debug purpose
  dbgPrintStateSensorData(pPldmResp);
  // TODO: save this to /tmp/cache_store for sensor-util

  return 0;
}


// generic response handler for all Platform Monitoring & Control responses
int pldmHandlePmcResp(pldm_response *pResp)
{
  int pldmCmd = pResp->common[PLDM_CMD_OFFSET];
  int ret = 0;

  switch (pldmCmd) {
    case CMD_GET_SENSOR_READING:
      ret = pldmHandleGetSensorReadingResp((PLDM_SensorReading_Response_t *)&(pResp->common[PLDM_CC_OFFSET]));
      break;
    case CMD_GET_SENSOR_THRESHOLDS:
      ret = pldmHandleGetSensorThreshResp((PLDM_SensorThresh_Response_t *)&(pResp->common[PLDM_CC_OFFSET]));
      break;
    case CMD_GET_STATE_SENSOR_READINGS:
      ret = pldmHandleGetStateSensorReadingResp((PLDM_StateSensorReading_Response_t *)&(pResp->common[PLDM_CC_OFFSET]));
      break;
    default:
      ret = -1;
      break;
  }
  return ret;
}
