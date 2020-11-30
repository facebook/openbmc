#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <string.h>
#include <ctype.h>
#include <openbmc/kv.h>
#include <openbmc/libgpio.h>
#include <openbmc/ipmb.h>
#include <openbmc/obmc-sensors.h>
#include <openbmc/obmc-i2c.h>
#include "pal.h"

static int read_adc_val(uint8_t adc_id, float *value);
static int read_temp(uint8_t id, float *value);
static int read_nic_temp(uint8_t nic_id, float *value);

static bool is_dpb_sensor_cached = false;
static bool is_scc_sensor_cached = false;

static sensor_info_t g_sinfo[MAX_SENSOR_NUM] = {0};
static bool is_sdr_init[FRU_CNT] = {false};

//{SensorName, ID, FUNCTION, PWR_STATUS, {UCR, UNC, UNR, LCR, LNC, LNR, Pos, Neg}, unit}
PAL_SENSOR_MAP uic_sensor_map[] = {
  [UIC_ADC_P12V_DPB] =
  {"UIC_ADC_P12V_DPB", ADC0, read_adc_val, true, {13.7, 0, 0, 11.3, 0, 0, 0, 0}, VOLT},
  [UIC_ADC_P12V_STBY] =
  {"UIC_ADC_P12V_STBY", ADC1, read_adc_val, true, {13.7, 0, 0, 11.3, 0, 0, 0, 0}, VOLT},
  [UIC_ADC_P5V_STBY] =
  {"UIC_ADC_P5V_STBY", ADC2, read_adc_val, true, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [UIC_ADC_P3V3_STBY] =
  {"UIC_ADC_P3V3_STBY", ADC3, read_adc_val, true, {3.6, 0, 0, 2.97, 0, 0, 0, 0}, VOLT},
  [UIC_ADC_P3V3_RGM] =
  {"UIC_ADC_P3V3_RGM", ADC4, read_adc_val, true, {3.6, 0, 0, 2.97, 0, 0, 0, 0}, VOLT},
  [UIC_ADC_P2V5_STBY] =
  {"UIC_ADC_P2V5_STBY", ADC5, read_adc_val, true, {2.75, 0, 0, 2.25, 0, 0, 0, 0}, VOLT},
  [UIC_ADC_P1V8_STBY] =
  {"UIC_ADC_P1V8_STBY", ADC6, read_adc_val, true, {1.98, 0, 0, 1.62, 0, 0, 0, 0}, VOLT},
  [UIC_ADC_P1V2_STBY] =
  {"UIC_ADC_P1V2_STBY", ADC7, read_adc_val, true, {1.26, 0, 0, 1.08, 0, 0, 0, 0}, VOLT},
  [UIC_ADC_P1V0_STBY] =
  {"UIC_ADC_P1V0_STBY", ADC8, read_adc_val, true, {1.1, 0, 0, 0.9, 0, 0, 0, 0}, VOLT},
  [UIC_P12V_ISENSE_CUR] =
  {"UIC_P12V_ISENSE_CUR", ADC9, read_adc_val, true, {1.71, 0, 0, 0, 0, 0, 0, 0}, CURR},
  [UIC_INLET_TEMP] =
  {"UIC_INLET_TEMP", TEMP_INLET, read_temp, true, {45, 0, 0, 0, 0, 0, 0, 0}, TEMP},
};

PAL_SENSOR_MAP dpb_sensor_map[] = {
  [HDD_SMART_TEMP_00] =
  {"HDD_SMART_TEMP_00", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_01] =
  {"HDD_SMART_TEMP_01", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_02] =
  {"HDD_SMART_TEMP_02", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_03] =
  {"HDD_SMART_TEMP_03", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_04] =
  {"HDD_SMART_TEMP_04", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_05] =
  {"HDD_SMART_TEMP_05", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_06] =
  {"HDD_SMART_TEMP_06", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_07] =
  {"HDD_SMART_TEMP_07", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_08] =
  {"HDD_SMART_TEMP_08", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_09] =
  {"HDD_SMART_TEMP_09", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_10] =
  {"HDD_SMART_TEMP_10", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_11] =
  {"HDD_SMART_TEMP_11", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_12] =
  {"HDD_SMART_TEMP_12", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_13] =
  {"HDD_SMART_TEMP_13", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_14] =
  {"HDD_SMART_TEMP_14", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_15] =
  {"HDD_SMART_TEMP_15", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_16] =
  {"HDD_SMART_TEMP_16", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_17] =
  {"HDD_SMART_TEMP_17", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_18] =
  {"HDD_SMART_TEMP_18", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_19] =
  {"HDD_SMART_TEMP_19", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_20] =
  {"HDD_SMART_TEMP_20", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_21] =
  {"HDD_SMART_TEMP_21", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_22] =
  {"HDD_SMART_TEMP_22", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_23] =
  {"HDD_SMART_TEMP_23", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_24] =
  {"HDD_SMART_TEMP_24", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_25] =
  {"HDD_SMART_TEMP_25", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_26] =
  {"HDD_SMART_TEMP_26", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_27] =
  {"HDD_SMART_TEMP_27", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_28] =
  {"HDD_SMART_TEMP_28", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_29] =
  {"HDD_SMART_TEMP_29", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_30] =
  {"HDD_SMART_TEMP_30", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_31] =
  {"HDD_SMART_TEMP_31", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_32] =
  {"HDD_SMART_TEMP_32", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_33] =
  {"HDD_SMART_TEMP_33", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_34] =
  {"HDD_SMART_TEMP_34", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_35] =
  {"HDD_SMART_TEMP_35", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_P5V_SENSE_0] =
  {"HDD_P5V_SENSE_0", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_0] =
  {"HDD_P12V_SENSE_0", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_1] =
  {"HDD_P5V_SENSE_1", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_1] =
  {"HDD_P12V_SENSE_1", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_2] =
  {"HDD_P5V_SENSE_2", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_2] =
  {"HDD_P12V_SENSE_2", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_3] =
  {"HDD_P5V_SENSE_3", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_3] =
  {"HDD_P12V_SENSE_3", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_4] =
  {"HDD_P5V_SENSE_4", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_4] =
  {"HDD_P12V_SENSE_4", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_5] =
  {"HDD_P5V_SENSE_5", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_5] =
  {"HDD_P12V_SENSE_5", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_6] =
  {"HDD_P5V_SENSE_6", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_6] =
  {"HDD_P12V_SENSE_6", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_7] =
  {"HDD_P5V_SENSE_7", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_7] =
  {"HDD_P12V_SENSE_7", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_8] =
  {"HDD_P5V_SENSE_8", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_8] =
  {"HDD_P12V_SENSE_8", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_9] =
  {"HDD_P5V_SENSE_9", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_9] =
  {"HDD_P12V_SENSE_9", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_10] =
  {"HDD_P5V_SENSE_10", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_10] =
  {"HDD_P12V_SENSE_10", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_11] =
  {"HDD_P5V_SENSE_11", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_11] =
  {"HDD_P12V_SENSE_11", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_12] =
  {"HDD_P5V_SENSE_12", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_12] =
  {"HDD_P12V_SENSE_12", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_13] =
  {"HDD_P5V_SENSE_13", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_13] =
  {"HDD_P12V_SENSE_13", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_14] =
  {"HDD_P5V_SENSE_14", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_14] =
  {"HDD_P12V_SENSE_14", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_15] =
  {"HDD_P5V_SENSE_15", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_15] =
  {"HDD_P12V_SENSE_15", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_16] =
  {"HDD_P5V_SENSE_16", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_16] =
  {"HDD_P12V_SENSE_16", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_17] =
  {"HDD_P5V_SENSE_17", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_17] =
  {"HDD_P12V_SENSE_17", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_18] =
  {"HDD_P5V_SENSE_18", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_18] =
  {"HDD_P12V_SENSE_18", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_19] =
  {"HDD_P5V_SENSE_19", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_19] =
  {"HDD_P12V_SENSE_19", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_20] =
  {"HDD_P5V_SENSE_20", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_20] =
  {"HDD_P12V_SENSE_20", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_21] =
  {"HDD_P5V_SENSE_21", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_21] =
  {"HDD_P12V_SENSE_21", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_22] =
  {"HDD_P5V_SENSE_22", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_22] =
  {"HDD_P12V_SENSE_22", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_23] =
  {"HDD_P5V_SENSE_23", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_23] =
  {"HDD_P12V_SENSE_23", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_24] =
  {"HDD_P5V_SENSE_24", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_24] =
  {"HDD_P12V_SENSE_24", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_25] =
  {"HDD_P5V_SENSE_25", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_25] =
  {"HDD_P12V_SENSE_25", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_26] =
  {"HDD_P5V_SENSE_26", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_26] =
  {"HDD_P12V_SENSE_26", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_27] =
  {"HDD_P5V_SENSE_27", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_27] =
  {"HDD_P12V_SENSE_27", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_28] =
  {"HDD_P5V_SENSE_28", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_28] =
  {"HDD_P12V_SENSE_28", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_29] =
  {"HDD_P5V_SENSE_29", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_29] =
  {"HDD_P12V_SENSE_29", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_30] =
  {"HDD_P5V_SENSE_30", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_30] =
  {"HDD_P12V_SENSE_30", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_31] =
  {"HDD_P5V_SENSE_31", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_31] =
  {"HDD_P12V_SENSE_31", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_32] =
  {"HDD_P5V_SENSE_32", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_32] =
  {"HDD_P12V_SENSE_32", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_33] =
  {"HDD_P5V_SENSE_33", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_33] =
  {"HDD_P12V_SENSE_33", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_34] =
  {"HDD_P5V_SENSE_34", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_34] =
  {"HDD_P12V_SENSE_34", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_35] =
  {"HDD_P5V_SENSE_35", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_35] =
  {"HDD_P12V_SENSE_35", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [DPB_INLET_TEMP_1] =
  {"DPB_INLET_TEMP_1", EXPANDER, NULL, 0, {45, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [DPB_INLET_TEMP_2] =
  {"DPB_INLET_TEMP_2", EXPANDER, NULL, 0, {45, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [DPB_OUTLET_TEMP] =
  {"DPB_OUTLET_TEMP", EXPANDER, NULL, 0, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [DPB_HSC_P12V] =
  {"DPB_HSC_P12V", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [DPB_P3V3_STBY_SENSE] =
  {"DPB_P3V3_STBY_SENSE", EXPANDER, NULL, 0, {3.73, 0, 0, 2.87, 0, 0, 0, 0}, VOLT},
  [DPB_P5V_1_SYS_SENSE] =
  {"DPB_P5V_1_SYS_SENSE", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [DPB_P5V_2_SYS_SENSE] =
  {"DPB_P5V_2_SYS_SENSE", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [DPB_P5V_3_SYS_SENSE] =
  {"DPB_P5V_3_SYS_SENSE", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [DPB_P5V_4_SYS_SENSE] =
  {"DPB_P5V_4_SYS_SENSE", EXPANDER, NULL, 0, {5.65, 0, 0, 4.35, 0, 0, 0, 0}, VOLT},
  [DPB_HSC_P12V_CLIP] =
  {"DPB_HSC_P12V_CLIP", EXPANDER, NULL, 0, {13.63, 0, 0, 11.38, 0, 0, 0, 0}, VOLT},
  [DPB_HSC_CUR] =
  {"DPB_HSC_CUR", EXPANDER, NULL, 0, {73.6, 0, 0, 0, 0, 0, 0, 0}, CURR},
  [DPB_HSC_CUR_CLIP] =
  {"DPB_HSC_CUR_CLIP", EXPANDER, NULL, 0, {165, 0, 0, 0, 0, 0, 0, 0}, CURR},
  [DPB_HSC_PWR] =
  {"DPB_HSC_PWR", EXPANDER, NULL, 0, {920, 0, 0, 0, 0, 0, 0, 0}, POWER},
  [DPB_HSC_PWR_CLIP] =
  {"DPB_HSC_PWR_CLIP", EXPANDER, NULL, 0, {2062, 0, 0, 0, 0, 0, 0, 0}, POWER},
  [FAN_1_FRONT] =
  {"FAN_1_FRONT", EXPANDER, NULL, 0, {13000, 0, 0, 500, 0, 0, 0, 0}, FAN},
  [FAN_1_REAR] =
  {"FAN_1_REAR", EXPANDER, NULL, 0, {13000, 0, 0, 500, 0, 0, 0, 0}, FAN},
  [FAN_2_FRONT] =
  {"FAN_2_FRONT", EXPANDER, NULL, 0, {13000, 0, 0, 500, 0, 0, 0, 0}, FAN},
  [FAN_2_REAR] =
  {"FAN_2_REAR", EXPANDER, NULL, 0, {13000, 0, 0, 500, 0, 0, 0, 0}, FAN},
  [FAN_3_FRONT] =
  {"FAN_3_FRONT", EXPANDER, NULL, 0, {13000, 0, 0, 500, 0, 0, 0, 0}, FAN},
  [FAN_3_REAR] =
  {"FAN_3_REAR", EXPANDER, NULL, 0, {13000, 0, 0, 500, 0, 0, 0, 0}, FAN},
  [FAN_4_FRONT] =
  {"FAN_4_FRONT", EXPANDER, NULL, 0, {13000, 0, 0, 500, 0, 0, 0, 0}, FAN},
  [FAN_4_REAR] =
  {"FAN_4_REAR", EXPANDER, NULL, 0, {13000, 0, 0, 500, 0, 0, 0, 0}, FAN},
};

PAL_SENSOR_MAP scc_sensor_map[] = {
  [SCC_EXP_TEMP] =
  {"SCC_EXP_TEMP", EXPANDER, NULL, 0, {95, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [SCC_IOC_TEMP] =
  {"SCC_IOC_TEMP", EXPANDER, NULL, 0, {95, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [SCC_TEMP_1] =
  {"SCC_TEMP_1", EXPANDER, NULL, 0, {93, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [SCC_TEMP_2] =
  {"SCC_TEMP_2", EXPANDER, NULL, 0, {93, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [SCC_P3V3_E_SENSE] =
  {"SCC_P3V3_E_SENSE", EXPANDER, NULL, 0, {3.63, 0, 0, 2.97, 0, 0, 0, 0}, VOLT},
  [SCC_P1V8_E_SENSE] =
  {"SCC_P1V8_E_SENSE", EXPANDER, NULL, 0, {1.89, 0, 0, 1.71, 0, 0, 0, 0}, VOLT},
  [SCC_P1V5_E_SENSE] =
  {"SCC_P1V5_E_SENSE", EXPANDER, NULL, 0, {1.49, 0, 0, 1.41, 0, 0, 0, 0}, VOLT},
  [SCC_P0V92_E_SENSE] =
  {"SCC_P0V92_E_SENSE", EXPANDER, NULL, 0, {0.95, 0, 0, 0.89, 0, 0, 0, 0}, VOLT},
  [SCC_P1V8_C_SENSE] =
  {"SCC_P1V8_C_SENSE", EXPANDER, NULL, 0, {1.89, 0, 0, 1.71, 0, 0, 0, 0}, VOLT},
  [SCC_P1V5_C_SENSE] =
  {"SCC_P1V5_C_SENSE", EXPANDER, NULL, 0, {1.49, 0, 0, 1.41, 0, 0, 0, 0}, VOLT},
  [SCC_P0V865_C_SENSE] =
  {"SCC_P0V865_C_SENSE", EXPANDER, NULL, 0, {0.88, 0, 0, 0.85, 0, 0, 0, 0}, VOLT},
  [SCC_HSC_P12V] =
  {"SCC_HSC_P12V", EXPANDER, NULL, 0, {13.75, 0, 0, 11.25, 0, 0, 0, 0}, VOLT},
  [SCC_HSC_CUR] =
  {"SCC_HSC_CUR", EXPANDER, NULL, 0, {7.6, 0, 0, 0, 0, 0, 0, 0}, CURR},
  [SCC_HSC_PWR] =
  {"SCC_HSC_PWR", EXPANDER, NULL, 0, {95, 0, 0, 0, 0, 0, 0, 0}, POWER},
};

PAL_SENSOR_MAP nic_sensor_map[] = {
  [MEZZ_SENSOR_TEMP] =
  {"MEZZ_SENSOR_TEMP", MEZZ, read_nic_temp, true, {95, 0, 0, 0, 0, 0, 0, 0}, TEMP},
};

const uint8_t server_sensor_list[] = {
  BS_INLET_TEMP,
  BS_P12V_STBY,
  BS_P3V3_STBY,
  BS_P1V05_STBY,
  BS_P3V_BAT,
  BS_PVNN_PCH_STBY,
  BS_HSC_TEMP,
  BS_HSC_IN_VOLT,
  BS_HSC_IN_PWR,
  BS_HSC_OUT_CUR,
  BS_BOOT_DRV_TEMP,
  BS_PCH_TEMP,
  BS_DIMM1_TEMP,
  BS_DIMM2_TEMP,
  BS_DIMM3_TEMP,
  BS_DIMM4_TEMP,
  BS_VR_VCCIO_TEMP,
  BS_CPU_TEMP,
};

const uint8_t uic_sensor_list[] = {
  UIC_ADC_P12V_DPB,
  UIC_ADC_P12V_STBY,
  UIC_ADC_P5V_STBY,
  UIC_ADC_P3V3_STBY,
  UIC_ADC_P3V3_RGM,
  UIC_ADC_P2V5_STBY,
  UIC_ADC_P1V8_STBY,
  UIC_ADC_P1V2_STBY,
  UIC_ADC_P1V0_STBY,
  UIC_P12V_ISENSE_CUR,
  UIC_INLET_TEMP,
};

const uint8_t dpb_sensor_list[] = {
  HDD_SMART_TEMP_00,
  HDD_SMART_TEMP_01,
  HDD_SMART_TEMP_02,
  HDD_SMART_TEMP_03,
  HDD_SMART_TEMP_04,
  HDD_SMART_TEMP_05,
  HDD_SMART_TEMP_06,
  HDD_SMART_TEMP_07,
  HDD_SMART_TEMP_08,
  HDD_SMART_TEMP_09,
  HDD_SMART_TEMP_10,
  HDD_SMART_TEMP_11,
  HDD_SMART_TEMP_12,
  HDD_SMART_TEMP_13,
  HDD_SMART_TEMP_14,
  HDD_SMART_TEMP_15,
  HDD_SMART_TEMP_16,
  HDD_SMART_TEMP_17,
  HDD_SMART_TEMP_18,
  HDD_SMART_TEMP_19,
  HDD_SMART_TEMP_20,
  HDD_SMART_TEMP_21,
  HDD_SMART_TEMP_22,
  HDD_SMART_TEMP_23,
  HDD_SMART_TEMP_24,
  HDD_SMART_TEMP_25,
  HDD_SMART_TEMP_26,
  HDD_SMART_TEMP_27,
  HDD_SMART_TEMP_28,
  HDD_SMART_TEMP_29,
  HDD_SMART_TEMP_30,
  HDD_SMART_TEMP_31,
  HDD_SMART_TEMP_32,
  HDD_SMART_TEMP_33,
  HDD_SMART_TEMP_34,
  HDD_SMART_TEMP_35,
  HDD_P5V_SENSE_0,
  HDD_P12V_SENSE_0,
  HDD_P5V_SENSE_1,
  HDD_P12V_SENSE_1,
  HDD_P5V_SENSE_2,
  HDD_P12V_SENSE_2,
  HDD_P5V_SENSE_3,
  HDD_P12V_SENSE_3,
  HDD_P5V_SENSE_4,
  HDD_P12V_SENSE_4,
  HDD_P5V_SENSE_5,
  HDD_P12V_SENSE_5,
  HDD_P5V_SENSE_6,
  HDD_P12V_SENSE_6,
  HDD_P5V_SENSE_7,
  HDD_P12V_SENSE_7,
  HDD_P5V_SENSE_8,
  HDD_P12V_SENSE_8,
  HDD_P5V_SENSE_9,
  HDD_P12V_SENSE_9,
  HDD_P5V_SENSE_10,
  HDD_P12V_SENSE_10,
  HDD_P5V_SENSE_11,
  HDD_P12V_SENSE_11,
  HDD_P5V_SENSE_12,
  HDD_P12V_SENSE_12,
  HDD_P5V_SENSE_13,
  HDD_P12V_SENSE_13,
  HDD_P5V_SENSE_14,
  HDD_P12V_SENSE_14,
  HDD_P5V_SENSE_15,
  HDD_P12V_SENSE_15,
  HDD_P5V_SENSE_16,
  HDD_P12V_SENSE_16,
  HDD_P5V_SENSE_17,
  HDD_P12V_SENSE_17,
  HDD_P5V_SENSE_18,
  HDD_P12V_SENSE_18,
  HDD_P5V_SENSE_19,
  HDD_P12V_SENSE_19,
  HDD_P5V_SENSE_20,
  HDD_P12V_SENSE_20,
  HDD_P5V_SENSE_21,
  HDD_P12V_SENSE_21,
  HDD_P5V_SENSE_22,
  HDD_P12V_SENSE_22,
  HDD_P5V_SENSE_23,
  HDD_P12V_SENSE_23,
  HDD_P5V_SENSE_24,
  HDD_P12V_SENSE_24,
  HDD_P5V_SENSE_25,
  HDD_P12V_SENSE_25,
  HDD_P5V_SENSE_26,
  HDD_P12V_SENSE_26,
  HDD_P5V_SENSE_27,
  HDD_P12V_SENSE_27,
  HDD_P5V_SENSE_28,
  HDD_P12V_SENSE_28,
  HDD_P5V_SENSE_29,
  HDD_P12V_SENSE_29,
  HDD_P5V_SENSE_30,
  HDD_P12V_SENSE_30,
  HDD_P5V_SENSE_31,
  HDD_P12V_SENSE_31,
  HDD_P5V_SENSE_32,
  HDD_P12V_SENSE_32,
  HDD_P5V_SENSE_33,
  HDD_P12V_SENSE_33,
  HDD_P5V_SENSE_34,
  HDD_P12V_SENSE_34,
  HDD_P5V_SENSE_35,
  HDD_P12V_SENSE_35,
  DPB_INLET_TEMP_1,
  DPB_INLET_TEMP_2,
  DPB_OUTLET_TEMP,
  DPB_HSC_P12V,
  DPB_P3V3_STBY_SENSE,
  DPB_P5V_1_SYS_SENSE,
  DPB_P5V_2_SYS_SENSE,
  DPB_P5V_3_SYS_SENSE,
  DPB_P5V_4_SYS_SENSE,
  DPB_HSC_P12V_CLIP,
  DPB_HSC_CUR,
  DPB_HSC_CUR_CLIP,
  DPB_HSC_PWR,
  DPB_HSC_PWR_CLIP,
  FAN_1_FRONT,
  FAN_1_REAR,
  FAN_2_FRONT,
  FAN_2_REAR,
  FAN_3_FRONT,
  FAN_3_REAR,
  FAN_4_FRONT,
  FAN_4_REAR,
};

const uint8_t scc_sensor_list[] = {
  SCC_EXP_TEMP,
  SCC_IOC_TEMP,
  SCC_TEMP_1,
  SCC_TEMP_2,
  SCC_P3V3_E_SENSE,
  SCC_P1V8_E_SENSE,
  SCC_P1V5_E_SENSE,
  SCC_P0V92_E_SENSE,
  SCC_P1V8_C_SENSE,
  SCC_P1V5_C_SENSE,
  SCC_P0V865_C_SENSE,
  SCC_HSC_P12V,
  SCC_HSC_CUR,
  SCC_HSC_PWR,
};

const uint8_t nic_sensor_list[] = {
  MEZZ_SENSOR_TEMP,
};

PAL_I2C_BUS_INFO nic_info_list[] = {
  {MEZZ, I2C_NIC_BUS, NIC_INFO_SLAVE_ADDR},
};

const char *adc_label[] = {
  "ADC_P12V_DPB",
  "ADC_P12V_STBY",
  "ADC_P5V_STBY",
  "ADC_P3V3_STBY",
  "ADC_P3V3_RGM",
  "ADC_P2V5_STBY",
  "ADC_P1V8_STBY",
  "ADC_P1V2_STBY",
  "ADC_P1V0_STBY",
  "UIC_P12V_ISENSE_CUR",
};

PAL_TEMP_DEV_INFO temp_dev_list[] = {
  {"tmp75-i2c-4-4a",  "UIC_INLET_TEMP"},
};

size_t server_sensor_cnt = ARRAY_SIZE(server_sensor_list);
size_t uic_sensor_cnt = ARRAY_SIZE(uic_sensor_list);
size_t dpb_sensor_cnt = ARRAY_SIZE(dpb_sensor_list);
size_t scc_sensor_cnt = ARRAY_SIZE(scc_sensor_list);
size_t nic_sensor_cnt = ARRAY_SIZE(nic_sensor_list);

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  switch(fru) {
  case FRU_SERVER:
    *sensor_list = (uint8_t *) server_sensor_list;
    *cnt = server_sensor_cnt;
    break;
  case FRU_UIC:
    *sensor_list = (uint8_t *) uic_sensor_list;
    *cnt = uic_sensor_cnt;
    break;
  case FRU_DPB:
    *sensor_list = (uint8_t *) dpb_sensor_list;
    *cnt = dpb_sensor_cnt;
    break;
  case FRU_SCC:
    *sensor_list = (uint8_t *) scc_sensor_list;
    *cnt = scc_sensor_cnt;
    break;
  case FRU_NIC:
    *sensor_list = (uint8_t *) nic_sensor_list;
    *cnt = nic_sensor_cnt;
    break;
  default:
    if (fru > MAX_NUM_FRUS) {
      return ERR_UNKNOWN_FRU;
    }
    // Nothing to read yet.
    *sensor_list = NULL;
    *cnt = 0;
    break;
  }

  return 0;
}

static int
read_adc_val(uint8_t adc_id, float *value) {
  int ret = 0;

  if (adc_id >= ARRAY_SIZE(adc_label)) {
    return ERR_SENSOR_NA;
  }

  ret = sensors_read_adc(adc_label[adc_id], value);

  if (adc_id == ADC9) {
    // Isense(A) = Vsense(V) * 10^6 / Igain(uA/A) / Rsense(ohm)
    *value = (*value) * 1000000 / MAX15090_IGAIN / MAX15090_RSENSE;
  }

  return ret;
}

static int
read_temp(uint8_t id, float *value) {
  if (id >= ARRAY_SIZE(temp_dev_list)) {
    return ERR_SENSOR_NA;
  }
  return sensors_read(temp_dev_list[id].chip, temp_dev_list[id].label, value);
}

static int
read_nic_temp(uint8_t nic_id, float *value) {
  int fd = 0, ret = -1;
  uint8_t retry = 3, tlen = 0, rlen = 0, addr = 0, bus = 0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  if (nic_id >= ARRAY_SIZE(nic_sensor_list)) {
    return ERR_SENSOR_NA;
  }

  bus = nic_info_list[nic_id].bus;
  addr = nic_info_list[nic_id].slv_addr;

  fd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open I2C bus %d\n", __func__, bus);
    return ERR_SENSOR_NA;
  }

  //Temp Register
  tbuf[0] = NIC_INFO_TEMP_CMD;
  tlen = 1;
  rlen = 1;

  while (ret < 0 && retry-- > 0 ) {
    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
  }

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s() Temp[%d]=%x bus=%x slavaddr=%x\n", __func__, nic_id, rbuf[0], bus, addr);
#endif

  if (ret >= 0) {
    *value = (float)(rbuf[0]);
  }
  close(fd);

  return ret;
}

static int
exp_read_sensor_wrapper(uint8_t fru, uint8_t *sensor_list, int sensor_cnt, uint8_t index) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t rlen = 0, tlen = 0;
  int ret = 0, i = 0;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char fru_name[32] = {0};
  char units[64] = {0};
  float value = 0;
  EXPANDER_SENSOR_DATA *p_sensor_data = NULL;

  if (pal_get_fru_name(fru, fru_name)) {
    syslog(LOG_WARNING, "%s() Fail to get fru%d name\n", __func__, fru);
    return ERR_SENSOR_NA;
  }

  tbuf[0] = sensor_cnt;
  for(i = index ; i < sensor_cnt; i++) {
    tbuf[i + 1] = sensor_list[i];  //feed sensor number to tbuf
  }
  tlen = sensor_cnt + 1;

  //send tbuf with sensor count and numbers to get spcific sensor data from exp
  ret = expander_ipmb_wrapper(NETFN_OEM_REQ, CMD_OEM_EXP_GET_SENSOR_READING, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() expander_ipmb_wrapper failed. ret: %d\n", __func__, ret);

    //if expander doesn't respond, set all sensors value to NA and save to cache
    for(i = 0; i < sensor_cnt; i++) {
      snprintf(key, sizeof(key), "%s_sensor%d", fru_name, tbuf[i + 1]);
      snprintf(str, sizeof(str), "NA");

      if(kv_set(key, str, 0, 0) < 0) {
        syslog(LOG_WARNING, "%s() cache_set key = %s, str = %s failed.\n", __func__ , key, str);
      }
    }
    return ret;
  }

  p_sensor_data = (EXPANDER_SENSOR_DATA *)(&rbuf[1]);

  for(i = 0; i < sensor_cnt; i++) {
    if (p_sensor_data[i].sensor_status != 0) {
      //if sensor status byte is not 0, means sensor reading is unavailable
      snprintf(str, sizeof(str), "NA");
    }
    else {
      // search the corresponding sensor table to fill up the raw data and status
      pal_get_sensor_units(fru, p_sensor_data[i].sensor_num, units);
      if (strncmp(units, "C", sizeof(units)) == 0) {
        value = p_sensor_data[i].raw_data_1;
      }
      else if (strncmp(units, "RPM", sizeof(units)) == 0) {
        value = (((p_sensor_data[i].raw_data_1 << 8) + p_sensor_data[i].raw_data_2));
        value *= 10;
      }
      else if (strncmp(units, "Watts", sizeof(units)) == 0) {
        value = (((p_sensor_data[i].raw_data_1 << 8) + p_sensor_data[i].raw_data_2));
      }
      else {
        value = (((p_sensor_data[i].raw_data_1 << 8) + p_sensor_data[i].raw_data_2));
        value /= 100;
      }
      snprintf(str, sizeof(str), "%.2f",(float)value);
    }

    //cache sensor reading
    snprintf(key, sizeof(key), "%s_sensor%d", fru_name, p_sensor_data[i].sensor_num);
    if (kv_set(key, str, 0, 0) < 0) {
      syslog(LOG_WARNING, "%s() cache_set key = %s, str = %s failed.\n", __func__ , key, str);
    }
  }

  return ret;
}

static int
sensor_num_to_index(uint8_t *sensor_list, int list_cnt, uint8_t sensor_num) {
  int i = 0;

  for (i = 0; i < list_cnt; i++) {
    if (sensor_list[i] == sensor_num) {
      return i;
    }
  }
  syslog(LOG_ERR, "%s(), Failed to get index, sensor_num: %d\n", __func__, sensor_num);

  return -1;
}

static int
expander_sensor_check(uint8_t fru, uint8_t sensor_num) {
  int ret = 0, remain = 0, sensor_cnt = 0, index = 0;
  uint8_t *sensor_list = NULL;
  int read_cnt = 1; // default is single sensor reading

  if (fru != FRU_SCC && fru != FRU_DPB) {
    syslog(LOG_ERR, "%s() Invalid FRU%u \n", __func__, fru);
    return -1;
  }

  ret = pal_get_fru_sensor_list(fru, &sensor_list, &sensor_cnt);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() get list failed \n", __func__);
    return ret;
  }

  index = sensor_num_to_index(sensor_list, sensor_cnt, sensor_num);
  if (index < 0) {
    syslog(LOG_ERR, "%s() index failed \n", __func__);
    return -1;
  }

  if ( (fru == FRU_DPB && sensor_num == dpb_sensor_list[0]) || (fru == FRU_SCC && sensor_num == scc_sensor_list[0]) ) {
    remain = sensor_cnt;

    // read all sensors
    while (remain > 0) {
      read_cnt = (remain > MAX_EXP_IPMB_SENSOR_COUNT) ? MAX_EXP_IPMB_SENSOR_COUNT : remain;
      ret = exp_read_sensor_wrapper(fru, sensor_list, read_cnt, index);
      if (ret < 0) {
        syslog(LOG_ERR, "%s() wrapper ret%d \n", __func__, ret);
        break;
      }
      remain -= read_cnt;
      index += read_cnt;
    };

    if (fru == FRU_DPB) {
      is_dpb_sensor_cached = true;
    } else {
      is_scc_sensor_cached = true;
    }
  }
  else {
    if ( (fru == FRU_DPB && is_dpb_sensor_cached == true) || (fru == FRU_SCC && is_scc_sensor_cached == true) ) {
      return 0;
    }
    // read single sensor
    ret = exp_read_sensor_wrapper(fru, sensor_list, read_cnt, index);
    if (ret < 0) {
      syslog(LOG_ERR, "%s() wrapper ret%d \n", __func__, ret);
    }
  }

  return ret;
}

static int
expander_sensor_read_cache(uint8_t fru, uint8_t sensor_num, char *key, void *value) {
  char cache_value[MAX_VALUE_LEN] = {0};
  int ret = 0;

  ret = kv_get(key, cache_value, NULL, 0);
  if (ret != 0) {
    syslog(LOG_ERR, "%s(), Failed to get key: %s, ret: %d\n", __func__, key, ret);
    return ret;
  }

  if (strncmp(cache_value, "NA", sizeof(cache_value)) == 0) {
    return ERR_SENSOR_NA;
  }

  *(float *) value = atof(cache_value);
  return ret;
}

static int
get_sdr_path(uint8_t fru, char *path) {
  char fru_name[MAX_FRU_CMD_STR] = {0};

  switch(fru) {
  case FRU_SERVER:
    if (pal_get_fru_name(fru, fru_name) < 0) {
      return PAL_ENOTSUP;
    }
    break;
  default:
    syslog(LOG_WARNING, "%s() Wrong fru id %u\n", __func__, fru);
    return PAL_ENOTSUP;
  }

  snprintf(path, MAX_SDR_PATH, SDR_PATH, fru_name);
  if (access(path, F_OK) == -1) {
    //Todo: enable it after BIC ready
    //syslog(LOG_WARNING, "%s() Failed to access %s, err: %s\n", __func__, path, strerror(errno));
    return PAL_ENOTSUP;
  }

  return PAL_EOK;
}

static int
read_sdr_info(char *path, sensor_info_t *sinfo) {
  int fd = 0, ret = 0;
  uint8_t bytes_rd = 0, snr_num = 0;
  sdr_full_t *sdr_buf = NULL;

  if (access(path, F_OK) == -1) {
    syslog(LOG_WARNING, "%s() Failed to access %s, err: %s\n", __func__, path, strerror(errno));
    return -1;
  }

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open %s, err: %s\n", __func__, path, strerror(errno));
    return -1;
  }

  ret = pal_flock_retry(fd);
  if (ret == -1) {
    syslog(LOG_WARNING, "%s() Failed to flock on %s, err: %s\n", __func__, path, strerror(errno));
    goto end;
  }

  sdr_buf = calloc(1, sizeof(sdr_full_t));
  if (sdr_buf == NULL) {
    syslog(LOG_WARNING, "%s() Failed to calloc sdr structure, size: %d bytes\n", __func__, sizeof(sdr_full_t));
    ret = -1;
    goto end;
  }

  while ((bytes_rd = read(fd, sdr_buf, sizeof(sdr_full_t))) > 0) {
    if (bytes_rd != sizeof(sdr_full_t)) {
      syslog(LOG_WARNING, "%s() Read error: returns %u bytes\n", __func__, bytes_rd);
      ret = -1;
      goto end;
    }

    snr_num = sdr_buf->sensor_num;
    sinfo[snr_num].valid = true;

    memcpy(&sinfo[snr_num].sdr, sdr_buf, sizeof(sdr_full_t));
  }

end:
  if (sdr_buf != NULL) {
    free(sdr_buf);
  }
  if (fd >= 0) {
    if (pal_unflock_retry(fd) < 0) {
      syslog(LOG_WARNING, "%s() Failed to unflock on %s, err: %s", __func__, path, strerror(errno));
    }
    close(fd);
  }

  return ret;
}

static int
sdr_init(uint8_t fru) {
  if (is_sdr_init[fru] == false) {
    if (pal_sensor_sdr_init(fru, g_sinfo) < 0) {
      return PAL_ENOTREADY;
    }
    is_sdr_init[fru] = true;
  }

  return 0;
}

/*
 * Ref: IPMI spec v2 Section:36.3
 *
 * y = (mx + b * 10^b_exp) * 10^r_exp
 */
static int
convert_sensor_reading(sdr_full_t *sdr, uint8_t sensor_value, float *out_value) {
  int x = 0;
  uint8_t m_lsb = 0, m_msb = 0;
  uint16_t m = 0;
  uint8_t b_lsb = 0, b_msb = 0;
  uint16_t b = 0;
  int8_t b_exp = 0, r_exp = 0;

  if ((sdr->sensor_units1 & 0xC0) == 0x00) {  // unsigned
    x = sensor_value;
  } else if ((sdr->sensor_units1 & 0xC0) == 0x40) {  // 1's complements
    x = (sensor_value & 0x80) ? (0-(~sensor_value)) : sensor_value;
  } else if ((sdr->sensor_units1 & 0xC0) == 0x80) {  // 2's complements
    x = (int8_t)sensor_value;
  } else { // Does not return reading
    return ERR_SENSOR_NA;
  }

  m_lsb = sdr->m_val;
  m_msb = sdr->m_tolerance >> 6;
  m = (m_msb << 8) | m_lsb;

  b_lsb = sdr->b_val;
  b_msb = sdr->b_accuracy >> 6;
  b = (b_msb << 8) | b_lsb;

  // exponents are 2's complement 4-bit number
  b_exp = sdr->rb_exp & 0xF;
  if (b_exp > 7) {
    b_exp = (~b_exp + 1) & 0xF;
    b_exp = -b_exp;
  }

  r_exp = (sdr->rb_exp >> 4) & 0xF;
  if (r_exp > 7) {
    r_exp = (~r_exp + 1) & 0xF;
    r_exp = -r_exp;
  }

  *out_value = (float)(((m * x) + (b * pow(10, b_exp))) * (pow(10, r_exp)));

  return 0;
}

static int
pal_bic_sensor_read_raw(uint8_t fru, uint8_t sensor_num, float *value) {
  ipmi_sensor_reading_t sensor = {0};
  sdr_full_t *sdr = NULL;

  if (access(SERVER_SENSOR_LOCK, F_OK) == 0) { // BIC is updating VR
    return READING_SKIP;
  }

  /* Todo: Wait BIC lib ready
  if (bic_get_sensor_reading(fru, sensor_num, &sensor, NONE_INTF) < 0) {
    syslog(LOG_WARNING, "%s() Failed to run bic_get_sensor_reading(). fru: %x, snr#0x%x", __func__, fru, sensor_num);
    return ERR_SENSOR_NA;
  } */

  if (sensor.flags & BIC_SNR_FLAG_READ_NA) {
    return ERR_SENSOR_NA;
  }

  sdr = &g_sinfo[sensor_num].sdr;
  if (sdr->type != SDR_TYPE_FULL_SENSOR_RECORD) {
    *value = sensor.value;
    return 0;
  }

  return convert_sensor_reading(sdr, sensor.value, value);
}

int
pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  char path[MAX_SDR_PATH] = {0};
  int retry = MAX_RETRY;
  int ret = 0;

  if (fru != FRU_SERVER) {
    return PAL_ENOTSUP;
  }

  if (is_sdr_init[fru] == true) {
    memcpy(sinfo, g_sinfo, sizeof(sensor_info_t) * MAX_SENSOR_NUM);
    goto end;
  }

  ret = get_sdr_path(fru, path);
  if (ret < 0) {
    goto end;
  }

  while (retry-- > 0) {
    ret = read_sdr_info(path, sinfo);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() Failed to run _sdr_init, retry..%d\n", __func__, retry);
      msleep(200);
      continue;
    } else {
      memcpy(g_sinfo, sinfo, sizeof(sensor_info_t) * MAX_SENSOR_NUM);
      is_sdr_init[fru] = true;
      break;
    }
  }

end:
  return ret;
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char fru_name[32] = {0};
  int ret = 0;
  uint8_t id = 0;

  if (pal_get_fru_name(fru, fru_name)) {
    syslog(LOG_WARNING, "%s() Fail to get fru%d name\n", __func__, fru);
    return ERR_SENSOR_NA;
  }

  snprintf(key, sizeof(key), "%s_sensor%d", fru_name, sensor_num);

  switch(fru) {
  case FRU_SERVER:
    if (sdr_init(fru) == PAL_ENOTREADY) {
      ret = ERR_SENSOR_NA;
    } else {
      ret = pal_bic_sensor_read_raw(fru, sensor_num, (float*)value);
    }
    break;
  case FRU_UIC:
    id = uic_sensor_map[sensor_num].id;
    ret = uic_sensor_map[sensor_num].read_sensor(id, (float*) value);
    break;
  case FRU_DPB:
  case FRU_SCC:
    if (0 != (ret = expander_sensor_check(fru, sensor_num))) {
       syslog(LOG_WARNING, "%s() expander_sensor_check error, ret:%d name\n", __func__, ret);
       break;
    }
    if (0 != (ret = expander_sensor_read_cache(fru, sensor_num, key, value))) {
       syslog(LOG_WARNING, "%s() expander_sensor_read_cache error, ret:%d name\n", __func__, ret);
    }
    break;
  case FRU_NIC:
    id = nic_sensor_map[sensor_num].id;
    ret = nic_sensor_map[sensor_num].read_sensor(id, (float*) value);
    break;
  default:
    return ERR_SENSOR_NA;
  }

  if (ret != 0) {
    if (ret == ERR_SENSOR_NA || ret == -1) {
      strncpy(str, "NA", sizeof(str));
    } else {
      return ret;
    }
  } else {
    snprintf(str, sizeof(str), "%.2f",*((float*)value));
  }

  if (kv_set(key, str, 0, 0) < 0) {
    syslog(LOG_WARNING, "%s() cache_set key = %s, str = %s failed.\n", __func__, key, str);
    return ERR_SENSOR_NA;
  }

  return ret;
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  switch(fru) {
  case FRU_UIC:
    snprintf(name, MAX_SENSOR_NAME_SIZE, "%s", uic_sensor_map[sensor_num].snr_name);
    break;
  case FRU_DPB:
    snprintf(name, MAX_SENSOR_NAME_SIZE, "%s", dpb_sensor_map[sensor_num].snr_name);
    break;
  case FRU_SCC:
    snprintf(name, MAX_SENSOR_NAME_SIZE, "%s", scc_sensor_map[sensor_num].snr_name);
    break;
  case FRU_NIC:
    snprintf(name, MAX_SENSOR_NAME_SIZE, "%s", nic_sensor_map[sensor_num].snr_name);
    break;
  default:
    return ERR_UNKNOWN_FRU;
  }

  return 0;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  float *val = (float*) value;
  PAL_SENSOR_MAP * sensor_map = NULL;

  switch (fru) {
  case FRU_UIC:
    sensor_map = uic_sensor_map;
    break;
  case FRU_DPB:
    sensor_map = dpb_sensor_map;
    break;
  case FRU_SCC:
    sensor_map = scc_sensor_map;
    break;
  case FRU_NIC:
    sensor_map = nic_sensor_map;
    break;
  default:
    return ERR_UNKNOWN_FRU;
  }

  switch(thresh) {
  case UCR_THRESH:
    *val = sensor_map[sensor_num].snr_thresh.ucr_thresh;
    break;
  case UNC_THRESH:
    *val = sensor_map[sensor_num].snr_thresh.unc_thresh;
    break;
  case UNR_THRESH:
    *val = sensor_map[sensor_num].snr_thresh.unr_thresh;
    break;
  case LCR_THRESH:
    *val = sensor_map[sensor_num].snr_thresh.lcr_thresh;
    break;
  case LNC_THRESH:
    *val = sensor_map[sensor_num].snr_thresh.lnc_thresh;
    break;
  case LNR_THRESH:
    *val = sensor_map[sensor_num].snr_thresh.lnr_thresh;
    break;
  case POS_HYST:
    *val = sensor_map[sensor_num].snr_thresh.pos_hyst;
    break;
  case NEG_HYST:
    *val = sensor_map[sensor_num].snr_thresh.neg_hyst;
    break;
  default:
    syslog(LOG_WARNING, "%s() Threshold type error value=%d\n", __func__, thresh);
    return -1;
  }

  return 0;
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  uint8_t sensor_units = 0;

  switch (fru) {
  case FRU_UIC:
    sensor_units = uic_sensor_map[sensor_num].units;
    break;
  case FRU_DPB:
    sensor_units = dpb_sensor_map[sensor_num].units;
    break;
  case FRU_SCC:
    sensor_units = scc_sensor_map[sensor_num].units;
    break;
  case FRU_NIC:
    sensor_units = nic_sensor_map[sensor_num].units;
    break;
  default:
    return ERR_UNKNOWN_FRU;
  }

  switch(sensor_units) {
  case UNSET_UNIT:
    strcpy(units, "");
    break;
  case TEMP:
    sprintf(units, "C");
    break;
  case FAN:
    sprintf(units, "RPM");
    break;
  case PERCENT:
    sprintf(units, "%%");
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
    syslog(LOG_WARNING, "%s() unit not found, sensor number: %x, unit: %u\n", __func__, sensor_num, sensor_units);
    break;
  }

  return 0;
}

bool
pal_sensor_is_cached(uint8_t fru, uint8_t sensor_num) {
  if (fru == FRU_DPB || fru == FRU_SCC) {
    return false;
  }
  return true;
}
