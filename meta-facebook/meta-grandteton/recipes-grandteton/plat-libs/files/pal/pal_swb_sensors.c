#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <libpldm-oem/pldm.h>
#include <openbmc/obmc-i2c.h>
#include "pal_swb_sensors.h"
#include "pal_sensors.h"
#include "syslog.h"

int
get_swb_sensor(uint8_t fru, uint8_t sensor_num, float *value)
{
  uint8_t tbuf[255] = {0};
  uint8_t* rbuf = (uint8_t *) NULL;//, tmp;
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

  rc = oem_pldm_send_recv(SWB_BUS_ID, SWB_BIC_EID, tbuf, tlen, &rbuf, &rlen);

  if (rc == 0) {
    resp= (struct pldm_snr_reading_t*)rbuf;
    if (resp->data.completion_code || resp->data.sensor_operational_state) {
      rc = -1;
      goto exit;
    }
    integer = resp->data.present_reading[0] | resp->data.present_reading[1] << 8;
    decimal = (float)(resp->data.present_reading[2] | resp->data.present_reading[3] << 8)/1000;

    if (integer >= 0)
      *value = (float)integer + decimal;
    else
      *value = (float)integer - decimal;
  }

exit:
  if (rbuf)
    free(rbuf);

  return rc;
}

static
int read_swb_cpld_health(uint8_t fru, uint8_t sensor_num, float *value) {
  int fd = 0, ret = -1;
  uint8_t tlen, rlen;
  uint8_t addr = SWB_CPLD_ADDR;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  static unsigned int retry;

  fd = open("/dev/i2c-32", O_RDWR);
  if (fd < 0) {
    return -1;
  }

  tbuf[0] = 0x06;
  tlen = 1;
  rlen = 1;

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
  if (ret || rbuf[0] != 0xff) {
    if (retry_err_handle(retry, 5) == READING_NA) {
      *value = 1;
    } else {
      retry++;
      *value = 0;
    }
  } else {
    *value = 0;
    retry = 0;
  }

#ifdef DEBUG
  syslog(LOG_INFO, "%s ret=%d val=%d rbuf[0]=%x\n", __func__, ret, (int)*value, rbuf[0]);
#endif
  close(fd);
  return 0;
}

PAL_SENSOR_MAP swb_sensor_map[] = {
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x00
  {"NIC0_TEMP", 0, get_swb_sensor, true, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x01
  {"NIC0_VOLT", 0, get_swb_sensor, true, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, // 0x02
  {"NIC0_CURR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x03
  {"NIC0_PWR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x04
  {"NIC1_TEMP", 0, get_swb_sensor, true, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x05
  {"NIC1_VOLT", 0, get_swb_sensor, true, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, // 0x06
  {"NIC1_CURR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x07
  {"NIC1_PWR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x08
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x09
  {"NIC2_TEMP", 0, get_swb_sensor, true, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x0A
  {"NIC2_VOLT", 0, get_swb_sensor, true, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, // 0x0B
  {"NIC2_CURR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x0C
  {"NIC2_PWR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x0D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x0E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x0F
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x10
  {"NIC3_TEMP", 0, get_swb_sensor, true, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x11
  {"NIC3_VOLT", 0, get_swb_sensor, true, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, // 0x12
  {"NIC3_CURR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x13
  {"NIC3_PWR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x14
  {"NIC4_TEMP", 0, get_swb_sensor, true, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x15
  {"NIC4_VOLT", 0, get_swb_sensor, true, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, // 0x16
  {"NIC4_CURR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x17
  {"NIC4_PWR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x18
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x19
  {"NIC5_TEMP", 0, get_swb_sensor, true, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x1A
  {"NIC5_VOLT", 0, get_swb_sensor, true, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, // 0x1B
  {"NIC5_CURR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x1C
  {"NIC5_PWR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x1D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x1E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x1F
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x20
  {"NIC6_TEMP", 0, get_swb_sensor, true, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x21
  {"NIC6_VOLT", 0, get_swb_sensor, true, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, // 0x22
  {"NIC6_CURR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x23
  {"NIC6_PWR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x24
  {"NIC7_TEMP", 0, get_swb_sensor, true, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x25
  {"NIC7_VOLT", 0, get_swb_sensor, true, {12.96, 0, 0, 10.56, 0, 0, 0, 0}, VOLT}, // 0x26
  {"NIC7_CURR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x27
  {"NIC7_PWR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x28
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x29
  {"HSC_TEMP", 0, get_swb_sensor, true, {120.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x2A
  {"HSC_VOUT", 0, get_swb_sensor, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, // 0x2B
  {"HSC_CURR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x2C
  {"HSC_PWR", 0, get_swb_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x2D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x2E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x2F
  {"P12V_AUX", 0, get_swb_sensor, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, // 0x30
  {"P5V_AUX", 0, get_swb_sensor, true, {5.25, 0, 0, 4.75, 0, 0, 0, 0}, VOLT}, // 0x31
  {"P3V3_AUX", 0, get_swb_sensor, true, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, // 0x32
  {"P1V2_AUX", 0, get_swb_sensor, true, {1.26, 0, 0, 1.14, 0, 0, 0, 0}, VOLT}, // 0x33
  {"P3V3", 0, get_swb_sensor, false, {3.47, 0, 0, 3.13, 0, 0, 0, 0}, VOLT}, // 0x34
  {"P1V8_PEX0", 0, get_swb_sensor, false, {1.98, 0, 0, 1.62, 0, 0, 0, 0}, VOLT}, // 0x35
  {"P1V8_PEX1", 0, get_swb_sensor, false, {1.98, 0, 0, 1.62, 0, 0, 0, 0}, VOLT}, // 0x36
  {"P1V8_PEX2", 0, get_swb_sensor, false, {1.98, 0, 0, 1.62, 0, 0, 0, 0}, VOLT}, // 0x37
  {"P1V8_PEX3", 0, get_swb_sensor, false, {1.98, 0, 0, 1.62, 0, 0, 0, 0}, VOLT}, // 0x38
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x39
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x3A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x3B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x3C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x3D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x3E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x3F
  {"PEX0_VR_TEMP", 0, get_swb_sensor, false, {125.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x40
  {"PEX0_P0V8_VOLT", 0, get_swb_sensor, false, {0.84, 0, 0, 0.76, 0, 0, 0, 0}, VOLT}, // 0x41
  {"PEX0_P0V8_CURR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x42
  {"PEX0_P0V8_PWR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x43
  {"PEX0_P1V25_VOLT", 0, get_swb_sensor, false, {1.3125, 0, 0, 1.1875, 0, 0, 0, 0}, VOLT}, // 0x44
  {"PEX0_P1V25_CURR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x45
  {"PEX0_P1V25_PWR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x46
  {"PEX1_VR_TEMP", 0, get_swb_sensor, false, {125.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x47
  {"PEX1_P0V8_VOLT", 0, get_swb_sensor, false, {0.84, 0, 0, 0.76, 0, 0, 0, 0}, VOLT}, // 0x48
  {"PEX1_P0V8_CURR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x49
  {"PEX1_P0V8_PWR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x4A
  {"PEX1_P1V25_VOLT", 0, get_swb_sensor, false, {1.3125, 0, 0, 1.1875, 0, 0, 0, 0}, VOLT}, // 0x4B
  {"PEX1_P1V25_CURR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x4C
  {"PEX1_P1V25_PWR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x4D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x4E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x4F
  {"PEX2_VR_TEMP", 0, get_swb_sensor, false, {125.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x50
  {"PEX2_P0V8_VOLT", 0, get_swb_sensor, false, {0.84, 0, 0, 0.76, 0, 0, 0, 0}, VOLT}, // 0x51
  {"PEX2_P0V8_CURR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x52
  {"PEX2_P0V8_PWR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x53
  {"PEX2_P1V25_VOLT", 0, get_swb_sensor, false, {1.3125, 0, 0, 1.1875, 0, 0, 0, 0}, VOLT}, // 0x54
  {"PEX2_P1V25_CURR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x55
  {"PEX2_P1V25_PWR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x56
  {"PEX3_VR_TEMP", 0, get_swb_sensor, false, {125.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x57
  {"PEX3_P0V8_VOLT", 0, get_swb_sensor, false, {0.84, 0, 0, 0.76, 0, 0, 0, 0}, VOLT}, // 0x58
  {"PEX3_P0V8_CURR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x59
  {"PEX3_P0V8_PWR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x5A
  {"PEX3_P1V25_VOLT", 0, get_swb_sensor, false, {1.3125, 0, 0, 1.1875, 0, 0, 0, 0}, VOLT}, // 0x5B
  {"PEX3_P1V25_CURR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x5C
  {"PEX3_P1V25_PWR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x5D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x5E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x5F
  {"PEX_P1V8_VOLT", 0, get_swb_sensor, false, {1.89, 0, 0, 1.71, 0, 0, 0, 0}, VOLT}, // 0x60
  {"PEX_P1V8_CURR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x61
  {"PEX_P1V8_PWR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x62
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x63
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x64
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x65
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x66
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x67
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x68
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x69
  {"PEX0_TEMP", 0, get_swb_sensor, false, {115.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x6A
  {"PEX1_TEMP", 0, get_swb_sensor, false, {115.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x6B
  {"PEX2_TEMP", 0, get_swb_sensor, false, {115.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x6C
  {"PEX3_TEMP", 0, get_swb_sensor, false, {115.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x6D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x6E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x6F
  {"SYSTEM_INLET_TEMP", 0, get_swb_sensor, false, {45.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x70
  {"OUTLET_TEMP_L1", 0, get_swb_sensor, false, {75.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x71
  {"OUTLET_TEMP_L2", 0, get_swb_sensor, false, {75.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x72
  {"OUTLET_TEMP_R1", 0, get_swb_sensor, false, {75.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x73
  {"OUTLET_TEMP_R2", 0, get_swb_sensor, false, {75.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x74
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
  {"E1S_0_TEMP", 0, get_swb_sensor, false, {70.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x80
  {"E1S_0_VOLT", 0, get_swb_sensor, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, // 0x81
  {"E1S_0_CURR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x82
  {"E1S_0_PWR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x83
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x84
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x85
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x86
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x87
  {"E1S_2_TEMP", 0, get_swb_sensor, false, {70.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x88
  {"E1S_2_VOLT", 0, get_swb_sensor, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, // 0x89
  {"E1S_2_CURR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x8A
  {"E1S_2_PWR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x8B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x8F
  {"E1S_4_TEMP", 0, get_swb_sensor, false, {70.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x90
  {"E1S_4_VOLT", 0, get_swb_sensor, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, // 0x91
  {"E1S_4_CURR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x92
  {"E1S_4_PWR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x93
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x94
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x95
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x96
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x97
  {"E1S_6_TEMP", 0, get_swb_sensor, false, {70.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0x98
  {"E1S_6_VOLT", 0, get_swb_sensor, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, // 0x99
  {"E1S_6_CURR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0x9A
  {"E1S_6_PWR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0x9B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9F
  {"E1S_8_TEMP", 0, get_swb_sensor, false, {70.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0xA0
  {"E1S_8_VOLT", 0, get_swb_sensor, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, // 0xA1
  {"E1S_8_CURR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0xA2
  {"E1S_8_PWR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0xA3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA7
  {"E1S_10_TEMP", 0, get_swb_sensor, false, {70.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0xA8
  {"E1S_10_VOLT", 0, get_swb_sensor, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, // 0xA9
  {"E1S_10_CURR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0xAA
  {"E1S_10_PWR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0xAB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAF
  {"E1S_12_TEMP", 0, get_swb_sensor, false, {70.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0xB0
  {"E1S_12_VOLT", 0, get_swb_sensor, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, // 0xB1
  {"E1S_12_CURR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0xB2
  {"E1S_12_PWR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0xB3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB7
  {"E1S_14_TEMP", 0, get_swb_sensor, false, {70.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, // 0xB8
  {"E1S_14_VOLT", 0, get_swb_sensor, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, // 0xB9
  {"E1S_14_CURR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, // 0xBA
  {"E1S_14_PWR", 0, get_swb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, // 0xBB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xBC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xBD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xBE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xBF

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

  {"CPLD_HEALTH", 0, read_swb_cpld_health, 0, {0, 0, 0, 0, 0, 0, 0, 0}, STATE}, //0xD0
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

const uint8_t swb_sensor_list[] = {
  SWB_SENSOR_TEMP_NIC_0,
  SWB_SENSOR_VOLT_NIC_0,
  SWB_SENSOR_IOUT_NIC_0,
  SWB_SENSOR_POUT_NIC_0,
  SWB_SENSOR_TEMP_NIC_1,
  SWB_SENSOR_VOLT_NIC_1,
  SWB_SENSOR_IOUT_NIC_1,
  SWB_SENSOR_POUT_NIC_1,
  SWB_SENSOR_TEMP_NIC_2,
  SWB_SENSOR_VOLT_NIC_2,
  SWB_SENSOR_IOUT_NIC_2,
  SWB_SENSOR_POUT_NIC_2,
  SWB_SENSOR_TEMP_NIC_3,
  SWB_SENSOR_VOLT_NIC_3,
  SWB_SENSOR_IOUT_NIC_3,
  SWB_SENSOR_POUT_NIC_3,
  SWB_SENSOR_TEMP_NIC_4,
  SWB_SENSOR_VOLT_NIC_4,
  SWB_SENSOR_IOUT_NIC_4,
  SWB_SENSOR_POUT_NIC_4,
  SWB_SENSOR_TEMP_NIC_5,
  SWB_SENSOR_VOLT_NIC_5,
  SWB_SENSOR_IOUT_NIC_5,
  SWB_SENSOR_POUT_NIC_5,
  SWB_SENSOR_TEMP_NIC_6,
  SWB_SENSOR_VOLT_NIC_6,
  SWB_SENSOR_IOUT_NIC_6,
  SWB_SENSOR_POUT_NIC_6,
  SWB_SENSOR_TEMP_NIC_7,
  SWB_SENSOR_VOLT_NIC_7,
  SWB_SENSOR_IOUT_NIC_7,
  SWB_SENSOR_POUT_NIC_7,
  SWB_SENSOR_BB_P12V_AUX,
  SWB_SENSOR_BB_P5V_AUX,
  SWB_SENSOR_BB_P3V3_AUX,
  SWB_SENSOR_BB_P1V2_AUX,
  SWB_SENSOR_BB_P3V3,
  SWB_SENSOR_BB_P1V8_PEX0,
  SWB_SENSOR_BB_P1V8_PEX1,
  SWB_SENSOR_BB_P1V8_PEX2,
  SWB_SENSOR_BB_P1V8_PEX3,
  SWB_SENSOR_VR_TEMP_PEX_0,
  SWB_SENSOR_P0V8_VOLT_PEX_0,
  SWB_SENSOR_P0V8_IOUT_PEX_0,
  SWB_SENSOR_P0V8_POUT_PEX_0,
  SWB_SENSOR_P1V25_VOLT_PEX_0,
  SWB_SENSOR_P1V25_IOUT_PEX_0,
  SWB_SENSOR_P1V25_POUT_PEX_0,
  SWB_SENSOR_VR_TEMP_PEX_1,
  SWB_SENSOR_P0V8_VOLT_PEX_1,
  SWB_SENSOR_P0V8_IOUT_PEX_1,
  SWB_SENSOR_P0V8_POUT_PEX_1,
  SWB_SENSOR_P1V25_VOLT_PEX_1,
  SWB_SENSOR_P1V25_IOUT_PEX_1,
  SWB_SENSOR_P1V25_POUT_PEX_1,
  SWB_SENSOR_VR_TEMP_PEX_2,
  SWB_SENSOR_P0V8_VOLT_PEX_2,
  SWB_SENSOR_P0V8_IOUT_PEX_2,
  SWB_SENSOR_P0V8_POUT_PEX_2,
  SWB_SENSOR_P1V25_VOLT_PEX_2,
  SWB_SENSOR_P1V25_IOUT_PEX_2,
  SWB_SENSOR_P1V25_POUT_PEX_2,
  SWB_SENSOR_VR_TEMP_PEX_3,
  SWB_SENSOR_P0V8_VOLT_PEX_3,
  SWB_SENSOR_P0V8_IOUT_PEX_3,
  SWB_SENSOR_P0V8_POUT_PEX_3,
  SWB_SENSOR_P1V25_VOLT_PEX_3,
  SWB_SENSOR_P1V25_IOUT_PEX_3,
  SWB_SENSOR_P1V25_POUT_PEX_3,
  SWB_SENSOR_P1V8_VOLT_PEX,
  SWB_SENSOR_P1V8_IOUT_PEX,
  SWB_SENSOR_P1V8_POUT_PEX,
  SWB_SENSOR_TEMP_PEX_0,
  SWB_SENSOR_TEMP_PEX_1,
  SWB_SENSOR_TEMP_PEX_2,
  SWB_SENSOR_TEMP_PEX_3,
  SWB_SENSOR_SYSTEM_INLET_TEMP,
  SWB_SENSOR_OUTLET_TEMP_L1,
  SWB_SENSOR_OUTLET_TEMP_L2,
  SWB_SENSOR_OUTLET_TEMP_R1,
  SWB_SENSOR_OUTLET_TEMP_R2,
  SWB_SENSOR_TEMP_E1S_0,
  SWB_SENSOR_VOLT_E1S_0,
  SWB_SENSOR_CURR_E1S_0,
  SWB_SENSOR_POUT_E1S_0,
  SWB_SENSOR_TEMP_E1S_2,
  SWB_SENSOR_VOLT_E1S_2,
  SWB_SENSOR_CURR_E1S_2,
  SWB_SENSOR_POUT_E1S_2,
  SWB_SENSOR_TEMP_E1S_4,
  SWB_SENSOR_VOLT_E1S_4,
  SWB_SENSOR_CURR_E1S_4,
  SWB_SENSOR_POUT_E1S_4,
  SWB_SENSOR_TEMP_E1S_6,
  SWB_SENSOR_VOLT_E1S_6,
  SWB_SENSOR_CURR_E1S_6,
  SWB_SENSOR_POUT_E1S_6,
  SWB_SENSOR_TEMP_E1S_8,
  SWB_SENSOR_VOLT_E1S_8,
  SWB_SENSOR_CURR_E1S_8,
  SWB_SENSOR_POUT_E1S_8,
  SWB_SENSOR_TEMP_E1S_10,
  SWB_SENSOR_VOLT_E1S_10,
  SWB_SENSOR_CURR_E1S_10,
  SWB_SENSOR_POUT_E1S_10,
  SWB_SENSOR_TEMP_E1S_12,
  SWB_SENSOR_VOLT_E1S_12,
  SWB_SENSOR_CURR_E1S_12,
  SWB_SENSOR_POUT_E1S_12,
  SWB_SENSOR_TEMP_E1S_14,
  SWB_SENSOR_VOLT_E1S_14,
  SWB_SENSOR_CURR_E1S_14,
  SWB_SENSOR_POUT_E1S_14,
};

const uint8_t shsc_sensor_list[] = {
  SWB_SENSOR_TEMP_PDB_HSC,
  SWB_SENSOR_VOUT_PDB_HSC,
  SWB_SENSOR_IOUT_PDB_HSC,
  SWB_SENSOR_POUT_PDB_HSC,
};

// List of SWB discrete sensors to be monitored
const uint8_t swb_discrete_sensor_list[] = {
  SWB_SENSOR_CPLD_HEALTH,
};

size_t swb_sensor_cnt = sizeof(swb_sensor_list)/sizeof(uint8_t);
size_t shsc_sensor_cnt = sizeof(shsc_sensor_list)/sizeof(uint8_t);
size_t swb_discrete_sensor_cnt = sizeof(swb_discrete_sensor_list)/sizeof(uint8_t);
