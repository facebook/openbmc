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

static int
get_accl_sensor(uint8_t fru, uint8_t sensor_num, float *value) 
{
  int ret = 0;
  int16_t integer = 0;
  float decimal=0;
  uint8_t txbuf[MAX_TXBUF_SIZE] = {0};
  uint8_t rxbuf[MAX_RXBUF_SIZE] = {0};
  uint8_t txlen = 0;
  size_t  rxlen = 0;
  oem_1s_sensor_reading_resp *resp;

  txbuf[txlen++] = fru;
  txbuf[txlen++] = sensor_num;
  ret = oem_pldm_ipmi_send_recv(ACB_BIC_BUS, ACB_BIC_EID, NETFN_OEM_1S_REQ,
                  CMD_OEM_1S_GET_SENSOR_READING, txbuf, txlen, rxbuf, &rxlen, true);

  if (ret != 0 || rxlen != sizeof(oem_1s_sensor_reading_resp)) {
    syslog(LOG_ERR, "%s failed, fru:%u snr:%u ret:%d rxlen: %d\n", __func__, fru, sensor_num, ret, rxlen);
    return READING_NA;
  }

  resp = (oem_1s_sensor_reading_resp*)rxbuf;
  if (resp->status) { // not present or not power on
    return READING_NA;
  }

  integer = resp->integer_l | resp->integer_h << 8;
  decimal = (float)(resp->fraction_l | resp->fraction_h << 8)/1000;

  if (integer >= 0) {
    *value = (float)integer + decimal;
  } else {
    *value = (float)integer - decimal;
  }

  return ret;
}

PAL_SENSOR_MAP acb_sensor_map[] = {
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x00
  {"OUTLET_1_TEMP", 0, get_acb_sensor, true,  {50, 0, 150, 0, 0, 0, 0, 0}, TEMP}, // 0x01
  {"OUTLET_2_TEMP", 0, get_acb_sensor, true,  {55, 0, 150, 0, 0, 0, 0, 0}, TEMP}, // 0x02
  {"HSC1_TEMP ",  0, get_acb_sensor, true,  {105, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x03
  {"HSC2_TEMP",   0, get_acb_sensor, true,  {105, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x04
  {"PEX0_TEMP",   0, get_acb_sensor, false, {0, 0, 115, 0, 0, 0, 0, 0},  TEMP}, // 0x05
  {"PEX1_TEMP",   0, get_acb_sensor, false, {0, 0, 115, 0, 0, 0, 0, 0},  TEMP}, // 0x06
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x07
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x08
  {"POWER_BRICK_1_TEMP", 0, get_acb_sensor, true, {130, 0, 139, 0, 0, 0, 0, 0}, TEMP}, // 0x09
  {"POWER_BRICK_2_TEMP", 0, get_acb_sensor, true, {130, 0, 139, 0, 0, 0, 0, 0}, TEMP}, // 0x0A
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
  {"P1V25_1_VDD", 0, get_acb_sensor, false, {1.2875, 1.275, 1.3125, 1.2125, 1.225, 1.1875, 0, 0}, VOLT}, // 0x20
  {"P1V25_2_VDD", 0, get_acb_sensor, false, {1.2875, 1.275, 1.3125, 1.2125, 1.225, 1.1875, 0, 0}, VOLT}, // 0x21
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
  {"P0V8_VDD_1",  0, get_acb_sensor, false, {0.824, 0.816, 0.84, 0.776, 0.784, 0.76, 0, 0},  VOLT}, // 0x57
  {"P0V8_VDD_2",  0, get_acb_sensor, false, {0.824, 0.816, 0.84, 0.776, 0.784, 0.76, 0, 0},  VOLT}, // 0x58
  {"P0V8_VDD_1",  0, get_acb_sensor, false, {47, 0, 57, 0, 0, 0, 0, 0},  CURR}, // 0x59
  {"P0V8_VDD_2",  0, get_acb_sensor, false, {47, 0, 57, 0, 0, 0, 0, 0},  CURR}, // 0x5A
  {"P0V8_VDD_1",  0, get_acb_sensor, false, {38.728, 0, 47.88, 0, 0, 0, 0, 0},  POWER}, // 0x5B
  {"P0V8_VDD_2",  0, get_acb_sensor, false, {38.728, 0, 47.88, 0, 0, 0, 0, 0},  POWER}, // 0x5C
  {"P51V_STBY_L", 0, get_acb_sensor, true,  {35, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x5D
  {"P51V_STBY_R", 0, get_acb_sensor, true,  {35, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x5E
  {"P0V8_1_VDD_TEMP", 0, get_acb_sensor, true,  {100, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x5F
  {"P0V8_2_VDD_TEMP", 0, get_acb_sensor, true,  {100, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x60
  {"P1V25_1_VDD(P12V_1_M_AUX)", 0, get_acb_sensor, false, {13.2, 12.96, 14.33, 10.8, 11.04, 10.091, 0, 0}, VOLT}, // 0x61
  {"P1V25_2_VDD(P12V_2_M_AUX)", 0, get_acb_sensor, false, {13.2, 12.96, 14.33, 10.8, 11.04, 10.091, 0, 0}, VOLT}, // 0x62
  {"P1V25_1_VDD(P12V_1_M_AUX)", 0, get_acb_sensor, false, {2.48, 0, 2.92, 0, 0, 0, 0, 0}, CURR}, // 0x63
  {"P1V25_2_VDD(P12V_2_M_AUX)", 0, get_acb_sensor, false, {2.48, 0, 2.92, 0, 0, 0, 0, 0}, CURR}, // 0x64
  {"P1V25_1_VDD(P12V_1_M_AUX)", 0, get_acb_sensor, false, {32.736, 0, 41.8436, 0, 0, 0, 0, 0}, POWER}, // 0x65
  {"P1V25_2_VDD(P12V_2_M_AUX)", 0, get_acb_sensor, false, {32.736, 0, 41.8436, 0, 0, 0, 0, 0}, POWER}, // 0x66
  {"INLET_TEMP", 0, get_acb_sensor, false, {50, 0, 150, 0, 0, 0, 0, 0}, TEMP}, // 0x67
};

PAL_SENSOR_MAP accl_sensor_map[] = {
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x00
  {"Freya_1_Temp",  0, get_accl_sensor, false,  {98, 85, 0, 0, 0, 0, 0, 0}, TEMP}, // 0x01
  {"Freya_2_Temp",  0, get_accl_sensor, false,  {98, 85, 0, 0, 0, 0, 0, 0}, TEMP}, // 0x02
  {"P12V_EFUSE_VOL",  0, get_accl_sensor, false,  {13.2, 12.96, 14.33, 10.8, 11.04, 10.091, 0, 0}, VOLT}, // 0x03
  {"P3V3_1_VOL",  0, get_accl_sensor, false,  {3.531, 3.465, 3.73, 3.069, 3.135, 2.739, 0, 0}, VOLT}, // 0x04
  {"P3V3_2_VOL",  0, get_accl_sensor, false,  {3.531, 3.465, 3.73, 3.069, 3.135, 2.739, 0, 0}, VOLT}, // 0x05
  {"Freya_1_VOL_1",  0, get_accl_sensor, false,  {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, // 0x06
  {"Freya_1_VOL_2",  0, get_accl_sensor, false,  {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, // 0x07
  {"P12V_EFUSE_CUR",  0, get_accl_sensor, false,  {5.5, 0, 7.2, 0, 0, 0, 0, 0}, CURR}, // 0x08
  {"P3V3_1_CUR",  0, get_accl_sensor, false,  {8.4, 0, 11.5, 0, 0, 0, 0, 0}, CURR}, // 0x09
  {"P3V3_2_CUR",  0, get_accl_sensor, false,  {8.4, 0, 11.5, 0, 0, 0, 0, 0}, CURR}, // 0x0A
  {"Freya_2_VOL_1",  0, get_accl_sensor, false,  {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, // 0x0B
  {"Freya_2_VOL_2",  0, get_accl_sensor, false,  {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, // 0x0C
  {"P12V_EFUSE_PWR",  0, get_accl_sensor, false,  {67.1, 0, 87.8, 0, 0, 0, 0, 0}, POWER}, // 0x0D
  {"P3V3_1_PWR",  0, get_accl_sensor, false,  {27.72, 0, 37.9, 0, 0, 0, 0, 0}, POWER}, // 0x0E
  {"P3V3_2_PWR",  0, get_accl_sensor, false,  {27.72, 0, 37.9, 0, 0, 0, 0, 0}, POWER}, // 0x0F
};

const uint8_t acb_sensor_list[] = {
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
  ACB_SENSOR_P1V25_1_VDD_P12V_1_M_AUX_VOL,
  ACB_SENSOR_P1V25_2_VDD_P12V_2_M_AUX_VOL,
  ACB_SENSOR_P1V25_1_VDD_P12V_1_M_AUX_CUR,
  ACB_SENSOR_P1V25_2_VDD_P12V_2_M_AUX_CUR,
  ACB_SENSOR_P1V25_1_VDD_P12V_1_M_AUX_PWR,
  ACB_SENSOR_P1V25_2_VDD_P12V_2_M_AUX_PWR,
};

const uint8_t accl_sensor_list[] = {
  ACCL_Freya_1_Temp,
  ACCL_Freya_2_Temp,
  ACCL_P12V_EFUSE_VOL,
  ACCL_P3V3_1_VOL,
  ACCL_P3V3_2_VOL,
  ACCL_Freya_1_VOL_1,
  ACCL_Freya_1_VOL_2,
  ACCL_P12V_EFUSE_CUR,
  ACCL_P3V3_1_CUR,
  ACCL_P3V3_2_CUR,
  ACCL_Freya_2_VOL_1,
  ACCL_Freya_2_VOL_2,
  ACCL_P12V_EFUSE_PWR,
  ACCL_P3V3_1_PWR,
  ACCL_P3V3_2_PWR,
};

size_t acb_sensor_cnt = sizeof(acb_sensor_list)/sizeof(uint8_t);
size_t accl_sensor_cnt = sizeof(accl_sensor_list)/sizeof(uint8_t);
