#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <libpldm-oem/pldm.h>
#include "pal_acb_sensors.h"
#include "pal.h"
#include <openbmc/obmc-i2c.h>
#include "syslog.h"

static int
get_acb_sensor(uint8_t fru, uint8_t sensor_num, float *value) 
{
  uint8_t tbuf[MAX_TXBUF_SIZE] = {0};
  uint8_t* rbuf = (uint8_t *) NULL;
  struct pldm_snr_reading_t* resp;
  uint8_t tlen = 0;
  size_t  rlen = 0;
  int16_t integer=0;
  float decimal=0;
  int rc;

  struct pldm_msg* pldmbuf = (struct pldm_msg *)tbuf;
  pldmbuf->hdr.request = 1;
  pldmbuf->hdr.type    = PLDM_PLATFORM;
  pldmbuf->hdr.command = PLDM_GET_SENSOR_READING;
  tlen = PLDM_HEADER_SIZE;
  tbuf[tlen++] = sensor_num;
  tbuf[tlen++] = 0;
  tbuf[tlen++] = 0;

  rc = oem_pldm_send_recv(ACB_BIC_BUS, ACB_BIC_EID, tbuf, tlen, &rbuf, &rlen);

  if (rc == 0) {
    resp= (struct pldm_snr_reading_t*)rbuf;
    if (resp->data.completion_code || resp->data.sensor_operational_state) {
      rc = -1;
      goto exit;
    }
    integer = resp->data.present_reading[0] | resp->data.present_reading[1] << 8;
    decimal = (float)(resp->data.present_reading[2] | resp->data.present_reading[3] << 8)/1000;

    if (integer >= 0) {
      *value = (float)integer + decimal;
    } else {
      *value = (float)integer - decimal;
    }
  }

exit:
  if (rbuf) {
    free(rbuf);
  }

  return rc;
}

PAL_SENSOR_MAP acb_sensor_map[] = {
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x00
  {"Inlet Temp",  0, get_acb_sensor, true,  {50, 0, 150, 0, 0, 0, 0, 0}, TEMP}, // 0x01
  {"Outlet Temp", 0, get_acb_sensor, true,  {55, 0, 150, 0, 0, 0, 0, 0}, TEMP}, // 0x02
  {"HSC1 TEMP ",  0, get_acb_sensor, true,  {105, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x03
  {"HSC2 TEMP",   0, get_acb_sensor, true,  {105, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x04
  {"PEX0 TEMP",   0, get_acb_sensor, false, {0, 0, 115, 0, 0, 0, 0, 0},  TEMP}, // 0x05
  {"PEX1 TEMP",   0, get_acb_sensor, false, {0, 0, 115, 0, 0, 0, 0, 0},  TEMP}, // 0x06
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x07
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x08
  {"P12V_AUX_1", 0, get_acb_sensor, true, {130, 0, 139, 0, 0, 0, 0, 0}, TEMP}, // 0x09
  {"P12V_AUX_2", 0, get_acb_sensor, true, {130, 0, 139, 0, 0, 0, 0, 0}, TEMP}, // 0x0A
  {"P51V_AUX_L", 0, get_acb_sensor, true, {52.53, 52.02, 62, 49.47, 49.98, 38, 0, 0}, VOLT}, // 0x0B
  {"P51V_AUX_R", 0, get_acb_sensor, true, {52.53, 52.02, 62, 49.47, 49.98, 38, 0, 0}, VOLT}, // 0x0C
  {"P12V_AUX_1", 0, get_acb_sensor, true, {13.2, 12.96, 14.33, 10.8, 11.04, 10.091, 0, 0}, VOLT}, // 0x0D
  {"P12V_AUX_2", 0, get_acb_sensor, true, {13.2, 12.96, 14.33, 10.8, 11.04, 10.091, 0, 0}, VOLT}, // 0x0E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x0F
  {"P5V_AUX",  0, get_acb_sensor, true,  {5.5,   5.4,   5.65, 4.5,   4.6,   4.15,  0, 0}, VOLT}, // 0x10
  {"P3V3_AUX", 0, get_acb_sensor, true,  {3.531, 3.465, 3.73, 3.069, 3.135, 2.739, 0, 0}, VOLT}, // 0x11
  {"P1V2_AUX", 0, get_acb_sensor, true,  {1.284, 1.26,  1.36, 1.116, 1.14,  0.996, 0, 0}, VOLT}, // 0x12
  {"P1V8_PEX", 0, get_acb_sensor, false, {1.926, 1.890, 0,    1.674, 1.71,  0,     0, 0}, VOLT}, // 0x13
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
  {"P1V8_1_VDD",  0, get_acb_sensor, false, {1.926, 1.89, 0, 1.674, 1.71, 0, 0, 0}, VOLT}, // 0x1E
  {"P1V8_2_VDD",  0, get_acb_sensor, false, {1.926, 1.89, 0, 1.674, 1.71, 0, 0, 0}, VOLT}, // 0x1F
  {"P1V25_1_VDD", 0, get_acb_sensor, false, {1.2875, 1.2625, 1.3125, 1.2125, 1.2375, 1.1875, 0, 0}, VOLT}, // 0x20
  {"P1V25_2_VDD", 0, get_acb_sensor, false, {1.2875, 1.2625, 1.3125, 1.2125, 1.2375, 1.1875, 0, 0}, VOLT}, // 0x21
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x22
  {"P12V_ACCL1",  0, get_acb_sensor, false, {13.2, 12.96, 14.33, 10.8, 11.04, 10.091, 0, 0}, VOLT}, // 0x23
  {"P12V_ACCL2",  0, get_acb_sensor, false, {13.2, 12.96, 14.33, 10.8, 11.04, 10.091, 0, 0}, VOLT}, // 0x24
  {"P12V_ACCL3",  0, get_acb_sensor, false, {13.2, 12.96, 14.33, 10.8, 11.04, 10.091, 0, 0}, VOLT}, // 0x25
  {"P12V_ACCL4",  0, get_acb_sensor, false, {13.2, 12.96, 14.33, 10.8, 11.04, 10.091, 0, 0}, VOLT}, // 0x26
  {"P12V_ACCL5",  0, get_acb_sensor, false, {13.2, 12.96, 14.33, 10.8, 11.04, 10.091, 0, 0}, VOLT}, // 0x27
  {"P12V_ACCL6",  0, get_acb_sensor, false, {13.2, 12.96, 14.33, 10.8, 11.04, 10.091, 0, 0}, VOLT}, // 0x28
  {"P12V_ACCL7",  0, get_acb_sensor, false, {13.2, 12.96, 14.33, 10.8, 11.04, 10.091, 0, 0}, VOLT}, // 0x29
  {"P12V_ACCL8",  0, get_acb_sensor, false, {13.2, 12.96, 14.33, 10.8, 11.04, 10.091, 0, 0}, VOLT}, // 0x2A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x2B
  {"P12V_ACCL9",  0, get_acb_sensor, false, {13.2, 12.96, 14.33, 10.8, 11.04, 10.091, 0, 0}, VOLT}, // 0x2C
  {"P12V_ACCL10", 0, get_acb_sensor, false, {13.2, 12.96, 14.33, 10.8, 11.04, 10.091, 0, 0}, VOLT}, // 0x2D
  {"P12V_ACCL11", 0, get_acb_sensor, false, {13.2, 12.96, 14.33, 10.8, 11.04, 10.091, 0, 0}, VOLT}, // 0x2E
  {"P12V_ACCL12", 0, get_acb_sensor, false, {13.2, 12.96, 14.33, 10.8, 11.04, 10.091, 0, 0}, VOLT}, // 0x2F
  //
  {"P51V_AUX_L", 0, get_acb_sensor, false, {35,  0,   0, 0, 0, 0, 0, 0}, CURR}, // 0x30
  {"P51V_AUX_R", 0, get_acb_sensor, false, {35,  0,   0, 0, 0, 0, 0, 0}, CURR}, // 0x31
  {"P12V_AUX_1", 0, get_acb_sensor, false, {110, 0, 132, 0, 0, 0, 0, 0}, CURR}, // 0x32
  {"P12V_AUX_2", 0, get_acb_sensor, false, {110, 0, 132, 0, 0, 0, 0, 0}, CURR}, // 0x33
  {"P12V_ACCL1", 0, get_acb_sensor, false, {18.3, 0, 25, 0, 0, 0, 0, 0}, CURR}, // 0x34
  {"P12V_ACCL2", 0, get_acb_sensor, false, {18.3, 0, 25, 0, 0, 0, 0, 0}, CURR}, // 0x35
  {"P12V_ACCL3", 0, get_acb_sensor, false, {18.3, 0, 25, 0, 0, 0, 0, 0}, CURR}, // 0x36
  {"P12V_ACCL4", 0, get_acb_sensor, false, {18.3, 0, 25, 0, 0, 0, 0, 0}, CURR}, // 0x37
  {"P12V_ACCL5", 0, get_acb_sensor, false, {18.3, 0, 25, 0, 0, 0, 0, 0}, CURR}, // 0x38
  {"P12V_ACCL6", 0, get_acb_sensor, false, {18.3, 0, 25, 0, 0, 0, 0, 0}, CURR},  // 0x39
  {"P12V_ACCL7", 0, get_acb_sensor, false, {18.3, 0, 25, 0, 0, 0, 0, 0}, CURR},  // 0x3A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x3B
  {"P12V_ACCL8",  0, get_acb_sensor, false, {18.3, 0, 25, 0, 0, 0, 0, 0}, CURR},  // 0x3C
  {"P12V_ACCL9",  0, get_acb_sensor, false, {18.3, 0, 25, 0, 0, 0, 0, 0}, CURR},  // 0x3D
  {"P12V_ACCL10", 0, get_acb_sensor, false, {18.3, 0, 25, 0, 0, 0, 0, 0}, CURR},  // 0x3E
  {"P12V_ACCL11", 0, get_acb_sensor, false, {18.3, 0, 25, 0, 0, 0, 0, 0}, CURR},  // 0x3F
  {"P12V_ACCL12", 0, get_acb_sensor, false, {18.3, 0, 25, 0, 0, 0, 0, 0}, CURR},  // 0x40
  //
  {"P51V_AUX_L", 0, get_acb_sensor, false, {1838.55, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x41
  {"P51V_AUX_R", 0, get_acb_sensor, false, {1838.55, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x42
  {"P12V_AUX_1", 0, get_acb_sensor, false, {1452, 0, 1891.56, 0, 0, 0, 0}, POWER}, // 0x43
  {"P12V_AUX_2", 0, get_acb_sensor, false, {1452, 0, 1891.56, 0, 0, 0, 0, 0}, POWER}, // 0x44
  {"P12V_ACCL1", 0, get_acb_sensor, false, {241.56, 0, 358.25, 0, 0, 0, 0, 0}, POWER}, // 0x45
  {"P12V_ACCL2", 0, get_acb_sensor, false, {241.56, 0, 358.25, 0, 0, 0, 0, 0}, POWER}, // 0x46
  {"P12V_ACCL3", 0, get_acb_sensor, false, {241.56, 0, 358.25, 0, 0, 0, 0, 0}, POWER}, // 0x47
  {"P12V_ACCL4", 0, get_acb_sensor, false, {241.56, 0, 358.25, 0, 0, 0, 0, 0}, POWER}, // 0x48
  {"P12V_ACCL5", 0, get_acb_sensor, false, {241.56, 0, 358.25, 0, 0, 0, 0, 0}, POWER}, // 0x49
  {"P12V_ACCL6", 0, get_acb_sensor, false, {241.56, 0, 358.25, 0, 0, 0, 0, 0}, POWER}, // 0x4A
  {"P12V_ACCL7", 0, get_acb_sensor, false, {241.56, 0, 358.25, 0, 0, 0, 0, 0}, POWER}, // 0x4B
  {"P12V_ACCL8", 0, get_acb_sensor, false, {241.56, 0, 358.25, 0, 0, 0, 0, 0}, POWER}, // 0x4C
  {"P12V_ACCL9", 0, get_acb_sensor, false, {241.56, 0, 358.25, 0, 0, 0, 0, 0}, POWER}, // 0x4D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x4E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x4F
  {"P12V_ACCL10", 0, get_acb_sensor, false, {241.56, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x50
  {"P12V_ACCL11", 0, get_acb_sensor, false, {241.56, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x51
  {"P12V_ACCL12", 0, get_acb_sensor, false, {241.56, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x52
  //
  {"P51V_STBY_L", 0, get_acb_sensor, true,  {52.53, 52.02, 62, 49.47, 49.98, 38, 0, 0},  VOLT}, // 0x53
  {"P51V_STBY_R", 0, get_acb_sensor, true,  {52.53, 52.02, 62, 49.47, 49.98, 38, 0, 0},  VOLT}, // 0x54
  {"P51V_STBY_L", 0, get_acb_sensor, true,  {1838.55, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x55
  {"P51V_STBY_R", 0, get_acb_sensor, true,  {1838.55, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x56
  {"P0V8_VDD_1",  0, get_acb_sensor, false, {0.824, 0.808, 0.84, 0.776, 0.792, 0.76, 0, 0},  VOLT}, // 0x57
  {"P0V8_VDD_2",  0, get_acb_sensor, false, {0.824, 0.808, 0.84, 0.776, 0.792, 0.76, 0, 0},  VOLT}, // 0x58
  {"P0V8_VDD_1",  0, get_acb_sensor, false, {47, 0, 57, 0, 0, 0, 0, 0},  CURR}, // 0x59
  {"P0V8_VDD_2",  0, get_acb_sensor, false, {47, 0, 57, 0, 0, 0, 0, 0},  CURR}, // 0x5A
  {"P0V8_VDD_1",  0, get_acb_sensor, false, {38.728, 0, 47.88, 0, 0, 0, 0, 0},  POWER}, // 0x5B
  {"P0V8_VDD_2",  0, get_acb_sensor, false, {38.728, 0, 47.88, 0, 0, 0, 0, 0},  POWER}, // 0x5C
  {"P51V_STBY_L", 0, get_acb_sensor, true,  {35, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x5D
  {"P51V_STBY_R", 0, get_acb_sensor, true,  {35, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x5E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x5F
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x60
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x61
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x62
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x63
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x64
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x65
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x66
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x67
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x68
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x69
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x6A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x6B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x6C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x6D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x6E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x6F
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x70
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x71
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x72
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x73
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x74
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x75
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x76
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x77
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x78
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x79
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x7A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x7B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x7C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x7D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x7E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x7F
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x80
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x81
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x82
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x83
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x84
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x85
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x86
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x87
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x88
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x89
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x8A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x8B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x8C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x8D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x8E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x8F
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x90
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x91
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x92
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x93
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x94
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x95
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x96
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x97
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x98
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x99
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x9A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x9B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x9C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x9D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x9E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x9F
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xA0
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xA1
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xA2
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xA3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xA4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xA5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xA6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xA7
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xA8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xA9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xAA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xAB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xAC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xAD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xAE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xAF
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xB0
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xB1
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xB2
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xB3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xB4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xB5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xB6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xB7
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xB8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xB9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xBA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xBB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xBC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xBD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xBE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0xBF

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC0
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC1
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC2
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC7
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCF

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD0
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD1
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD2
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD7
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDF


};

const uint8_t acb_sensor_list[] = {
  ACB_SENSOR_INLET_TEMP,
  ACB_SENSOR_OUTLET_TEMP,
  ACB_SENSOR_HSC_1_TEMP,
  ACB_SENSOR_HSC_2_TEMP,
  ACB_SENSOR_PEX0_TEMP,
  ACB_SENSOR_PEX1_TEMP,
  ACB_SENSOR_P12V_AUX_1_TEMP,
  ACB_SENSOR_P12V_AUX_2_TEMP,
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
};

size_t acb_sensor_cnt = sizeof(acb_sensor_list)/sizeof(uint8_t);
