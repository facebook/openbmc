#include <stdint.h>
#include <stdio.h>
#include <syslog.h>
#include "pal.h"
#include "pal_hgx_sensors.h"
#include <openbmc/hgx.h>
#include <openbmc/kv.h>

struct ubb_snr_info {
  char *snr_name;
} UBB_SNR_INFO[] = {
  {"NULL"}, {"NULL"}, {"NULL"}, {"NULL"},
  {"NULL"}, {"NULL"}, {"NULL"}, {"NULL"},
  {"NULL"}, {"NULL"}, {"NULL"}, 
  {"UBB_POWER"},                            //0x0B
  {"NULL"},                                 //0x0C
  {"GPU_TOTAL_POWER"},                      //0x0D
  {"NULL"}, {"NULL"},

  {"UBB_FPGA_TEMP"},                        //0x10
  {"OAM_0_1_HSC_TEMP"},                     //0x11
  {"OAM_0_1_HSC_TEMP"},                     //0x12
  {"OAM_2_3_HSC_TEMP"},                     //0x13
  {"OAM_2_3_HSC_TEMP"},                     //0x14
  {"OAM_4_5_HSC_TEMP"},                     //0x15
  {"OAM_4_5_HSC_TEMP"},                     //0x16
  {"OAM_6_7_HSC_TEMP"},                     //0x17
  {"OAM_6_7_HSC_TEMP"},                     //0x18
  {"NULL"}, {"NULL"}, {"NULL"},
  {"UBB_TEMP_FRONT"},                       //0x1C
  {"UBB_TEMP_BACK"},                        //0x1D
  {"NULL"}, 
  {"IBC_TEMP"},                             //0x1F

  {"UBB_TEMP_OAM7"},                        //0x20
  {"UBB_TEMP_IBC"},                         //0x21
  {"UBB_TEMP_UFPGA"},                       //0x22
  {"UBB_TEMP_OAM1"},                        //0x23
  {"RETIMER_0_TEMP"},                       //0x24
  {"RETIMER_1_TEMP"},                       //0x25
  {"RETIMER_2_TEMP"},                       //0x26
  {"RETIMER_3_TEMP"},                       //0x27
  {"RETIMER_4_TEMP"},                       //0x28
  {"RETIMER_5_TEMP"},                       //0x29
  {"RETIMER_6_TEMP"},                       //0x2A
  {"RETIMER_7_TEMP"},                       //0x2B
  {"NULL"},
  {"RETIMER_MAX_TEMP"},                     //0x2D
  {"RETIMER_MAX_VR_TEMP"},                  //0x2E
  {"NULL"},

  //GPU0 - GPU1
  {"NULL"},
  {"GPU_0_POWER"},                         //0x31
  {"NULL"},
  {"GPU_0_DIE_TEMP" },                     //0x33
  {"NULL"},
  {"NULL"},
  {"GPU_0_MEMORY_TEMP"},                   //0x36
  {"NULL"},
  {"NULL"}, 
  {"GPU_1_POWER"},                         //0x39
  {"NULL"},
  {"GPU_1_DIE_TEMP" },                     //0x3B
  {"NULL"},
  {"NULL"},
  {"GPU_1_MEMORY_TEMP"},                   //0x3E
  {"NULL"},

  //GPU2 - GPU3
  {"NULL"},
  {"GPU_2_POWER"},                         //0x41
  {"NULL"},
  {"GPU_2_DIE_TEMP" },                     //0x43
  {"NULL"},
  {"NULL"},
  {"GPU_2_MEMORY_TEMP"},                   //0x46
  {"NULL"},
  {"NULL"}, 
  {"GPU_3_POWER"},                         //0x49
  {"NULL"},
  {"GPU_3_DIE_TEMP" },                     //0x4B
  {"NULL"},
  {"NULL"},
  {"GPU_3_MEMORY_TEMP"},                   //0x4E
  {"NULL"},

  //GPU4 - GPU5
  {"NULL"},
  {"GPU_4_POWER"},                         //0x51
  {"NULL"},
  {"GPU_4_DIE_TEMP" },                     //0x53
  {"NULL"},
  {"NULL"},
  {"GPU_4_MEMORY_TEMP"},                   //0x56
  {"NULL"},
  {"NULL"}, 
  {"GPU_5_POWER"},                         //0x59
  {"NULL"},
  {"GPU_5_DIE_TEMP" },                     //0x5B
  {"NULL"},
  {"NULL"},
  {"GPU_5_MEMORY_TEMP"},                   //0x5E
  {"NULL"},

  //GPU6 - GPU7
  {"NULL"},
  {"GPU_6_POWER"},                         //0x61
  {"NULL"},
  {"GPU_6_DIE_TEMP" },                     //0x63
  {"NULL"},
  {"NULL"},
  {"GPU_6_MEMORY_TEMP"},                   //0x66
  {"GPU_WARMEST_DIE_TEMP"},                //0x67
  {"NULL"}, 
  {"GPU_7_POWER"},                         //0x69
  {"NULL"},
  {"GPU_7_DIE_TEMP" },                     //0x6B
  {"NULL"},
  {"NULL"},
  {"GPU_7_MEMORY_TEMP"},                   //0x6E
  {"GPU_WARMEST_MEMORY_TEMP"},             //0x6F
};

PAL_SENSOR_MAP ubb_sensor_map[];

static int
read_kv_snr(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = READING_NA;
  char data[32] = {0};

  if (!ubb_sensor_map[sensor_num].stby_read &&
      (kv_get("gpu_snr_valid", data, NULL, 0) || strcmp(data, "valid"))) {
    return READING_NA;
  }

  ret = kv_get(UBB_SNR_INFO[sensor_num].snr_name, data, NULL, 0);

  if (ret || !strcmp(data, "NA")) {
    return READING_NA;
  }

  *value = atof(data);
  return 0;
}

static int
read_snr(uint8_t fru, uint8_t sensor_num, float *value) {
  int ret = READING_NA;
  static uint8_t snr_retry = 0;

  if(sensor_num == HGX_SNR_PWR_GB_HSC10) {
    ret = hgx_get_metric_reports();
    if (ret) {
      snr_retry++;
      ret = retry_err_handle(snr_retry, 5);
      return ret;
    }
    snr_retry = 0;
  }

  ret = read_kv_snr(fru, sensor_num, value);

  return ret;
}

PAL_SENSOR_MAP ubb_sensor_map[] = {
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                       //0x00
  {"HSC_0_Power" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x01
  {"HSC_1_Power" , 0 , NULL, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},     //0x02
  {"HSC_2_Power" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x03
  {"HSC_3_Power" , 0 , NULL, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},     //0x04
  {"HSC_4_Power" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x05
  {"HSC_5_Power" , 0 , NULL, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},     //0x06
  {"HSC_6_Power" , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x07
  {"HSC_7_Power" , 0 , NULL, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},     //0x08
  {"HSC_8_Power" , 0 , NULL, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},     //0x09
  {"HSC_9_Power" , 0 , NULL, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},     //0x0A
  {"Standby_HSC_Power",  0 ,read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},//0x0B
  {"Total_HSC_Power" ,   0, read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},//0x0C
  {"Total_GPU_Power"   , 0, read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},//0x0D
  {"Altitude_Pressure0", 0, NULL, true, {0, 0, 0, 0, 0, 0, 0, 0}, PRESS},    //0x0E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                           //0x0F

  {"FPGA_0_TEMP", 0 , read_snr, true, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x10
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
  {"Inlet_0_TEMP", 0, read_snr, true, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP},  //0x1C
  {"Inlet_1_TEMP", 0, read_snr, true, {55.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP},  //0x1D
  {NULL, 0,  NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                          //0x1E
  {"IBC_TEMP"    , 0 , read_snr, true, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},      //0x1F

  {"PCB_0_TEMP",  0 , read_snr, true, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x20
  {"PCB_1_TEMP" , 0 , read_snr, true, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x21
  {"PCB_2_TEMP" , 0 , read_snr, true, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x22
  {"PCB_3_TEMP" , 0 , read_snr, true, {85.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x23
  {"PCIeRetimer_0_TEMP" , 0 , read_snr, false, {110.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x24
  {"PCIeRetimer_1_TEMP" , 0 , read_snr, false, {110.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x25
  {"PCIeRetimer_2_TEMP" , 0 , read_snr, false, {110.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x26
  {"PCIeRetimer_3_TEMP" , 0 , read_snr, false, {110.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x27
  {"PCIeRetimer_4_TEMP" , 0 , read_snr, false, {110.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x28
  {"PCIeRetimer_5_TEMP" , 0 , read_snr, false, {110.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x29
  {"PCIeRetimer_6_TEMP" , 0 , read_snr, false, {110.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x2A
  {"PCIeRetimer_7_TEMP" , 0 , read_snr, false, {110.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x2B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                                    //0x2C
  {"PCIeRetimer_MAX_TEMP", 0, read_snr,   false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},     //0x2D
  {"PCIeRetimer_MAX_VR_TEMP", 0, read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP},    //0x2E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                                    //0x2F

  {"OAM_0_ENRGY", 0,        NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, ENRGY},      //0x30
  {"OAM_0_PWR"  , 0,    read_snr, false, {1020.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x31
  {"OAM_0_VOL"  , 0,        NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},       //0x32
  {"OAM_0_TEMP_0", 0,   read_snr, false, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x33
  {"OAM_0_TEMP_1", 0,       NULL, false, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x34
  {"OAM_0_DRAM_PWR" , 0,    NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},      //0x35
  {"OAM_0_DRAM_TEMP", 0,read_snr, false, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x36
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                              //0x37
  {"OAM_1_ENRGY", 0,        NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, ENRGY},      //0x38
  {"OAM_1_PWR"  , 0,    read_snr, false, {1020.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x39
  {"OAM_1_VOL"  , 0,        NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},       //0x3A
  {"OAM_1_TEMP_0" , 0,  read_snr, false, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x3B
  {"OAM_1_TEMP_1" , 0,      NULL, false, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x3C
  {"OAM_1_DRAM_PWR" , 0,    NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},      //0x3D
  {"OAM_1_DRAM_TEMP", 0,read_snr, false, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x3E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                              //0x3F

  {"OAM_2_ENRGY", 0,        NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, ENRGY},      //0x40
  {"OAM_2_PWR"  , 0,    read_snr, false, {1020.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x41
  {"OAM_2_VOL"  , 0,        NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},       //0x42
  {"OAM_2_TEMP_0", 0,   read_snr, false, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x43
  {"OAM_2_TEMP_1", 0,       NULL, false, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x44
  {"OAM_2_DRAM_PWR" , 0,    NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},      //0x45
  {"OAM_2_DRAM_TEMP", 0,read_snr, false, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x46
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                              //0x47
  {"OAM_3_ENRGY", 0,        NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, ENRGY},      //0x48
  {"OAM_3_PWR"  , 0,    read_snr, false, {1020.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x49
  {"OAM_3_VOL"  , 0,        NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},       //0x4A
  {"OAM_3_TEMP_0" , 0,  read_snr, false, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x4B
  {"OAM_3_TEMP_1" , 0,      NULL, false, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x4C
  {"OAM_3_DRAM_PWR" , 0,    NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},      //0x4D
  {"OAM_3_DRAM_TEMP", 0,read_snr, false, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x4E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                              //0x4F

  {"OAM_4_ENRGY", 0,        NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, ENRGY},      //0x50
  {"OAM_4_PWR"  , 0,    read_snr, false, {1020.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x51
  {"OAM_4_VOL"  , 0,        NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},       //0x52
  {"OAM_4_TEMP_0", 0,   read_snr, false, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x53
  {"OAM_4_TEMP_1", 0,       NULL, false, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x54
  {"OAM_4_DRAM_PWR" , 0,    NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},      //0x55
  {"OAM_4_DRAM_TEMP", 0,read_snr, false, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x56
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                              //0x57
  {"OAM_5_ENRGY", 0,        NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, ENRGY},      //0x58
  {"OAM_5_PWR"  , 0,    read_snr, false, {1020.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x59
  {"OAM_5_VOL"  , 0,        NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},       //0x5A
  {"OAM_5_TEMP_0" , 0,  read_snr, false, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x5B
  {"OAM_5_TEMP_1" , 0,      NULL, false, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x5C
  {"OAM_5_DRAM_PWR" , 0,    NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},      //0x5D
  {"OAM_5_DRAM_TEMP", 0,read_snr, false, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x5E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0},                              //0x5F

  {"OAM_6_ENRGY", 0,        NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, ENRGY},      //0x60
  {"OAM_6_PWR"  , 0,    read_snr, false, {1020.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x61
  {"OAM_6_VOL"  , 0,        NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},       //0x62
  {"OAM_6_TEMP_0", 0,   read_snr, false, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x63
  {"OAM_6_TEMP_1", 0,       NULL, false, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x64
  {"OAM_6_DRAM_PWR" , 0,    NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},      //0x65
  {"OAM_6_DRAM_TEMP", 0,read_snr, false, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x66
  {"GPU_WARMEST_DIE_TEMP", 0, read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x67
  {"OAM_7_ENRGY", 0,        NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, ENRGY},      //0x68
  {"OAM_7_PWR"  , 0,    read_snr, false, {1020.0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x69
  {"OAM_7_VOL"  , 0,        NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT},       //0x6A
  {"OAM_7_TEMP_0" , 0,  read_snr, false, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x6B
  {"OAM_7_TEMP_1" , 0,      NULL, false, {100.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x6C
  {"OAM_7_DRAM_PWR" , 0,    NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, POWER},      //0x6D
  {"OAM_7_DRAM_TEMP", 0,read_snr, false, {105.0, 0, 0, 5.0, 0, 0, 0, 0}, TEMP}, //0x6E
  {"GPU_WARMEST_MEM_TEMP", 0, read_snr, false, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x6F
};

const uint8_t ubb_sensor_list[] = {
  HGX_SNR_PWR_GB_HSC10,
  Total_GPU_Power,

  TEMP_GB_FPGA,
  TEMP_GB_HSC0,
  TEMP_GB_HSC1,
  TEMP_GB_HSC2,
  TEMP_GB_HSC3,
  TEMP_GB_HSC4,
  TEMP_GB_HSC5,
  TEMP_GB_HSC6,
  TEMP_GB_HSC7,
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
  GPU_RETIMER_MAX_TEMP,
  GPU_RETIMER_MAX_VR_TEMP,
  TEMP_IBC,

  GPU1_PWR,
  GPU1_TEMP_0,
  GPU1_DRAM_TEMP,

  GPU2_PWR,
  GPU2_TEMP_0,
  GPU2_DRAM_TEMP,

  GPU3_PWR,
  GPU3_TEMP_0,
  GPU3_DRAM_TEMP,

  GPU4_PWR,
  GPU4_TEMP_0,
  GPU4_DRAM_TEMP,

  GPU5_PWR,
  GPU5_TEMP_0,
  GPU5_DRAM_TEMP,

  GPU6_PWR,
  GPU6_TEMP_0,
  GPU6_DRAM_TEMP,

  GPU7_PWR,
  GPU7_TEMP_0,
  GPU7_DRAM_TEMP,

  GPU8_PWR,
  GPU8_TEMP_0,
  GPU8_DRAM_TEMP,

  GPU_WARMEST_DIE_TEMP,
  GPU_WARMEST_MEMORY_TEMP,
};

size_t ubb_sensor_cnt = sizeof(ubb_sensor_list)/sizeof(uint8_t);
