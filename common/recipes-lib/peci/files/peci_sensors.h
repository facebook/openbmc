#ifndef __PECI_SENSORS_H__
#define __PECI_SENSORS_H__

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

//PECI CMD INFO
#define PECI_RETRY_TIMES                        (10)
#define PECI_CMD_RD_PKG_CONFIG                  (0xA1)
#define PECI_INDEX_PKG_TEMP                     (2)
#define PECI_INDEX_ACCUMULATED_ENERGY_STATUS    (3)
#define PECI_INDEX_THERMAL_MARGIN               (10)
#define PECI_INDEX_DIMM_THERMAL_RW              (14)
#define PECI_INDEX_TEMP_TARGET                  (16)
#define PECI_INDEX_TOTAL_TIME                   (31)
#define PECI_INDEX_PKG_IDENTIFIER               (0)
#define PECI_THERMAL_DIMM0                      (0)
#define PECI_THERMAL_DIMM1                      (1)
#define INTERNAL_CPU_ERROR_MASK                 (0x1C)
#define EXTERNAL_CPU_ERROR_MASK                 (0xE0)
#define BOTH_CPU_ERROR_MASK                     (0xFC)
#define PECI_SUCCESS                            (0x40)


typedef struct {
  uint8_t cpu_addr;
  uint8_t index;
  uint8_t dev_info;
  uint8_t para_l;
  uint8_t para_h;
} PECI_RD_PKG_CONFIG_INFO;

typedef struct {
  uint8_t cpu_id;
  uint8_t cpu_addr;
}COMMON_CPU_INFO;

//PECI CPU INFO
enum {
  CPU_ID0 = 0,
  CPU_ID1,
  CPU_ID2,
  CPU_ID3,
  CPU_ID4,
  CPU_ID5,
  CPU_ID6,
  CPU_ID7,
  CPU_ID_NUM,
};

enum {
  DIMM_CRPA = 0,
  DIMM_CRPB,
  DIMM_CRPC,
  DIMM_CRPD,
  DIMM_CRPE,
  DIMM_CRPF,
  DIMM_CRPG,
  DIMM_CRPH,
  DIMM_CNT,
};



int lib_get_cpu_temp(uint8_t cpu_id, uint8_t cpu_addr, float *value);
int lib_get_cpu_dimm_temp(uint8_t cpu_addr, uint8_t dimm_id, float *value);
int lib_get_dimm_temp_per_channel(uint8_t cpu_addr, uint8_t channel, uint8_t *value);
int lib_get_cpu_pkg_pwr(uint8_t cpu_id, uint8_t cpu_addr, float *value);
int lib_get_cpu_tjmax(uint8_t cpu_id, float *value);
int lib_get_cpu_thermal_margin(uint8_t cpu_id, float *value);
int cmd_peci_rdpkgconfig(PECI_RD_PKG_CONFIG_INFO* info, uint8_t* rx_buf, uint8_t rx_len);
int cmd_peci_get_thermal_margin(uint8_t cpu_addr, float* value);
int cmd_peci_get_tjmax(uint8_t cpu_addr, int* tjmax);
int cmd_peci_dimm_thermal_reading(uint8_t cpu_addr, uint8_t channel, uint8_t* temp);
int cmd_peci_get_cpu_err_num(uint8_t cpu_addr, int* num, uint8_t is_caterr);
int cmd_peci_accumulated_energy(uint8_t cpu_addr, uint32_t* value);
int cmd_peci_total_time(uint8_t cpu_addr, uint32_t* value);


#ifdef __cplusplus
}
#endif

#endif
