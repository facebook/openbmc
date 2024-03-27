#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <libpldm-oem/pldm.h>
#include "pal_acb_sensors.h"
#include "pal.h"
#include <openbmc/obmc-i2c.h>
#include "syslog.h"
#include <openbmc/kv.h>

static int
get_acb_sensor(uint8_t fru, uint8_t sensor_num, float *value) 
{
  return get_pldm_sensor(ACB_BIC_BUS, ACB_BIC_EID, sensor_num, value);
}

static int
get_accl_sensor(uint8_t fru, uint8_t sensor_num, float *value)
{
  uint8_t comp_id = 0;
  uint8_t asic_index = 0;

  switch (sensor_num) {
    case ACCL1_ASIC_1_TEMP:
    case ACCL1_ASIC_1_0V8_VOLT:
    case ACCL1_ASIC_1_1V05_VOLT:
    case ACCL1_ASIC_1_12V_AUX_VOLT:
      comp_id = CB_ACCL1_DEV1_COMP;
      break;
    case ACCL1_ASIC_2_TEMP:
    case ACCL1_ASIC_2_0V8_VOLT:
    case ACCL1_ASIC_2_1V05_VOLT:
    case ACCL1_ASIC_2_12V_AUX_VOLT:
      comp_id = CB_ACCL1_DEV2_COMP;
      break;
    case ACCL2_ASIC_1_TEMP:
    case ACCL2_ASIC_1_0V8_VOLT:
    case ACCL2_ASIC_1_1V05_VOLT:
    case ACCL2_ASIC_1_12V_AUX_VOLT:
      comp_id = CB_ACCL2_DEV1_COMP;
      break;
    case ACCL2_ASIC_2_TEMP:
    case ACCL2_ASIC_2_0V8_VOLT:
    case ACCL2_ASIC_2_1V05_VOLT:
    case ACCL2_ASIC_2_12V_AUX_VOLT:
      comp_id = CB_ACCL2_DEV2_COMP;
      break;
    case ACCL3_ASIC_1_TEMP:
    case ACCL3_ASIC_1_0V8_VOLT:
    case ACCL3_ASIC_1_1V05_VOLT:
    case ACCL3_ASIC_1_12V_AUX_VOLT:
      comp_id = CB_ACCL3_DEV1_COMP;
      break;
    case ACCL3_ASIC_2_TEMP:
    case ACCL3_ASIC_2_0V8_VOLT:
    case ACCL3_ASIC_2_1V05_VOLT:
    case ACCL3_ASIC_2_12V_AUX_VOLT:
      comp_id = CB_ACCL3_DEV2_COMP;
      break;
    case ACCL4_ASIC_1_TEMP:
    case ACCL4_ASIC_1_0V8_VOLT:
    case ACCL4_ASIC_1_1V05_VOLT:
    case ACCL4_ASIC_1_12V_AUX_VOLT:
      comp_id = CB_ACCL4_DEV1_COMP;
      break;
    case ACCL4_ASIC_2_TEMP:
    case ACCL4_ASIC_2_0V8_VOLT:
    case ACCL4_ASIC_2_1V05_VOLT:
    case ACCL4_ASIC_2_12V_AUX_VOLT:
      comp_id = CB_ACCL4_DEV2_COMP;
      break;
    case ACCL5_ASIC_1_TEMP:
    case ACCL5_ASIC_1_0V8_VOLT:
    case ACCL5_ASIC_1_1V05_VOLT:
    case ACCL5_ASIC_1_12V_AUX_VOLT:
      comp_id = CB_ACCL5_DEV1_COMP;
      break;
    case ACCL5_ASIC_2_TEMP:
    case ACCL5_ASIC_2_0V8_VOLT:
    case ACCL5_ASIC_2_1V05_VOLT:
    case ACCL5_ASIC_2_12V_AUX_VOLT:
      comp_id = CB_ACCL5_DEV2_COMP;
      break;
    case ACCL6_ASIC_1_TEMP:
    case ACCL6_ASIC_1_0V8_VOLT:
    case ACCL6_ASIC_1_1V05_VOLT:
    case ACCL6_ASIC_1_12V_AUX_VOLT:
      comp_id = CB_ACCL6_DEV1_COMP;
      break;
    case ACCL6_ASIC_2_TEMP:
    case ACCL6_ASIC_2_0V8_VOLT:
    case ACCL6_ASIC_2_1V05_VOLT:
    case ACCL6_ASIC_2_12V_AUX_VOLT:
      comp_id = CB_ACCL6_DEV2_COMP;
      break;
    case ACCL7_ASIC_1_TEMP:
    case ACCL7_ASIC_1_0V8_VOLT:
    case ACCL7_ASIC_1_1V05_VOLT:
    case ACCL7_ASIC_1_12V_AUX_VOLT:
      comp_id = CB_ACCL7_DEV1_COMP;
      break;
    case ACCL7_ASIC_2_TEMP:
    case ACCL7_ASIC_2_0V8_VOLT:
    case ACCL7_ASIC_2_1V05_VOLT:
    case ACCL7_ASIC_2_12V_AUX_VOLT:
      comp_id = CB_ACCL7_DEV2_COMP;
      break;
    case ACCL8_ASIC_1_TEMP:
    case ACCL8_ASIC_1_0V8_VOLT:
    case ACCL8_ASIC_1_1V05_VOLT:
    case ACCL8_ASIC_1_12V_AUX_VOLT:
      comp_id = CB_ACCL8_DEV1_COMP;
      break;
    case ACCL8_ASIC_2_TEMP:
    case ACCL8_ASIC_2_0V8_VOLT:
    case ACCL8_ASIC_2_1V05_VOLT:
    case ACCL8_ASIC_2_12V_AUX_VOLT:
      comp_id = CB_ACCL8_DEV2_COMP;
      break;
    case ACCL9_ASIC_1_TEMP:
    case ACCL9_ASIC_1_0V8_VOLT:
    case ACCL9_ASIC_1_1V05_VOLT:
    case ACCL9_ASIC_1_12V_AUX_VOLT:
      comp_id = CB_ACCL9_DEV1_COMP;
      break;
    case ACCL9_ASIC_2_TEMP:
    case ACCL9_ASIC_2_0V8_VOLT:
    case ACCL9_ASIC_2_1V05_VOLT:
    case ACCL9_ASIC_2_12V_AUX_VOLT:
      comp_id = CB_ACCL9_DEV2_COMP;
      break;
    case ACCL10_ASIC_1_TEMP:
    case ACCL10_ASIC_1_0V8_VOLT:
    case ACCL10_ASIC_1_1V05_VOLT:
    case ACCL10_ASIC_1_12V_AUX_VOLT:
      comp_id = CB_ACCL10_DEV1_COMP;
      break;
    case ACCL10_ASIC_2_TEMP:
    case ACCL10_ASIC_2_0V8_VOLT:
    case ACCL10_ASIC_2_1V05_VOLT:
    case ACCL10_ASIC_2_12V_AUX_VOLT:
      comp_id = CB_ACCL11_DEV2_COMP;
      break;
    case ACCL11_ASIC_1_TEMP:
    case ACCL11_ASIC_1_0V8_VOLT:
    case ACCL11_ASIC_1_1V05_VOLT:
    case ACCL11_ASIC_1_12V_AUX_VOLT:
      comp_id = CB_ACCL11_DEV1_COMP;
      break;
    case ACCL11_ASIC_2_TEMP:
    case ACCL11_ASIC_2_0V8_VOLT:
    case ACCL11_ASIC_2_1V05_VOLT:
    case ACCL11_ASIC_2_12V_AUX_VOLT:
      comp_id = CB_ACCL11_DEV2_COMP;
      break;
    case ACCL12_ASIC_1_TEMP:
    case ACCL12_ASIC_1_0V8_VOLT:
    case ACCL12_ASIC_1_1V05_VOLT:
    case ACCL12_ASIC_1_12V_AUX_VOLT:
      comp_id = CB_ACCL12_DEV1_COMP;
      break;
    case ACCL12_ASIC_2_TEMP:
    case ACCL12_ASIC_2_0V8_VOLT:
    case ACCL12_ASIC_2_1V05_VOLT:
    case ACCL12_ASIC_2_12V_AUX_VOLT:
      comp_id = CB_ACCL12_DEV2_COMP;
      break;
    default:
      return -1;
  }

  asic_index = comp_id - CB_ACCL1_DEV1_COMP;
  if (pal_is_asic_nvme_ready(asic_index) != true) {
    return -1;
  }

  return get_acb_sensor(fru, sensor_num, value);
}

PAL_SENSOR_MAP acb_artemis_sensor_map[] = {
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x00
  {"OUTLET_1_TEMP", 0, get_acb_sensor, true,  {85, 0, 150, 0, 0, 0, 0, 0}, TEMP}, // 0x01
  {"OUTLET_2_TEMP", 0, get_acb_sensor, true,  {85, 0, 150, 0, 0, 0, 0, 0}, TEMP}, // 0x02
  {"HSC1_TEMP",  0, get_acb_sensor, true,  {105, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x03
  {"HSC2_TEMP",   0, get_acb_sensor, true,  {105, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x04
  {"PEX0_TEMP",   0, get_acb_sensor, false, {105, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x05
  {"PEX1_TEMP",   0, get_acb_sensor, false, {105, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x06
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x07
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x08
  {"POWER_BRICK_1_TEMP", 0, get_acb_sensor, true, {115, 0, 0, 0, 0, 0, 0, 0}, TEMP}, // 0x09
  {"POWER_BRICK_2_TEMP", 0, get_acb_sensor, true, {115, 0, 0, 0, 0, 0, 0, 0}, TEMP}, // 0x0A
  {"P51V_AUX_L_VOLT", 0, get_acb_sensor, true, {52, 0, 0, 46, 0, 0, 0, 0}, VOLT}, // 0x0B
  {"P51V_AUX_R_VOLT", 0, get_acb_sensor, true, {52, 0, 0, 46, 0, 0, 0, 0}, VOLT}, // 0x0C
  {"P12V_AUX_1_VOLT", 0, get_acb_sensor, true, {13.2, 0, 0, 11.6, 0, 0, 0, 0}, VOLT}, // 0x0D
  {"P12V_AUX_2_VOLT", 0, get_acb_sensor, true, {13.2, 0, 0, 11.6, 0, 0, 0, 0}, VOLT}, // 0x0E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x0F
  {"P5V_AUX_VOLT",  0, get_acb_sensor, true,  {5.25,  0, 0,  4.75, 0, 0, 0, 0}, VOLT}, // 0x10
  {"P3V3_AUX_VOLT", 0, get_acb_sensor, true,  {3.531, 0, 0,  3.07, 0, 0, 0, 0}, VOLT}, // 0x11
  {"P1V2_AUX_VOLT", 0, get_acb_sensor, true,  {1.284, 0, 0, 1.116, 0, 0, 0, 0}, VOLT}, // 0x12
  {"P1V8_PEX_VOLT", 0, get_acb_sensor, false, {1.89,  0, 0, 1.71,  0, 0, 0, 0}, VOLT}, // 0x13
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x14
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x15
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x16
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x17
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x18
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x19
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x1A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x1B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x1C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x1D
  {"P1V8_1_VDD_VOLT",  0, get_acb_sensor, false, {1.89, 0, 0, 1.71, 0, 0, 0, 0}, VOLT}, // 0x1E
  {"P1V8_2_VDD_VOLT",  0, get_acb_sensor, false, {1.89, 0, 0, 1.71, 0, 0, 0, 0}, VOLT}, // 0x1F
  {"P1V25_1_VDD_VOLT", 0, get_acb_sensor, false, {1.313, 0, 0, 1.188, 0, 0, 0, 0}, VOLT}, // 0x20
  {"P1V25_2_VDD_VOLT", 0, get_acb_sensor, false, {1.313, 0, 0, 1.188, 0, 0, 0, 0}, VOLT}, // 0x21
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x22
  {"P12V_ACCL1_VOLT",  0, get_acb_sensor, false, {13.176, 0, 0, 11.224, 0, 0, 0, 0}, VOLT}, // 0x23
  {"P12V_ACCL2_VOLT",  0, get_acb_sensor, false, {13.176, 0, 0, 11.224, 0, 0, 0, 0}, VOLT}, // 0x24
  {"P12V_ACCL3_VOLT",  0, get_acb_sensor, false, {13.176, 0, 0, 11.224, 0, 0, 0, 0}, VOLT}, // 0x25
  {"P12V_ACCL4_VOLT",  0, get_acb_sensor, false, {13.176, 0, 0, 11.224, 0, 0, 0, 0}, VOLT}, // 0x26
  {"P12V_ACCL5_VOLT",  0, get_acb_sensor, false, {13.176, 0, 0, 11.224, 0, 0, 0, 0}, VOLT}, // 0x27
  {"P12V_ACCL6_VOLT",  0, get_acb_sensor, false, {13.176, 0, 0, 11.224, 0, 0, 0, 0}, VOLT}, // 0x28
  {"P12V_ACCL7_VOLT",  0, get_acb_sensor, false, {13.176, 0, 0, 11.224, 0, 0, 0, 0}, VOLT}, // 0x29
  {"P12V_ACCL8_VOLT",  0, get_acb_sensor, false, {13.176, 0, 0, 11.224, 0, 0, 0, 0}, VOLT}, // 0x2A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x2B
  {"P12V_ACCL9_VOLT",  0, get_acb_sensor, false, {13.176, 0, 0, 11.224, 0, 0, 0, 0}, VOLT}, // 0x2C
  {"P12V_ACCL10_VOLT", 0, get_acb_sensor, false, {13.176, 0, 0, 11.224, 0, 0, 0, 0}, VOLT}, // 0x2D
  {"P12V_ACCL11_VOLT", 0, get_acb_sensor, false, {13.176, 0, 0, 11.224, 0, 0, 0, 0}, VOLT}, // 0x2E
  {"P12V_ACCL12_VOLT", 0, get_acb_sensor, false, {13.176, 0, 0, 11.224, 0, 0, 0, 0}, VOLT}, // 0x2F
  //
  {"P51V_AUX_L_CURR", 0, get_acb_sensor, false, {35,  0,   0, 0, 0, 0, 0, 0}, CURR}, // 0x30
  {"P51V_AUX_R_CURR", 0, get_acb_sensor, false, {35,  0,   0, 0, 0, 0, 0, 0}, CURR}, // 0x31
  {"P12V_AUX_1_CURR", 0, get_acb_sensor, false, {160, 0,   0, 0, 0, 0, 0, 0}, CURR}, // 0x32
  {"P12V_AUX_2_CURR", 0, get_acb_sensor, false, {160, 0,   0, 0, 0, 0, 0, 0}, CURR}, // 0x33
  {"P12V_ACCL1_CURR", 0, get_acb_sensor, false, {22.56, 0, 28.20, 0, 0, 0, 0, 0}, CURR}, // 0x34
  {"P12V_ACCL2_CURR", 0, get_acb_sensor, false, {22.56, 0, 28.20, 0, 0, 0, 0, 0}, CURR}, // 0x35
  {"P12V_ACCL3_CURR", 0, get_acb_sensor, false, {22.56, 0, 28.20, 0, 0, 0, 0, 0}, CURR}, // 0x36
  {"P12V_ACCL4_CURR", 0, get_acb_sensor, false, {22.56, 0, 28.20, 0, 0, 0, 0, 0}, CURR}, // 0x37
  {"P12V_ACCL5_CURR", 0, get_acb_sensor, false, {22.56, 0, 28.20, 0, 0, 0, 0, 0}, CURR}, // 0x38
  {"P12V_ACCL6_CURR", 0, get_acb_sensor, false, {22.56, 0, 28.20, 0, 0, 0, 0, 0}, CURR},  // 0x39
  {"P12V_ACCL7_CURR", 0, get_acb_sensor, false, {22.56, 0, 28.20, 0, 0, 0, 0, 0}, CURR},  // 0x3A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x3B
  {"P12V_ACCL8_CURR",  0, get_acb_sensor, false, {22.56, 0, 28.20, 0, 0, 0, 0, 0}, CURR},  // 0x3C
  {"P12V_ACCL9_CURR",  0, get_acb_sensor, false, {22.56, 0, 28.20, 0, 0, 0, 0, 0}, CURR},  // 0x3D
  {"P12V_ACCL10_CURR", 0, get_acb_sensor, false, {22.56, 0, 28.20, 0, 0, 0, 0, 0}, CURR},  // 0x3E
  {"P12V_ACCL11_CURR", 0, get_acb_sensor, false, {22.56, 0, 28.20, 0, 0, 0, 0, 0}, CURR},  // 0x3F
  {"P12V_ACCL12_CURR", 0, get_acb_sensor, false, {22.56, 0, 28.20, 0, 0, 0, 0, 0}, CURR},  // 0x40
  //
  {"P51V_AUX_L_PWR", 0, get_acb_sensor, false, {1820, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x41
  {"P51V_AUX_R_PWR", 0, get_acb_sensor, false, {1820, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x42
  {"P12V_AUX_1_PWR", 0, get_acb_sensor, false, {1920, 0, 0, 0, 0, 0, 0}, POWER}, // 0x43
  {"P12V_AUX_2_PWR", 0, get_acb_sensor, false, {1920, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x44
  {"P12V_ACCL1_PWR", 0, get_acb_sensor, false, {275.24, 0, 344.04, 0, 0, 0, 0, 0}, POWER}, // 0x45
  {"P12V_ACCL2_PWR", 0, get_acb_sensor, false, {275.24, 0, 344.04, 0, 0, 0, 0, 0}, POWER}, // 0x46
  {"P12V_ACCL3_PWR", 0, get_acb_sensor, false, {275.24, 0, 344.04, 0, 0, 0, 0, 0}, POWER}, // 0x47
  {"P12V_ACCL4_PWR", 0, get_acb_sensor, false, {275.24, 0, 344.04, 0, 0, 0, 0, 0}, POWER}, // 0x48
  {"P12V_ACCL5_PWR", 0, get_acb_sensor, false, {275.24, 0, 344.04, 0, 0, 0, 0, 0}, POWER}, // 0x49
  {"P12V_ACCL6_PWR", 0, get_acb_sensor, false, {275.24, 0, 344.04, 0, 0, 0, 0, 0}, POWER}, // 0x4A
  {"P12V_ACCL7_PWR", 0, get_acb_sensor, false, {275.24, 0, 344.04, 0, 0, 0, 0, 0}, POWER}, // 0x4B
  {"P12V_ACCL8_PWR", 0, get_acb_sensor, false, {275.24, 0, 344.04, 0, 0, 0, 0, 0}, POWER}, // 0x4C
  {"P12V_ACCL9_PWR", 0, get_acb_sensor, false, {275.24, 0, 344.04, 0, 0, 0, 0, 0}, POWER}, // 0x4D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x4E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x4F
  {"P12V_ACCL10_PWR", 0, get_acb_sensor, false, {275.24, 0, 344.04, 0, 0, 0, 0, 0}, POWER}, // 0x50
  {"P12V_ACCL11_PWR", 0, get_acb_sensor, false, {275.24, 0, 344.04, 0, 0, 0, 0, 0}, POWER}, // 0x51
  {"P12V_ACCL12_PWR", 0, get_acb_sensor, false, {275.24, 0, 344.04, 0, 0, 0, 0, 0}, POWER}, // 0x52
  //
  {"P51V_STBY_L_VOLT", 0, get_acb_sensor, true,  {52, 0, 0, 46, 0, 0, 0, 0},  VOLT}, // 0x53
  {"P51V_STBY_R_VOLT", 0, get_acb_sensor, true,  {52, 0, 0, 46, 0, 0, 0, 0},  VOLT}, // 0x54
  {"P51V_STBY_L_PWR", 0, get_acb_sensor, true,  {1838.55, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x55
  {"P51V_STBY_R_PWR", 0, get_acb_sensor, true,  {1838.55, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x56
  {"P0V8_VDD_1_VOLT",  0, get_acb_sensor, false, {0.84, 0, 0, 0.76, 0, 0, 0, 0},  VOLT}, // 0x57
  {"P0V8_VDD_2_VOLT",  0, get_acb_sensor, false, {0.84, 0, 0, 0.76, 0, 0, 0, 0},  VOLT}, // 0x58
  {"P0V8_VDD_1_CURR",  0, get_acb_sensor, false, {47, 0, 57, 0, 0, 0, 0, 0},  CURR}, // 0x59
  {"P0V8_VDD_2_CURR",  0, get_acb_sensor, false, {47, 0, 57, 0, 0, 0, 0, 0},  CURR}, // 0x5A
  {"P0V8_VDD_1_PWR",  0, get_acb_sensor, false, {38.728, 0, 47.88, 0, 0, 0, 0, 0},  POWER}, // 0x5B
  {"P0V8_VDD_2_PWR",  0, get_acb_sensor, false, {38.728, 0, 47.88, 0, 0, 0, 0, 0},  POWER}, // 0x5C
  {"P51V_STBY_L_CURR", 0, get_acb_sensor, true,  {35, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x5D
  {"P51V_STBY_R_CURR", 0, get_acb_sensor, true,  {35, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x5E
  {"P0V8_1_VDD_TEMP", 0, get_acb_sensor, true,  {100, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x5F
  {"P0V8_2_VDD_TEMP", 0, get_acb_sensor, true,  {100, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x60
  {"P1V25_1_12VIN_VOLT", 0, get_acb_sensor, false, {13.2, 0, 0, 11.6, 0, 0, 0, 0}, VOLT}, // 0x61
  {"P1V25_2_12VIN_VOLT", 0, get_acb_sensor, false, {13.2, 0, 0, 11.6, 0, 0, 0, 0}, VOLT}, // 0x62
  {"P1V25_1_12VIN_CURR", 0, get_acb_sensor, false, {2.48, 0, 2.92, 0, 0, 0, 0, 0}, CURR}, // 0x63
  {"P1V25_2_12VIN_CURR", 0, get_acb_sensor, false, {2.48, 0, 2.92, 0, 0, 0, 0, 0}, CURR}, // 0x64
  {"P1V25_1_12VIN_PWR", 0, get_acb_sensor, false, {32.736, 0, 41.8436, 0, 0, 0, 0, 0}, POWER}, // 0x65
  {"P1V25_2_12VIN_PWR", 0, get_acb_sensor, false, {32.736, 0, 41.8436, 0, 0, 0, 0, 0}, POWER}, // 0x66
  {"INLET_TEMP", 0, get_acb_sensor, false, {55, 0, 0, 0, 0, 0, 0, 0}, TEMP}, // 0x67
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x68
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x69
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x6A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x6B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x6C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x6D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x6E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x6F
  {"ACCL1_ASIC_1_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0x70
  {"ACCL1_ASIC_1_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0x71
  {"ACCL1_ASIC_1_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0x72
  {"ACCL1_ASIC_2_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0x73
  {"ACCL1_ASIC_2_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0x74
  {"ACCL1_ASIC_2_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0x75
  {"ACCL2_ASIC_1_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0x76
  {"ACCL2_ASIC_1_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0x77
  {"ACCL2_ASIC_1_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0x78
  {"ACCL2_ASIC_2_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0x79
  {"ACCL2_ASIC_2_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0x7A
  {"ACCL2_ASIC_2_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0x7B
  {"ACCL3_ASIC_1_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0x7C
  {"ACCL3_ASIC_1_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0x7D
  {"ACCL3_ASIC_1_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0x7E
  {"ACCL3_ASIC_2_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0x7F
  {"ACCL3_ASIC_2_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0x80
  {"ACCL3_ASIC_2_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0x81
  {"ACCL4_ASIC_1_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0x82
  {"ACCL4_ASIC_1_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0x83
  {"ACCL4_ASIC_1_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0x84
  {"ACCL4_ASIC_2_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0x85
  {"ACCL4_ASIC_2_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0x86
  {"ACCL4_ASIC_2_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0x87
  {"ACCL5_ASIC_1_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0x88
  {"ACCL5_ASIC_1_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0x89
  {"ACCL5_ASIC_1_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0x8A
  {"ACCL5_ASIC_2_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0x8B
  {"ACCL5_ASIC_2_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0x8C
  {"ACCL5_ASIC_2_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0x8D
  {"ACCL6_ASIC_1_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0x8E
  {"ACCL6_ASIC_1_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0x8F
  {"ACCL6_ASIC_1_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0x90
  {"ACCL6_ASIC_2_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0x91
  {"ACCL6_ASIC_2_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0x92
  {"ACCL6_ASIC_2_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0x93
  {"ACCL7_ASIC_1_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0x94
  {"ACCL7_ASIC_1_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0x95
  {"ACCL7_ASIC_1_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0x96
  {"ACCL7_ASIC_2_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0x97
  {"ACCL7_ASIC_2_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0x98
  {"ACCL7_ASIC_2_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0x99
  {"ACCL8_ASIC_1_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0x9A
  {"ACCL8_ASIC_1_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0x9B
  {"ACCL8_ASIC_1_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0x9C
  {"ACCL8_ASIC_2_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0x9D
  {"ACCL8_ASIC_2_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0x9E
  {"ACCL8_ASIC_2_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0x9F
  {"ACCL9_ASIC_1_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0xA0
  {"ACCL9_ASIC_1_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0xA1
  {"ACCL9_ASIC_1_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0xA2
  {"ACCL9_ASIC_2_TEMP",   0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0xA3
  {"ACCL9_ASIC_2_0V8_VOLT",  0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0xA4
  {"ACCL9_ASIC_2_1V05_VOLT",  0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0xA5
  {"ACCL10_ASIC_1_TEMP",  0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0xA6
  {"ACCL10_ASIC_1_0V8_VOLT", 0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0xA7
  {"ACCL10_ASIC_1_1V05_VOLT", 0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0xA8
  {"ACCL10_ASIC_2_TEMP",  0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0xA9
  {"ACCL10_ASIC_2_0V8_VOLT", 0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0xAA
  {"ACCL10_ASIC_2_1V05_VOLT", 0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0xAB
  {"ACCL11_ASIC_1_TEMP",  0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0xAC
  {"ACCL11_ASIC_1_0V8_VOLT", 0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0xAD
  {"ACCL11_ASIC_1_1V05_VOLT", 0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0xAE
  {"ACCL11_ASIC_2_TEMP",  0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0xAF
  {"ACCL11_ASIC_2_0V8_VOLT", 0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0xB0
  {"ACCL11_ASIC_2_1V05_VOLT", 0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0xB1
  {"ACCL12_ASIC_1_TEMP",  0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0xB2
  {"ACCL12_ASIC_1_0V8_VOLT", 0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0xB3
  {"ACCL12_ASIC_1_1V05_VOLT", 0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0xB4
  {"ACCL12_ASIC_2_TEMP",  0, get_accl_sensor, false, {90, 0, 0, 10, 0, 0, 0, 0}, TEMP},    // 0xB5
  {"ACCL12_ASIC_2_0V8_VOLT", 0, get_accl_sensor, false, {0.93, 0.9, 0, 0.76, 0.81, 0, 0, 0}, VOLT}, // 0xB6
  {"ACCL12_ASIC_2_1V05_VOLT", 0, get_accl_sensor, false, {1.12, 0, 0, 1.01, 0, 0, 0, 0}, VOLT}, // 0xB7
  {"ACCL1_ASIC_1_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xB8
  {"ACCL1_ASIC_2_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xB9
  {"ACCL2_ASIC_1_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xBA
  {"ACCL2_ASIC_2_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xBB
  {"ACCL3_ASIC_1_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xBC
  {"ACCL3_ASIC_2_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xBD
  {"ACCL4_ASIC_1_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xBE
  {"ACCL4_ASIC_2_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xBF
  {"ACCL5_ASIC_1_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xC0
  {"ACCL5_ASIC_2_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xC1
  {"ACCL6_ASIC_1_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xC2
  {"ACCL6_ASIC_2_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xC3
  {"ACCL7_ASIC_1_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xC4
  {"ACCL7_ASIC_2_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xC5
  {"ACCL8_ASIC_1_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xC6
  {"ACCL8_ASIC_2_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xC7
  {"ACCL9_ASIC_1_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xC8
  {"ACCL9_ASIC_2_12V_AUX_VOLT",  0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xC9
  {"ACCL10_ASIC_1_12V_AUX_VOLT", 0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xCA
  {"ACCL10_ASIC_2_12V_AUX_VOLT", 0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xCB
  {"ACCL11_ASIC_1_12V_AUX_VOLT", 0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xCC
  {"ACCL11_ASIC_2_12V_AUX_VOLT", 0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xCD
  {"ACCL12_ASIC_1_12V_AUX_VOLT", 0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xCE
  {"ACCL12_ASIC_2_12V_AUX_VOLT", 0, get_accl_sensor, false, {12.6, 0, 0, 11.4, 0, 0, 0, 0}, VOLT}, // 0xCF
};


const uint8_t acb_artemis_sensor_list[] = {
  ACB_SENSOR_INLET_TEMP,
  ACB_SENSOR_OUTLET_1_TEMP,
  ACB_SENSOR_OUTLET_2_TEMP,
  ACB_SENSOR_HSC_1_TEMP,
  ACB_SENSOR_HSC_2_TEMP,
  ACB_SENSOR_PEX0_TEMP,
  ACB_SENSOR_PEX1_TEMP,
  ACB_SENSOR_POWER_BRICK_1_TEMP,
  ACB_SENSOR_POWER_BRICK_2_TEMP,
  ACB_SENSOR_P51V_AUX_L_VOL,
  ACB_SENSOR_P51V_AUX_R_VOL,
  ACB_SENSOR_P12V_AUX_1_VOL,
  ACB_SENSOR_P12V_AUX_2_VOL,
  ACB_SENSOR_P5V_AUX_VOL,
  ACB_SENSOR_P3V3_AUX_VOL,
  ACB_SENSOR_P1V2_AUX_VOL,
  ACB_SENSOR_P1V8_PEX_VOL,
  ACB_SENSOR_P1V8_1_VDD_VOL,
  ACB_SENSOR_P1V8_2_VDD_VOL,

  ACB_SENSOR_P1V25_1_VDD_VOL,
  ACB_SENSOR_P1V25_2_VDD_VOL,
  ACB_SENSOR_P12V_ACCL1_VOL,
  ACB_SENSOR_P12V_ACCL2_VOL,
  ACB_SENSOR_P12V_ACCL3_VOL,
  ACB_SENSOR_P12V_ACCL4_VOL,
  ACB_SENSOR_P12V_ACCL5_VOL,
  ACB_SENSOR_P12V_ACCL6_VOL,
  ACB_SENSOR_P12V_ACCL7_VOL,
  ACB_SENSOR_P12V_ACCL8_VOL,
  ACB_SENSOR_P12V_ACCL9_VOL,
  ACB_SENSOR_P12V_ACCL10_VOL,
  ACB_SENSOR_P12V_ACCL11_VOL,
  ACB_SENSOR_P12V_ACCL12_VOL,

  ACB_SENSOR_P51V_AUX_L_CUR,
  ACB_SENSOR_P51V_AUX_R_CUR,
  ACB_SENSOR_P12V_AUX_1_CUR,
  ACB_SENSOR_P12V_AUX_2_CUR,
  ACB_SENSOR_P12V_ACCL1_CUR,
  ACB_SENSOR_P12V_ACCL2_CUR,
  ACB_SENSOR_P12V_ACCL3_CUR,
  ACB_SENSOR_P12V_ACCL4_CUR,
  ACB_SENSOR_P12V_ACCL5_CUR,
  ACB_SENSOR_P12V_ACCL6_CUR,
  ACB_SENSOR_P12V_ACCL7_CUR,
  ACB_SENSOR_P12V_ACCL8_CUR,
  ACB_SENSOR_P12V_ACCL9_CUR,
  ACB_SENSOR_P12V_ACCL10_CUR,
  ACB_SENSOR_P12V_ACCL11_CUR,
  ACB_SENSOR_P12V_ACCL12_CUR,

  ACB_SENSOR_P51V_AUX_L_PWR,
  ACB_SENSOR_P51V_AUX_R_PWR,
  ACB_SENSOR_P12V_AUX_1_PWR,
  ACB_SENSOR_P12V_AUX_2_PWR,
  ACB_SENSOR_P12V_ACCL1_PWR,
  ACB_SENSOR_P12V_ACCL2_PWR,
  ACB_SENSOR_P12V_ACCL3_PWR,
  ACB_SENSOR_P12V_ACCL4_PWR,
  ACB_SENSOR_P12V_ACCL5_PWR,
  ACB_SENSOR_P12V_ACCL6_PWR,
  ACB_SENSOR_P12V_ACCL7_PWR,
  ACB_SENSOR_P12V_ACCL8_PWR,
  ACB_SENSOR_P12V_ACCL9_PWR,
  ACB_SENSOR_P12V_ACCL10_PWR,
  ACB_SENSOR_P12V_ACCL11_PWR,
  ACB_SENSOR_P12V_ACCL12_PWR,

  ACB_SENSOR_P51V_STBY_L_VOL,
  ACB_SENSOR_P51V_STBY_R_VOL,
  ACB_SENSOR_P51V_STBY_L_PWR,
  ACB_SENSOR_P51V_STBY_R_PWR,
  ACB_SENSOR_P0V8_VDD_1_VOL,
  ACB_SENSOR_P0V8_VDD_2_VOL,
  ACB_SENSOR_P0V8_VDD_1_CUR,
  ACB_SENSOR_P0V8_VDD_2_CUR,
  ACB_SENSOR_P0V8_VDD_1_PWR,
  ACB_SENSOR_P0V8_VDD_2_PWR,
  ACB_SENSOR_P51V_STBY_L_CUR,
  ACB_SENSOR_P51V_STBY_R_CUR,
  ACB_SENSOR_P0V8_1_VDD_TEMP,
  ACB_SENSOR_P0V8_2_VDD_TEMP,
  ACB_SENSOR_P1V25_1_12VIN_VOLT,
  ACB_SENSOR_P1V25_2_12VIN_VOLT,
  ACB_SENSOR_P1V25_1_12VIN_CURR,
  ACB_SENSOR_P1V25_2_12VIN_CURR,
  ACB_SENSOR_P1V25_1_12VIN_PWR,
  ACB_SENSOR_P1V25_2_12VIN_PWR,

  ACCL1_ASIC_1_TEMP,
  ACCL1_ASIC_1_0V8_VOLT,
  ACCL1_ASIC_1_1V05_VOLT,
  ACCL1_ASIC_1_12V_AUX_VOLT,
  ACCL1_ASIC_2_TEMP,
  ACCL1_ASIC_2_0V8_VOLT,
  ACCL1_ASIC_2_1V05_VOLT,
  ACCL1_ASIC_2_12V_AUX_VOLT,
  ACCL2_ASIC_1_TEMP,
  ACCL2_ASIC_1_0V8_VOLT,
  ACCL2_ASIC_1_1V05_VOLT,
  ACCL2_ASIC_1_12V_AUX_VOLT,
  ACCL2_ASIC_2_TEMP,
  ACCL2_ASIC_2_0V8_VOLT,
  ACCL2_ASIC_2_1V05_VOLT,
  ACCL2_ASIC_2_12V_AUX_VOLT,
  ACCL3_ASIC_1_TEMP,
  ACCL3_ASIC_1_0V8_VOLT,
  ACCL3_ASIC_1_1V05_VOLT,
  ACCL3_ASIC_1_12V_AUX_VOLT,
  ACCL3_ASIC_2_TEMP,
  ACCL3_ASIC_2_0V8_VOLT,
  ACCL3_ASIC_2_1V05_VOLT,
  ACCL3_ASIC_2_12V_AUX_VOLT,
  ACCL4_ASIC_1_TEMP,
  ACCL4_ASIC_1_0V8_VOLT,
  ACCL4_ASIC_1_1V05_VOLT,
  ACCL4_ASIC_1_12V_AUX_VOLT,
  ACCL4_ASIC_2_TEMP,
  ACCL4_ASIC_2_0V8_VOLT,
  ACCL4_ASIC_2_1V05_VOLT,
  ACCL4_ASIC_2_12V_AUX_VOLT,
  ACCL5_ASIC_1_TEMP,
  ACCL5_ASIC_1_0V8_VOLT,
  ACCL5_ASIC_1_1V05_VOLT,
  ACCL5_ASIC_1_12V_AUX_VOLT,
  ACCL5_ASIC_2_TEMP,
  ACCL5_ASIC_2_0V8_VOLT,
  ACCL5_ASIC_2_1V05_VOLT,
  ACCL5_ASIC_2_12V_AUX_VOLT,
  ACCL6_ASIC_1_TEMP,
  ACCL6_ASIC_1_0V8_VOLT,
  ACCL6_ASIC_1_1V05_VOLT,
  ACCL6_ASIC_1_12V_AUX_VOLT,
  ACCL6_ASIC_2_TEMP,
  ACCL6_ASIC_2_0V8_VOLT,
  ACCL6_ASIC_2_1V05_VOLT,
  ACCL6_ASIC_2_12V_AUX_VOLT,
  ACCL7_ASIC_1_TEMP,
  ACCL7_ASIC_1_0V8_VOLT,
  ACCL7_ASIC_1_1V05_VOLT,
  ACCL7_ASIC_1_12V_AUX_VOLT,
  ACCL7_ASIC_2_TEMP,
  ACCL7_ASIC_2_0V8_VOLT,
  ACCL7_ASIC_2_1V05_VOLT,
  ACCL7_ASIC_2_12V_AUX_VOLT,
  ACCL8_ASIC_1_TEMP,
  ACCL8_ASIC_1_0V8_VOLT,
  ACCL8_ASIC_1_1V05_VOLT,
  ACCL8_ASIC_1_12V_AUX_VOLT,
  ACCL8_ASIC_2_TEMP,
  ACCL8_ASIC_2_0V8_VOLT,
  ACCL8_ASIC_2_1V05_VOLT,
  ACCL8_ASIC_2_12V_AUX_VOLT,
  ACCL9_ASIC_1_TEMP,
  ACCL9_ASIC_1_0V8_VOLT,
  ACCL9_ASIC_1_1V05_VOLT,
  ACCL9_ASIC_1_12V_AUX_VOLT,
  ACCL9_ASIC_2_TEMP,
  ACCL9_ASIC_2_0V8_VOLT,
  ACCL9_ASIC_2_1V05_VOLT,
  ACCL9_ASIC_2_12V_AUX_VOLT,
  ACCL10_ASIC_1_TEMP,
  ACCL10_ASIC_1_0V8_VOLT,
  ACCL10_ASIC_1_1V05_VOLT,
  ACCL10_ASIC_1_12V_AUX_VOLT,
  ACCL10_ASIC_2_TEMP,
  ACCL10_ASIC_2_0V8_VOLT,
  ACCL10_ASIC_2_1V05_VOLT,
  ACCL10_ASIC_2_12V_AUX_VOLT,
  ACCL11_ASIC_1_TEMP,
  ACCL11_ASIC_1_0V8_VOLT,
  ACCL11_ASIC_1_1V05_VOLT,
  ACCL11_ASIC_1_12V_AUX_VOLT,
  ACCL11_ASIC_2_TEMP,
  ACCL11_ASIC_2_0V8_VOLT,
  ACCL11_ASIC_2_1V05_VOLT,
  ACCL11_ASIC_2_12V_AUX_VOLT,
  ACCL12_ASIC_1_TEMP,
  ACCL12_ASIC_1_0V8_VOLT,
  ACCL12_ASIC_1_1V05_VOLT,
  ACCL12_ASIC_1_12V_AUX_VOLT,
  ACCL12_ASIC_2_TEMP,
  ACCL12_ASIC_2_0V8_VOLT,
  ACCL12_ASIC_2_1V05_VOLT,
  ACCL12_ASIC_2_12V_AUX_VOLT,
};

size_t acb_artemis_sensor_cnt = sizeof(acb_artemis_sensor_list)/sizeof(uint8_t);
