#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>

#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include <openbmc/ipmb.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-sensors.h>
#include <facebook/bic.h>
#include <facebook/expander.h>
#include "pal.h"
#include "pal_sensors.h"

const uint8_t iom_sensor_list[] = {
  IOM_Temp,
  IOM_INA_Vol,
  IOM_INA_Cur,
  IOM_INA_PWR,
};

const uint8_t mb_sensor_list[] = {
  //Temp
  MB_Inlet_Temp,
  MB_Outlet_Temp,
  MB_PCH_Temp,
  MB_CPU_Temp,
  MB_SOC_Therm_Margin,
  MB_CPU_TjMax,
  MB_SSD0_Temp,
  MB_HSC_Temp,
  MB_DIMMA_Temp,
  MB_DIMMC_Temp,
  MB_DIMME_Temp,
  MB_DIMMG_Temp,
  MB_VCCIN_SPS_Temp,
  MB_FIVRA_SPS_Temp,
  MB_EHV_SPS_Temp,
  MB_VCCD_SPS_Temp,
  MB_FAON_SPS_Temp,
  //Vol
  MB_P12V_STBY_Vol,
  MB_P3V_BAT_Vol,
  MB_P3V3_STBY_Vol,
  MB_P1V8_STBY_Vol,
  MB_P1V05_PCH_Vol,
  MB_P5V_STBY_Vol,
  MB_P12V_DIMM_Vol,
  MB_P1V2_STBY_Vol,
  MB_P3V3_M2_Vol,
  MB_HSC_Input_Vol,
  MB_VCCIN_VR_Vol,
  MB_FIVRA_VR_Vol,
  MB_EHV_VR_Vol,
  MB_VCCD_VR_Vol,
  MB_FAON_VR_Vol,
  //Cur
  MB_HSC_Output_Cur,
  MB_VCCIN_VR_Cur,
  MB_FIVRA_VR_Cur,
  MB_EHV_VR_Cur,
  MB_VCCD_VR_Cur,
  MB_VCCD_VR_Cur_in,
  MB_FAON_VR_Cur,
  //Power
  MB_CPU_PWR,
  MB_HSC_Input_Pwr,
  MB_DIMMA_PMIC_Pout,
  MB_DIMMC_PMIC_Pout,
  MB_DIMME_PMIC_Pout,
  MB_DIMMG_PMIC_Pout,
  MB_VCCIN_VR_Pout,
  MB_FIVRA_VR_Pout,
  MB_EHV_VR_Pout,
  MB_VCCD_VR_Pout,
  MB_FAON_VR_Pout,
};

const uint8_t scb_sensor_list[] = {
  SCB0_TEMP,
  SCB0_POWER_CURRENT,
  SCB0_EXP_P0V9_VOLTAGE,
  SCB0_IOC_P0V865_VOLTAGE,
  SCB0_IOC_TEMP,
  SCB0_EXP_TEMP,
  SCB1_TEMP,
  SCB1_POWER_CURRENT,
  SCB1_EXP_P0V9_VOLTAGE,
  SCB1_IOC_P0V865_VOLTAGE,
  SCB1_IOC_TEMP,
  SCB1_EXP_TEMP,
};

const uint8_t scm_sensor_list[] = {
  SCM_P12V,    
  SCM_P5V ,     
  SCM_P3V3,
  SCM_P2V5,
  SCM_P1V8,
  SCM_P1V2,
  SCM_P1V0,
};

const uint8_t hdbp_sensor_list[] = {
  SYSTEM_INLET_0_TEMP,
  SYSTEM_INLET_1_TEMP,
  HDD_L_REAR_TEMP,
  HDD_L_HSC_CURRENT,
  HDD_L_HSC_VOLTAGE,
  HDD_L_HSC_POWER,
  HDD_L_0_TEMP,
  HDD_L_1_TEMP,
  HDD_L_2_TEMP,
  HDD_L_3_TEMP,
  HDD_L_4_TEMP,
  HDD_L_5_TEMP,
  HDD_L_6_TEMP,
  HDD_L_7_TEMP,
  HDD_L_8_TEMP,
  HDD_L_9_TEMP,
  HDD_L_10_TEMP,
  HDD_L_11_TEMP,
  HDD_L_12_TEMP,
  HDD_L_13_TEMP,
  HDD_L_14_TEMP,
  HDD_L_15_TEMP,
  HDD_L_16_TEMP,
  HDD_L_17_TEMP,
  HDD_L_18_TEMP,
  HDD_L_19_TEMP,
  HDD_L_20_TEMP,
  HDD_L_21_TEMP,
  HDD_L_22_TEMP,
  HDD_L_23_TEMP,
  HDD_L_24_TEMP,
  HDD_L_25_TEMP,
  HDD_L_26_TEMP,
  HDD_L_27_TEMP,
  HDD_L_28_TEMP,
  HDD_L_29_TEMP,
  HDD_L_30_TEMP,
  HDD_L_31_TEMP,
  HDD_L_32_TEMP,
  HDD_L_33_TEMP,
  HDD_L_34_TEMP,
  HDD_L_35_TEMP,
  HDD_R_REAR_TEMP,
  HDD_R_HSC_POWER_CURRENT,
  HDD_R_HSC_VOLTAGE,
  HDD_R_HSC_POWER,
  HDD_R_0_TEMP,
  HDD_R_1_TEMP,
  HDD_R_2_TEMP,
  HDD_R_3_TEMP,
  HDD_R_4_TEMP,
  HDD_R_5_TEMP,
  HDD_R_6_TEMP,
  HDD_R_7_TEMP,
  HDD_R_8_TEMP,
  HDD_R_9_TEMP,
  HDD_R_10_TEMP,
  HDD_R_11_TEMP,
  HDD_R_12_TEMP,
  HDD_R_13_TEMP,
  HDD_R_14_TEMP,
  HDD_R_15_TEMP,
  HDD_R_16_TEMP,
  HDD_R_17_TEMP,
  HDD_R_18_TEMP,
  HDD_R_19_TEMP,
  HDD_R_20_TEMP,
  HDD_R_21_TEMP,
  HDD_R_22_TEMP,
  HDD_R_23_TEMP,
  HDD_R_24_TEMP,
  HDD_R_25_TEMP,
  HDD_R_26_TEMP,
  HDD_R_27_TEMP,
  HDD_R_28_TEMP,
  HDD_R_29_TEMP,
  HDD_R_30_TEMP,
  HDD_R_31_TEMP,
  HDD_R_32_TEMP,
  HDD_R_33_TEMP,
  HDD_R_34_TEMP,
  HDD_R_35_TEMP,
};


const uint8_t pdb_sensor_list[] = {
  SYSTEM_OUTLET_0_TEMP,
  SYSTEM_OUTLET_1_TEMP,
  SYSTEM_HSC_INPUT_VOLTAGE,
  SYSTEM_HSC_OUTPUT_VOLTAGE,
  SYSTEM_HSC_CURRENT,
  SYSTEM_HSC_POWER,
  FAN0_INLET_SPEED,
  FAN0_OUTLET_SPEED,
  FAN0_POWER_CURRENT,
  FAN1_INLET_SPEED,
  FAN1_OUTLET_SPEED,
  FAN1_POWER_CURRENT,
  FAN2_INLET_SPEED,
  FAN2_OUTLET_SPEED,
  FAN2_POWER_CURRENT,
  FAN3_INLET_SPEED,
  FAN3_OUTLET_SPEED,
  FAN3_POWER_CURRENT,
};

size_t iom_sensor_cnt = ARRAY_SIZE(iom_sensor_list);
size_t mb_sensor_cnt = ARRAY_SIZE(mb_sensor_list);
size_t scb_sensor_cnt = ARRAY_SIZE(scb_sensor_list);
size_t scm_sensor_cnt = ARRAY_SIZE(scm_sensor_list);
size_t hdbp_sensor_cnt = ARRAY_SIZE(hdbp_sensor_list);
size_t pdb_sensor_cnt = ARRAY_SIZE(pdb_sensor_list);

//{SensorNum, SensorName, ID, FUNCTION, PWR_STATUS, {UCR, UNC, UNR, LCR, LNC, LNR, Pos, Neg}, Unit}
PAL_SENSOR_MAP iom_sensor_map[] = {
  {IOM_Temp,  "Temp",    0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {IOM_INA_Vol, "INA Vol", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  {IOM_INA_Cur, "INA Cur", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},
  {IOM_INA_PWR, "INA PWR", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},
};

PAL_SENSOR_MAP mb_sensor_map[] = {
  {0x1,  "MB Inlet Temp",    0, NULL, 0, {48 , 150 , 0, 0, 0, 0, 0, 0}, TEMP},
  {0x2,  "MB Outlet Temp",   0, NULL, 0, {93 , 150 , 0, 0, 0, 0, 0, 0}, TEMP},
  {0x4,  "PCH Temp",         0, NULL, 0, {74 , 0   , 0, 0, 0, 0, 0, 0}, TEMP},
  {0x5,  "CPU Temp",         0, NULL, 0, {84 , 0   , 0, 0, 0, 0, 0, 0}, TEMP},
  {0x6,  "SOC Therm Margin", 0, NULL, 0, {0  , 125 , 0, 0, 0, 0, 0, 0}, TEMP},
  {0x7,  "CPU TjMax",        0, NULL, 0, {0  , 125 , 0, 0, 0, 0, 0, 0}, TEMP},
  {0x8,  "SSD0 Temp",        0, NULL, 0, {75 , 0   , 0, 0, 0, 0, 0, 0}, TEMP},
  {0x9,  "HSC Temp",         0, NULL, 0, {86 , 125 , 0, 0, 0, 0, 0, 0}, TEMP},
  {0xA,  "DIMMA Temp",       0, NULL, 0, {85 , 0   , 0, 0, 0, 0, 0, 0}, TEMP},
  {0xB,  "DIMMC Temp",       0, NULL, 0, {85 , 0   , 0, 0, 0, 0, 0, 0}, TEMP},
  {0xC,  "DIMME Temp",       0, NULL, 0, {85 , 0   , 0, 0, 0, 0, 0, 0}, TEMP},
  {0xD,  "DIMMG Temp",       0, NULL, 0, {85 , 0   , 0, 0, 0, 0, 0, 0}, TEMP},
  {0xE,  "VCCIN SPS Temp",   0, NULL, 0, {100, 125 , 0, 0, 0, 0, 0, 0}, TEMP},
  {0xF,  "FIVRA SPS Temp",   0, NULL, 0, {100, 125 , 0, 0, 0, 0, 0, 0}, TEMP},
  {0x10, "EHV SPS Temp",     0, NULL, 0, {100, 125 , 0, 0, 0, 0, 0, 0}, TEMP},
  {0x11, "VCCD SPS Temp",    0, NULL, 0, {100, 125 , 0, 0, 0, 0, 0, 0}, TEMP},
  {0x12, "FAON SPS Temp",    0, NULL, 0, {100, 125 , 0, 0, 0, 0, 0, 0}, TEMP},
  {0x20, "P12V_STBY Vol",    0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, VOLT},      
  {0x21, "P3V_BAT Vol",      0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, VOLT},     
  {0x22, "P3V3_STBY Vol",    0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, VOLT},       
  {0x23, "P1V8_STBY Vol",    0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, VOLT},       
  {0x24, "P1V05_PCH Vol",    0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, VOLT},       
  {0x25, "P5V_STBY Vol",     0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, VOLT},       
  {0x26, "P12V_DIMM Vol",    0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, VOLT},       
  {0x27, "P1V2_STBY Vol",    0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, VOLT},       
  {0x28, "P3V3_M2 Vol",      0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, VOLT},     
  {0x29, "HSC Input Vol",    0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, VOLT},       
  {0x2A, "VCCIN VR Vol",     0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, VOLT},       
  {0x2B, "FIVRA VR Vol",     0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, VOLT},       
  {0x2C, "EHV VR Vol",       0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, VOLT},     
  {0x2D, "VCCD VR Vol",      0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, VOLT},     
  {0x2E, "FAON VR Vol",      0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, VOLT},     
  {0x40, "HSC Output Cur",   0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, CURR},       
  {0x41, "VCCIN VR Cur",     0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, CURR},     
  {0x42, "FIVRA VR Cur",     0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, CURR},     
  {0x43, "EHV VR Cur",       0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, CURR},   
  {0x44, "VCCD VR Cur",      0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, CURR},    
  {0x45, "VCCD VR Cur in",   0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, CURR},       
  {0x46, "FAON VR Cur",      0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, CURR},
  {0x60, "CPU PWR",          0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, POWER},   
  {0x61, "HSC Input Pwr",    0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, POWER},            
  {0x62, "DIMMA PMIC_Pout",  0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, POWER},              
  {0x63, "DIMMC PMIC_Pout",  0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, POWER},              
  {0x64, "DIMME PMIC_Pout",  0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, POWER},              
  {0x65, "DIMMG PMIC_Pout",  0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, POWER},              
  {0x66, "VCCIN VR Pout",    0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, POWER},            
  {0x67, "FIVRA VR Pout",    0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, POWER},            
  {0x68, "EHV VR Pout",      0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, POWER},          
  {0x69, "VCCD VR Pout",     0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, POWER},           
  {0x6A, "FAON VR Pout",     0, NULL, 0, {0, 0 , 0, 0, 0, 0, 0, 0}, POWER},           
};

PAL_SENSOR_MAP scb_sensor_map[] = {
  {SCB0_TEMP,               "0 TEMP",    0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {SCB0_POWER_CURRENT,      "0 POWER CURRENT", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},
  {SCB0_EXP_P0V9_VOLTAGE,   "0 EXP P0V9 VOLTAGE", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  {SCB0_IOC_P0V865_VOLTAGE, "0 IOC P0V865 VOLTAGE", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  {SCB0_IOC_TEMP,           "0 IOC TEMP", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {SCB0_EXP_TEMP,           "0 EXP TEMP", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {SCB1_TEMP,               "1 TEMP",    0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {SCB1_POWER_CURRENT,      "1 POWER CURRENT", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},
  {SCB1_EXP_P0V9_VOLTAGE,   "1 EXP P0V9 VOLTAGE", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  {SCB1_IOC_P0V865_VOLTAGE, "1 IOC P0V865 VOLTAGE", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  {SCB1_IOC_TEMP,           "1 IOC TEMP", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {SCB1_EXP_TEMP,           "1 EXP TEMP", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
};

PAL_SENSOR_MAP scm_sensor_map[] = {
//{SensorNum, SensorName, ID, FUNCTION, PWR_STATUS, {UCR, UNC, UNR, LCR, LNC, LNR, Pos, Neg}, Unit}
  {SCM_P12V, "P12V",    ADC0,  read_adc_val, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  {SCM_P5V,  "P5V",     ADC1,  read_adc_val, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  {SCM_P3V3, "P3V3",    ADC2,  read_adc_val, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  {SCM_P2V5, "P2V5",    ADC3,  read_adc_val, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  {SCM_P1V8, "P1V8",    ADC4,  read_adc_val, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  {SCM_P1V2, "P1V2",    ADC6,  read_adc_val, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  {SCM_P1V0, "P1V0",    ADC10, read_adc_val, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
};

PAL_SENSOR_MAP hdbp_sensor_map[] = {
//{SensorNum, SensorName, ID, FUNCTION, PWR_STATUS, {UCR, UNC, UNR, LCR, LNC, LNR, Pos, Neg}, Unit}
  {SYSTEM_INLET_0_TEMP, "Sys Inlet 0 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {SYSTEM_INLET_1_TEMP, "Sys Inlet 1 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_REAR_TEMP, "HDD-L Rear Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_HSC_CURRENT, "HDD-L HSC Current", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},
  {HDD_L_HSC_VOLTAGE, "HDD-L HSC Voltage", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  {HDD_L_HSC_POWER, "HDD-L HSC Power", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},
  {HDD_L_0_TEMP, "HDD-L 0 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_1_TEMP, "HDD-L 1 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_2_TEMP, "HDD-L 2 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_3_TEMP, "HDD-L 3 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_4_TEMP, "HDD-L 4 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_5_TEMP, "HDD-L 5 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_6_TEMP, "HDD-L 6 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_7_TEMP, "HDD-L 7 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_8_TEMP, "HDD-L 8 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_9_TEMP, "HDD-L 9 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_10_TEMP, "HDD-L 10 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_11_TEMP, "HDD-L 11 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_12_TEMP, "HDD-L 12 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_13_TEMP, "HDD-L 13 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_14_TEMP, "HDD-L 14 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_15_TEMP, "HDD-L 15 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_16_TEMP, "HDD-L 16 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_17_TEMP, "HDD-L 17 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_18_TEMP, "HDD-L 18 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_19_TEMP, "HDD-L 19 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_20_TEMP, "HDD-L 20 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_21_TEMP, "HDD-L 21 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_22_TEMP, "HDD-L 22 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_23_TEMP, "HDD-L 23 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_24_TEMP, "HDD-L 24 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_25_TEMP, "HDD-L 25 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_26_TEMP, "HDD-L 26 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_27_TEMP, "HDD-L 27 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_28_TEMP, "HDD-L 28 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_29_TEMP, "HDD-L 29 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_30_TEMP, "HDD-L 30 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_31_TEMP, "HDD-L 31 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_32_TEMP, "HDD-L 32 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_33_TEMP, "HDD-L 33 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_34_TEMP, "HDD-L 34 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_L_35_TEMP, "HDD-L 35 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_REAR_TEMP, "HDD-R Rear Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_HSC_POWER_CURRENT, "HDD-R HSC Power Current", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},
  {HDD_R_HSC_VOLTAGE, "HDD-R HSC Voltage", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  {HDD_R_HSC_POWER, "HDD-R HSC Power", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},
  {HDD_R_0_TEMP, "HDD-R 0 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_1_TEMP, "HDD-R 1 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_2_TEMP, "HDD-R 2 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_3_TEMP, "HDD-R 3 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_4_TEMP, "HDD-R 4 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_5_TEMP, "HDD-R 5 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_6_TEMP, "HDD-R 6 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_7_TEMP, "HDD-R 7 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_8_TEMP, "HDD-R 8 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_9_TEMP, "HDD-R 9 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_10_TEMP, "HDD-R 10 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_11_TEMP, "HDD-R 11 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_12_TEMP, "HDD-R 12 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_13_TEMP, "HDD-R 13 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_14_TEMP, "HDD-R 14 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_15_TEMP, "HDD-R 15 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_16_TEMP, "HDD-R 16 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_17_TEMP, "HDD-R 17 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_18_TEMP, "HDD-R 18 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_19_TEMP, "HDD-R 19 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_20_TEMP, "HDD-R 20 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_21_TEMP, "HDD-R 21 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_22_TEMP, "HDD-R 22 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_23_TEMP, "HDD-R 23 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_24_TEMP, "HDD-R 24 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_25_TEMP, "HDD-R 25 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_26_TEMP, "HDD-R 26 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_27_TEMP, "HDD-R 27 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_28_TEMP, "HDD-R 28 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_29_TEMP, "HDD-R 29 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_30_TEMP, "HDD-R 30 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_31_TEMP, "HDD-R 31 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_32_TEMP, "HDD-R 32 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_33_TEMP, "HDD-R 33 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_34_TEMP, "HDD-R 34 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {HDD_R_35_TEMP, "HDD-R 35 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
};

PAL_SENSOR_MAP pdb_sensor_map[] = {
//{SensorNum, SensorName, ID, FUNCTION, PWR_STATUS, {UCR, UNC, UNR, LCR, LNC, LNR, Pos, Neg}, Unit}
  {SYSTEM_OUTLET_0_TEMP, "Sys Outlet 0 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {SYSTEM_OUTLET_1_TEMP, "Sys Outlet 1 Temp", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  {SYSTEM_HSC_INPUT_VOLTAGE, "Sys HSC Input Voltage", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  {SYSTEM_HSC_OUTPUT_VOLTAGE, "Sys HSC Output Voltage", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},
  {SYSTEM_HSC_CURRENT, "Sys HSC Current", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},
  {SYSTEM_HSC_POWER, "Sys HSC Power", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},
  {FAN0_INLET_SPEED, "FAN0 Inlet Speed", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, FAN},
  {FAN0_OUTLET_SPEED, "FAN0 Outlet Speed ", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, FAN},
  {FAN0_POWER_CURRENT, "FAN0 Power Current", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},
  {FAN1_INLET_SPEED, "FAN1 Inlet Speed", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, FAN},
  {FAN1_OUTLET_SPEED, "FAN1 Outlet Speed", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, FAN},
  {FAN1_POWER_CURRENT, "FAN1 Power Current", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},
  {FAN2_INLET_SPEED, "FAN2 Inlet Speed", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, FAN},
  {FAN2_OUTLET_SPEED, "FAN2 Outlet Speed", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, FAN},
  {FAN2_POWER_CURRENT, "FAN2 Power Current", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},
  {FAN3_INLET_SPEED, "FAN3 Inlet Speed", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, FAN},
  {FAN3_OUTLET_SPEED, "FAN3 Outlet Speed", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, FAN},
  {FAN3_POWER_CURRENT, "FAN3 Power Current", 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},
};

FRU_MAP sensor_map[] = {
  // The order must be same as fbwc_common.h
  {0, NULL},    //FRU_ALL
  {ARRAY_SIZE(iom_sensor_map), iom_sensor_map},         //FRU_IOM
  {ARRAY_SIZE(mb_sensor_map), mb_sensor_map},           //FRU_MB
  {ARRAY_SIZE(scb_sensor_map), scb_sensor_map},         //FRU_SCB
  {0, NULL},    // FRU_BMC
  {ARRAY_SIZE(scm_sensor_map), scm_sensor_map},         //FRU_SCM
  {0, NULL},    // FRU_BSM
  {ARRAY_SIZE(hdbp_sensor_map), hdbp_sensor_map},    // FRU_HDBP
  {ARRAY_SIZE(pdb_sensor_map), pdb_sensor_map},         // FRU_PDB
  {0, NULL},    // FRU_NIC
  {0, NULL},    // FRU_FAN0
  {0, NULL},    // FRU_FAN1
  {0, NULL},    // FRU_FAN2
  {0, NULL},    // FRU_FAN3
};


int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {

  switch(fru) {
  case FRU_IOM:
    *sensor_list = (uint8_t *) iom_sensor_list;
    *cnt = iom_sensor_cnt;
    break;
  case FRU_MB:
    *sensor_list = (uint8_t *) mb_sensor_list;
    *cnt = mb_sensor_cnt;
    break;
  case FRU_SCB:
    *sensor_list = (uint8_t *) scb_sensor_list;
    *cnt = scb_sensor_cnt;
    break;
  case FRU_SCM:
    *sensor_list = (uint8_t *) scm_sensor_list;
    *cnt = scm_sensor_cnt;
    break;
  case FRU_HDBP:
    *sensor_list = (uint8_t *) hdbp_sensor_list;
    *cnt = hdbp_sensor_cnt;
    break;
  case FRU_PDB:
    *sensor_list = (uint8_t *) pdb_sensor_list;
    *cnt = pdb_sensor_cnt;
    break;

  default:
    // Nothing to read yet.
    *sensor_list = NULL;
    *cnt = 0;
    syslog(LOG_WARNING, "%s() failed, fru:%d is not expected.", __func__, fru);
    break;
  }

  return 0;
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  int ret = 0;
  int found = 0;
  int index = 0;
  char fru_name[MAX_FRU_NAME_STR] = {0};

  ret = pal_get_fru_name(fru, fru_name);
  if (ret) {
    syslog(LOG_WARNING, "%s() failed, fru:%d is invalid.", __func__, fru);
    return -1;
  }

  for (int i = 0; i < strlen(fru_name); i++) {
    fru_name[i] = toupper(fru_name[i]);
  }

  for (index = 0; index < sensor_map[fru].map_size; index++) {
    if (sensor_map[fru].map[index].snr_num == sensor_num) {
      found = 1;
      break;
    }
  }

  if (found == 0) {
    syslog(LOG_WARNING, "%s() failed, fru:%d sensor_num:%d map not found.", __func__, fru, sensor_num);
    return -1;
  }

  sprintf(name, "%s %s", fru_name, sensor_map[fru].map[index].snr_name);
  return 0;
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  uint8_t scale = 0;

  for (int i = 0; i < sensor_map[fru].map_size; i++) {
    if (sensor_map[fru].map[i].snr_num == sensor_num) {
      scale = sensor_map[fru].map[i].units;
      break;
    }
  }
  
  switch(scale) {
    case TEMP:
      sprintf(units, "C");
      break;
    case FAN:
      sprintf(units, "RPM");
      break;
    case VOLT:
      sprintf(units, "Volts");
      break;
    case CURR:
      sprintf(units, "Amps");
      break;
    case POWER:
      sprintf(units, "Watts");
      break;
    default:
      return -1;
  }

  return 0;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  float *val = (float*) value;
  int found = 0;
  PAL_SENSOR_MAP map;

  for (int i = 0; i < sensor_map[fru].map_size; i++) {
    if (sensor_map[fru].map[i].snr_num == sensor_num) {
      map = sensor_map[fru].map[i];
      found = 1;
      break;
    }
  }
  
  if (found == 0) {
    syslog(LOG_WARNING, "%s() failed, fru:%d sensor_num:%d map not found.", __func__, fru, sensor_num);
    return -1;
  }
  
  switch(thresh) {
    case UCR_THRESH:
      *val = map.snr_thresh.ucr_thresh;
      break;
    case UNC_THRESH:
      *val = map.snr_thresh.unc_thresh;
      break;
    case UNR_THRESH:
      *val = map.snr_thresh.unr_thresh;
      break;
    case LCR_THRESH:
      *val = map.snr_thresh.lcr_thresh;
      break;
    case LNC_THRESH:
      *val = map.snr_thresh.lnc_thresh;
      break;
    case LNR_THRESH:
      *val = map.snr_thresh.lnr_thresh;
      break;
    case POS_HYST:
      *val = map.snr_thresh.pos_hyst;
      break;
    case NEG_HYST:
      *val = map.snr_thresh.neg_hyst;
      break;
    default:
      return -1;
  }
  return 0;
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {
  int found = 0;
  PAL_SENSOR_MAP map;

  switch(fru) {
    case FRU_IOM:
      return bic_get_sensor_reading(sensor_num, (float *)value);

    case FRU_MB:
      return bic_get_sensor_reading(sensor_num, (float *)value);
      
    case FRU_SCB:
      return exp_get_sensor_reading(sensor_num, (float *)value);
      
    case FRU_SCM:
      for (int i = 0; i < sensor_map[fru].map_size; i++) {
        if (sensor_map[fru].map[i].snr_num == sensor_num) {
          map = sensor_map[fru].map[i];
          found = 1;
          break;
        }
      }
      
      if (found == 0) {
        syslog(LOG_WARNING, "%s() failed, fru:%d sensor_num:%d map not found.", __func__, fru, sensor_num);
        return ERR_SENSOR_NA;
      }
      
      return map.read_sensor(map.snr_name, (float*)value);
      
    case FRU_HDBP:
      return exp_get_sensor_reading(sensor_num, (float *)value);
      
    case FRU_PDB:
      return exp_get_sensor_reading(sensor_num, (float *)value);

    default:
      syslog(LOG_WARNING, "%s() failed, fru:%d is not expected.", __func__, fru);
      return ERR_SENSOR_NA;
  }

  return 0;
}

int
read_adc_val(char *snr_name, float *value) {
  return sensors_read_adc(snr_name, value);
}
