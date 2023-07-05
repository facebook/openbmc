#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <libpldm-oem/pldm.h>
#include "pal_meb_sensors.h"
#include "pal.h"
#include <openbmc/obmc-i2c.h>
#include "syslog.h"
#include <openbmc/obmc-sensors.h>

extern struct snr_map sensor_map[];

static int
get_meb_sensor(uint8_t fru, uint8_t sensor_num, float *value) 
{
  return get_pldm_sensor(MEB_BIC_BUS, MEB_BIC_EID, sensor_num, value);
}

static int
get_meb_jcn_sensor(uint8_t fru, uint8_t sensor_num, float *value) 
{
  int ret = 0;
  int16_t integer = 0;
  float decimal=0;
  uint8_t txbuf[MAX_TXBUF_SIZE] = {0};
  uint8_t rxbuf[MAX_RXBUF_SIZE] = {0};
  uint8_t txlen = 0;
  size_t  rxlen = 0;
  oem_1s_sensor_reading_resp *resp;

  if (sensor_num >= CXL_DIMM_A_TEMP && sensor_num <= CXL_DIMM_D_TEMP) {
    if (pal_bios_completed(FRU_MB) != true) {
      return READING_NA;
    }
  }

  txbuf[txlen++] = fru;
  txbuf[txlen++] = sensor_num;
  ret = oem_pldm_ipmi_send_recv(MEB_BIC_BUS, MEB_BIC_EID, NETFN_OEM_1S_REQ,
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

static int
get_meb_hsc_sensor(uint8_t fru, uint8_t sensor_num, float *value)
{
  return sensors_read(NULL, sensor_map[fru].map[sensor_num].snr_name, value);
}

PAL_SENSOR_MAP meb_sensor_map[] = {
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x00
  {"Inlet Temp", 0, get_meb_sensor, false, {65, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x01
  {"Outlet Temp", 0, get_meb_sensor, false, {90, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x02
  {"PU4 Temp", 0, get_meb_hsc_sensor, false, {125, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x03
  {"P12V_AUX_VOL", 0, get_meb_hsc_sensor, false, {13.2, 0, 0, 11.8, 0, 0, 0, 0},  VOLT}, // 0x04
  {"P3V3_AUX_VOL", 0, get_meb_sensor, false, {3.531, 0, 0, 3.07, 0, 0, 0, 0},  VOLT}, // 0x05
  {"P1V2_AUX_VOL", 0, get_meb_sensor, false, {1.284, 0, 0, 1.116, 0, 0, 0, 0},  VOLT}, // 0x06
  {"P3V3_VOL", 0, get_meb_sensor, false, {3.531, 0, 0, 3.07, 0, 0, 0, 0},  VOLT}, // 0x07
  {"P12V_AUX_CXL8_VOL", 0, get_meb_sensor, false, {12.81, 0, 0, 11.59, 0, 0, 0, 0},  VOLT}, // 0x08
  {"P12V_AUX_CXL7_VOL", 0, get_meb_sensor, false, {12.81, 0, 0, 11.59, 0, 0, 0, 0},  VOLT}, // 0x09
  {"P12V_AUX_CXL6_VOL", 0, get_meb_sensor, false, {12.81, 0, 0, 11.59, 0, 0, 0, 0},  VOLT}, // 0x0A
  {"P12V_AUX_CXL5_VOL", 0, get_meb_sensor, false, {12.81, 0, 0, 11.59, 0, 0, 0, 0},  VOLT}, // 0x0B
  {"P12V_AUX_SSD4_VOL", 0, get_meb_sensor, false, {13.42, 0, 0, 10.98, 0, 0, 0, 0},  VOLT}, // 0x0C
  {"P12V_AUX_SSD3_VOL", 0, get_meb_sensor, false, {13.42, 0, 0, 10.98, 0, 0, 0, 0},  VOLT}, // 0x0D
  {"P12V_AUX_SSD2_VOL", 0, get_meb_sensor, false, {13.42, 0, 0, 10.98, 0, 0, 0, 0},  VOLT}, // 0x0E
  {"P12V_AUX_SSD1_VOL", 0, get_meb_sensor, false, {13.42, 0, 0, 10.98, 0, 0, 0, 0},  VOLT}, // 0x0F
  {"P12V_AUX_CXL3_VOL", 0, get_meb_sensor, false, {12.81, 0, 0, 11.59, 0, 0, 0, 0},  VOLT}, // 0x10
  {"P12V_AUX_CXL4_VOL", 0, get_meb_sensor, false, {12.81, 0, 0, 11.59, 0, 0, 0, 0},  VOLT}, // 0x11
  {"P12V_AUX_CXL1_VOL", 0, get_meb_sensor, false, {12.81, 0, 0, 11.59, 0, 0, 0, 0},  VOLT}, // 0x12
  {"P12V_AUX_CXL2_VOL", 0, get_meb_sensor, false, {12.81, 0, 0, 11.59, 0, 0, 0, 0},  VOLT}, // 0x13
  {"P12V_AUX_SW_4C_2_VOL", 0, get_meb_sensor, false, {13.176, 0, 0, 10.736, 0, 0, 0, 0},  VOLT}, // 0x14
  {"P12V_AUX_SW_4C_1_VOL", 0, get_meb_sensor, false, {13.176, 0, 0, 10.736, 0, 0, 0, 0},  VOLT}, // 0x15
  {"P12V_AUX_CUR", 0, get_meb_hsc_sensor, false, {73.1, 0, 95, 0, 0, 0, 0, 0},  CURR}, // 0x16
  {"P12V_AUX_CXL8_CUR", 0, get_meb_sensor, false, {6, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x17
  {"P12V_AUX_CXL7_CUR", 0, get_meb_sensor, false, {6, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x18
  {"P12V_AUX_CXL6_CUR", 0, get_meb_sensor, false, {6, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x19
  {"P12V_AUX_CXL5_CUR", 0, get_meb_sensor, false, {6, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x1A
  {"P12V_AUX_SSD4_CUR", 0, get_meb_sensor, false, {2.8, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x1B
  {"P12V_AUX_SSD3_CUR", 0, get_meb_sensor, false, {2.8, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x1C
  {"P12V_AUX_SSD2_CUR", 0, get_meb_sensor, false, {2.8, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x1D
  {"P12V_AUX_SSD1_CUR", 0, get_meb_sensor, false, {2.8, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x1E
  {"P12V_AUX_CXL3_CUR", 0, get_meb_sensor, false, {6, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x1F
  {"P12V_AUX_CXL4_CUR", 0, get_meb_sensor, false, {6, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x20
  {"P12V_AUX_CXL1_CUR", 0, get_meb_sensor, false, {6, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x21
  {"P12V_AUX_CXL2_CUR", 0, get_meb_sensor, false, {6, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x22
  {"P12V_AUX_SW_4C_2_CUR", 0, get_meb_sensor, false, {6.6, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x23
  {"P12V_AUX_SW_4C_1_CUR", 0, get_meb_sensor, false, {6.6, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x24
  {"P12V_AUX_PWR", 0, get_meb_hsc_sensor, false, {918.575, 0, 1267, 0, 0, 0, 0, 0},  POWER}, // 0x25
  {"P12V_AUX_CXL8_PWR", 0, get_meb_sensor, false, {72, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x26
  {"P12V_AUX_CXL7_PWR", 0, get_meb_sensor, false, {72, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x27
  {"P12V_AUX_CXL6_PWR", 0, get_meb_sensor, false, {72, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x28
  {"P12V_AUX_CXL5_PWR", 0, get_meb_sensor, false, {72, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x29
  {"P12V_AUX_SSD4_PWR", 0, get_meb_sensor, false, {25, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x2A
  {"P12V_AUX_SSD3_PWR", 0, get_meb_sensor, false, {25, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x2B
  {"P12V_AUX_SSD2_PWR", 0, get_meb_sensor, false, {25, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x2C
  {"P12V_AUX_SSD1_PWR", 0, get_meb_sensor, false, {25, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x2D
  {"P12V_AUX_CXL3_PWR", 0, get_meb_sensor, false, {72, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x2E
  {"P12V_AUX_CXL4_PWR", 0, get_meb_sensor, false, {72, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x2F
  {"P12V_AUX_CXL1_PWR", 0, get_meb_sensor, false, {72, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x30
  {"P12V_AUX_CXL2_PWR", 0, get_meb_sensor, false, {72, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x31
  {"P12V_AUX_SW_4C_2_PWR", 0, get_meb_sensor, false, {80, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x32
  {"P12V_AUX_SW_4C_1_PWR", 0, get_meb_sensor, false, {80, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x33
  {"E1S_0_TEMP", 0, get_meb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x34
  {"E1S_1_TEMP", 0, get_meb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x35
  {"E1S_2_TEMP", 0, get_meb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x36
  {"E1S_3_TEMP", 0, get_meb_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x37
};

PAL_SENSOR_MAP meb_clx_sensor_map[] = {
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x00
  {"CXL Inlet Temp", 0, get_meb_jcn_sensor, false, {50, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x01
  {"CXL CTRL Temp", 0, get_meb_jcn_sensor, false, {90, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x02
  {"P12V_STBY_4CP_VOL", 0, get_meb_jcn_sensor, true, {0, 13.125, 0, 0, 11.875, 0, 0, 0},  VOLT}, // 0x03
  {"P3V3_STBY_4CP_VOL", 0, get_meb_jcn_sensor, true, {0, 3.465, 0, 0, 3.135, 0, 0, 0},  VOLT}, // 0x04
  {"P5V_STBY_VOL", 0, get_meb_jcn_sensor, true, {5.625, 5.25, 6, 4.375, 4.75, 4, 0, 0},  VOLT}, // 0x05
  {"P1V8_ASIC_VOL", 0, get_meb_jcn_sensor, false, {2.045, 0, 0, 0, 0, 0, 0, 0},  VOLT}, // 0x06
  {"P12V_STBY_VOL", 0, get_meb_jcn_sensor, true, {0, 13.125, 0, 0, 11.875, 0, 0, 0},  VOLT}, // 0x07
  {"P3V3_STBY_VOL", 0, get_meb_jcn_sensor, true, {0, 3.465, 0, 0, 3.135, 0, 0, 0},  VOLT}, // 0x08
  {"PVPP_AB_VOL", 0, get_meb_jcn_sensor, false, {2.875, 2.75, 3, 2.1875, 2.375, 2, 0, 0},  VOLT}, // 0x09
  {"PVTT_AB_VOL", 0, get_meb_jcn_sensor, false, {0, 0.632, 0, 0, 0.568, 0, 0, 0},  VOLT}, // 0x0A
  {"PVPP_CD_VOL", 0, get_meb_jcn_sensor, false, {2.875, 2.75, 3, 2.1875, 2.375, 2, 0, 0},  VOLT}, // 0x0B
  {"PVTT_CD_VOL", 0, get_meb_jcn_sensor, false, {0, 0.632, 0, 0, 0.568, 0, 0, 0},  VOLT}, // 0x0C
  {"P0V8_ASICA_VOL", 0, get_meb_jcn_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0},  VOLT}, // 0x0D
  {"P0V9_ASICA_VOL", 0, get_meb_jcn_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0},  VOLT}, // 0x0E
  {"P0V8_ASICD_VOL", 0, get_meb_jcn_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0},  VOLT}, // 0x0F
  {"PVDDQ_AB_VOL", 0, get_meb_jcn_sensor, false, {1.38, 1.26, 1.5, 1.02, 1.14, 0.9, 0, 0},  VOLT}, // 0x10
  {"PVDDQ_CD_VOL", 0, get_meb_jcn_sensor, false, {1.38, 1.26, 1.5, 1.02, 1.14, 0.9, 0, 0},  VOLT}, // 0x11
  {"P12V_STBY_4CP_CUR", 0, get_meb_jcn_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x12
  {"P3V3_STBY_4CP_CUR", 0, get_meb_jcn_sensor, true, {0.33424, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x13
  {"P0V8_ASICA_CUR", 0, get_meb_jcn_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x14
  {"P0V9_ASICA_CUR", 0, get_meb_jcn_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x15
  {"P0V8_ASICD_CUR", 0, get_meb_jcn_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x16
  {"PVDDQ_AB_CUR", 0, get_meb_jcn_sensor, false, {11.05, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x17
  {"PVDDQ_CD_CUR", 0, get_meb_jcn_sensor, false, {11.05, 0, 0, 0, 0, 0, 0, 0},  CURR}, // 0x18
  {"P12V_STBY_4CP_PWR", 0, get_meb_jcn_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x19
  {"P3V3_STBY_4CP_PWR", 0, get_meb_jcn_sensor, true, {0, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x1A
  {"P0V8_ASICA_PWR", 0, get_meb_jcn_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x1B
  {"P0V9_ASICA_PWR", 0, get_meb_jcn_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x1C
  {"P0V8_ASICD_PWR", 0, get_meb_jcn_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x1D
  {"PVDDQ_AB_PWR", 0, get_meb_jcn_sensor, false, {15.249, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x1E
  {"PVDDQ_CD_PWR", 0, get_meb_jcn_sensor, false, {15.249, 0, 0, 0, 0, 0, 0, 0},  POWER}, // 0x1F
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, // 0x20
  {"DIMM_A_TEMP", 0, get_meb_jcn_sensor, false, {95, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x21
  {"DIMM_B_TEMP", 0, get_meb_jcn_sensor, false, {85, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x22
  {"DIMM_C_TEMP", 0, get_meb_jcn_sensor, false, {85, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x23
  {"DIMM_D_TEMP", 0, get_meb_jcn_sensor, false, {85, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x24
};

PAL_SENSOR_MAP meb_e1s_sensor_map[] = {
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x00
  {"E1S_0_TEMP", 0, get_meb_jcn_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x01
  {"E1S_1_TEMP", 0, get_meb_jcn_sensor, false, {0, 0, 0, 0, 0, 0, 0, 0},  TEMP}, // 0x02
};

const uint8_t meb_sensor_list[] = {
  MEB_SENSOR_INLET_TEMP,
  MEB_SENSOR_OUTLET_TEMP,
  MEB_SENSOR_PU4_TEMP,
  MEB_SENSOR_P12V_AUX_VOL,
  MEB_SENSOR_P3V3_AUX_VOL,
  MEB_SENSOR_P1V2_AUX_VOL,
  MEB_SENSOR_P3V3_VOL,
  MEB_SENSOR_P12V_AUX_CXL1_VOL,
  MEB_SENSOR_P12V_AUX_CXL2_VOL,
  MEB_SENSOR_P12V_AUX_CXL3_VOL,
  MEB_SENSOR_P12V_AUX_CXL4_VOL,
  MEB_SENSOR_P12V_AUX_CXL5_VOL,
  MEB_SENSOR_P12V_AUX_CXL6_VOL,
  MEB_SENSOR_P12V_AUX_CXL7_VOL,
  MEB_SENSOR_P12V_AUX_CXL8_VOL,
  MEB_SENSOR_P12V_AUX_SSD1_VOL,
  MEB_SENSOR_P12V_AUX_SSD2_VOL,
  MEB_SENSOR_P12V_AUX_SSD3_VOL,
  MEB_SENSOR_P12V_AUX_SSD4_VOL,
  MEB_SENSOR_P12V_AUX_SW_4C_1_VOL,
  MEB_SENSOR_P12V_AUX_SW_4C_2_VOL,
  MEB_SENSOR_P12V_AUX_CUR,
  MEB_SENSOR_P12V_AUX_CXL1_CUR,
  MEB_SENSOR_P12V_AUX_CXL2_CUR,
  MEB_SENSOR_P12V_AUX_CXL3_CUR,
  MEB_SENSOR_P12V_AUX_CXL4_CUR,
  MEB_SENSOR_P12V_AUX_CXL5_CUR,
  MEB_SENSOR_P12V_AUX_CXL6_CUR,
  MEB_SENSOR_P12V_AUX_CXL7_CUR,
  MEB_SENSOR_P12V_AUX_CXL8_CUR,
  MEB_SENSOR_P12V_AUX_SSD1_CUR,
  MEB_SENSOR_P12V_AUX_SSD2_CUR,
  MEB_SENSOR_P12V_AUX_SSD3_CUR,
  MEB_SENSOR_P12V_AUX_SSD4_CUR,
  MEB_SENSOR_P12V_AUX_SW_4C_1_CUR,
  MEB_SENSOR_P12V_AUX_SW_4C_2_CUR,
  MEB_SENSOR_P12V_AUX_PWR,
  MEB_SENSOR_P12V_AUX_CXL1_PWR,
  MEB_SENSOR_P12V_AUX_CXL2_PWR,
  MEB_SENSOR_P12V_AUX_CXL3_PWR,
  MEB_SENSOR_P12V_AUX_CXL4_PWR,
  MEB_SENSOR_P12V_AUX_CXL5_PWR,
  MEB_SENSOR_P12V_AUX_CXL6_PWR,
  MEB_SENSOR_P12V_AUX_CXL7_PWR,
  MEB_SENSOR_P12V_AUX_CXL8_PWR,
  MEB_SENSOR_P12V_AUX_SSD1_PWR,
  MEB_SENSOR_P12V_AUX_SSD2_PWR,
  MEB_SENSOR_P12V_AUX_SSD3_PWR,
  MEB_SENSOR_P12V_AUX_SSD4_PWR,
  MEB_SENSOR_P12V_AUX_SW_4C_1_PWR,
  MEB_SENSOR_P12V_AUX_SW_4C_2_PWR,
  MEB_SENSOR_E1S_0_TEMP,
  MEB_SENSOR_E1S_1_TEMP,
  MEB_SENSOR_E1S_2_TEMP,
  MEB_SENSOR_E1S_3_TEMP,
};

const uint8_t meb_cxl_sensor_list[] = {
  CXL_SENSOR_Inlet_TEMP,
  CXL_SENSOR_CTRL_TEMP,
  CXL_SENSOR_P12V_STBY_4CP_VOL,
  CXL_SENSOR_P3V3_STBY_4CP_VOL,
  CXL_SENSOR_P5V_STBY_VOL,
  CXL_SENSOR_P1V8_ASIC_VOL,
  CXL_SENSOR_P12V_STBY_VOL,
  CXL_SENSOR_P3V3_STBY_VOL,
  CXL_SENSOR_PVPP_AB_VOL,
  CXL_SENSOR_PVTT_AB_VOL,
  CXL_SENSOR_PVPP_CD_VOL,
  CXL_SENSOR_PVTT_CD_VOL,
  CXL_SENSOR_P0V8_ASICA_VOL,
  CXL_SENSOR_P0V9_ASICA_VOL,
  CXL_SENSOR_P0V8_ASICD_VOL,
  CXL_SENSOR_PVDDQ_AB_VOL,
  CXL_SENSOR_PVDDQ_CD_VOL,
  CXL_SENSOR_P12V_STBY_4CP_CUR,
  CXL_SENSOR_P3V3_STBY_4CP_CUR,
  CXL_SENSOR_P0V8_ASICA_CUR,
  CXL_SENSOR_P0V9_ASICA_CUR,
  CXL_SENSOR_P0V8_ASICD_CUR,
  CXL_SENSOR_PVDDQ_AB_CUR,
  CXL_SENSOR_PVDDQ_CD_CUR,
  CXL_SENSOR_P12V_STBY_4CP_PWR,
  CXL_SENSOR_P3V3_STBY_4CP_PWR,
  CXL_SENSOR_P0V8_ASICA_PWR,
  CXL_SENSOR_P0V9_ASICA_PWR,
  CXL_SENSOR_P0V8_ASICD_PWR,
  CXL_SENSOR_PVDDQ_AB_PWR,
  CXL_SENSOR_PVDDQ_CD_PWR,
  //CXL_CTRL_TEMP,
  CXL_DIMM_A_TEMP,
  CXL_DIMM_B_TEMP,
  CXL_DIMM_C_TEMP,
  CXL_DIMM_D_TEMP,
};

const uint8_t meb_e1s_sensor_list[] = {
  E1S_SENSOR_E1S_0_TEMP,
  E1S_SENSOR_E1S_1_TEMP,
};

size_t meb_sensor_cnt = sizeof(meb_sensor_list)/sizeof(uint8_t);
size_t meb_cxl_sensor_cnt = sizeof(meb_cxl_sensor_list)/sizeof(uint8_t);
size_t meb_e1s_sensor_cnt = sizeof(meb_e1s_sensor_list)/sizeof(uint8_t);

