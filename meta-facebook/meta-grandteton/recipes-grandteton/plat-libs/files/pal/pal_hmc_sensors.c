#include <stdint.h>
#include <stdio.h>
#include <syslog.h>
#include "pal.h"
#include "pal_hmc_sensors.h"
#include <openbmc/hgx.h>

struct hmc_snr_info {
  char *evt_hmc_comp;
  char *evt_snr_name;

  char *dvt_hmc_comp;
  char *dvt_snr_name;
} HMC_SNR_INFO[] = {
  //Baseboard
  {"NULL", "NULL", "NULL", "NULL"},
  {"Baseboard", "PWR_GB_HSC0", "HGX_Chassis_0", "HGX_Chassis_0_HSC_0_Power_0"},
  {"Baseboard", "PWR_GB_HSC1", "HGX_Chassis_0", "HGX_Chassis_0_HSC_1_Power_0"},
  {"Baseboard", "PWR_GB_HSC2", "HGX_Chassis_0", "HGX_Chassis_0_HSC_2_Power_0"},
  {"Baseboard", "PWR_GB_HSC3", "HGX_Chassis_0", "HGX_Chassis_0_HSC_3_Power_0"},
  {"Baseboard", "PWR_GB_HSC4", "HGX_Chassis_0", "HGX_Chassis_0_HSC_4_Power_0"},
  {"Baseboard", "PWR_GB_HSC5", "HGX_Chassis_0", "HGX_Chassis_0_HSC_5_Power_0"},
  {"Baseboard", "PWR_GB_HSC6", "HGX_Chassis_0", "HGX_Chassis_0_HSC_6_Power_0"},
  {"Baseboard", "PWR_GB_HSC7", "HGX_Chassis_0", "HGX_Chassis_0_HSC_7_Power_0"},
  {"Baseboard", "PWR_GB_HSC8", "HGX_Chassis_0", "HGX_Chassis_0_HSC_8_Power_0"},
  {"Baseboard", "PWR_GB_HSC9", "HGX_Chassis_0", "HGX_Chassis_0_HSC_9_Power_0"},
  {"Baseboard", "PWR_GB_HSC10","HGX_Chassis_0", "HGX_Chassis_0_StandbyHSC_0_Power_0"},
  {"Baseboard", "Total_Power" ,"HGX_Chassis_0", "HGX_Chassis_0_TotalHSC_Power_0" },
  {"Baseboard", "Total_GPU_Power",   "HGX_Chassis_0", "HGX_Chassis_0_TotalGPU_Power_0"},
  {"Baseboard", "Altitude_Pressure0","HGX_Chassis_0", "HGX_Chassis_0_AltitudePressure_0"},
  {"NULL", "NULL", "NULL", "NULL"},

  {"Baseboard", "TEMP_GB_FPGA", "HGX_FPGA_0"   , "HGX_FPGA_0_TEMP_0"},
  {"Baseboard", "TEMP_GB_HSC0", "HGX_Chassis_0", "HGX_Chassis_0_HSC_0_Temp_0"},
  {"Baseboard", "TEMP_GB_HSC1", "HGX_Chassis_0", "HGX_Chassis_0_HSC_1_Temp_0"},
  {"Baseboard", "TEMP_GB_HSC2", "HGX_Chassis_0", "HGX_Chassis_0_HSC_2_Temp_0"},
  {"Baseboard", "TEMP_GB_HSC3", "HGX_Chassis_0", "HGX_Chassis_0_HSC_3_Temp_0"},
  {"Baseboard", "TEMP_GB_HSC4", "HGX_Chassis_0", "HGX_Chassis_0_HSC_4_Temp_0"},
  {"Baseboard", "TEMP_GB_HSC5", "HGX_Chassis_0", "HGX_Chassis_0_HSC_5_Temp_0"},
  {"Baseboard", "TEMP_GB_HSC6", "HGX_Chassis_0", "HGX_Chassis_0_HSC_6_Temp_0"},
  {"Baseboard", "TEMP_GB_HSC7", "HGX_Chassis_0", "HGX_Chassis_0_HSC_7_Temp_0"},
  {"Baseboard", "TEMP_GB_HSC8", "HGX_Chassis_0", "HGX_Chassis_0_HSC_8_Temp_0"},
  {"Baseboard", "TEMP_GB_HSC9", "HGX_Chassis_0", "HGX_Chassis_0_HSC_9_Temp_0"},
  {"Baseboard", "TEMP_GB_HSC10","HGX_Chassis_0", "HGX_Chassis_0_StandbyHSC_0_Temp_0"},
  {"Baseboard", "TEMP_GB_INLET0", "HGX_Chassis_0", "HGX_Chassis_0_Inlet_0_Temp_0"},
  {"Baseboard", "TEMP_GB_INLET1", "HGX_Chassis_0", "HGX_Chassis_0_Inlet_1_Temp_0"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"NULL", "NULL", "NULL", "NULL"},

  {"Baseboard", "TEMP_GB_PCB0", "HGX_Chassis_0", "HGX_Chassis_0_PCB_0_Temp_0"},
  {"Baseboard", "TEMP_GB_PCB1", "HGX_Chassis_0", "HGX_Chassis_0_PCB_1_Temp_0"},
  {"Baseboard", "TEMP_GB_PCB2", "HGX_Chassis_0", "HGX_Chassis_0_PCB_2_Temp_0"},
  {"Baseboard", "TEMP_GB_PCB3", "HGX_Chassis_0", "HGX_Chassis_0_PCB_3_Temp_0"},
  {"Baseboard", "TEMP_GB_PCIERETIMER0", "HGX_PCIeRetimer_0", "HGX_PCIeRetimer_0_TEMP_0"},
  {"Baseboard", "TEMP_GB_PCIERETIMER1", "HGX_PCIeRetimer_1", "HGX_PCIeRetimer_1_TEMP_0"},
  {"Baseboard", "TEMP_GB_PCIERETIMER2", "HGX_PCIeRetimer_2", "HGX_PCIeRetimer_2_TEMP_0"},
  {"Baseboard", "TEMP_GB_PCIERETIMER3", "HGX_PCIeRetimer_3", "HGX_PCIeRetimer_3_TEMP_0"},
  {"Baseboard", "TEMP_GB_PCIERETIMER4", "HGX_PCIeRetimer_4", "HGX_PCIeRetimer_4_TEMP_0"},
  {"Baseboard", "TEMP_GB_PCIERETIMER5", "HGX_PCIeRetimer_5", "HGX_PCIeRetimer_5_TEMP_0"},
  {"Baseboard", "TEMP_GB_PCIERETIMER6", "HGX_PCIeRetimer_6", "HGX_PCIeRetimer_6_TEMP_0"},
  {"Baseboard", "TEMP_GB_PCIERETIMER7", "HGX_PCIeRetimer_7", "HGX_PCIeRetimer_7_TEMP_0"},
  {"Baseboard", "TEMP_GB_PCIESWITCH0" , "HGX_PCIeSwitch_0", "HGX_PCIeSwitch_0_TEMP_0"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"NULL", "NULL", "NULL", "NULL"},

  //GPU0 - GPU1
  {"GPU0", "EG_GB_GPU0"     , "HGX_GPU_SXM_1", "HGX_GPU_SXM_1_Energy_0"},
  {"GPU0", "PWR_GB_GPU0"    , "HGX_GPU_SXM_1", "HGX_GPU_SXM_1_Power_0"},
  {"GPU0", "PWR_GB_HBM_GPU0", "HGX_GPU_SXM_1", "HGX_GPU_SXM_1_DRAM_0_Power_0"},
  {"GPU0", "TEMP_GB_GPU0"   , "HGX_GPU_SXM_1", "HGX_GPU_SXM_1_TEMP_0" },
  {"GPU0", "TEMP_GB_GPU0_M" , "HGX_GPU_SXM_1", "HGX_GPU_SXM_1_DRAM_0_Temp_0"},
  {"GPU0", "VOLT_GB_GPU0"   , "HGX_GPU_SXM_1", "HGX_GPU_SXM_1_Voltage_0"},
  {"GPU1", "EG_GB_GPU1"     , "HGX_GPU_SXM_2", "HGX_GPU_SXM_2_Energy_0"},
  {"GPU1", "PWR_GB_GPU1"    , "HGX_GPU_SXM_2", "HGX_GPU_SXM_2_Power_0"},
  {"GPU1", "PWR_GB_HBM_GPU1", "HGX_GPU_SXM_2", "HGX_GPU_SXM_2_DRAM_0_Power_0"},
  {"GPU1", "TEMP_GB_GPU1"   , "HGX_GPU_SXM_2", "HGX_GPU_SXM_2_TEMP_0" },
  {"GPU1", "TEMP_GB_GPU1_M" , "HGX_GPU_SXM_2", "HGX_GPU_SXM_2_DRAM_0_Temp_0"},
  {"GPU1", "VOLT_GB_GPU1"   , "HGX_GPU_SXM_2", "HGX_GPU_SXM_2_Voltage_0"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"NULL", "NULL", "NULL", "NULL"},

  //GPU2 - GPU3
  {"GPU2", "EG_GB_GPU2"     , "HGX_GPU_SXM_3", "HGX_GPU_SXM_3_Energy_0"},
  {"GPU2", "PWR_GB_GPU2"    , "HGX_GPU_SXM_3", "HGX_GPU_SXM_3_Power_0"},
  {"GPU2", "PWR_GB_HBM_GPU2", "HGX_GPU_SXM_3", "HGX_GPU_SXM_3_DRAM_0_Power_0"},
  {"GPU2", "TEMP_GB_GPU2"   , "HGX_GPU_SXM_3", "HGX_GPU_SXM_3_TEMP_0" },
  {"GPU2", "TEMP_GB_GPU2_M" , "HGX_GPU_SXM_3", "HGX_GPU_SXM_3_DRAM_0_Temp_0"},
  {"GPU2", "VOLT_GB_GPU2"   , "HGX_GPU_SXM_3", "HGX_GPU_SXM_3_Voltage_0"},
  {"GPU3", "EG_GB_GPU3"     , "HGX_GPU_SXM_4", "HGX_GPU_SXM_4_Energy_0"},
  {"GPU3", "PWR_GB_GPU3"    , "HGX_GPU_SXM_4", "HGX_GPU_SXM_4_Power_0"},
  {"GPU3", "PWR_GB_HBM_GPU3", "HGX_GPU_SXM_4", "HGX_GPU_SXM_4_DRAM_0_Power_0"},
  {"GPU3", "TEMP_GB_GPU3"   , "HGX_GPU_SXM_4", "HGX_GPU_SXM_4_TEMP_0" },
  {"GPU3", "TEMP_GB_GPU3_M" , "HGX_GPU_SXM_4", "HGX_GPU_SXM_4_DRAM_0_Temp_0"},
  {"GPU3", "VOLT_GB_GPU3"   , "HGX_GPU_SXM_4", "HGX_GPU_SXM_4_Voltage_0"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"NULL", "NULL", "NULL", "NULL"},

  //GPU4 - GPU5
  {"GPU4", "EG_GB_GPU4"     , "HGX_GPU_SXM_5", "HGX_GPU_SXM_5_Energy_0"},
  {"GPU4", "PWR_GB_GPU4"    , "HGX_GPU_SXM_5", "HGX_GPU_SXM_5_Power_0"},
  {"GPU4", "PWR_GB_HBM_GPU4", "HGX_GPU_SXM_5", "HGX_GPU_SXM_5_DRAM_0_Power_0"},
  {"GPU4", "TEMP_GB_GPU4"   , "HGX_GPU_SXM_5", "HGX_GPU_SXM_5_TEMP_0" },
  {"GPU4", "TEMP_GB_GPU4_M" , "HGX_GPU_SXM_5", "HGX_GPU_SXM_5_DRAM_0_Temp_0"},
  {"GPU4", "VOLT_GB_GPU4"   , "HGX_GPU_SXM_5", "HGX_GPU_SXM_5_Voltage_0"},
  {"GPU5", "EG_GB_GPU5"     , "HGX_GPU_SXM_6", "HGX_GPU_SXM_6_Energy_0"},
  {"GPU5", "PWR_GB_GPU5"    , "HGX_GPU_SXM_6", "HGX_GPU_SXM_6_Power_0"},
  {"GPU5", "PWR_GB_HBM_GPU5", "HGX_GPU_SXM_6", "HGX_GPU_SXM_6_DRAM_0_Power_0"},
  {"GPU5", "TEMP_GB_GPU5"   , "HGX_GPU_SXM_6", "HGX_GPU_SXM_6_TEMP_0" },
  {"GPU5", "TEMP_GB_GPU5_M" , "HGX_GPU_SXM_6", "HGX_GPU_SXM_6_DRAM_0_Temp_0"},
  {"GPU5", "VOLT_GB_GPU5"   , "HGX_GPU_SXM_6", "HGX_GPU_SXM_6_Voltage_0"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"NULL", "NULL", "NULL", "NULL"},

  //GPU6 - GPU7
  {"GPU6", "EG_GB_GPU6"     , "HGX_GPU_SXM_7", "HGX_GPU_SXM_7_Energy_0"},
  {"GPU6", "PWR_GB_GPU6"    , "HGX_GPU_SXM_7", "HGX_GPU_SXM_7_Power_0"},
  {"GPU6", "PWR_GB_HBM_GPU6", "HGX_GPU_SXM_7", "HGX_GPU_SXM_7_DRAM_0_Power_0"},
  {"GPU6", "TEMP_GB_GPU6"   , "HGX_GPU_SXM_7", "HGX_GPU_SXM_7_TEMP_0" },
  {"GPU6", "TEMP_GB_GPU6_M" , "HGX_GPU_SXM_7", "HGX_GPU_SXM_7_DRAM_0_Temp_0"},
  {"GPU6", "VOLT_GB_GPU6"   , "HGX_GPU_SXM_7", "HGX_GPU_SXM_7_Voltage_0"},
  {"GPU7", "EG_GB_GPU7"     , "HGX_GPU_SXM_8", "HGX_GPU_SXM_8_Energy_0"},
  {"GPU7", "PWR_GB_GPU7"    , "HGX_GPU_SXM_8", "HGX_GPU_SXM_8_Power_0"},
  {"GPU7", "PWR_GB_HBM_GPU7", "HGX_GPU_SXM_8", "HGX_GPU_SXM_8_DRAM_0_Power_0"},
  {"GPU7", "TEMP_GB_GPU7"   , "HGX_GPU_SXM_8", "HGX_GPU_SXM_8_TEMP_0" },
  {"GPU7", "TEMP_GB_GPU7_M" , "HGX_GPU_SXM_8", "HGX_GPU_SXM_8_DRAM_0_Temp_0"},
  {"GPU7", "VOLT_GB_GPU7"   , "HGX_GPU_SXM_8", "HGX_GPU_SXM_8_Voltage_0"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"NULL", "NULL", "NULL", "NULL"},

  //NVSwitch0 - NVSwitch3
  {"NVSwitch0", "TEMP_GB_NVS0", "HGX_NVSwitch_0", "HGX_NVSwitch_0_TEMP_0"},
  {"NVSwitch1", "TEMP_GB_NVS1", "HGX_NVSwitch_1", "HGX_NVSwitch_1_TEMP_0"},
  {"NVSwitch2", "TEMP_GB_NVS2", "HGX_NVSwitch_2", "HGX_NVSwitch_2_TEMP_0"},
  {"NVSwitch3", "TEMP_GB_NVS3", "HGX_NVSwitch_3", "HGX_NVSwitch_3_TEMP_0"},
};


static int
read_snr(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = -1;
  float val = 0;
  char hmc_version[256] = {0};
  static int build_stage = NONE;
  static uint8_t retry = 0;
  static uint8_t snr_retry[HMC_SNR_CNT] = {0};

  if (sensor_num == HMC_SNR_PWR_GB_HSC0 && build_stage == NONE && retry <= 10) {
    retry++;
    if (get_hmc_ver("HMC_Firmware", hmc_version) == 0) {
      build_stage = EVT;
    }
    else if (get_hmc_ver("HGX_FW_HMC_0", hmc_version) == 0) {
      build_stage = DVT;
    }
    else {
      syslog(LOG_WARNING, "Can't read HGX FW version");
      return -1;
    }
  }

  if (build_stage == EVT) {
	ret = get_hmc_sensor(HMC_SNR_INFO[sensor_num].evt_hmc_comp, HMC_SNR_INFO[sensor_num].evt_snr_name, &val);
  }
  else if (build_stage == DVT) {
	ret = get_hmc_sensor(HMC_SNR_INFO[sensor_num].dvt_hmc_comp, HMC_SNR_INFO[sensor_num].dvt_snr_name, &val);
  }
  else {
    return -1;
  }

  if (ret) {
    snr_retry[sensor_num]++;
    return retry_err_handle(snr_retry[sensor_num], 3);
  }
  else {
    *value = val;
  }
  
  return ret;
}

PAL_SENSOR_MAP hmc_sensor_map[] = {
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0
  {"PWR_GB_HSC0" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x01
  {"PWR_GB_HSC1" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x02
  {"PWR_GB_HSC2" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x03
  {"PWR_GB_HSC3" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x04
  {"PWR_GB_HSC4" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x05
  {"PWR_GB_HSC5" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x06
  {"PWR_GB_HSC6" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x07
  {"PWR_GB_HSC7" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x08
  {"PWR_GB_HSC8" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x09
  {"PWR_GB_HSC9" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x0A
  {"PWR_GB_HSC10", 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x0B
  {"Total_Power" , 0,  read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x0C
  {"Total_GPU_Power"   , 0, read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0D
  {"Altitude_Pressure0", 0, read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0F

  {"TEMP_GB_FPGA",  0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x10
  {"TEMP_GB_HSC0" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x11
  {"TEMP_GB_HSC1" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x12
  {"TEMP_GB_HSC2" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x13
  {"TEMP_GB_HSC3" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x14
  {"TEMP_GB_HSC4" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x15
  {"TEMP_GB_HSC5" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x16
  {"TEMP_GB_HSC6" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x17
  {"TEMP_GB_HSC7" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x18
  {"TEMP_GB_HSC8" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x19
  {"TEMP_GB_HSC9" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x1A
  {"TEMP_GB_HSC10", 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x1B
  {"TEMP_GB_INLET0", 0, read_snr, true, {110, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x1C
  {"TEMP_GB_INLET1", 0, read_snr, true, {110, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x1D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x1E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x1F

  {"TEMP_GB_PCB0",  0 , read_snr, true, {80, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x20
  {"TEMP_GB_PCB1" , 0 , read_snr, true, {80, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x21
  {"TEMP_GB_PCB2" , 0 , read_snr, true, {80, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x22
  {"TEMP_GB_PCB3" , 0 , read_snr, true, {80, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x23
  {"TEMP_GB_PCIERETIMER0" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x24
  {"TEMP_GB_PCIERETIMER1" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x25
  {"TEMP_GB_PCIERETIMER2" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x26
  {"TEMP_GB_PCIERETIMER3" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x27
  {"TEMP_GB_PCIERETIMER4" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x28
  {"TEMP_GB_PCIERETIMER5" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x29
  {"TEMP_GB_PCIERETIMER6" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x2A
  {"TEMP_GB_PCIERETIMER7" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x2B
  {"TEMP_GB_PCIESWITCH0"  , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x2C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2F

  {"EG_GB_GPU0"     , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0},     0}, //0x30
  {"PWR_GB_GPU0"    , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x31
  {"PWR_GB_HBM_GPU0", 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x32
  {"TEMP_GB_GPU0"   , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x33
  {"TEMP_GB_GPU0_M" , 0 , read_snr, true, {90, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x34
  {"VOLT_GB_GPU0"   , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},  //0x35
  {"EG_GB_GPU1"     , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0},     0}, //0x36
  {"PWR_GB_GPU1"    , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x37
  {"PWR_GB_HBM_GPU1", 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x38
  {"TEMP_GB_GPU1"   , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x39
  {"TEMP_GB_GPU1_M" , 0 , read_snr, true, {90, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x3A
  {"VOLT_GB_GPU1"   , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},  //0x3B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x3C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x3D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x3E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x3F

  {"EG_GB_GPU2"     , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0},     0}, //0x40
  {"PWR_GB_GPU2"    , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x41
  {"PWR_GB_HBM_GPU2", 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x42
  {"TEMP_GB_GPU2"   , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x43
  {"TEMP_GB_GPU2_M" , 0 , read_snr, true, {90, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x44
  {"VOLT_GB_GPU2"   , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},  //0x45
  {"EG_GB_GPU3"     , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0},     0}, //0x46
  {"PWR_GB_GPU3"    , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x47
  {"PWR_GB_HBM_GPU3", 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x48
  {"TEMP_GB_GPU3"   , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x49
  {"TEMP_GB_GPU3_M" , 0 , read_snr, true, {90, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x4A
  {"VOLT_GB_GPU3"   , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},  //0x4B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x4C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x4D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x4E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x4F

  {"EG_GB_GPU4"     , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0},     0}, //0x50
  {"PWR_GB_GPU4"    , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x51
  {"PWR_GB_HBM_GPU4", 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x52
  {"TEMP_GB_GPU4"   , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x53
  {"TEMP_GB_GPU4_M" , 0 , read_snr, true, {90, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x54
  {"VOLT_GB_GPU4"   , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},  //0x55
  {"EG_GB_GPU5"     , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0},     0}, //0x56
  {"PWR_GB_GPU5"    , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x57
  {"PWR_GB_HBM_GPU5", 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x58
  {"TEMP_GB_GPU5"   , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x59
  {"TEMP_GB_GPU5_M" , 0 , read_snr, true, {90, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x5A
  {"VOLT_GB_GPU5"   , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},  //0x5B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5F

  {"EG_GB_GPU6"     , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0},     0}, //0x60
  {"PWR_GB_GPU6"    , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x61
  {"PWR_GB_HBM_GPU6", 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x62
  {"TEMP_GB_GPU6"   , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x63
  {"TEMP_GB_GPU6_M" , 0 , read_snr, true, {90, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x64
  {"VOLT_GB_GPU6"   , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},  //0x65
  {"EG_GB_GPU7"     , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0},     0}, //0x66
  {"PWR_GB_GPU7"    , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x67
  {"PWR_GB_HBM_GPU7", 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x68
  {"TEMP_GB_GPU7"   , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x69
  {"TEMP_GB_GPU7_M" , 0 , read_snr, true, {90, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x6A
  {"VOLT_GB_GPU7"   , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},  //0x6B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x6C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x6D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x6E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x6F

  {"TEMP_GB_NVS0" , 0 , read_snr, true, {110, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x70
  {"TEMP_GB_NVS1" , 0 , read_snr, true, {110, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x71
  {"TEMP_GB_NVS2" , 0 , read_snr, true, {110, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x72
  {"TEMP_GB_NVS3" , 0 , read_snr, true, {110, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x73
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x74
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x75
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x76
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x77
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x78
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x79
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x7A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x7B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x7C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x7D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x7E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x7F
};

const uint8_t hmc_sensor_list[] = {
  //Baseboard
  HMC_SNR_PWR_GB_HSC0,
  HMC_SNR_PWR_GB_HSC1,
  HMC_SNR_PWR_GB_HSC2,
  HMC_SNR_PWR_GB_HSC3,
  HMC_SNR_PWR_GB_HSC4,
  HMC_SNR_PWR_GB_HSC5,
  HMC_SNR_PWR_GB_HSC6,
  HMC_SNR_PWR_GB_HSC7,
  HMC_SNR_PWR_GB_HSC8,
  HMC_SNR_PWR_GB_HSC9,
  HMC_SNR_PWR_GB_HSC10,
  Total_Power,
  //Total_GPU_Power,
  Altitude_Pressure0,

  TEMP_GB_FPGA,
  TEMP_GB_HSC0,
  TEMP_GB_HSC1,
  TEMP_GB_HSC2,
  TEMP_GB_HSC3,
  TEMP_GB_HSC4,
  TEMP_GB_HSC5,
  TEMP_GB_HSC6,
  TEMP_GB_HSC7,
  TEMP_GB_HSC8,
  TEMP_GB_HSC9,
  TEMP_GB_HSC10,
  TEMP_GB_INLET0,
  TEMP_GB_INLET1,

  TEMP_GB_PCB0,
  TEMP_GB_PCB1,
  TEMP_GB_PCB2,
  TEMP_GB_PCB3,
  TEMP_GB_PCIERETIMER0,
  TEMP_GB_PCIERETIMER1,
  TEMP_GB_PCIERETIMER2,
  TEMP_GB_PCIERETIMER3,
  TEMP_GB_PCIERETIMER4,
  TEMP_GB_PCIERETIMER5,
  TEMP_GB_PCIERETIMER6,
  TEMP_GB_PCIERETIMER7,
  TEMP_GB_PCIESWITCH0,
 
  //GPU0 and GPU1
  /*EG_GB_GPU0,
  PWR_GB_GPU0,
  PWR_GB_HBM_GPU0,*/
  TEMP_GB_GPU0,
  TEMP_GB_GPU0_M,
  /*VOLT_GB_GPU0,
  EG_GB_GPU1,
  PWR_GB_GPU1,
  PWR_GB_HBM_GPU1,*/
  TEMP_GB_GPU1,
  TEMP_GB_GPU1_M,
  //VOLT_GB_GPU1,

  //GPU2 and GPU3
  /*EG_GB_GPU2,
  PWR_GB_GPU2,
  PWR_GB_HBM_GPU2,*/
  TEMP_GB_GPU2,
  TEMP_GB_GPU2_M,
  /*VOLT_GB_GPU2,
  EG_GB_GPU3,
  PWR_GB_GPU3,
  PWR_GB_HBM_GPU3,*/
  TEMP_GB_GPU3,
  TEMP_GB_GPU3_M,
  //VOLT_GB_GPU3,

  //GPU4 and GPU5
  /*EG_GB_GPU4,
  PWR_GB_GPU4,
  PWR_GB_HBM_GPU4,*/
  TEMP_GB_GPU4,
  TEMP_GB_GPU4_M,
  /*VOLT_GB_GPU4,
  EG_GB_GPU5,
  PWR_GB_GPU5,
  PWR_GB_HBM_GPU5,*/
  TEMP_GB_GPU5,
  TEMP_GB_GPU5_M,
  //VOLT_GB_GPU5,

  //GPU6 and GPU7
  /*EG_GB_GPU6,
  PWR_GB_GPU6,
  PWR_GB_HBM_GPU6,*/
  TEMP_GB_GPU6,
  TEMP_GB_GPU6_M,
  /*VOLT_GB_GPU6,
  EG_GB_GPU7,
  PWR_GB_GPU7,
  PWR_GB_HBM_GPU7,*/
  TEMP_GB_GPU7,
  TEMP_GB_GPU7_M,
  //VOLT_GB_GPU7,

  //NV Switch
  TEMP_GB_NVS0,
  TEMP_GB_NVS1,
  TEMP_GB_NVS2,
  TEMP_GB_NVS3,
};

size_t hmc_sensor_cnt = sizeof(hmc_sensor_list)/sizeof(uint8_t);
