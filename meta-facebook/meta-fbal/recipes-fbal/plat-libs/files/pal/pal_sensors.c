#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <string.h>
#include <ctype.h>
#include <openbmc/kv.h>
#include "pal.h"
#include "pal_sensors.h"

const uint8_t mb_sensor_list[] = {
  MB_SENSOR_INLET_TEMP,
  MB_SENSOR_OUTLET_TEMP,
};

// List of MB discrete sensors to be monitored
const uint8_t mb_discrete_sensor_list[] = {
  MB_SENSOR_POWER_FAIL,
  MB_SENSOR_MEMORY_LOOP_FAIL,
  MB_SENSOR_PROCESSOR_FAIL,
};

//{SensorNumber, SensorName, {UCR, UNR, UNC, LCR, LNR, LNC, Pos, Neg }}
PAL_SENSOR_MAP mb_sensor_map[] = {
  {0x00, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x01, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x02, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x03, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x04, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x05, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x06, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x07, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x08, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x09, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x0A, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x0B, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x0C, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x0D, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x0E, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x0F, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x10, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x11, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x12, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x13, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x14, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x15, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x16, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x17, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x18, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x19, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x1A, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x1B, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x1C, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x1D, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x1E, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x1F, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x20, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x21, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x22, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x23, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x24, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x25, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x26, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x27, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x28, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x29, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x2A, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x2B, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x2C, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x2D, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x2E, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x2F, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x30, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x31, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x32, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x33, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x34, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x35, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x36, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x37, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x38, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x39, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x3A, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x3B, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x3C, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x3D, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x3E, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x3F, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x40, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x41, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x42, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x43, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x44, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x45, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x46, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x47, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x48, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x49, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x4A, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x4B, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x4C, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x4D, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x4E, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x4F, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x50, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x51, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x52, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x53, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x54, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x55, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x56, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x57, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x58, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x59, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x5A, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x5B, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x5C, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x5D, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x5E, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x5F, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x60, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x61, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x62, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x63, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x64, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x65, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x66, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x67, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x68, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x69, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x6A, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x6B, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x6C, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x6D, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x6E, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x6F, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x70, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x71, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x72, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x73, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x74, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x75, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x76, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x77, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x78, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x79, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x7A, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x7B, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x7C, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x7D, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x7E, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x7F, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x80, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x81, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x82, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x83, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x84, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x85, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x86, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x87, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x88, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x89, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x8A, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x8B, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x8C, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x8D, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x8E, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x8F, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x90, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x91, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x92, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x93, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x94, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x95, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x96, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x97, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x98, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x99, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x9A, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x9B, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x9C, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x9D, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x9E, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0x9F, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {MB_SENSOR_INLET_TEMP , "MB_INLET_TEMP", {40, 0, 0, 20, 0, 0, 0, 0} },
  {MB_SENSOR_OUTLET_TEMP , "MB_OUTLET_TEMP", {80, 0, 0, 20, 0, 0, 0, 0} },
  {0xA2, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xA3, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xA4, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xA5, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xA6, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xA7, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xA8, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xA9, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xAA, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xAB, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xAC, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xAD, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xAE, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xAF, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xB0, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xB1, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xB2, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xB3, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xB4, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xB5, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xB6, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xB7, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xB8, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xB9, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xBA, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xBB, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xBC, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xBD, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xBE, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xBF, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xC0, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xC1, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xC2, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xC3, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xC4, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xC5, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xC6, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xC7, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xC8, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xC9, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xCA, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xCB, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xCC, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xCD, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xCE, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xCF, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xD0, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xD1, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xD2, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xD3, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xD4, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xD5, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xD6, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xD7, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xD8, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xD9, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xDA, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xDB, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xDC, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xDD, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xDE, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xDF, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xE0, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xE1, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xE2, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xE3, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xE4, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xE5, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xE6, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xE7, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xE8, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xE9, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xEA, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xEB, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xEC, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xED, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xEE, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xEF, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xF0, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xF1, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xF2, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xF3, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xF4, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xF5, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xF6, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xF7, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xF8, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xF9, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xFA, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xFB, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xFC, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xFD, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xFE, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
  {0xFF, NULL, {0, 0, 0, 0, 0, 0, 0, 0} },
};

size_t mb_sensor_cnt = sizeof(mb_sensor_list)/sizeof(uint8_t);
size_t mb_discrete_sensor_cnt = sizeof(mb_discrete_sensor_list)/sizeof(uint8_t);

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  switch(fru) {
  case FRU_MB:
    *sensor_list = (uint8_t *) mb_sensor_list;
    *cnt = mb_sensor_cnt;
    break;
  default:
    if (fru > MAX_NUM_FRUS)
      return -1;
    // Nothing to read yet.
    *sensor_list = NULL;
    *cnt = 0;
  }
  return 0;
}


int
pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  switch(fru) {
  case FRU_MB:
    *sensor_list = (uint8_t *) mb_discrete_sensor_list;
    *cnt = mb_discrete_sensor_cnt;
    break;
  default:
    if (fru > MAX_NUM_FRUS)
      return -1;
    // Nothing to read yet.
    *sensor_list = NULL;
    *cnt = 0;
  }
    return 0;
}

static int
read_temp_attr(uint8_t sensor_num, const char *device, const char *attr, float *value) {
  char full_name[LARGEST_DEVICE_NAME + 1];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  int tmp;
  FILE *fp;
  int size;
  static unsigned int retry[4] = {0};
  uint8_t i_retry = -1;
  float thresh;

  switch(sensor_num) {
    case MB_SENSOR_INLET_TEMP:
      i_retry = 0; break;
    case MB_SENSOR_OUTLET_TEMP:
      i_retry = 1; break;
    case MB_SENSOR_INLET_REMOTE_TEMP:
      i_retry = 2; break;
    case MB_SENSOR_OUTLET_REMOTE_TEMP:
      i_retry = 3; break;
    default:
      break;
  }

  // Get current working directory
  snprintf(full_name, LARGEST_DEVICE_NAME, "cd %s;pwd", device);
 
  fp = popen(full_name, "r");
  fgets(dir_name, LARGEST_DEVICE_NAME, fp);
  pclose(fp);

  // Remove the newline character at the end
  size = strlen(dir_name);
  dir_name[size-1] = '\0';

  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", dir_name, attr);

  if (read_device(full_name, &tmp)) {
    return -1;
  }

  *value = ((float)tmp)/UNIT_DIV;

  if( i_retry != -1) {
    thresh = mb_sensor_map[sensor_num].snr_thresh.ucr_thresh;

    if( ( *value > thresh  || *value < 0 ) && retry[i_retry] < 5) {
      retry[i_retry]++;
      return READING_SKIP;
    } else {
      retry[i_retry] = 0;
    }
  }
  return 0;
}

int
read_temp_attr1(int sensor_num, const char *device, float *value) {
  return read_temp_attr(sensor_num, device, "temp1_input", value);
}

int
read_temp_attr2(int sensor_num, const char *device, float *value) {
  return read_temp_attr(sensor_num, device, "temp2_input", value);
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char fru_name[32];
  int ret=0;
  static uint8_t poweron_10s_flag = 0;
  bool server_off;

  pal_get_fru_name(fru, fru_name);
  sprintf(key, "%s_sensor%d", fru_name, sensor_num);
  server_off = is_server_off();

  switch(fru) {
  case FRU_MB:
    switch(sensor_num) {
    //Temp. Sensors
    case MB_SENSOR_INLET_TEMP:
      ret = read_temp_attr1(MB_SENSOR_INLET_TEMP, MB_INLET_TEMP_DEVICE, (float*) value);
      break;
    case MB_SENSOR_OUTLET_TEMP:
      ret = read_temp_attr1(MB_SENSOR_OUTLET_TEMP, MB_OUTLET_TEMP_DEVICE, (float*) value);
      break;
    default:
      //Power On     
      if (!server_off) {
        if((poweron_10s_flag < 5) && ((sensor_num == MB_SENSOR_HSC_IN_VOLT) ||
           (sensor_num == MB_SENSOR_HSC_OUT_CURR) || (sensor_num == MB_SENSOR_HSC_IN_POWER) ||
           (sensor_num == MB_SENSOR_HSC_TEMP) ||
           (sensor_num == MB_SENSOR_FAN0_TACH) || (sensor_num == MB_SENSOR_FAN1_TACH))) {
  
           if(sensor_num == MB_SENSOR_HSC_IN_POWER){
             poweron_10s_flag++;
           }
  
           ret = READING_NA;
           break;
        }
       
        switch(sensor_num) {
        default:
          ret = READING_NA;
          break;
        } 
      } else {
        ret = READING_NA;
      }
      break;
    }
    if (is_server_off() != server_off) {
      /* server power status changed while we were reading the sensor.
       * this sensor is potentially NA. */
      return pal_sensor_read_raw(fru, sensor_num, value);
    }
    break;

  case FRU_NIC0:
    break;
      
  default:
    return -1;
  }

  if (ret) {
    if (ret == READING_NA || ret == -1) {
      strcpy(str, "NA");
    } else {
      return ret;
    }
  } else {
    sprintf(str, "%.2f",*((float*)value));
  }
  if(kv_set(key, str, 0, 0) < 0) {
    syslog(LOG_WARNING, "pal_sensor_read_raw: cache_set key = %s, str = %s failed.", key, str);
    return -1;
  } else {
    return ret;
  }

  return 0;
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  switch(fru) {
  case FRU_MB:
    sprintf(name, mb_sensor_map[sensor_num].snr_name);
    break;
    
  default:
    return -1;
  }
  return 0;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  float *val = (float*) value;
  switch(fru) {
  case FRU_MB:
    switch(thresh) {
    case UCR_THRESH:
      *val = mb_sensor_map[sensor_num].snr_thresh.ucr_thresh;
      break;  
    case UNC_THRESH:
      *val = mb_sensor_map[sensor_num].snr_thresh.unc_thresh;
      break;
    case UNR_THRESH:
      *val = mb_sensor_map[sensor_num].snr_thresh.unr_thresh;
      break;
    case LCR_THRESH:
      *val = mb_sensor_map[sensor_num].snr_thresh.lcr_thresh;
      break;
    case LNC_THRESH:
      *val = mb_sensor_map[sensor_num].snr_thresh.lnc_thresh;
      break;
    case LNR_THRESH:
      *val = mb_sensor_map[sensor_num].snr_thresh.lnr_thresh;
      break;
    case POS_HYST:
      *val = mb_sensor_map[sensor_num].snr_thresh.pos_hyst;
      break;
    case NEG_HYST:
      *val = mb_sensor_map[sensor_num].snr_thresh.neg_hyst;
      break;
    default:
      syslog(LOG_WARNING, "Threshold type error value=%d\n", thresh);
      return -1;
    }    
    break;
  default:
    return -1;
  }
  return 0;
}

