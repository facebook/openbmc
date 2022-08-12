#ifndef __PAL_SENSORS_H__
#define __PAL_SENSORS_H__

#include <openbmc/obmc_pal_sensors.h>

typedef struct {
  float ucr_thresh;
  float unc_thresh;
  float unr_thresh;
  float lcr_thresh;
  float lnc_thresh;
  float lnr_thresh;
  float pos_hyst;
  float neg_hyst;
} PAL_SENSOR_THRESHOLD;

enum {
  TEMP = 1,
  CURR,
  VOLT,
  FAN,
  POWER,
  STATE,
};

typedef struct {
  uint8_t snr_num;
  char* snr_name;
  uint8_t id;
  int (*read_sensor) (char* snr_name, float *value);
  uint8_t stby_read;
  PAL_SENSOR_THRESHOLD snr_thresh;
  uint8_t units;
} PAL_SENSOR_MAP;

typedef struct {
  uint8_t map_size;
  PAL_SENSOR_MAP *map;
} FRU_MAP;


enum {
  MB_Inlet_Temp = 0x01,
  MB_Outlet_Temp = 0x02,
  IOM_Temp = 0x03,            //IOM
  MB_PCH_Temp = 0x04,
  MB_CPU_Temp = 0x05,
  MB_SOC_Therm_Margin = 0x06,
  MB_CPU_TjMax = 0x07,
  MB_SSD0_Temp = 0x08,
  MB_HSC_Temp = 0x09,
  MB_DIMMA_Temp = 0x0A,
  MB_DIMMC_Temp = 0x0B,
  MB_DIMME_Temp = 0x0C,
  MB_DIMMG_Temp = 0x0D,
  MB_VCCIN_SPS_Temp = 0x0E,
  MB_FIVRA_SPS_Temp = 0x0F,
  MB_EHV_SPS_Temp = 0x10,
  MB_VCCD_SPS_Temp = 0x11,
  MB_FAON_SPS_Temp = 0x12,
  //0x13 ~ 0x1F
  MB_P12V_STBY_Vol = 0x20,
  MB_P3V_BAT_Vol = 0x21,
  MB_P3V3_STBY_Vol = 0x22,
  MB_P1V8_STBY_Vol = 0x23,
  MB_P1V05_PCH_Vol = 0x24,
  MB_P5V_STBY_Vol = 0x25,
  MB_P12V_DIMM_Vol = 0x26,
  MB_P1V2_STBY_Vol = 0x27,
  MB_P3V3_M2_Vol = 0x28,
  MB_HSC_Input_Vol = 0x29,
  MB_VCCIN_VR_Vol = 0x2A,
  MB_FIVRA_VR_Vol = 0x2B,
  MB_EHV_VR_Vol = 0x2C,
  MB_VCCD_VR_Vol = 0x2D,
  MB_FAON_VR_Vol = 0x2E,
  IOM_INA_Vol = 0x2F,         //IOM
  //0x30 ~ 0x3F
  MB_HSC_Output_Cur = 0x40,
  MB_VCCIN_VR_Cur = 0x41,
  MB_FIVRA_VR_Cur = 0x42,
  MB_EHV_VR_Cur = 0x43,
  MB_VCCD_VR_Cur = 0x44,
  MB_VCCD_VR_Cur_in = 0x45,
  MB_FAON_VR_Cur = 0x46,
  IOM_INA_Cur = 0x47,         //IOM
  //0x48 ~ 0x5F
  MB_CPU_PWR = 0x60,
  MB_HSC_Input_Pwr = 0x61,
  MB_DIMMA_PMIC_Pout = 0x62,
  MB_DIMMC_PMIC_Pout = 0x63,
  MB_DIMME_PMIC_Pout = 0x64,
  MB_DIMMG_PMIC_Pout = 0x65,
  MB_VCCIN_VR_Pout = 0x66,
  MB_FIVRA_VR_Pout = 0x67,
  MB_EHV_VR_Pout = 0x68,
  MB_VCCD_VR_Pout = 0x69,
  MB_FAON_VR_Pout = 0x6A,
  IOM_INA_PWR = 0x6B,         //IOM
};

enum {
  ////
  SYSTEM_OUTLET_0_TEMP = 3,
  SYSTEM_OUTLET_1_TEMP = 4,
  SYSTEM_HSC_INPUT_VOLTAGE = 5,
  SYSTEM_HSC_OUTPUT_VOLTAGE = 6 ,
  SYSTEM_HSC_CURRENT = 7,
  SYSTEM_HSC_POWER = 8,
  FAN0_INLET_SPEED = 9,
  FAN0_OUTLET_SPEED = 10,
  FAN0_POWER_CURRENT = 11,
  FAN1_INLET_SPEED = 12,
  FAN1_OUTLET_SPEED = 13,
  FAN1_POWER_CURRENT = 14,
  FAN2_INLET_SPEED = 15,
  FAN2_OUTLET_SPEED = 16,
  FAN2_POWER_CURRENT = 17,
  FAN3_INLET_SPEED = 18,
  FAN3_OUTLET_SPEED = 19,
  FAN3_POWER_CURRENT = 20,
  ////
  SCB0_TEMP = 125,
  SCB0_POWER_CURRENT = 126,
  SCB0_EXP_P0V9_VOLTAGE = 127,
  SCB0_IOC_P0V865_VOLTAGE = 128,
  SCB0_IOC_TEMP = 129,
  SCB0_EXP_TEMP = 130,
  SCB1_TEMP = 131,
  SCB1_POWER_CURRENT = 132,
  SCB1_EXP_P0V9_VOLTAGE = 133,
  SCB1_IOC_P0V865_VOLTAGE = 134,
  SCB1_IOC_TEMP = 135,
  SCB1_EXP_TEMP = 136,
  //
  SYSTEM_INLET_0_TEMP = 1,
  SYSTEM_INLET_1_TEMP = 2,
  HDD_L_REAR_TEMP = 151,
  HDD_L_HSC_CURRENT = 152,
  HDD_L_HSC_VOLTAGE = 153,
  HDD_L_HSC_POWER = 154,
  HDD_L_0_TEMP = 155,
  HDD_L_1_TEMP = 156,
  HDD_L_2_TEMP = 157,
  HDD_L_3_TEMP = 158,
  HDD_L_4_TEMP = 159,
  HDD_L_5_TEMP = 160,
  HDD_L_6_TEMP = 161,
  HDD_L_7_TEMP = 162,
  HDD_L_8_TEMP = 163,
  HDD_L_9_TEMP = 164,
  HDD_L_10_TEMP = 165,
  HDD_L_11_TEMP = 166,
  HDD_L_12_TEMP = 167,
  HDD_L_13_TEMP = 168,
  HDD_L_14_TEMP = 169,
  HDD_L_15_TEMP = 170,
  HDD_L_16_TEMP = 171,
  HDD_L_17_TEMP = 172,
  HDD_L_18_TEMP = 173,
  HDD_L_19_TEMP = 174,
  HDD_L_20_TEMP = 175,
  HDD_L_21_TEMP = 176,
  HDD_L_22_TEMP = 177,
  HDD_L_23_TEMP = 178,
  HDD_L_24_TEMP = 179,
  HDD_L_25_TEMP = 180,
  HDD_L_26_TEMP = 181,
  HDD_L_27_TEMP = 182,
  HDD_L_28_TEMP = 183,
  HDD_L_29_TEMP = 184,
  HDD_L_30_TEMP = 185,
  HDD_L_31_TEMP = 186,
  HDD_L_32_TEMP = 187,
  HDD_L_33_TEMP = 188,
  HDD_L_34_TEMP = 189,
  HDD_L_35_TEMP = 190,
  HDD_R_REAR_TEMP = 191,
  HDD_R_HSC_POWER_CURRENT = 192,
  HDD_R_HSC_VOLTAGE = 193,
  HDD_R_HSC_POWER = 194,
  HDD_R_0_TEMP = 195,
  HDD_R_1_TEMP = 196,
  HDD_R_2_TEMP = 197,
  HDD_R_3_TEMP = 198,
  HDD_R_4_TEMP = 199,
  HDD_R_5_TEMP = 200,
  HDD_R_6_TEMP = 201,
  HDD_R_7_TEMP = 202,
  HDD_R_8_TEMP = 203,
  HDD_R_9_TEMP = 204,
  HDD_R_10_TEMP = 205,
  HDD_R_11_TEMP = 206,
  HDD_R_12_TEMP = 207,
  HDD_R_13_TEMP = 208,
  HDD_R_14_TEMP = 209,
  HDD_R_15_TEMP = 210,
  HDD_R_16_TEMP = 211,
  HDD_R_17_TEMP = 212,
  HDD_R_18_TEMP = 213,
  HDD_R_19_TEMP = 214,
  HDD_R_20_TEMP = 215,
  HDD_R_21_TEMP = 216,
  HDD_R_22_TEMP = 217,
  HDD_R_23_TEMP = 218,
  HDD_R_24_TEMP = 219,
  HDD_R_25_TEMP = 220,
  HDD_R_26_TEMP = 221,
  HDD_R_27_TEMP = 222,
  HDD_R_28_TEMP = 223,
  HDD_R_29_TEMP = 224,
  HDD_R_30_TEMP = 225,
  HDD_R_31_TEMP = 226,
  HDD_R_32_TEMP = 227,
  HDD_R_33_TEMP = 228,
  HDD_R_34_TEMP = 229,
  HDD_R_35_TEMP = 230,
};


enum {
  SCM_P12V = 0x1,
  SCM_P5V  = 0x2,
  SCM_P3V3 = 0x3,
  SCM_P2V5 = 0x4,
  SCM_P1V8 = 0x5,
  SCM_P1V2 = 0x6,
  SCM_P1V0 = 0x7,
  SCM_TEMP = 0x8,
};

enum {
  ADC0 = 0,
  ADC1,
  ADC2,
  ADC3,
  ADC4,
  ADC5,
  ADC6,
  ADC7,
  ADC8,
  ADC9,
  ADC10,
  ADC11,
  ADC12,
  ADC13,
  ADC14,
  ADC15,
  ADC_NUM_CNT,
};


int read_adc_val(char *snr_name, float *value);

#endif /* __PAL_SENSORS_H__ */
