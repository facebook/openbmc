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
#include <openbmc/obmc-sensors.h>
#include <openbmc/obmc-i2c.h>
#include <facebook/fbgc_gpio.h>
#include <facebook/bic.h>
#include "pal.h"

#define ADC128_E1S_PIN_CNT  6
#define MAX_E1S_VOL_SNR_SKIP   2

static int read_adc_val(uint8_t adc_id, float *value);
static int read_temp(uint8_t id, float *value);
static int read_nic_temp(uint8_t nic_id, float *value);
static int read_e1s_temp(uint8_t e1s_id, float *value);
static int read_voltage_e1s(uint8_t id, float *value);
static int read_current_iocm(uint8_t id, float *value);
static int read_voltage_nic(uint8_t id, float *value);
static int read_voltage_iocm(uint8_t id, float *value);
static int read_ioc_temp(uint8_t id, float *value);
static int set_e1s_sensor_name(char *sensor_name, char side);
static void apply_inlet_correction(float *value, inlet_corr_t *ict, size_t ict_cnt);
static int read_dpb_vol_wrapper(uint8_t id, float *value);

static bool is_dpb_sensor_cached = false;
static bool is_scc_sensor_cached = false;

static sensor_info_t g_sinfo[MAX_SENSOR_NUM + 1] = {0};
static bool is_sdr_init[FRU_CNT] = {false};

static uint8_t dpb_source_info = UNKNOWN_SOURCE;
static uint8_t iocm_source_info = UNKNOWN_SOURCE;

static bool owning_iocm_snr_flag = false;
static bool iocm_snr_ready = false;
static uint8_t pre_status = 0xff;
static bool e1s_removed[2] = {0};
static bool iocm_removed = false;
static uint8_t e1s_adc_skip_times[ADC128_E1S_PIN_CNT] = {0};
static uint8_t max_iocm_reinit_times = MAX_RETRY;
static bool scc_thresh_init = false;
static bool dpb_thresh_init = false;

//{SensorName, ID, FUNCTION, STBY_READ, {UCR, UNC, UNR, LCR, LNC, LNR, Pos, Neg}, unit}
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
  {"HDD_SMART_TEMP_00", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_01] =
  {"HDD_SMART_TEMP_01", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_02] =
  {"HDD_SMART_TEMP_02", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_03] =
  {"HDD_SMART_TEMP_03", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_04] =
  {"HDD_SMART_TEMP_04", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_05] =
  {"HDD_SMART_TEMP_05", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_06] =
  {"HDD_SMART_TEMP_06", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_07] =
  {"HDD_SMART_TEMP_07", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_08] =
  {"HDD_SMART_TEMP_08", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_09] =
  {"HDD_SMART_TEMP_09", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_10] =
  {"HDD_SMART_TEMP_10", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_11] =
  {"HDD_SMART_TEMP_11", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_12] =
  {"HDD_SMART_TEMP_12", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_13] =
  {"HDD_SMART_TEMP_13", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_14] =
  {"HDD_SMART_TEMP_14", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_15] =
  {"HDD_SMART_TEMP_15", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_16] =
  {"HDD_SMART_TEMP_16", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_17] =
  {"HDD_SMART_TEMP_17", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_18] =
  {"HDD_SMART_TEMP_18", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_19] =
  {"HDD_SMART_TEMP_19", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_20] =
  {"HDD_SMART_TEMP_20", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_21] =
  {"HDD_SMART_TEMP_21", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_22] =
  {"HDD_SMART_TEMP_22", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_23] =
  {"HDD_SMART_TEMP_23", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_24] =
  {"HDD_SMART_TEMP_24", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_25] =
  {"HDD_SMART_TEMP_25", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_26] =
  {"HDD_SMART_TEMP_26", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_27] =
  {"HDD_SMART_TEMP_27", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_28] =
  {"HDD_SMART_TEMP_28", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_29] =
  {"HDD_SMART_TEMP_29", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_30] =
  {"HDD_SMART_TEMP_30", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_31] =
  {"HDD_SMART_TEMP_31", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_32] =
  {"HDD_SMART_TEMP_32", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_33] =
  {"HDD_SMART_TEMP_33", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_34] =
  {"HDD_SMART_TEMP_34", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_SMART_TEMP_35] =
  {"HDD_SMART_TEMP_35", EXPANDER, NULL, false, {61, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [HDD_P5V_SENSE_0] =
  {"HDD_P5V_SENSE_0", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_0] =
  {"HDD_P12V_SENSE_0", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_1] =
  {"HDD_P5V_SENSE_1", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_1] =
  {"HDD_P12V_SENSE_1", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_2] =
  {"HDD_P5V_SENSE_2", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_2] =
  {"HDD_P12V_SENSE_2", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_3] =
  {"HDD_P5V_SENSE_3", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_3] =
  {"HDD_P12V_SENSE_3", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_4] =
  {"HDD_P5V_SENSE_4", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_4] =
  {"HDD_P12V_SENSE_4", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_5] =
  {"HDD_P5V_SENSE_5", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_5] =
  {"HDD_P12V_SENSE_5", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_6] =
  {"HDD_P5V_SENSE_6", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_6] =
  {"HDD_P12V_SENSE_6", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_7] =
  {"HDD_P5V_SENSE_7", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_7] =
  {"HDD_P12V_SENSE_7", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_8] =
  {"HDD_P5V_SENSE_8", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_8] =
  {"HDD_P12V_SENSE_8", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_9] =
  {"HDD_P5V_SENSE_9", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_9] =
  {"HDD_P12V_SENSE_9", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_10] =
  {"HDD_P5V_SENSE_10", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_10] =
  {"HDD_P12V_SENSE_10", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_11] =
  {"HDD_P5V_SENSE_11", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_11] =
  {"HDD_P12V_SENSE_11", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_12] =
  {"HDD_P5V_SENSE_12", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_12] =
  {"HDD_P12V_SENSE_12", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_13] =
  {"HDD_P5V_SENSE_13", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_13] =
  {"HDD_P12V_SENSE_13", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_14] =
  {"HDD_P5V_SENSE_14", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_14] =
  {"HDD_P12V_SENSE_14", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_15] =
  {"HDD_P5V_SENSE_15", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_15] =
  {"HDD_P12V_SENSE_15", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_16] =
  {"HDD_P5V_SENSE_16", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_16] =
  {"HDD_P12V_SENSE_16", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_17] =
  {"HDD_P5V_SENSE_17", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_17] =
  {"HDD_P12V_SENSE_17", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_18] =
  {"HDD_P5V_SENSE_18", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_18] =
  {"HDD_P12V_SENSE_18", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_19] =
  {"HDD_P5V_SENSE_19", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_19] =
  {"HDD_P12V_SENSE_19", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_20] =
  {"HDD_P5V_SENSE_20", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_20] =
  {"HDD_P12V_SENSE_20", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_21] =
  {"HDD_P5V_SENSE_21", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_21] =
  {"HDD_P12V_SENSE_21", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_22] =
  {"HDD_P5V_SENSE_22", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_22] =
  {"HDD_P12V_SENSE_22", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_23] =
  {"HDD_P5V_SENSE_23", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_23] =
  {"HDD_P12V_SENSE_23", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_24] =
  {"HDD_P5V_SENSE_24", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_24] =
  {"HDD_P12V_SENSE_24", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_25] =
  {"HDD_P5V_SENSE_25", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_25] =
  {"HDD_P12V_SENSE_25", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_26] =
  {"HDD_P5V_SENSE_26", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_26] =
  {"HDD_P12V_SENSE_26", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_27] =
  {"HDD_P5V_SENSE_27", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_27] =
  {"HDD_P12V_SENSE_27", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_28] =
  {"HDD_P5V_SENSE_28", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_28] =
  {"HDD_P12V_SENSE_28", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_29] =
  {"HDD_P5V_SENSE_29", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_29] =
  {"HDD_P12V_SENSE_29", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_30] =
  {"HDD_P5V_SENSE_30", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_30] =
  {"HDD_P12V_SENSE_30", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_31] =
  {"HDD_P5V_SENSE_31", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_31] =
  {"HDD_P12V_SENSE_31", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_32] =
  {"HDD_P5V_SENSE_32", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_32] =
  {"HDD_P12V_SENSE_32", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_33] =
  {"HDD_P5V_SENSE_33", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_33] =
  {"HDD_P12V_SENSE_33", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_34] =
  {"HDD_P5V_SENSE_34", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_34] =
  {"HDD_P12V_SENSE_34", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [HDD_P5V_SENSE_35] =
  {"HDD_P5V_SENSE_35", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [HDD_P12V_SENSE_35] =
  {"HDD_P12V_SENSE_35", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [DPB_INLET_TEMP_1] =
  {"DPB_INLET_TEMP_1", EXPANDER, NULL, false, {50, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [DPB_INLET_TEMP_2] =
  {"DPB_INLET_TEMP_2", EXPANDER, NULL, false, {50, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [DPB_OUTLET_TEMP] =
  {"DPB_OUTLET_TEMP", EXPANDER, NULL, false, {60, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [DPB_HSC_P12V] =
  {"DPB_HSC_P12V", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [DPB_P3V3_STBY_SENSE] =
  {"DPB_P3V3_STBY_SENSE", EXPANDER, NULL, false, {3.63, 0, 0, 2.97, 0, 0, 0, 0}, VOLT},
  [DPB_P3V3_IN_SENSE] =
  {"DPB_P3V3_IN_SENSE", EXPANDER, NULL, false, {3.63, 0, 0, 2.97, 0, 0, 0, 0}, VOLT},
  [DPB_P5V_1_SYS_SENSE] =
  {"DPB_P5V_1_SYS_SENSE", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [DPB_P5V_2_SYS_SENSE] =
  {"DPB_P5V_2_SYS_SENSE", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [DPB_P5V_3_SYS_SENSE] =
  {"DPB_P5V_3_SYS_SENSE", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [DPB_P5V_4_SYS_SENSE] =
  {"DPB_P5V_4_SYS_SENSE", EXPANDER, NULL, false, {5.5, 0, 0, 4.5, 0, 0, 0, 0}, VOLT},
  [DPB_HSC_P12V_CLIP] =
  {"DPB_HSC_P12V_CLIP", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [DPB_HSC_CUR] =
  {"DPB_HSC_CUR", EXPANDER, NULL, false, {70, 0, 0, 0, 0, 0, 0, 0}, CURR},
  [DPB_HSC_CUR_CLIP] =
  {"DPB_HSC_CUR_CLIP", EXPANDER, NULL, false, {165, 0, 0, 0, 0, 0, 0, 0}, CURR},
  [DPB_HSC_PWR] =
  {"DPB_HSC_PWR", EXPANDER, NULL, false, {875, 0, 0, 0, 0, 0, 0, 0}, POWER},
  [DPB_HSC_PWR_CLIP] =
  {"DPB_HSC_PWR_CLIP", EXPANDER, NULL, false, {2062, 0, 0, 0, 0, 0, 0, 0}, POWER},
  [FAN_0_FRONT] =
  {"FAN_0_FRONT", EXPANDER, NULL, false, {13000, 12700, 0, 800, 700, 0, 0, 0}, FAN},
  [FAN_0_REAR] =
  {"FAN_0_REAR", EXPANDER, NULL, false, {13000, 12700, 0, 800, 700, 0, 0, 0}, FAN},
  [FAN_1_FRONT] =
  {"FAN_1_FRONT", EXPANDER, NULL, false, {13000, 12700, 0, 800, 700, 0, 0, 0}, FAN},
  [FAN_1_REAR] =
  {"FAN_1_REAR", EXPANDER, NULL, false, {13000, 12700, 0, 800, 700, 0, 0, 0}, FAN},
  [FAN_2_FRONT] =
  {"FAN_2_FRONT", EXPANDER, NULL, false, {13000, 12700, 0, 800, 700, 0, 0, 0}, FAN},
  [FAN_2_REAR] =
  {"FAN_2_REAR", EXPANDER, NULL, false, {13000, 12700, 0, 800, 700, 0, 0, 0}, FAN},
  [FAN_3_FRONT] =
  {"FAN_3_FRONT", EXPANDER, NULL, false, {13000, 12700, 0, 800, 700, 0, 0, 0}, FAN},
  [FAN_3_REAR] =
  {"FAN_3_REAR", EXPANDER, NULL, false, {13000, 12700, 0, 800, 700, 0, 0, 0}, FAN},
  [AIRFLOW] =
  {"AIRFLOW", EXPANDER, NULL, false, {0, 0, 0, 0, 0, 0, 0, 0}, FLOW},
};

PAL_SENSOR_MAP scc_sensor_map[] = {
  [SCC_EXP_TEMP] =
  {"SCC_EXP_TEMP", EXPANDER, NULL, false, {95, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [SCC_IOC_TEMP] =
  {"SCC_IOC_TEMP", EXPANDER, read_ioc_temp, false, {95, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [SCC_TEMP_1] =
  {"SCC_TEMP_1", EXPANDER, NULL, false, {95, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [SCC_TEMP_2] =
  {"SCC_TEMP_2", EXPANDER, NULL, false, {95, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [SCC_P3V3_E_SENSE] =
  {"SCC_P3V3_E_SENSE", EXPANDER, NULL, false, {3.63, 0, 0, 2.97, 0, 0, 0, 0}, VOLT},
  [SCC_P1V8_E_SENSE] =
  {"SCC_P1V8_E_SENSE", EXPANDER, NULL, false, {1.89, 0, 0, 1.71, 0, 0, 0, 0}, VOLT},
  [SCC_P1V5_E_SENSE] =
  {"SCC_P1V5_E_SENSE", EXPANDER, NULL, false, {1.52, 0, 0, 1.37, 0, 0, 0, 0}, VOLT},
  [SCC_P0V92_E_SENSE] =
  {"SCC_P0V92_E_SENSE", EXPANDER, NULL, false, {0.95, 0, 0, 0.89, 0, 0, 0, 0}, VOLT},
  [SCC_P1V8_C_SENSE] =
  {"SCC_P1V8_C_SENSE", EXPANDER, NULL, false, {1.89, 0, 0, 1.71, 0, 0, 0, 0}, VOLT},
  [SCC_P1V5_C_SENSE] =
  {"SCC_P1V5_C_SENSE", EXPANDER, NULL, false, {1.52, 0, 0, 1.37, 0, 0, 0, 0}, VOLT},
  [SCC_P0V865_C_SENSE] =
  {"SCC_P0V865_C_SENSE", EXPANDER, NULL, false, {0.89, 0, 0, 0.83, 0, 0, 0, 0}, VOLT},
  [SCC_HSC_P12V] =
  {"SCC_HSC_P12V", EXPANDER, NULL, false, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT},
  [SCC_HSC_CUR] =
  {"SCC_HSC_CUR", EXPANDER, NULL, false, {7.58, 0, 0, 0, 0, 0, 0, 0}, CURR},
  [SCC_HSC_PWR] =
  {"SCC_HSC_PWR", EXPANDER, NULL, false, {95, 0, 0, 0, 0, 0, 0, 0}, POWER},
};

PAL_SENSOR_MAP nic_sensor_map[] = {
  [NIC_SENSOR_TEMP] =
  {"NIC_SENSOR_TEMP", NIC, read_nic_temp, true, {95, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [NIC_SENSOR_P12V] =
  {"NIC_SENSOR_P12V", ADC128_IN6, read_voltage_nic, false, {13, 0, 0, 11, 0, 0, 0, 0}, VOLT},
  [NIC_SENSOR_CUR] =
  {"NIC_SENSOR_CUR", ADC128_IN7, read_voltage_nic, false, {2.2, 0, 0, 0, 0, 0, 0, 0}, CURR},
};

PAL_SENSOR_MAP e1s_sensor_map[] = {
  [E1S0_CUR] =
  {"E1S_X0_CUR", ADC128_IN0, read_voltage_e1s, false, {1.6, 0, 0, 0, 0, 0, 0, 0}, CURR},
  [E1S1_CUR] =
  {"E1S_X1_CUR", ADC128_IN1, read_voltage_e1s, false, {1.6, 0, 0, 0, 0, 0, 0, 0}, CURR},
  [E1S0_TEMP] =
  {"E1S_X0_TEMP", T5_E1S0_T7_IOC_AVENGER, read_e1s_temp, false, {70, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [E1S1_TEMP] =
  {"E1S_X1_TEMP", T5_E1S1_T7_IOCM_VOLT, read_e1s_temp, false, {70, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [E1S0_P12V] =
  {"E1S_X0_P12V", ADC128_IN2, read_voltage_e1s, false, {13, 0, 0, 11, 0, 0, 0, 0}, VOLT},
  [E1S1_P12V] =
  {"E1S_X1_P12V", ADC128_IN3, read_voltage_e1s, false, {13, 0, 0, 11, 0, 0, 0, 0}, VOLT},
  [E1S0_P3V3] =
  {"E1S_X0_P3V3", ADC128_IN4, read_voltage_e1s, false, {3.465, 0, 0, 2.97, 0, 0, 0, 0}, VOLT},
  [E1S1_P3V3] =
  {"E1S_X1_P3V3", ADC128_IN5, read_voltage_e1s, false, {3.465, 0, 0, 2.97, 0, 0, 0, 0}, VOLT},
};

PAL_SENSOR_MAP iocm_sensor_map[] = {
  [IOCM_P3V3_STBY] =
  {"IOCM_P3V3_STBY", ADS1015_IN0, read_voltage_iocm, false, {3.63, 0, 0, 2.97, 0, 0, 0, 0}, VOLT},
  [IOCM_P1V8] =
  {"IOCM_P1V8", ADS1015_IN1, read_voltage_iocm, false, {1.89, 0, 0, 1.71, 0, 0, 0, 0}, VOLT},
  [IOCM_P1V5] =
  {"IOCM_P1V5", ADS1015_IN2, read_voltage_iocm, false, {1.49, 0, 0, 1.41, 0, 0, 0, 0}, VOLT},
  [IOCM_P0V865] =
  {"IOCM_P0V865", ADS1015_IN3, read_voltage_iocm, false, {0.89, 0, 0, 0.85, 0, 0, 0, 0}, VOLT},
  [IOCM_CUR] =
  {"IOCM_CUR", ADC128_IN0, read_current_iocm, false, {0, 0, 0, 0, 0, 0, 0, 0}, CURR},
  [IOCM_TEMP] =
  {"IOCM_TEMP", TEMP_IOCM, read_temp, false, {93, 0, 0, 0, 0, 0, 0, 0}, TEMP},
  [IOCM_IOC_TEMP] =
  {"IOCM_IOC_TEMP", TEMP_IOCM, read_ioc_temp, false, {95, 0, 0, 0, 0, 0, 0, 0}, TEMP},
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
  BS_DIMM0_TEMP,
  BS_DIMM1_TEMP,
  BS_DIMM2_TEMP,
  BS_DIMM3_TEMP,
  BS_CPU_TEMP,
  BS_THERMAL_MARGIN,
  BS_CPU_TJMAX,
  BS_CPU_PKG_PWR,
  BS_HSC_IN_AVGPWR,
  BS_VR_VCCIN_TEMP,
  BS_VR_VCCIN_CUR,
  BS_VR_VCCIN_VOLT,
  BS_VR_VCCIN_PWR,
  BS_VR_VCCSA_TEMP,
  BS_VR_VCCSA_CUR,
  BS_VR_VCCSA_VOLT,
  BS_VR_VCCSA_PWR,
  BS_VR_VCCIO_TEMP,
  BS_VR_VCCIO_CUR,
  BS_VR_VCCIO_VOLT,
  BS_VR_VCCIO_PWR,
  BS_VR_DIMM_AB_TEMP,
  BS_VR_DIMM_AB_CUR,
  BS_VR_DIMM_AB_VOLT,
  BS_VR_DIMM_AB_PWR,
  BS_VR_DIMM_DE_TEMP,
  BS_VR_DIMM_DE_CUR,
  BS_VR_DIMM_DE_VOLT,
  BS_VR_DIMM_DE_PWR,
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
  DPB_P3V3_IN_SENSE,
  DPB_P5V_1_SYS_SENSE,
  DPB_P5V_2_SYS_SENSE,
  DPB_P5V_3_SYS_SENSE,
  DPB_P5V_4_SYS_SENSE,
  DPB_HSC_P12V_CLIP,
  DPB_HSC_CUR,
  DPB_HSC_CUR_CLIP,
  DPB_HSC_PWR,
  DPB_HSC_PWR_CLIP,
  FAN_0_FRONT,
  FAN_0_REAR,
  FAN_1_FRONT,
  FAN_1_REAR,
  FAN_2_FRONT,
  FAN_2_REAR,
  FAN_3_FRONT,
  FAN_3_REAR,
  AIRFLOW,
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
  NIC_SENSOR_TEMP,
  NIC_SENSOR_P12V,
  NIC_SENSOR_CUR,
};

const uint8_t e1s_sensor_list[] = {
  E1S0_CUR,
  E1S1_CUR,
  E1S0_TEMP,
  E1S1_TEMP,
  E1S0_P12V,
  E1S1_P12V,
  E1S0_P3V3,
  E1S1_P3V3,
};

const uint8_t iocm_sensor_list[] = {
  IOCM_P3V3_STBY,
  IOCM_P1V8,
  IOCM_P1V5,
  IOCM_P0V865,
  IOCM_CUR,
  IOCM_TEMP,
  IOCM_IOC_TEMP,
};

PAL_I2C_BUS_INFO nic_info_list[] = {
  {NIC, I2C_NIC_BUS, NIC_INFO_SLAVE_ADDR},
};

PAL_I2C_BUS_INFO e1s_info_list[] = {
  {T5_E1S0_T7_IOC_AVENGER, I2C_T5E1S0_T7IOC_BUS, NVMe_SMBUS_ADDR},
  {T5_E1S1_T7_IOCM_VOLT, I2C_T5E1S1_T7IOC_BUS, NVMe_SMBUS_ADDR},
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

PAL_DEV_INFO temp_dev_list[] = {
  {"tmp75-i2c-4-4a",  "UIC_INLET_TEMP"},
  {"tmp75-i2c-13-4a", "IOCM_TEMP"},
};

PAL_DEV_INFO adc128_dev_list[] = {
  {"adc128d818-i2c-9-1d",  "E1S0_CUR"},
  {"adc128d818-i2c-9-1d",  "E1S1_CUR"},
  {"adc128d818-i2c-9-1d",  "E1S0_P12V"},
  {"adc128d818-i2c-9-1d",  "E1S1_P12V"},
  {"adc128d818-i2c-9-1d",  "E1S0_P3V3"},
  {"adc128d818-i2c-9-1d",  "E1S1_P3V3"},
  {"adc128d818-i2c-9-1d",  "NIC_P12V"},
  {"adc128d818-i2c-9-1d",  "NIC_CUR"},
};

PAL_DEV_INFO ltc2990_dev_list[] = {
  {"ltc2990-i2c-13-4c",  "IOCM_P3V3"},
  {"ltc2990-i2c-13-4c",  "IOCM_P1V8"},
  {"ltc2990-i2c-13-4c",  "IOCM_P1V5"},
  {"ltc2990-i2c-13-4c",  "IOCM_P0V865"},
};

PAL_DEV_INFO ltc2991_dev_list[] = {
  {"ltc2991-i2c-9-48",  "E1S0_CUR"},
  {"ltc2991-i2c-9-48",  "E1S1_CUR"},
  {"ltc2991-i2c-9-48",  "E1S0_P12V"},
  {"ltc2991-i2c-9-48",  "E1S1_P12V"},
  {"ltc2991-i2c-9-48",  "E1S0_P3V3"},
  {"ltc2991-i2c-9-48",  "E1S1_P3V3"},
  {"ltc2991-i2c-9-48",  "NIC_P12V"},
  {"ltc2991-i2c-9-48",  "NIC_CUR"},
};

// ADS1015 PGA settings in DTS of each channel, unit: mV
static int ads1015_pga_setting[] = {
  4096, 2048, 2048, 1024
};

static inlet_corr_t server_ict[] = {
  //airflow, offset value
  { 0,  8 },    // airflow: 0-44
  { 45, 5 },    // airflow: 45-63
  { 64, 4 },    // airflow: 64-74
  { 75, 3.5 },  // airflow: 75-96
  { 97, 3 },    // airflow: 97-109
  { 110, 2.5 }, // airflow: 110-140
  { 141, 2 },   // airflow: 141-295
  { 296, 1.5 }, // airflow: 296-369
};

static inlet_corr_t uic_t5_ict[] = {
  //airflow, offset value
  { 0, 7.27 },   // airflow: 0-39
  { 40, 5.91 },  // airflow: 40-48
  { 49, 4.79 },  // airflow: 49-60
  { 61, 3.8 },   // airflow: 61-72
  { 73, 3.33 },  // airflow: 73-81
  { 82, 3.03 },  // airflow: 82-93
  { 94, 2.79 },  // airflow: 94-103
  { 104, 2.67 }, // airflow: 104-127
  { 128, 2.36 }, // airflow: 128-164
  { 165, 2 },    // airflow: 165-204
  { 205, 1.68 }, // airflow: 205-286
  { 287, 1.33 }, // airflow: 287-367
  { 368, 1.3 },  // airflow: 368-
};

static inlet_corr_t uic_t7_ict[] = {
  //airflow, offset value
  { 0, 9.5 },    // airflow: 0-39
  { 40, 7.41 },  // airflow: 40-48
  { 49, 6.79 },  // airflow: 49-60
  { 61, 5 },     // airflow: 61-72
  { 73, 4 },     // airflow: 73-81
  { 82, 3.03 },  // airflow: 82-93
  { 94, 2.79 },  // airflow: 94-103
  { 104, 2.67 }, // airflow: 104-127
  { 128, 2.36 }, // airflow: 128-164
  { 165, 2 },    // airflow: 165-204
  { 205, 1.68 }, // airflow: 205-286
  { 287, 1.33 }, // airflow: 287-367
  { 368, 1.3 },  // airflow: 368-
};

static size_t server_ict_count = ARRAY_SIZE(server_ict);
static size_t uic_t5_ict_count = ARRAY_SIZE(uic_t5_ict);
static size_t uic_t7_ict_count = ARRAY_SIZE(uic_t7_ict);

size_t server_sensor_cnt = ARRAY_SIZE(server_sensor_list);
size_t uic_sensor_cnt = ARRAY_SIZE(uic_sensor_list);
size_t dpb_sensor_cnt = ARRAY_SIZE(dpb_sensor_list);
size_t scc_sensor_cnt = ARRAY_SIZE(scc_sensor_list);
size_t nic_sensor_cnt = ARRAY_SIZE(nic_sensor_list);
size_t e1s_sensor_cnt = ARRAY_SIZE(e1s_sensor_list);
size_t iocm_sensor_cnt = ARRAY_SIZE(iocm_sensor_list);

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  uint8_t chassis_type = 0;

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
  case FRU_E1S_IOCM:
    if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
      syslog(LOG_WARNING, "%s() Failed to get chassis type\n", __func__);
      return ERR_UNKNOWN_FRU;
    }
    if (chassis_type == CHASSIS_TYPE5) {
      *sensor_list = (uint8_t *) e1s_sensor_list;
      *cnt = e1s_sensor_cnt;
      break;
    } else if (chassis_type == CHASSIS_TYPE7) {
      *sensor_list = (uint8_t *) iocm_sensor_list;
      *cnt = iocm_sensor_cnt;
      break;
    }
    else {
      syslog(LOG_WARNING, "%s() Unknow chassis type %u\n", __func__, chassis_type);
      return ERR_UNKNOWN_FRU;
    }
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
sensors_read_wrapper(const char *chip, const char *label, float *value) {
  int ret = 0;

  ret = sensors_read(chip, label, value);
  if (ret == -1) ret = ERR_FAILURE; // normalize the error code
  return ret;
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

  return sensors_read_wrapper(temp_dev_list[id].chip, temp_dev_list[id].label, value);
}

/* Check the valid range of NIC Temperature. */
static int
nic_temp_value_check(uint8_t value, float *value_check) {

  if (value_check == NULL) {
    syslog(LOG_ERR, "%s(): fail to check NIC temperature due to the NULL parameter.\n", __func__);
    return -1;
  }

  if (value <= MAX_NIC_TEMPERATURE) {
    *value_check = (float)value;
  } else {
    return -1;
  }

  return 0;
}

static int
read_nic_temp(uint8_t nic_id, float *value) {
  int fd = 0, ret = -1;
  uint8_t retry = MAX_RETRY, tlen = 0, rlen = 0, addr = 0, bus = 0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t res_temp = 0;
  static int nic_temp_retry = 0;

  if (nic_id >= ARRAY_SIZE(nic_info_list)) {
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
    res_temp = rbuf[0];
  }

  ret = nic_temp_value_check(res_temp, value);

  // Temperature within valid range
  if (ret == 0) {
    nic_temp_retry = 0;
  } else {
    if (nic_temp_retry <= MAX_NIC_TEMP_RETRY) {
      ret = READING_SKIP;
      nic_temp_retry++;
    } else {
      ret = ERR_SENSOR_NA;
    }
  }
  close(fd);

  return ret;
}

bool
is_e1s_iocm_present(uint8_t id) {
  uint8_t e1s_iocm_present_flag = 0;

  if (pal_is_fru_prsnt(FRU_E1S_IOCM, &e1s_iocm_present_flag) < 0) {
    syslog(LOG_WARNING, "%s()  pal_is_fru_prsnt error\n", __func__);
    return false;
  }

  if (((id == T5_E1S0_T7_IOC_AVENGER) && (e1s_iocm_present_flag & E1S0_IOCM_PRESENT_BIT) == 0) ||
     (((id == T5_E1S1_T7_IOCM_VOLT) && (e1s_iocm_present_flag & E1S1_IOCM_PRESENT_BIT) == 0))) {
#ifdef DEBUG
    syslog(LOG_WARNING, "%s() E1S/IOCM not present, id: %u: flag: %d\n", __func__, id, e1s_iocm_present_flag);
#endif
    return false;
  }

  return true;
}

bool
is_e1s_iocm_i2c_enabled(uint8_t id) {
  gpio_value_t val = 0;

  if (id == T5_E1S0_T7_IOC_AVENGER) {
    val = gpio_get_value_by_shadow(fbgc_get_gpio_name(GPIO_E1S_1_P3V3_PG_R));
  } else if (id == T5_E1S1_T7_IOCM_VOLT) {
    val = gpio_get_value_by_shadow(fbgc_get_gpio_name(GPIO_E1S_2_P3V3_PG_R));
  } else {
    syslog(LOG_WARNING, "%s()  invalid id %u\n", __func__, id);
    return false;
  }
  if (val == GPIO_VALUE_INVALID) {
    syslog(LOG_WARNING, "%s()  gpio_get_value_by_shadow failed, id: %u\n", __func__, id);
    return false;
  }

  if (val == GPIO_VALUE_LOW) {
    return false;
  }

  return true;
}

static int
read_e1s_temp(uint8_t e1s_id, float *value) {
  int ret = 0, fd = 0;
  uint8_t tlen = 0, rlen = 0, bus = 0, addr = 0;
  uint8_t retry = MAX_RETRY;
  uint8_t tbuf[1] = {0};
  uint8_t rbuf[NVMe_GET_STATUS_LEN] = {0};

  if ((e1s_id >= ARRAY_SIZE(e1s_info_list)) ||
      (is_e1s_iocm_present(e1s_id) == false) ||
      (is_e1s_iocm_i2c_enabled(e1s_id) == false)) {
    return ERR_SENSOR_NA;
  }

  bus = e1s_info_list[e1s_id].bus;
  addr = e1s_info_list[e1s_id].slv_addr;

  fd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open I2C bus %d\n", __func__, bus);
    return ERR_SENSOR_NA;
  }

  tbuf[0] = NVMe_GET_STATUS_CMD;
  tlen = sizeof(tbuf);
  rlen = NVMe_GET_STATUS_LEN;

  do {
    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
    retry--;
  } while (ret < 0 && retry > 0 );

  if (ret >= 0) {
    // valid temperature range: -60C(0xC4) ~ +127C(0x7F)
    // C4h-FFh is two's complement, means -60 to -1
    ret = nvme_temp_value_check((int)rbuf[NVMe_TEMP_REG], value);
    if (ret != 0) {
      ret = ERR_SENSOR_NA;
    }
  } else {
    ret = ERR_SENSOR_NA;
  }
  close(fd);

  return ret;
}

static bool
is_e1s_power_good(uint8_t id) {
  uint8_t e1s_pwr_status = 0, type = 0;

  if (pal_get_device_power(FRU_SERVER, (id + 1), &e1s_pwr_status, &type) < 0) { // id is 1 base here
    return false;
  }

  if (e1s_pwr_status == DEVICE_POWER_OFF) {
    return false;
  }

  return true;
}

static int
read_dpb_vol_wrapper(uint8_t id, float *value) {
  int ret = 0;

  if (value == NULL) {
    syslog(LOG_ERR, "%s: Null parameter", __func__);
    return ERR_SENSOR_NA;
  }

  if (dpb_source_info == MAIN_SOURCE) {
    ret = sensors_read_wrapper(adc128_dev_list[id].chip, adc128_dev_list[id].label, value);
  } else if (dpb_source_info == SECOND_SOURCE) {
    ret = sensors_read_wrapper(ltc2991_dev_list[id].chip, ltc2991_dev_list[id].label, value);
  } else {
    ret = ERR_SENSOR_NA;
  }

  return ret;
}

static int
read_voltage_e1s(uint8_t id, float *value) {
  int ret = 0;

  if (id >= ARRAY_SIZE(adc128_dev_list)) {
    return ERR_SENSOR_NA;
  }

  if (is_e1s_power_good(id % 2) == false) { // power-util power off E1.S
    e1s_adc_skip_times[id] = MAX_E1S_VOL_SNR_SKIP;
    return ERR_SENSOR_NA;
  }

  if (e1s_adc_skip_times[id] != 0) { // wait sensor reading ready while E1.S power on
    e1s_adc_skip_times[id]--;
    return READING_SKIP;
  }

  ret = read_dpb_vol_wrapper(id, value);
  return ret;
}

static void
handle_nic_p12v_status_timestamp() {
  uint8_t nic_p12v_status = 0;
  int ret = 0, nic_p12v_timestamp = 0, current_time = 0;
  char status_key[MAX_KEY_LEN] = {0}, timestamp_key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0}, tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts = {0};

  // Get NIC P12V status
  snprintf(status_key, sizeof(status_key), NIC_P12V_STATUS_STR);
  ret = kv_get(status_key, value, NULL, 0);
  if (ret < 0) {
    // Set NIC P12V status is normal
    if (pal_set_cached_value(status_key, STR_VALUE_0) < 0) {
      syslog(LOG_WARNING, "%s() Failed to set NIC P12V status\n", __func__);
      return;
    }
  } else {
    nic_p12v_status = atoi(value);
  }

  // Get NIC P12V deassert timestamp
  snprintf(timestamp_key, sizeof(timestamp_key), NIC_P12V_TIMESTAMP_STR);
  ret = kv_get(timestamp_key, value, NULL, 0);
  if (ret < 0) {
    // Clear NIC P12V deassert timestamp to 0
    if (pal_set_cached_value(timestamp_key, STR_VALUE_0) < 0) {
      syslog(LOG_WARNING, "%s() Failed to clear NIC P12V deassert timestamp\n", __func__);
      return;
    }
  } else {
    nic_p12v_timestamp = atoi(value);
  }

  if ((nic_p12v_status == NIC_P12V_IS_DROPPED) && (nic_p12v_timestamp != 0)) {
    // Get current timestamp
    clock_gettime(CLOCK_REALTIME, &ts);
    snprintf(tstr, sizeof(tstr), "%ld", ts.tv_sec);
    current_time = atoi(tstr);

    if (abs(current_time - nic_p12v_timestamp) > 5) {
      // Set NIC P12V status to normal
      if (pal_set_cached_value(status_key, STR_VALUE_0) < 0) {
        syslog(LOG_WARNING, "%s() Failed to set NIC P12V status is normal\n", __func__);
        return;
      }
      // Set NIC P12V deassert timestamp to 0
      if (pal_set_cached_value(timestamp_key, STR_VALUE_0) < 0) {
        syslog(LOG_WARNING, "%s() Failed to clear NIC P12V deassert timestamp\n", __func__);
        return;
      }
    }
  }
}

static int
read_voltage_nic(uint8_t id, float *value) {
  int ret = 0;
  uint8_t prsnt_status = 0;

  if (id >= ARRAY_SIZE(adc128_dev_list)) {
    return ERR_SENSOR_NA;
  }

  if ((pal_is_fru_prsnt(FRU_NIC, &prsnt_status) < 0) || 
      (prsnt_status == FRU_ABSENT)) {
    return ERR_SENSOR_NA;
  }

  // NIC P12V voltage sensor
  if (id == ADC128_IN6) {
    handle_nic_p12v_status_timestamp();
  }

  ret = read_dpb_vol_wrapper(id, value);

  if ((id == ADC128_IN7) && (*value < 0)) { // NIC current doesn't support negative value
    *value = 0;
  }

  return ret;
}

static bool
is_iocm_power_good(void) { // check IOCM power from main connector
  gpio_value_t val = 0;
  
  val = gpio_get_value_by_shadow(fbgc_get_gpio_name(GPIO_E1S_1_3V3EFUSE_PGOOD));

  if (val == GPIO_VALUE_INVALID) {
    syslog(LOG_WARNING, "%s() Faild to get GPIO: E1S_1_3V3EFUSE_PGOOD", __func__);
    return false;
  }

  if (val == GPIO_VALUE_HIGH) {
    return true;
  }

  return false;  
}

static int
read_current_iocm(uint8_t id, float *value) {
  int ret = 0;

  if (id >= ARRAY_SIZE(adc128_dev_list)) {
    return ERR_SENSOR_NA;
  }

  if (is_iocm_power_good() == false) {
    return ERR_SENSOR_NA;
  }

  ret = read_dpb_vol_wrapper(id, value);

  return ret;
}

int
get_current_dir(const char *device, char *dir_name) {
  char cmd[MAX_PATH_LEN + 1] = {0};
  FILE *fp = NULL;
  int size = 0;

  if (device == NULL || dir_name == NULL) {
    syslog(LOG_ERR, "%s: Invalid parameter", __func__);
    return -1;
  }

  // Get current working directory
  snprintf(cmd, sizeof(cmd), "cd %s;pwd", device);

  fp = popen(cmd, "r");
  if (fp == NULL) {
     syslog(LOG_ERR, "%s: popen failed, error: %s ", __func__, strerror(errno));
     return -1;
  }
  if (fgets(dir_name, MAX_PATH_LEN, fp) == NULL) {
     syslog(LOG_ERR, "%s: fget() failed, error: %s ", __func__, strerror(errno));
     pclose(fp);
     return -1;
  }

  if (pclose(fp) == -1) {
     syslog(LOG_ERR, "%s: pclose() failed, error %s", __func__, strerror(errno));
  }

  // Remove the newline character at the end
  size = strlen(dir_name);
  dir_name[size - 1] = '\0';

  return 0;
}

int
read_device(const char *device, int *value) {
  FILE *fp = NULL;

  if (device == NULL || value == NULL) {
    syslog(LOG_ERR, "%s: Invalid parameter", __func__);
    return -1;
  }

  fp = fopen(device, "r");
  if (fp == NULL) {
#ifdef DEBUG
    syslog(LOG_INFO, "%s failed to open device %s error: %s", __func__, device, strerror(errno));
#endif
    return -1;
  }

  if (fscanf(fp, "%d", value) != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "%s failed to read device %s error: %s", __func__, device, strerror(errno));
#endif
    fclose(fp);
    return -1;
  }
  fclose(fp);

  return 0;
}

int
write_device(const char *device, const char *value) {
  FILE *fp;
  int rc;

  if (device == NULL || value == NULL) {
    syslog(LOG_ERR, "%s: Invalid parameter", __func__);
    return -1;
  }

  fp = fopen(device, "w");
  if (fp == NULL) {
    int err = errno;
    syslog(LOG_INFO, "failed to open device for write %s error: %s", device, strerror(errno));
    return err;
  }

  rc = fputs(value, fp);
  fclose(fp);

  if (rc < 0) {
    syslog(LOG_INFO, "failed to write device %s error: %s", device, strerror(errno));
    return -1;
  } else {
    return 0;
  }
}

static int
read_ads1015(uint8_t id, float *value) {
  int read_value = 0;
  char full_dir_name[MAX_PATH_LEN * 2] = {0};
  char dir_name[MAX_PATH_LEN] = {0};

  if (get_current_dir(IOCM_VOLTAGE_SENSOR_DIR, dir_name) < 0) {
    syslog(LOG_WARNING, "%s() Failed to get dir: %s\n", __func__, IOCM_VOLTAGE_SENSOR_PATH);
    return ERR_SENSOR_NA;
  }

  snprintf(full_dir_name, sizeof(full_dir_name), IOCM_VOLTAGE_SENSOR_PATH, dir_name, id);

  if (read_device(full_dir_name, &read_value) < 0) {
     if (is_iocm_power_good() == true) {
       syslog(LOG_WARNING, "%s() Failed to read device: %s\n", __func__, full_dir_name);
     }
     return ERR_SENSOR_NA;
  }
  // output V = input mV * reference mV / default mV / 1000
  *value = ((float)read_value) * ads1015_pga_setting[id] / ADS1015_PGA_DEFAULT / ADS1015_DIV_UNIT;

  return 0;
}

static int
read_ltc2990(uint8_t id, float *value) {
  if (id >= ARRAY_SIZE(ltc2990_dev_list)) {
    return ERR_SENSOR_NA;
  }

  return sensors_read_wrapper(ltc2990_dev_list[id].chip, ltc2990_dev_list[id].label, value);
}

static int
read_voltage_iocm(uint8_t id, float *value) {
  if (value == NULL) {
    syslog(LOG_ERR, "%s: Null parameter", __func__);
    return ERR_SENSOR_NA;
  }

  if (iocm_source_info == MAIN_SOURCE) {
    return read_ads1015(id, value);
  } else if (iocm_source_info == SECOND_SOURCE) {
    return read_ltc2990(id, value);
  } else {
    return ERR_SENSOR_NA;
  }
}

static int
read_ioc_temp(uint8_t id, float *value) {
  char key[MAX_KEY_LEN] = {0};
  char cache_value[MAX_VALUE_LEN] = {0};
  char fru_name[MAX_FRU_CMD_STR] = {0};
  int ret = 0;

  if (value == NULL) {
    syslog(LOG_ERR, "%s(), Failed to read ioc temperature due to NULL parameter.\n", __func__);
    return ERR_SENSOR_NA;
  }

  memset(key, 0, sizeof(key));
  memset(cache_value, 0, sizeof(cache_value));

  if (id == EXPANDER) {
    if (pal_get_fru_name(FRU_SCC, fru_name) < 0) {
      syslog(LOG_WARNING, "%s() Fail to get fru%d name\n", __func__, FRU_SCC);
      return ERR_SENSOR_NA;
    }
    snprintf(key, sizeof(key), "%s_ioc_sensor%d", fru_name, SCC_IOC_TEMP);
  } else if (id == TEMP_IOCM) {
    if (pal_get_fru_name(FRU_E1S_IOCM, fru_name) < 0) {
      syslog(LOG_WARNING, "%s() Fail to get fru%d name\n", __func__, FRU_E1S_IOCM);
      return ERR_SENSOR_NA;
    }
    if (is_e1s_iocm_present(T5_E1S0_T7_IOC_AVENGER) == false) {
      return ERR_SENSOR_NA;
    }
    snprintf(key, sizeof(key), "%s_ioc_sensor%d", fru_name, IOCM_IOC_TEMP);
  } else {
    return ERR_SENSOR_NA;
  }

  ret = kv_get(key, cache_value, NULL, 0);
  if (ret != 0) {
    syslog(LOG_ERR, "%s(), Failed to get key: %s, ret: %d\n", __func__, key, ret);
    return ret;
  }

  if (strcmp(cache_value, "NA") == 0) {
    return ERR_SENSOR_NA;
  }

  *value = atof(cache_value);

  return ret;
}

static int
exp_read_sensor_thresh_wrapper(uint8_t fru, uint8_t *sensor_list, thresh_sensor_t *snr_thresh, int sensor_cnt, uint8_t index) {
  uint8_t tbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rlen = 0, tlen = 0;
  uint8_t snr_num = 0;
  int ret = 0, i = 0, retry = 0;
  int tach_cnt = 0;
  float high_crit = 0, high_warn = 0, low_crit = 0, low_warn = 0;
  EXPANDER_THRES_DATA *p_thres_data = NULL;

  if ((sensor_list == NULL) || (snr_thresh == NULL)) {
    syslog(LOG_WARNING, "%s() failed to get sensor threshold from expander because NULL pointer\n", __func__);
    return -1;
  }

  tbuf[0] = sensor_cnt;
  for(i = 0 ; i < sensor_cnt; i++) {
    tbuf[i + 1] = sensor_list[i + index];  //feed sensor number to tbuf
  }
  tlen = sensor_cnt + 1;

  //send tbuf with sensor count and numbers to get specific sensor threshold data from exp
  do {
    ret = expander_ipmb_wrapper(NETFN_OEM_REQ, CMD_OEM_EXP_GET_SENSOR_THRESHOLD, tbuf, tlen, rbuf, &rlen);
    retry++;
  } while ((ret < 0) && (retry < MAX_RETRY));

  if (ret < 0) {
    syslog(LOG_WARNING, "%s() expander_ipmb_wrapper failed. ret: %d\n", __func__, ret);
    return ret;
  }

  tach_cnt = pal_get_tach_cnt();

  p_thres_data = (EXPANDER_THRES_DATA *)(&rbuf[1]);

  for(i = 0; i < sensor_cnt; i++) {
    snr_num = p_thres_data[i].sensor_num;
    snr_thresh[snr_num].flag = GETMASK(SENSOR_VALID) | GETMASK(UCR_THRESH) | 
      GETMASK(UNC_THRESH) | GETMASK(LCR_THRESH) | GETMASK(LNC_THRESH);
    pal_get_sensor_name(fru, snr_num, snr_thresh[snr_num].name);
    pal_get_sensor_units(fru, snr_num, snr_thresh[snr_num].units);

    if (strncmp(snr_thresh[snr_num].units, "C", sizeof(snr_thresh[snr_num].units)) == 0) {
      high_crit = p_thres_data[i].high_crit_2;
      high_warn = p_thres_data[i].high_warn_2;
      low_crit = p_thres_data[i].low_crit_2;
      low_warn = p_thres_data[i].low_warn_2;
    } else if (strncmp(snr_thresh[snr_num].units, "RPM", sizeof(snr_thresh[snr_num].units)) == 0) {
      high_crit = (((p_thres_data[i].high_crit_1 << 8) + p_thres_data[i].high_crit_2)) * 10;
      high_warn = (((p_thres_data[i].high_warn_1 << 8) + p_thres_data[i].high_warn_2)) * 10;
      low_crit = (((p_thres_data[i].low_crit_1 << 8) + p_thres_data[i].low_crit_2)) * 10;
      low_warn = (((p_thres_data[i].low_warn_1 << 8) + p_thres_data[i].low_warn_2)) * 10;

      if (tach_cnt == SINGLE_FAN_CNT) {
        if ((snr_num == FAN_0_REAR) || (snr_num == FAN_1_REAR)
          || (snr_num == FAN_2_REAR) || (snr_num == FAN_3_REAR)) {
            continue;
          }
      } else if (tach_cnt == UNKNOWN_FAN_CNT) {
        continue;
      }
    } else if (strncmp(snr_thresh[snr_num].units, "Watts", sizeof(snr_thresh[snr_num].units)) == 0) {
      high_crit = (((p_thres_data[i].high_crit_1 << 8) + p_thres_data[i].high_crit_2));
      high_warn = (((p_thres_data[i].high_warn_1 << 8) + p_thres_data[i].high_warn_2));
      low_crit = (((p_thres_data[i].low_crit_1 << 8) + p_thres_data[i].low_crit_2));
      low_warn = (((p_thres_data[i].low_warn_1 << 8) + p_thres_data[i].low_warn_2));
    } else {
      high_crit = (float)(((p_thres_data[i].high_crit_1 << 8) + p_thres_data[i].high_crit_2)) / 100;
      high_warn = (float)(((p_thres_data[i].high_warn_1 << 8) + p_thres_data[i].high_warn_2)) / 100;
      low_crit = (float)(((p_thres_data[i].low_crit_1 << 8) + p_thres_data[i].low_crit_2)) / 100;
      low_warn = (float)(((p_thres_data[i].low_warn_1 << 8) + p_thres_data[i].low_warn_2)) / 100;
    }

    // Get threshold of SCC_IOC_TEMP from sensor map
    if ((fru == FRU_SCC) && (snr_num == SCC_IOC_TEMP)) {
      high_crit = scc_sensor_map[snr_num].snr_thresh.ucr_thresh;
      high_warn = scc_sensor_map[snr_num].snr_thresh.unc_thresh;
      low_crit = scc_sensor_map[snr_num].snr_thresh.lcr_thresh;
      low_warn = scc_sensor_map[snr_num].snr_thresh.lnc_thresh;
    }
    snr_thresh[snr_num].ucr_thresh = high_crit;
    snr_thresh[snr_num].unc_thresh = high_warn;
    snr_thresh[snr_num].lcr_thresh = low_crit;
    snr_thresh[snr_num].lnc_thresh = low_warn;
    if (snr_thresh[snr_num].ucr_thresh == 0) {
      snr_thresh[snr_num].flag = CLEARBIT(snr_thresh[snr_num].flag, UCR_THRESH);
    }
    if (snr_thresh[snr_num].unc_thresh == 0) {
      snr_thresh[snr_num].flag = CLEARBIT(snr_thresh[snr_num].flag, UNC_THRESH);
    }
    if (snr_thresh[snr_num].lcr_thresh == 0) {
      snr_thresh[snr_num].flag = CLEARBIT(snr_thresh[snr_num].flag, LCR_THRESH);
    }
    if (snr_thresh[snr_num].lnc_thresh == 0) {
      snr_thresh[snr_num].flag = CLEARBIT(snr_thresh[snr_num].flag, LNC_THRESH);
    }
  }

  return ret;
}

static int
exp_get_sensor_thresh_from_file(uint8_t fru) {
  uint8_t *sensor_list = NULL;
  uint8_t snr_num = 0, bytes_rd = 0;
  uint8_t buf[MAX_THERSH_LEN] = {0};
  int fd = 0, cnt = 0, sensor_cnt = 0, ret = 0;
  char fru_name[MAX_FRU_CMD_STR] = {0};
  char fpath[MAX_PATH_LEN] = {0};
  thresh_sensor_t snr_thresh[MAX_SENSOR_NUM + 1] = {0};
  PAL_SENSOR_MAP *sensor_map = NULL;

  switch (fru) {
    case FRU_DPB:
      sensor_map = dpb_sensor_map;
      break;
    case FRU_SCC:
      sensor_map = scc_sensor_map;
      break;
    default:
      syslog(LOG_WARNING, "%s: Unknown FRU:%d", __func__, fru);
      return ERR_UNKNOWN_FRU;
  }

  ret = pal_get_fru_sensor_list(fru, &sensor_list, &sensor_cnt);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: fail to get sensor list of FRU:%d", __func__, fru);
    return ret;
  }

  ret = pal_get_fru_name(fru, fru_name);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: fail to get FRU:%d name", __func__, fru);
    return ret;
  }

  sprintf(fpath, INIT_THRESHOLD_BIN, fru_name);
  // INIT_THRESHOLD_BIN doesn't exist, use initial sensor map.
  if (access(fpath, F_OK) == -1) {
    return 0;
  }

  fd = open(fpath, O_RDONLY);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s: open failed for %s, errno : %d %s\n", __func__, fpath, errno, strerror(errno));
    return -1;
  }

  while ((bytes_rd = read(fd, buf, sizeof(thresh_sensor_t))) > 0) {
    if (bytes_rd != sizeof(thresh_sensor_t)) {
      syslog(LOG_WARNING, "%s: read returns %d bytes\n", __func__, bytes_rd);
      close(fd);
      return -1;
    }

    snr_num = sensor_list[cnt];
    memcpy(&snr_thresh[snr_num], &buf, sizeof(thresh_sensor_t));
    memcpy(&(sensor_map[snr_num].snr_thresh.ucr_thresh), &(snr_thresh[snr_num].ucr_thresh), sizeof(PAL_SENSOR_THRESHOLD));
    memset(buf, 0, sizeof(buf));
    cnt++;

    if (cnt == sensor_cnt) {
      break;
    }
  }

  close(fd);
  return ret;
}

int
pal_exp_sensor_threshold_init(uint8_t fru) {
  int ret = 0, remain = 0, sensor_cnt = 0, read_cnt = 0, index = 0;
  uint8_t *sensor_list = NULL;
  char fru_name[MAX_FRU_CMD_STR] = {0};
  char fpath[MAX_PATH_LEN] = {0};
  char initpath[MAX_PATH_LEN] = {0};
  char cmd[MAX_SYS_CMD_REQ_LEN + MAX_PATH_LEN*2] = {0};
  thresh_sensor_t snr_thresh[MAX_SENSOR_NUM + 1] = {0};

  // Get sensors' threshold of SCC and DPB
  ret = pal_get_fru_sensor_list(fru, &sensor_list, &sensor_cnt);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() get sensor list failed \n", __func__);
    return ret;
  }

  ret = pal_get_fru_name(fru, fru_name);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() fail to get FRU:%d name\n", __func__, fru);
    return ret;
  }

  remain = sensor_cnt;
  while (remain > 0) {
    read_cnt = (remain > MAX_EXP_IPMB_THRESH_COUNT) ? MAX_EXP_IPMB_THRESH_COUNT : remain;
    ret = exp_read_sensor_thresh_wrapper(fru, sensor_list, snr_thresh, read_cnt, index);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() fail to get sensors' threshold of FRU:%d from expander\n", __func__, fru);
      return ret;
    }
    remain -= read_cnt;
    index += read_cnt;
  }

  if (access(THRESHOLD_PATH, F_OK) == -1) {
    mkdir(THRESHOLD_PATH, 0777);
  }

  ret = pal_copy_all_thresh_to_file(fru, snr_thresh);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: fail to copy thresh to file for FRU: %d", __func__, fru);
    return ret;
  }

  // Create THRESHOLD_BIN for the threshold initialization of sensord.
  sprintf(fpath, THRESHOLD_BIN, fru_name);
  sprintf(initpath, INIT_THRESHOLD_BIN, fru_name);
  if (access(fpath, F_OK) != 0) {
    sprintf(cmd,"cp -rf %s %s", initpath, fpath);
    if (system(cmd) != 0) {
      syslog(LOG_WARNING, "%s failed", cmd);
      ret = -1;
    }
  }

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
  int tach_cnt = 0;

  if (pal_get_fru_name(fru, fru_name)) {
    syslog(LOG_WARNING, "%s() Fail to get fru%d name\n", __func__, fru);
    return ERR_SENSOR_NA;
  }

  tbuf[0] = sensor_cnt;
  for(i = 0 ; i < sensor_cnt; i++) {
    tbuf[i + 1] = sensor_list[i + index];  //feed sensor number to tbuf
  }
  tlen = sensor_cnt + 1;

  //send tbuf with sensor count and numbers to get spcific sensor data from exp
  ret = expander_ipmb_wrapper(NETFN_OEM_REQ, CMD_OEM_EXP_GET_SENSOR_READING, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() expander_ipmb_wrapper failed. ret: %d\n", __func__, ret);

    //if expander doesn't respond, set all sensors value to NA and save to cache
    for(i = 0; i < sensor_cnt; i++) {
      snprintf(key, sizeof(key), "%s_sensor%d_tmp", fru_name, tbuf[i + 1]);
      snprintf(str, sizeof(str), "NA");

      if(kv_set(key, str, 0, 0) < 0) {
        syslog(LOG_WARNING, "%s() cache_set key = %s, str = %s failed.\n", __func__ , key, str);
      }
    }
    return ret;
  }
  
  tach_cnt = pal_get_tach_cnt();

  p_sensor_data = (EXPANDER_SENSOR_DATA *)(&rbuf[1]);

  for(i = 0; i < sensor_cnt; i++) {
    if ((p_sensor_data[i].sensor_status != EXP_SENSOR_STATUS_OK) &&
        (p_sensor_data[i].sensor_status != EXP_SENSOR_STATUS_CRITICAL)) {
      //if sensor status byte is not 'OK' or 'Critical', means sensor reading is unavailable
      snprintf(str, sizeof(str), "NA");
    } else if ((p_sensor_data[i].raw_data_1 == 0xFF) && (p_sensor_data[i].raw_data_2 == 0xFF)) {
      // Sensor value is not ready
      snprintf(str, sizeof(str), "NA");
    } else {
      // search the corresponding sensor table to fill up the raw data and status
      pal_get_sensor_units(fru, p_sensor_data[i].sensor_num, units);
      if (strncmp(units, "C", sizeof(units)) == 0) {
        value = p_sensor_data[i].raw_data_1;
      }
      else if (strncmp(units, "RPM", sizeof(units)) == 0) {
        value = (((p_sensor_data[i].raw_data_1 << 8) + p_sensor_data[i].raw_data_2));
        value *= 10;
        
        if (tach_cnt == SINGLE_FAN_CNT) {
          if ((p_sensor_data[i].sensor_num == FAN_0_REAR) || (p_sensor_data[i].sensor_num == FAN_1_REAR) 
           || (p_sensor_data[i].sensor_num == FAN_2_REAR) || (p_sensor_data[i].sensor_num == FAN_3_REAR)) {
             continue;
           }
        } else if (tach_cnt == UNKNOWN_FAN_CNT) {
          continue;
        }
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
    snprintf(key, sizeof(key), "%s_sensor%d_tmp", fru_name, p_sensor_data[i].sensor_num);
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
  int timestamp = 0, current_time = 0;
  char key[MAX_KEY_LEN] = {0};
  char cvalue[MAX_VALUE_LEN] = {0};
  char tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts = {0};

  switch(fru) {
    case FRU_DPB:
      snprintf(key, sizeof(key), "dpb_sensor_timestamp");
      break;
    case FRU_SCC:
      snprintf(key, sizeof(key), "scc_sensor_timestamp");
      break;
    default:
      syslog(LOG_ERR, "%s() Invalid FRU%u \n", __func__, fru);
      return -1;
  }

  // Get sensor timestamp
  ret = pal_get_cached_value(key, cvalue);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Failed to get FRU%u sensor timestamp", __func__, fru);
    return ret;
  }
  timestamp = atoi(cvalue);

  // Get current timestamp
  clock_gettime(CLOCK_REALTIME, &ts);
  snprintf(tstr, sizeof(tstr), "%ld", ts.tv_sec);
  current_time = atoi(tstr);

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

  // Read DPB_HSC_PWR from expander directly
  if ((fru == FRU_DPB) && (sensor_num == DPB_HSC_PWR)) {
    ret = exp_read_sensor_wrapper(fru, sensor_list, read_cnt, index);
    if (ret < 0) {
      syslog(LOG_ERR, "%s() wrapper ret%d \n", __func__, ret);
    }
    return ret;
  }

  if (abs(current_time - timestamp) > EXP_SENSOR_WAIT_TIME) {
    if ( (fru == FRU_DPB && sensor_num == dpb_sensor_list[0]) || (fru == FRU_SCC && sensor_num == scc_sensor_list[0]) ) {
      remain = sensor_cnt;

      // read all sensors
      while (remain > 0) {
        read_cnt = (remain > MAX_EXP_IPMB_SENSOR_COUNT) ? MAX_EXP_IPMB_SENSOR_COUNT : remain;
        ret = exp_read_sensor_wrapper(fru, sensor_list, read_cnt, index);
        if (ret < 0) {
          syslog(LOG_ERR, "%s() wrapper ret%d \n", __func__, ret);
        }
        remain -= read_cnt;
        index += read_cnt;
      };

      // Update timestamp only after all sensors updated
      memset(tstr, 0, sizeof(tstr));
      clock_gettime(CLOCK_REALTIME, &ts);
      snprintf(tstr, sizeof(tstr), "%ld", ts.tv_sec);

      ret = pal_set_cached_value(key, tstr);
      if (ret < 0) {
        syslog(LOG_WARNING, "%s() Failed to set FRU%u sensor timestamp", __func__, fru);
        return ret;
      }

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
  }

  return ret;
}

static int
expander_sensor_read_cache(uint8_t fru, uint8_t sensor_num, void *value) {
  char cache_value[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  char fru_name[32] = {0};
  int ret = 0;
  uint8_t chassis_type = 0;

  if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
    syslog(LOG_WARNING, "%s() Failed to get chassis type\n", __func__);
    return ERR_UNKNOWN_FRU;
  }

  if (chassis_type == CHASSIS_TYPE7 && fru == FRU_DPB &&
     (sensor_num == FAN_0_REAR || sensor_num == FAN_1_REAR || sensor_num == FAN_2_REAR || sensor_num == FAN_3_REAR)) {
    return ERR_SENSOR_NA;
  }

  if (pal_get_fru_name(fru, fru_name) != 0) {
    syslog(LOG_WARNING, "%s() Fail to get fru%d name\n", __func__, fru);
    return ERR_SENSOR_NA;
  }

  snprintf(key, sizeof(key), "%s_sensor%d_tmp", fru_name, sensor_num);
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

  if ((sdr->sensor_units1 & 0xC0) == 0x00) {  // unsigned
    x = sensor_value;
  } else if ((sdr->sensor_units1 & 0xC0) == 0x40) {  // 1's complements
    x = (sensor_value & 0x80) ? (0-(~sensor_value)) : sensor_value;
  } else if ((sdr->sensor_units1 & 0xC0) == 0x80) {  // 2's complements
    x = (int8_t)sensor_value;
  } else { // Does not return reading
    return ERR_SENSOR_NA;
  }

  if (pal_convert_sensor_reading(sdr, x, out_value) < 0) {
    return ERR_SENSOR_NA;
  }

  return 0;
}

static int
set_e1s_sensor_name(char *sensor_name, char side) {
  char *index = NULL;

  if (sensor_name == NULL) {
    syslog(LOG_WARNING, "%s() failed to set E1.S sensor name due to NULL parameter", __func__);
    return -1;
  }

  index = strchr(sensor_name, E1S_SENSOR_NAME_WILDCARD);

  if (index == NULL) {
    syslog(LOG_WARNING, "%s() failed to set E1.S sensor name because didn't find the replaced character.", __func__);
    return -1;
  }

  *index = side;

  return 0;
}

static int
pal_bic_sensor_read_raw(uint8_t fru, uint8_t sensor_num, float *value) {
  ipmi_sensor_reading_t sensor = {0};
  sdr_full_t *sdr = NULL;
  int ret = 0;

  if (access(SERVER_SENSOR_LOCK, F_OK) == 0) { // BIC is updating VR
    if ((sensor_num >= BS_VR_VCCIN_TEMP) && (sensor_num <= BS_VR_DIMM_DE_PWR)) {
      return READING_SKIP;
    }
  }

  ret = bic_get_sensor_reading(sensor_num, &sensor);
  if (ret < 0) {
    if (ret != BIC_STATUS_12V_OFF) {
      syslog(LOG_WARNING, "%s() Failed to run bic_get_sensor_reading(). fru: %x, snr#0x%x", __func__, fru, sensor_num);
    }
    return ERR_SENSOR_NA;
  }

  if (sensor.flags & BIC_SNR_FLAG_READ_NA) {
    return ERR_SENSOR_NA;
  }

  sdr = &g_sinfo[sensor_num].sdr;
  if (sdr->type != SDR_TYPE_FULL_SENSOR_RECORD) {
    *value = sensor.value;
    return 0;
  }

  ret = convert_sensor_reading(sdr, sensor.value, value);

  // Correct the sensor reading of cpu thermal margin
  if ((ret == 0) && (sensor_num == BS_THERMAL_MARGIN)) {
    if ( *value > 0 ) {
        *value = -(*value);
    }
  }

  return ret;
}

int
pal_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  char path[MAX_SDR_PATH] = {0};
  int retry = BIC_MAX_RETRY;
  int ret = 0;

  if (fru != FRU_SERVER) {
    return PAL_ENOTSUP;
  }

  if (is_sdr_init[fru] == true) {
    memcpy(sinfo, g_sinfo, sizeof(sensor_info_t) * MAX_SENSOR_NUM);
    goto end;
  }

  // Waiting for bic-cached to create Server SDR file.
  while (retry-- > 0) {
    ret = get_sdr_path(fru, path);
    if (ret < 0) {
      sleep(1);
      continue;
    } else {
      break;
    }
  }

  if ((retry == 0) && (ret < 0)) {
    ret = SDR_NOT_READY;
    syslog(LOG_CRIT, "%s() Failed to get SDR path.\n", __func__);
    goto end;
  }

  retry = MAX_RETRY;
  while (retry-- > 0) {
    ret = read_sdr_info(path, sinfo);
    if (ret < 0) {
      ret = SDR_NOT_READY;
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

static void
get_current_source(const char *key, uint8_t *source_info) {
  char value[MAX_VALUE_LEN] = {0};

  if (key == NULL || source_info == NULL) {
    syslog(LOG_WARNING, "%s Failed by null parameter\n", __func__);
    return;
  }

  if (*source_info != UNKNOWN_SOURCE) { // cached before
    return;
  }

  if (kv_get(key, value, NULL, 0) == 0) {
    if (strcmp(value, STR_MAIN_SOURCE) == 0) {
      *source_info = MAIN_SOURCE;
    } else if (strcmp(value, STR_SECOND_SOURCE) == 0) {
      *source_info = SECOND_SOURCE;
    } else {
      syslog(LOG_WARNING, "%s() Unknown source info: %s. Switch to main source configuration\n", __func__, value);
      kv_set(key, STR_MAIN_SOURCE, 0, 0);
      *source_info = MAIN_SOURCE; // set to default
    }
  }

  return;
}

int pal_sensor_monitor_initial(void) {
  owning_iocm_snr_flag = true;
  kv_set(KEY_IOCM_SNR_READY, IOCM_SNR_NOT_READY, 0, 0);
  // get current dpb source and load corresponding driver
  run_command("/usr/local/bin/check_2nd_source.sh dpb > /dev/NULL 2>&1");
  // reload sensor map
  sensors_reinit();
  return 0;
}

static bool
get_iocm_snr_ready_flag() {
  char value[MAX_VALUE_LEN] = {0};

  if (owning_iocm_snr_flag) {
    return iocm_snr_ready;
  } else {
    if (kv_get(KEY_IOCM_SNR_READY, value, NULL, 0) == 0) {
      if (strcmp(value, IOCM_SNR_READY) == 0) {
        return true;
      }
    }
    return false;
  }
}

static void
set_iocm_snr_ready_flag(bool value) {
  if (owning_iocm_snr_flag) {
    iocm_snr_ready = value;
    if (value) {
      kv_set(KEY_IOCM_SNR_READY, IOCM_SNR_READY, 0, 0);
    } else {
      kv_set(KEY_IOCM_SNR_READY, IOCM_SNR_NOT_READY, 0, 0);
    }
  }
}

static void
do_i2c_isolation(const char *shadow, gpio_value_t value) {
  if (!owning_iocm_snr_flag) {
    return;
  }

  if (gpio_set_init_value_by_shadow(shadow, value) < 0) {
    syslog(LOG_ERR, "%s() Failed to set GPIO %s to %x\n", __func__, shadow, value);
  }
}

static int
check_e1s_iocm_present(uint8_t sensor_num, uint8_t chassis_type) {
  uint8_t id = 0;

  // Disable I2C while E1.S/IOCM is missing
  if (chassis_type == CHASSIS_TYPE5) {
    id = e1s_sensor_map[sensor_num].id % 2;
    if (is_e1s_iocm_present(id) == false) {
      if (id == T5_E1S0_T7_IOC_AVENGER) {
        do_i2c_isolation(fbgc_get_gpio_name(GPIO_E1S_1_P3V3_PG_R), GPIO_VALUE_LOW);
      } else if (id == T5_E1S1_T7_IOCM_VOLT) {
        do_i2c_isolation(fbgc_get_gpio_name(GPIO_E1S_2_P3V3_PG_R), GPIO_VALUE_LOW);
      }
      e1s_removed[id] = true;
      return ERR_SENSOR_NA;
    }
  } else if (chassis_type == CHASSIS_TYPE7) {
    if ((is_e1s_iocm_present(T5_E1S0_T7_IOC_AVENGER) == false) ||
        (is_e1s_iocm_present(T5_E1S1_T7_IOCM_VOLT) == false)) {
      do_i2c_isolation(fbgc_get_gpio_name(GPIO_E1S_1_P3V3_PG_R), GPIO_VALUE_LOW);
      do_i2c_isolation(fbgc_get_gpio_name(GPIO_E1S_2_P3V3_PG_R), GPIO_VALUE_LOW);
      set_iocm_snr_ready_flag(false);
      iocm_removed = true;
      return ERR_SENSOR_NA;
    }
  }

  return 0;
}

static bool
is_e1s_iocm_removed(uint8_t sensor_num, uint8_t chassis_type) {
  uint8_t id = 0;
  if (chassis_type == CHASSIS_TYPE5) {
    id = e1s_sensor_map[sensor_num].id % 2;
    return e1s_removed[id];
  } else if (chassis_type == CHASSIS_TYPE7) {
    return iocm_removed;
  }

  return false;
}

static void
set_e1s_adc_skip_times(uint8_t e1s_id) {
  int i = 0, start = 0;

  start = (e1s_id == T5_E1S0_T7_IOC_AVENGER) ? 0 : 1;
  for (i = start; i < ADC128_E1S_PIN_CNT; i+=2) {
    e1s_adc_skip_times[i] = MAX_E1S_VOL_SNR_SKIP;
  }
}

static int
check_server_dc_power(uint8_t sensor_num, uint8_t chassis_type) {
  uint8_t status = 0;

  if (pal_get_server_power(FRU_SERVER, &status) < 0) {
    do_i2c_isolation(fbgc_get_gpio_name(GPIO_E1S_1_P3V3_PG_R), GPIO_VALUE_LOW);
    do_i2c_isolation(fbgc_get_gpio_name(GPIO_E1S_2_P3V3_PG_R), GPIO_VALUE_LOW);
    return ERR_SENSOR_NA;
  }

  /* Skip isolation actions while:
   *  1. Not running as sensord 
   *  2. No power change and hotplug happened
   */
  if (!owning_iocm_snr_flag || 
      (status == pre_status && is_e1s_iocm_removed(sensor_num, chassis_type) == false)) {
    return (status == SERVER_POWER_ON) ? 0 : ERR_SENSOR_NA;
  }

  // do i2c isolation by server DC power
  if (status == SERVER_POWER_ON) {  // E1.S & IOCM are powered on
    if (is_e1s_iocm_present(T5_E1S0_T7_IOC_AVENGER)) {
      set_e1s_adc_skip_times(T5_E1S0_T7_IOC_AVENGER);
      do_i2c_isolation(fbgc_get_gpio_name(GPIO_E1S_1_P3V3_PG_R), GPIO_VALUE_HIGH);
      e1s_removed[T5_E1S0_T7_IOC_AVENGER] = false;
      iocm_removed = false;
    }
    if (is_e1s_iocm_present(T5_E1S1_T7_IOCM_VOLT)) {
      set_e1s_adc_skip_times(T5_E1S1_T7_IOCM_VOLT);
      do_i2c_isolation(fbgc_get_gpio_name(GPIO_E1S_2_P3V3_PG_R), GPIO_VALUE_HIGH);
      e1s_removed[T5_E1S1_T7_IOCM_VOLT] = false;
      iocm_removed = false;
    }
    pre_status = status;
  } else { // E1.S & IOCM are powered off
    do_i2c_isolation(fbgc_get_gpio_name(GPIO_E1S_1_P3V3_PG_R), GPIO_VALUE_LOW);
    do_i2c_isolation(fbgc_get_gpio_name(GPIO_E1S_2_P3V3_PG_R), GPIO_VALUE_LOW);
    if (chassis_type == CHASSIS_TYPE7) {
      set_iocm_snr_ready_flag(false);
    }
    pre_status = status;
    return ERR_SENSOR_NA;
  }

  return 0;
}

static void
read_iocm_fru() {
  char path[MAX_PATH_LEN] = {0};

  if (access(FRU_IOCM_BIN, F_OK) == -1 && access(IOCM_EEPROM_BIND_DIR, F_OK) == 0) {
    snprintf(path, sizeof(path), EEPROM_PATH, I2C_T5E1S1_T7IOC_BUS, IOCM_FRU_ADDR);
    if ((pal_copy_eeprom_to_bin(path, FRU_IOCM_BIN)) < 0) {
      syslog(LOG_WARNING, "%s() Failed to copy %s to %s", __func__, path, FRU_IOCM_BIN);
    }
    if (pal_check_fru_is_valid(FRU_IOCM_BIN, LOG_CRIT) < 0) {
      syslog(LOG_WARNING, "%s() The FRU %s is wrong.", __func__, FRU_IOCM_BIN);
    }
  }
}

static void
reload_iocm_sensors() {
  char value[MAX_VALUE_LEN] = {0};

  // clean up
  iocm_source_info = UNKNOWN_SOURCE;
  unlink(FRU_IOCM_BIN);
  kv_del(KEY_IOCM_SOURCE_INFO, 0);

  pal_unbind_i2c_device(I2C_T5E1S1_T7IOC_BUS, IOCM_EEPROM_ADDR, IOCM_EEPROM_DRIVER_NAME, IOCM_EEPROM_BIND_DIR);
  pal_unbind_i2c_device(I2C_T5E1S1_T7IOC_BUS, IOCM_TMP75_ADDR, IOCM_TMP75_DRIVER_NAME, IOCM_TMP75_BIND_DIR);
  pal_unbind_i2c_device(I2C_T5E1S1_T7IOC_BUS, IOCM_LTC2990_ADDR, IOCM_LTC2990_DRIVER_NAME, IOCM_LTC2990_BIND_DIR);
  pal_unbind_i2c_device(I2C_T5E1S1_T7IOC_BUS, IOCM_ADS1015_ADDR, IOCM_ADS1015_DRIVER_NAME, IOCM_ADS1015_BIND_DIR);

  pal_bind_i2c_device(I2C_T5E1S1_T7IOC_BUS, IOCM_EEPROM_ADDR, IOCM_EEPROM_DRIVER_NAME, IOCM_EEPROM_BIND_DIR);
  pal_bind_i2c_device(I2C_T5E1S1_T7IOC_BUS, IOCM_TMP75_ADDR, IOCM_TMP75_DRIVER_NAME, IOCM_TMP75_BIND_DIR);
  read_iocm_fru();
  // check IOCM source
  run_command("/usr/local/bin/check_2nd_source.sh e1s_iocm > /dev/NULL 2>&1");

  if (kv_get(KEY_IOCM_SOURCE_INFO, value, NULL, 0) == 0) {
    if (strcmp(value, STR_MAIN_SOURCE) == 0) {
      pal_bind_i2c_device(I2C_T5E1S1_T7IOC_BUS, IOCM_ADS1015_ADDR, IOCM_ADS1015_DRIVER_NAME, IOCM_ADS1015_BIND_DIR);
    } else if (strcmp(value, STR_SECOND_SOURCE) == 0) {
      pal_bind_i2c_device(I2C_T5E1S1_T7IOC_BUS, IOCM_LTC2990_ADDR, IOCM_LTC2990_DRIVER_NAME, IOCM_LTC2990_BIND_DIR);
    }
  }
  // reload sensor map
  sensors_reinit();
  set_iocm_snr_ready_flag(true);
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {
  int ret = 0;
  uint8_t chassis_type = 0;
  uint8_t id = 0;

  switch(fru) {
  case FRU_SERVER:
    if (sdr_init(fru) == PAL_ENOTREADY) {
      ret = ERR_SENSOR_NA;
    } else {
      ret = pal_bic_sensor_read_raw(fru, sensor_num, (float*)value);
      
      // Inlet sensor correction
      if (sensor_num == BS_INLET_TEMP) {
        apply_inlet_correction((float *)value, server_ict, server_ict_count);
      }
    }
    break;
  case FRU_UIC:
    id = uic_sensor_map[sensor_num].id;
    ret = uic_sensor_map[sensor_num].read_sensor(id, (float*) value);

    if (sensor_num == UIC_INLET_TEMP) {
      if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
        syslog(LOG_WARNING, "%s() Failed to get chassis type, use Type 5 UIC Inlet temperature correction table\n", __func__);
      }

      // UIC Inlet temperature correction
      if (chassis_type == CHASSIS_TYPE7) {
        apply_inlet_correction((float *)value, uic_t7_ict, uic_t7_ict_count);
      } else {
        apply_inlet_correction((float *)value, uic_t5_ict, uic_t5_ict_count);
      }
    }
    break;
  case FRU_DPB:
  case FRU_SCC:
    if (sensor_num == SCC_IOC_TEMP) {
      id = scc_sensor_map[sensor_num].id;
      ret = scc_sensor_map[sensor_num].read_sensor(id, (float*) value);
    } else {
      if (0 != (ret = expander_sensor_check(fru, sensor_num))) {
        break;
      }
      ret = expander_sensor_read_cache(fru, sensor_num, value);
    }
    break;
  case FRU_NIC:
    get_current_source(KEY_DPB_SOURCE_INFO, &dpb_source_info);
    id = nic_sensor_map[sensor_num].id;
    ret = nic_sensor_map[sensor_num].read_sensor(id, (float*) value);
    break;
  case FRU_E1S_IOCM:
    if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
      syslog(LOG_WARNING, "%s() Failed to get chassis type\n", __func__);
      return ERR_UNKNOWN_FRU;
    }
    if ((ret = check_e1s_iocm_present(sensor_num, chassis_type)) < 0) {
      break;
    }
    if ((ret = check_server_dc_power(sensor_num, chassis_type)) < 0) {
      break;
    }
    get_current_source(KEY_DPB_SOURCE_INFO, &dpb_source_info);
    if (chassis_type == CHASSIS_TYPE5) {
      id = e1s_sensor_map[sensor_num].id;
      ret = e1s_sensor_map[sensor_num].read_sensor(id, (float*) value);
      break;
    } else if (chassis_type == CHASSIS_TYPE7) {
      if (get_iocm_snr_ready_flag() == false) {
        if (owning_iocm_snr_flag) {
          reload_iocm_sensors();
        } else {
          ret = ERR_SENSOR_NA;
          break;
        }
      }
      get_current_source(KEY_IOCM_SOURCE_INFO, &iocm_source_info);
      id = iocm_sensor_map[sensor_num].id;
      ret = iocm_sensor_map[sensor_num].read_sensor(id, (float*) value);
      if (ret == ERR_FAILURE) { // hotswap happened during sensor monitor interval, reinit IOCM sensors
        if (max_iocm_reinit_times-- > 0) {
          set_iocm_snr_ready_flag(false);
        }
      } else if (ret == 0) {
        max_iocm_reinit_times = MAX_RETRY;
      }
      break;
    } else {
      syslog(LOG_WARNING, "%s() Unknow chassis type %u\n", __func__, chassis_type);
      return ERR_UNKNOWN_FRU;
    }
  default:
    return ERR_SENSOR_NA;
  }

  return ret;
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  uint8_t chassis_type = 0;
  uint8_t uic_location = 0;

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
  case FRU_E1S_IOCM:
    if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
      syslog(LOG_WARNING, "%s() Failed to get chassis type\n", __func__);
      return ERR_UNKNOWN_FRU;
    }
    if (chassis_type == CHASSIS_TYPE5) {
      if (pal_get_uic_location(&uic_location) < 0) {
        syslog(LOG_WARNING, "%s() Failed to get uic location\n", __func__);
        return ERR_FAILURE;
      }

      snprintf(name, MAX_SENSOR_NAME_SIZE, "%s", e1s_sensor_map[sensor_num].snr_name);

      if(uic_location == UIC_SIDEA) {
        if (set_e1s_sensor_name(name, E1S_SENSOR_NAME_SIDE_A) < 0) {
          syslog(LOG_WARNING, "%s() Failed to set E1.S sensor name with A side\n", __func__);
          return ERR_FAILURE;
        }
      } else if(uic_location == UIC_SIDEB) {
        if (set_e1s_sensor_name(name, E1S_SENSOR_NAME_SIDE_B) < 0) {
          syslog(LOG_WARNING, "%s() Failed to set E1.S sensor name with B side\n", __func__);
          return ERR_FAILURE;
        }
      } else {
        syslog(LOG_WARNING, "%s() The UIC location is unknown, please check UIC location\n", __func__);
        return ERR_FAILURE;
      }
      break;
    } else if (chassis_type == CHASSIS_TYPE7) {
      snprintf(name, MAX_SENSOR_NAME_SIZE, "%s", iocm_sensor_map[sensor_num].snr_name);
      break;
    } else {
      syslog(LOG_WARNING, "%s() Unknow chassis type %u\n", __func__, chassis_type);
      return ERR_UNKNOWN_FRU;
    }
  default:
    return ERR_UNKNOWN_FRU;
  }

  return 0;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  float *val = (float*) value;
  bool *exp_thresh_init = NULL;
  uint8_t chassis_type = 0;
  PAL_SENSOR_MAP * sensor_map = NULL;

  switch (fru) {
  case FRU_UIC:
    sensor_map = uic_sensor_map;
    break;
  case FRU_DPB:
    exp_thresh_init = &dpb_thresh_init;
    sensor_map = dpb_sensor_map;
    break;
  case FRU_SCC:
    exp_thresh_init = &scc_thresh_init;
    sensor_map = scc_sensor_map;
    break;
  case FRU_NIC:
    sensor_map = nic_sensor_map;
    break;
  case FRU_E1S_IOCM:
    if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
      syslog(LOG_WARNING, "%s() Failed to get chassis type\n", __func__);
      return ERR_UNKNOWN_FRU;
    }
    if (chassis_type == CHASSIS_TYPE5) {
      sensor_map = e1s_sensor_map;
      break;
    } else if (chassis_type == CHASSIS_TYPE7) {
      sensor_map = iocm_sensor_map;
      break;
    } else {
      syslog(LOG_WARNING, "%s() Unknow chassis type %u\n", __func__, chassis_type);
      return ERR_UNKNOWN_FRU;
    }
  default:
    return ERR_UNKNOWN_FRU;
  }

  if ((fru == FRU_DPB) || (fru == FRU_SCC)) {
    // sensord don't need to get threshold of SCC_IOC_TEMP from file.
    if ((owning_iocm_snr_flag == false) && (*exp_thresh_init == false)) {
      if (exp_get_sensor_thresh_from_file(fru) < 0) {
        syslog(LOG_WARNING, "%s:fail to get sensor threshold of FRU:%d, use initial sensors' threshold\n", __func__, fru);
      }
      *exp_thresh_init = true;
    }
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
  uint8_t chassis_type = 0;

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
  case FRU_E1S_IOCM:
    if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
      syslog(LOG_WARNING, "%s() Failed to get chassis type\n", __func__);
      return ERR_UNKNOWN_FRU;
    }
    if (chassis_type == CHASSIS_TYPE5) {
      sensor_units = e1s_sensor_map[sensor_num].units;
      break;
    } else if (chassis_type == CHASSIS_TYPE7) {
      sensor_units = iocm_sensor_map[sensor_num].units;
      break;
    } else {
      syslog(LOG_WARNING, "%s() Unknow chassis type %u\n", __func__, chassis_type);
      return ERR_UNKNOWN_FRU;
    }
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
  case FLOW:
    sprintf(units, "CFM");
    break;
  default:
    syslog(LOG_WARNING, "%s() unit not found, sensor number: %x, unit: %u\n", __func__, sensor_num, sensor_units);
    break;
  }

  return 0;
}

bool
pal_sensor_is_cached(uint8_t fru, uint8_t sensor_num) {
  // Read DPB HSC power from expander directly
  if ((fru == FRU_DPB) && (sensor_num == DPB_HSC_PWR)) {
    return false;
  }
  return true;
}

int
pal_get_fan_speed(uint8_t fan, int *rpm) {
  int ret = 0;
  float value = 0;
  int fan_cnt = pal_get_tach_cnt();

  if (rpm == NULL) {
    syslog(LOG_WARNING, "%s() pointer \"rpm\" is NULL\n", __func__);
    return -1;
  }

  if (fan >= fan_cnt) {
    syslog(LOG_WARNING, "%s: Invalid fan index: %d, fan count: %d", __func__, fan, fan_cnt);
    return -1;
  }

  if (fan_cnt == SINGLE_FAN_CNT) {
    ret = sensor_cache_read(FRU_DPB, FAN_0_FRONT + (fan * 2), &value);
  } else if (fan_cnt == DUAL_FAN_CNT) {
    ret = sensor_cache_read(FRU_DPB, FAN_0_FRONT + fan, &value);
  } else {
    syslog(LOG_WARNING, "%s: Unknown fan count", __func__);
    return -1;
  }

  if (ret == 0) {
    *rpm = (int) value;
  } else {
    syslog(LOG_ERR, "%s() failed to get fan%d speed\n", __func__, fan);
  }

  return ret;
}

static void apply_inlet_correction(float *value, inlet_corr_t *ict, size_t ict_cnt) {
  float offset_value = 0;
  float airflow_value = 0;
  int i = 0, ret = 0;

  if (value == NULL) {
    syslog(LOG_WARNING, "%s(): fail to regulate inlet sensor because NULL parameter: *value", __func__);
    return;
  }

  // Get airflow value
  ret = sensor_cache_read(FRU_DPB, AIRFLOW, &airflow_value);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s(): fail to regulate inlet sensor because fail to get airflow", __func__);
    return;
  }

  // Scan the correction table to get correction value
  offset_value = ict[0].offset_value;
  for (i = 1; i < ict_cnt; i++) {
    if (airflow_value >= ict[i].airflow_value) {
      offset_value = ict[i].offset_value;
    } else {
      break;
    }
  }

  // Apply correction for the sensor
  *(float*)value -= offset_value;
}

int pal_register_sensor_failure_tolerance_policy(uint8_t fru) {
  int sensor_cnt = 0, ret = 0, i = 0;
  int tolerance_time = 0;
  uint8_t *sensor_list = NULL;

  if ((fru == FRU_DPB) || (fru == FRU_SCC)) {
    tolerance_time = EXP_SNR_FAIL_TOLERANCE_TIME;
  }

  ret = pal_get_fru_sensor_list(fru, &sensor_list, &sensor_cnt);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: Fail to get fru%d sensor list\n", __func__, fru);
    return ret;
  }

  for (i = 0; i < sensor_cnt; i++) {
    switch (fru) {
      case FRU_SERVER:
        if ((sensor_list[i] == BS_INLET_TEMP) || (sensor_list[i] == BS_BOOT_DRV_TEMP)) {
          tolerance_time = SERVER_SNR_FAIL_TOL_TIME;
        } else {
          tolerance_time = 0;
        }
        break;
      default:
        break;
    }

    if (register_sensor_failure_tolerance_policy(fru, sensor_list[i], tolerance_time) < 0) {
      syslog(LOG_WARNING, "%s: Fail to register failure tolerance policy for fru:%d sensor num:%x\n", __func__, fru, sensor_list[i]);
    }
  }

  return 0;
}

void
pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh)
{
  char key[MAX_KEY_LEN] = {0};

  if ((fru == FRU_NIC) && (snr_num == NIC_SENSOR_P12V) && (thresh == LCR_THRESH)) {
    // Set NIC P12V status is dropped.
    snprintf(key, sizeof(key), NIC_P12V_STATUS_STR);
    if (pal_set_cached_value(key, STR_VALUE_1) < 0) {
      syslog(LOG_WARNING, "%s: Failed to set NIC P12V status when NIC P12V is dropped.\n", __func__);
    }

    // Clear NIC P12V deassert timestamp
    snprintf(key, sizeof(key), NIC_P12V_TIMESTAMP_STR);
    if (pal_set_cached_value(key, STR_VALUE_0) < 0) {
      syslog(LOG_WARNING, "%s() Failed to clear NIC P12V LCR deassert timestamp", __func__);
    }
  }

  return;
}

void
pal_sensor_deassert_handle(uint8_t fru, uint8_t snr_num, float val, uint8_t thresh)
{
  char key[MAX_KEY_LEN] = {0};
  char tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts = {0};

  if ((fru == FRU_NIC) && (snr_num == NIC_SENSOR_P12V) && (thresh == LCR_THRESH)) {
    snprintf(key, sizeof(key), NIC_P12V_TIMESTAMP_STR);

    // Set NIC P12V deassert timestamp
    clock_gettime(CLOCK_REALTIME, &ts);
    snprintf(tstr, sizeof(tstr), "%ld", ts.tv_sec);
    if (pal_set_cached_value(key, tstr) < 0) {
      syslog(LOG_WARNING, "%s() Failed to set NIC P12V LCR deassert timestamp", __func__);
    }
  }

  return;
}
