#include <stdint.h>
#include <stdio.h>
#include <syslog.h>
#include "pal.h"
#include "pal_hgx_sensors.h"
#include <openbmc/hgx.h>
#include <openbmc/kv.h>

struct hgx_snr_info {
  char *evt_hgx_comp;
  char *evt_snr_name;

  char *dvt_hgx_comp;
  char *dvt_snr_name;
} HGX_SNR_INFO[] = {
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
  {"GPU0", "VOLT_GB_GPU0"   , "HGX_GPU_SXM_1", "HGX_GPU_SXM_1_Voltage_0"},
  {"GPU0", "TEMP_GB_GPU0"   , "HGX_GPU_SXM_1", "HGX_GPU_SXM_1_TEMP_0" },
  {"NULL", "NULL"           , "HGX_GPU_SXM_1", "HGX_GPU_SXM_1_TEMP_1" },
  {"GPU0", "PWR_GB_HBM_GPU0", "HGX_GPU_SXM_1", "HGX_GPU_SXM_1_DRAM_0_Power_0"},
  {"GPU0", "TEMP_GB_GPU0_M" , "HGX_GPU_SXM_1", "HGX_GPU_SXM_1_DRAM_0_Temp_0"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"GPU1", "EG_GB_GPU1"     , "HGX_GPU_SXM_2", "HGX_GPU_SXM_2_Energy_0"},
  {"GPU1", "PWR_GB_GPU1"    , "HGX_GPU_SXM_2", "HGX_GPU_SXM_2_Power_0"},
  {"GPU1", "VOLT_GB_GPU1"   , "HGX_GPU_SXM_2", "HGX_GPU_SXM_2_Voltage_0"},
  {"GPU1", "TEMP_GB_GPU1"   , "HGX_GPU_SXM_2", "HGX_GPU_SXM_2_TEMP_0" },
  {"GPU1", "TEMP_GB_GPU1"   , "HGX_GPU_SXM_2", "HGX_GPU_SXM_2_TEMP_1" },
  {"GPU1", "PWR_GB_HBM_GPU0", "HGX_GPU_SXM_2", "HGX_GPU_SXM_2_DRAM_0_Power_0"},
  {"GPU1", "TEMP_GB_GPU0_M" , "HGX_GPU_SXM_2", "HGX_GPU_SXM_2_DRAM_0_Temp_0"},
  {"NULL", "NULL", "NULL", "NULL"},

  //GPU2 - GPU3
  {"GPU2", "EG_GB_GPU2"     , "HGX_GPU_SXM_3", "HGX_GPU_SXM_3_Energy_0"},
  {"GPU2", "PWR_GB_GPU2"    , "HGX_GPU_SXM_3", "HGX_GPU_SXM_3_Power_0"},
  {"GPU2", "VOLT_GB_GPU2"   , "HGX_GPU_SXM_3", "HGX_GPU_SXM_3_Voltage_0"},
  {"GPU2", "TEMP_GB_GPU2"   , "HGX_GPU_SXM_3", "HGX_GPU_SXM_3_TEMP_0" },
  {"NULL", "NULL"           , "HGX_GPU_SXM_3", "HGX_GPU_SXM_3_TEMP_1" },
  {"GPU2", "PWR_GB_HBM_GPU2", "HGX_GPU_SXM_3", "HGX_GPU_SXM_3_DRAM_0_Power_0"},
  {"GPU2", "TEMP_GB_GPU2_M" , "HGX_GPU_SXM_3", "HGX_GPU_SXM_3_DRAM_0_Temp_0"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"GPU3", "EG_GB_GPU3"     , "HGX_GPU_SXM_4", "HGX_GPU_SXM_4_Energy_0"},
  {"GPU3", "PWR_GB_GPU3"    , "HGX_GPU_SXM_4", "HGX_GPU_SXM_4_Power_0"},
  {"GPU3", "VOLT_GB_GPU3"   , "HGX_GPU_SXM_4", "HGX_GPU_SXM_4_Voltage_0"},
  {"GPU3", "TEMP_GB_GPU3"   , "HGX_GPU_SXM_4", "HGX_GPU_SXM_4_TEMP_0" },
  {"NULL", "NULL"           , "HGX_GPU_SXM_4", "HGX_GPU_SXM_4_TEMP_1" },
  {"GPU3", "PWR_GB_HBM_GPU3", "HGX_GPU_SXM_4", "HGX_GPU_SXM_4_DRAM_0_Power_0"},
  {"GPU3", "TEMP_GB_GPU3_M" , "HGX_GPU_SXM_4", "HGX_GPU_SXM_4_DRAM_0_Temp_0"},
  {"NULL", "NULL", "NULL", "NULL"},

  //GPU4 - GPU5
  {"GPU4", "EG_GB_GPU4"     , "HGX_GPU_SXM_5", "HGX_GPU_SXM_5_Energy_0"},
  {"GPU4", "PWR_GB_GPU4"    , "HGX_GPU_SXM_5", "HGX_GPU_SXM_5_Power_0"},
  {"GPU4", "VOLT_GB_GPU4"   , "HGX_GPU_SXM_5", "HGX_GPU_SXM_5_Voltage_0"},
  {"GPU4", "TEMP_GB_GPU4"   , "HGX_GPU_SXM_5", "HGX_GPU_SXM_5_TEMP_0" },
  {"NULL", "NULL"           , "HGX_GPU_SXM_5", "HGX_GPU_SXM_5_TEMP_1" },
  {"GPU4", "PWR_GB_HBM_GPU4", "HGX_GPU_SXM_5", "HGX_GPU_SXM_5_DRAM_0_Power_0"},
  {"GPU4", "TEMP_GB_GPU4_M" , "HGX_GPU_SXM_5", "HGX_GPU_SXM_5_DRAM_0_Temp_0"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"GPU5", "EG_GB_GPU5"     , "HGX_GPU_SXM_6", "HGX_GPU_SXM_6_Energy_0"},
  {"GPU5", "PWR_GB_GPU5"    , "HGX_GPU_SXM_6", "HGX_GPU_SXM_6_Power_0"},
  {"GPU5", "VOLT_GB_GPU5"   , "HGX_GPU_SXM_6", "HGX_GPU_SXM_6_Voltage_0"},
  {"GPU5", "TEMP_GB_GPU5"   , "HGX_GPU_SXM_6", "HGX_GPU_SXM_6_TEMP_0" },
  {"NULL", "NULL"           , "HGX_GPU_SXM_6", "HGX_GPU_SXM_6_TEMP_1" },
  {"GPU5", "PWR_GB_HBM_GPU5", "HGX_GPU_SXM_6", "HGX_GPU_SXM_6_DRAM_0_Power_0"},
  {"GPU5", "TEMP_GB_GPU5_M" , "HGX_GPU_SXM_6", "HGX_GPU_SXM_6_DRAM_0_Temp_0"},
  {"NULL", "NULL", "NULL", "NULL"},

  //GPU6 - GPU7
  {"GPU6", "EG_GB_GPU6"     , "HGX_GPU_SXM_7", "HGX_GPU_SXM_7_Energy_0"},
  {"GPU6", "PWR_GB_GPU6"    , "HGX_GPU_SXM_7", "HGX_GPU_SXM_7_Power_0"},
  {"GPU6", "VOLT_GB_GPU6"   , "HGX_GPU_SXM_7", "HGX_GPU_SXM_7_Voltage_0"},
  {"GPU6", "TEMP_GB_GPU6"   , "HGX_GPU_SXM_7", "HGX_GPU_SXM_7_TEMP_0" },
  {"NULL", "NULL"           , "HGX_GPU_SXM_7", "HGX_GPU_SXM_7_TEMP_1" },
  {"GPU6", "PWR_GB_HBM_GPU6", "HGX_GPU_SXM_7", "HGX_GPU_SXM_7_DRAM_0_Power_0"},
  {"GPU6", "TEMP_GB_GPU6_M" , "HGX_GPU_SXM_7", "HGX_GPU_SXM_7_DRAM_0_Temp_0"},
  {"NULL", "NULL", "NULL", "NULL"},
  {"GPU7", "EG_GB_GPU7"     , "HGX_GPU_SXM_8", "HGX_GPU_SXM_8_Energy_0"},
  {"GPU7", "PWR_GB_GPU7"    , "HGX_GPU_SXM_8", "HGX_GPU_SXM_8_Power_0"},
  {"GPU7", "VOLT_GB_GPU7"   , "HGX_GPU_SXM_8", "HGX_GPU_SXM_8_Voltage_0"},
  {"GPU7", "TEMP_GB_GPU7"   , "HGX_GPU_SXM_8", "HGX_GPU_SXM_8_TEMP_0" },
  {"NULL", "NULL"           , "HGX_GPU_SXM_8", "HGX_GPU_SXM_8_TEMP_1" },
  {"GPU7", "PWR_GB_HBM_GPU7", "HGX_GPU_SXM_8", "HGX_GPU_SXM_8_DRAM_0_Power_0"},
  {"GPU7", "TEMP_GB_GPU7_M" , "HGX_GPU_SXM_8", "HGX_GPU_SXM_8_DRAM_0_Temp_0"},
  {"NULL", "NULL", "NULL", "NULL"},

  //NVSwitch0 - NVSwitch3
  {"NVSwitch0", "TEMP_GB_NVS0", "HGX_NVSwitch_0", "HGX_NVSwitch_0_TEMP_0"},
  {"NVSwitch1", "TEMP_GB_NVS1", "HGX_NVSwitch_1", "HGX_NVSwitch_1_TEMP_0"},
  {"NVSwitch2", "TEMP_GB_NVS2", "HGX_NVSwitch_2", "HGX_NVSwitch_2_TEMP_0"},
  {"NVSwitch3", "TEMP_GB_NVS3", "HGX_NVSwitch_3", "HGX_NVSwitch_3_TEMP_0"},
};


static int
read_single_snr(uint8_t fru, uint8_t sensor_num, float *value, int build_stage) {
  int ret = READING_NA;
  float val = 0;
  static uint8_t snr_retry = 0;

  if (build_stage == EVT) {
    ret = get_hgx_sensor(HGX_SNR_INFO[sensor_num].evt_hgx_comp, HGX_SNR_INFO[sensor_num].evt_snr_name, &val);
  }
  else if (build_stage == DVT) {
    ret = get_hgx_sensor(HGX_SNR_INFO[sensor_num].dvt_hgx_comp, HGX_SNR_INFO[sensor_num].dvt_snr_name, &val);
  }

  if (ret) {
    snr_retry++;
    ret = retry_err_handle(snr_retry, 5);
  }
  else {
    snr_retry = 0;
    *value = val;
  }

  return ret;
}

static int
read_kv_snr(uint8_t fru, uint8_t sensor_num, float *value, int build_stage) {
  int ret = READING_NA;
  char tmp[32] = {0};

  if (build_stage == EVT) {
    ret = kv_get(HGX_SNR_INFO[sensor_num].evt_snr_name, tmp, NULL, 0);
  }
  else if (build_stage == DVT) {
    ret = kv_get(HGX_SNR_INFO[sensor_num].dvt_snr_name, tmp, NULL, 0);
  }

  if (ret) {
    return READING_NA;
  }

  *value = atof(tmp);
  return 0;
}

static int
read_snr(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = READING_NA;
  char tmp[8] = {0};
  static int build_stage = NONE;
  static uint8_t snr_retry = 0;

  ret = kv_get("is_usbnet_ready", tmp, NULL, 0);
  if (ret || !strncmp(tmp, "0", 1)) {
    return READING_NA;
  }

  if (sensor_num == HGX_SNR_PWR_GB_HSC0 || build_stage == NONE) {
    HMCPhase phase = get_hgx_phase();
    switch (phase) {
      case HMC_FW_DVT:
      case BMC_FW_DVT:
        build_stage = DVT;
        break;
      case HMC_FW_EVT:
        build_stage = EVT;
        break;
      default:
        return READING_NA;
    }
    ret = hgx_get_metric_reports();
    if (ret) {
      snr_retry++;
      ret = retry_err_handle(snr_retry, 5);
      return ret;
    }
    snr_retry = 0;
  }

  if (sensor_num == Altitude_Pressure0 ||
      sensor_num == GPU1_VOL || sensor_num == GPU2_VOL ||
      sensor_num == GPU3_VOL || sensor_num == GPU4_VOL ||
      sensor_num == GPU5_VOL || sensor_num == GPU6_VOL ||
      sensor_num == GPU7_VOL || sensor_num == GPU8_VOL) {
    ret = read_single_snr(fru, sensor_num, value, build_stage);
  }
  else {
    ret = read_kv_snr(fru, sensor_num, value, build_stage);
  }

  return ret;
}

PAL_SENSOR_MAP hgx_sensor_map[] = {
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0
  {"HSC_0_Power" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x01
  {"HSC_1_Power" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x02
  {"HSC_2_Power" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x03
  {"HSC_3_Power" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x04
  {"HSC_4_Power" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x05
  {"HSC_5_Power" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x06
  {"HSC_6_Power" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x07
  {"HSC_7_Power" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x08
  {"HSC_8_Power" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x09
  {"HSC_9_Power" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x0A
  {"Standby_HSC_Power", 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x0B
  {"Total_HSC_Power" , 0,  read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x0C
  {"Total_GPU_Power"   , 0, read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x0D
  {"Altitude_Pressure0", 0, read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, PRESS}, //0x0E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0F

  {"FPGA_0_TEMP",  0 , read_snr, true, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x10
  {"HSC_0_TEMP" , 0 , read_snr, true, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x11
  {"HSC_1_TEMP" , 0 , read_snr, true, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x12
  {"HSC_2_TEMP" , 0 , read_snr, true, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x13
  {"HSC_3_TEMP" , 0 , read_snr, true, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x14
  {"HSC_4_TEMP" , 0 , read_snr, true, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x15
  {"HSC_5_TEMP" , 0 , read_snr, true, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x16
  {"HSC_6_TEMP" , 0 , read_snr, true, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x17
  {"HSC_7_TEMP" , 0 , read_snr, true, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x18
  {"HSC_8_TEMP" , 0 , read_snr, true, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x19
  {"HSC_9_TEMP" , 0 , read_snr, true, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x1A
  {"Standby_HSC_TEMP", 0 , read_snr, true, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x1B
  {"Inlet_0_TEMP", 0, read_snr, true, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x1C
  {"Inlet_1_TEMP", 0, read_snr, true, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x1D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x1E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x1F

  {"PCB_0_TEMP",  0 , read_snr, true, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x20
  {"PCB_1_TEMP" , 0 , read_snr, true, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x21
  {"PCB_2_TEMP" , 0 , read_snr, true, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x22
  {"PCB_3_TEMP" , 0 , read_snr, true, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x23
  {"PCIeRetimer_0_TEMP" , 0 , read_snr, true, {110.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x24
  {"PCIeRetimer_1_TEMP" , 0 , read_snr, true, {110.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x25
  {"PCIeRetimer_2_TEMP" , 0 , read_snr, true, {110.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x26
  {"PCIeRetimer_3_TEMP" , 0 , read_snr, true, {110.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x27
  {"PCIeRetimer_4_TEMP" , 0 , read_snr, true, {110.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x28
  {"PCIeRetimer_5_TEMP" , 0 , read_snr, true, {110.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x29
  {"PCIeRetimer_6_TEMP" , 0 , read_snr, true, {110.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x2A
  {"PCIeRetimer_7_TEMP" , 0 , read_snr, true, {110.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x2B
  {"PCIeSwitch_0_TEMP"  , 0 , read_snr, true, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x2C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x2F

  {"GPU_SXM_1_ENRGY"    , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, ENRGY}, //0x30
  {"GPU_SXM_1_PWR"      , 0 , read_snr, false, {1020.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x31
  {"GPU_SXM_1_VOL"      , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x32
  {"GPU_SXM_1_TEMP_0"   , 0 , read_snr, false, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x33
  {"GPU_SXM_1_TEMP_1"   , 0 , read_snr, false, {0, 0, 0, -0.01, 0, 0, 0, 0}, TEMP}, //0x34
  {"GPU_SXM_1_DRAM_PWR" , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x35
  {"GPU_SXM_1_DRAM_TEMP", 0 , read_snr, false, {95.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x36
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                                 //0x37
  {"GPU_SXM_2_ENRGY"    , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, ENRGY}, //0x38
  {"GPU_SXM_2_PWR"      , 0 , read_snr, false, {1020.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x39
  {"GPU_SXM_2_VOL"      , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x3A
  {"GPU_SXM_2_TEMP_0"   , 0 , read_snr, false, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x3B
  {"GPU_SXM_2_TEMP_1"   , 0 , read_snr, false, {0, 0, 0, -0.01, 0, 0, 0, 0}, TEMP}, //0x3C
  {"GPU_SXM_2_DRAM_PWR" , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x3D
  {"GPU_SXM_2_DRAM_TEMP", 0 , read_snr, false,  {95, 0, 0, 0, 0, 0, 0, 0}, TEMP},  //0x3E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                                 //0x3F

  {"GPU_SXM_3_ENRGY"    , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, ENRGY}, //0x40
  {"GPU_SXM_3_PWR"      , 0 , read_snr, false, {1020.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x41
  {"GPU_SXM_3_VOL"      , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x42
  {"GPU_SXM_3_TEMP_0"   , 0 , read_snr, false, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x43
  {"GPU_SXM_3_TEMP_1"   , 0 , read_snr, false, {0, 0, 0, -0.01, 0, 0, 0, 0}, TEMP}, //0x44
  {"GPU_SXM_3_DRAM_PWR" , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x45
  {"GPU_SXM_3_DRAM_TEMP", 0 , read_snr, false, {95.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x46
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                                 //0x47
  {"GPU_SXM_4_ENRGY"    , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, ENRGY}, //0x48
  {"GPU_SXM_4_PWR"      , 0 , read_snr, false, {1020.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x49
  {"GPU_SXM_4_VOL"      , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x4A
  {"GPU_SXM_4_TEMP_0"   , 0 , read_snr, false, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x4B
  {"GPU_SXM_4_TEMP_1"   , 0 , read_snr, false, {0, 0, 0, -0.01, 0, 0, 0, 0}, TEMP}, //0x4C
  {"GPU_SXM_4_DRAM_PWR" , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x4D
  {"GPU_SXM_4_DRAM_TEMP", 0 , read_snr, false, {95.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x4E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                                 //0x4F

  {"GPU_SXM_5_ENRGY"    , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, ENRGY}, //0x50
  {"GPU_SXM_5_PWR"      , 0 , read_snr, false, {1020.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x51
  {"GPU_SXM_5_VOL"      , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x52
  {"GPU_SXM_5_TEMP_0"   , 0 , read_snr, false, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x53
  {"GPU_SXM_5_TEMP_1"   , 0 , read_snr, false, {0, 0, 0, -0.01, 0, 0, 0, 0}, TEMP}, //0x54
  {"GPU_SXM_5_DRAM_PWR" , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x55
  {"GPU_SXM_5_DRAM_TEMP", 0 , read_snr, false, {95.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x56
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                                 //0x57
  {"GPU_SXM_6_ENRGY"    , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, ENRGY}, //0x58
  {"GPU_SXM_6_PWR"      , 0 , read_snr, false, {1020.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x59
  {"GPU_SXM_6_VOL"      , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x5A
  {"GPU_SXM_6_TEMP_0"   , 0 , read_snr, false, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x5B
  {"GPU_SXM_6_TEMP_1"   , 0 , read_snr, false, {0, 0, 0, -0.01, 0, 0, 0, 0}, TEMP}, //0x5C
  {"GPU_SXM_6_DRAM_PWR" , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x5D
  {"GPU_SXM_6_DRAM_TEMP", 0 , read_snr, false, {95.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x5E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                                 //0x5F

  {"GPU_SXM_7_ENRGY"    , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, ENRGY}, //0x60
  {"GPU_SXM_7_PWR"      , 0 , read_snr, false, {1020.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x61
  {"GPU_SXM_7_VOL"      , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x62
  {"GPU_SXM_7_TEMP_0"   , 0 , read_snr, false, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x63
  {"GPU_SXM_7_TEMP_1"   , 0 , read_snr, false, {0, 0, 0, -0.01, 0, 0, 0, 0}, TEMP}, //0x64
  {"GPU_SXM_7_DRAM_PWR" , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x65
  {"GPU_SXM_7_DRAM_TEMP", 0 , read_snr, false, {95.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x66
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                                 //0x67
  {"GPU_SXM_8_ENRGY"    , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, ENRGY}, //0x68
  {"GPU_SXM_8_PWR"      , 0 , read_snr, false, {1020.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x69
  {"GPU_SXM_8_VOL"      , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x6A
  {"GPU_SXM_8_TEMP_0"   , 0 , read_snr, false, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x6B
  {"GPU_SXM_8_TEMP_1"   , 0 , read_snr, false, {0, 0, 0, -0.01, 0, 0, 0, 0}, TEMP}, //0x6C
  {"GPU_SXM_8_DRAM_PWR" , 0 , read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6D
  {"GPU_SXM_8_DRAM_TEMP", 0 , read_snr, false, {95.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x6E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                                 //0x6F

  {"NVSwitch_0_TEMP" , 0 , read_snr, false, {90.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x70
  {"NVSwitch_1_TEMP" , 0 , read_snr, false, {90.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x71
  {"NVSwitch_2_TEMP" , 0 , read_snr, false, {90.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x72
  {"NVSwitch_3_TEMP" , 0 , read_snr, false, {90.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x73
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

const uint8_t hgx_sensor_list[] = {
  //Baseboard
  HGX_SNR_PWR_GB_HSC0,
  HGX_SNR_PWR_GB_HSC1,
  HGX_SNR_PWR_GB_HSC2,
  HGX_SNR_PWR_GB_HSC3,
  HGX_SNR_PWR_GB_HSC4,
  HGX_SNR_PWR_GB_HSC5,
  HGX_SNR_PWR_GB_HSC6,
  HGX_SNR_PWR_GB_HSC7,
  HGX_SNR_PWR_GB_HSC8,
  HGX_SNR_PWR_GB_HSC9,
  HGX_SNR_PWR_GB_HSC10,
  Total_Power,
  Total_GPU_Power,
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

  //GPU1 and GPU2
  GPU1_ENG,
  GPU1_PWR,
  GPU1_VOL,
  GPU1_TEMP_0,
  GPU1_TEMP_1,
  GPU1_DRAM_PWR,
  GPU1_DRAM_TEMP,

  GPU2_ENG,
  GPU2_PWR,
  GPU2_VOL,
  GPU2_TEMP_0,
  GPU2_TEMP_1,
  GPU2_DRAM_PWR,
  GPU2_DRAM_TEMP,

  //GPU3 and GPU4
  GPU3_ENG,
  GPU3_PWR,
  GPU3_VOL,
  GPU3_TEMP_0,
  GPU3_TEMP_1,
  GPU3_DRAM_PWR,
  GPU3_DRAM_TEMP,

  GPU4_ENG,
  GPU4_PWR,
  GPU4_VOL,
  GPU4_TEMP_0,
  GPU4_TEMP_1,
  GPU4_DRAM_PWR,
  GPU4_DRAM_TEMP,

  //GPU5 and GPU6
  GPU5_ENG,
  GPU5_PWR,
  GPU5_VOL,
  GPU5_TEMP_0,
  GPU5_TEMP_1,
  GPU5_DRAM_PWR,
  GPU5_DRAM_TEMP,

  GPU6_ENG,
  GPU6_PWR,
  GPU6_VOL,
  GPU6_TEMP_0,
  GPU6_TEMP_1,
  GPU6_DRAM_PWR,
  GPU6_DRAM_TEMP,

  //GPU7 and GPU8
  GPU7_ENG,
  GPU7_PWR,
  GPU7_VOL,
  GPU7_TEMP_0,
  GPU7_TEMP_1,
  GPU7_DRAM_PWR,
  GPU7_DRAM_TEMP,

  GPU8_ENG,
  GPU8_PWR,
  GPU8_VOL,
  GPU8_TEMP_0,
  GPU8_TEMP_1,
  GPU8_DRAM_PWR,
  GPU8_DRAM_TEMP,

  //NV Switch
  TEMP_GB_NVS0,
  TEMP_GB_NVS1,
  TEMP_GB_NVS2,
  TEMP_GB_NVS3,
};

size_t hgx_sensor_cnt = sizeof(hgx_sensor_list)/sizeof(uint8_t);
