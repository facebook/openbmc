/* Copyright 2017-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __NVME_MI_H__
#define __NVME_MI_H__

#define I2C_NVME_INTF_ADDR 0x6A

#define NVME_SFLGS_REG 0x01
#define NVME_WARNING_REG 0x02
#define NVME_TEMP_REG 0x03
#define NVME_PDLU_REG 0x04
#define NVME_VENDOR_REG 0x09
#define NVME_SERIAL_NUM_REG 0x0B
#define SERIAL_NUM_SIZE 20
#define PART_NUM_SIZE 40

/* NVMe-MI Temperature Definition Code */
#define TEMP_HIGHER_THAN_127 0x7F
#define TEPM_LOWER_THAN_n60 0xC4
#define TEMP_NO_UPDATE 0x80
#define TEMP_SENSOR_FAIL 0x81

/* NVMe-MI Vendor ID Code */
#define VENDOR_ID_HGST 0x1C58
#define VENDOR_ID_HYNIX 0x1C5C
#define VENDOR_ID_INTEL 0x8086
#define VENDOR_ID_LITEON 0x14A4
#define VENDOR_ID_MICRON 0x1344
#define VENDOR_ID_SAMSUNG 0x144D
#define VENDOR_ID_SEAGATE 0x1BB1
#define VENDOR_ID_TOSHIBA 0x1179
#define VENDOR_ID_FACEBOOK 0x1D9B
#define VENDOR_ID_BROARDCOM 0x14E4
#define VENDOR_ID_QUALCOMM 0x17CB
#define VENDOR_ID_SSSTC 0x1E95

/* NVMe-MI Management End Point Form Factor */
#define MEFF_M_2_22110 0x35
#define MEFF_Dual_M_2 0xF0

/* NVMe-MI Form Factor Information 0 Register */
#define FFI_0_STORAGE 0x00
#define FFI_0_ACCELERATOR 0x01

/* NVMe-MI Power State */
#define POWER_STATE_FULL_MODE 0x01
#define POWER_STATE_REDUCED_MODE 0x02
#define POWER_STATE_LOWER_MODE 0x03

/* NVMe-MI SMBus/I2C Frquency */
#define I2C_FREQ_100K 0x01
#define I2C_FREQ_400K 0x02
#define I2C_FREQ_1M 0x03

/* NVMe-MI SMBus/I2C Frquency */
#define TDP_LEVEL1 0x01
#define TDP_LEVEL2 0x02
#define TDP_LEVEL3 0x03

/* NVMe-MI Storage Information 0*/
#define SINFO_0_PLP_NOT_DEFINED 0x0
#define SINFO_0_PLP_DEFINED 0x1

#define ASIC_CORE_VOL_UNIT 0.0001   // 100uV
#define POWER_RAIL_VOL_UNIT 0.0001  // 100uV


typedef struct {
  uint8_t sflgs;                    //Status Flags
  uint8_t warning;                  //SMART Warnings
  uint8_t temp;                     //Composite Temperature
  uint8_t pdlu;                     //Percentage Drive Life Used
  uint16_t vendor;                  //Vendor ID
  uint8_t serial_num[20];           //Serial Number
  /* For GPv2 M.2 Devices               */
  /* Module Indentifier Area  Offset 32 */
  uint8_t block_len_module_id_area; //Block length of Module Indentifier Area
  uint8_t fb_defined;               //Standard FB Device Indetifier Defined (0x1)
  uint8_t part_num[40];             //Module Product Part Number
  uint8_t meff;                     //Management End Point Form Factor
  uint8_t ffi_0;                    //Form Factor Information 0 Register
  /* Storage Area             Offset 87 */
  uint8_t ssd_ver;                  //Storage version
  uint16_t ssd_capacity;            //Storage Capacity
  uint8_t ssd_pwr;                  //Storage Power
  uint8_t ssd_sinfo_0;              //Storage Information 0
  /* Module Status Area       Offset 96 */
  uint8_t block_len_module_stat_area; //Block length of Module Status Area
  uint8_t module_helath;            //Module health
  uint8_t lower_theshold;           //Lower Thermal Threshold
  uint8_t upper_threshold;          //Upper Thermal Threshold
  uint8_t power_state;              //Power State
  uint8_t i2c_freq;                 //SMBus/I2C Frquency
  uint8_t tdp_level;                //Module Static TDP level setting
  /* Version Area             Offset 104*/
  uint8_t block_len_ver_area;       //Block length of Version Area
  uint8_t asic_version;             //ASIC version
  uint8_t fw_major_ver;             //FW version Major Revision
  uint8_t fw_minor_ver;             //FW version Minor Revision
  /* Monitor Area             Offset 112*/
  uint8_t block_len_mon_area;       //Block length of Monitor Area
  uint16_t asic_core_vol1;          //ASIC core voltage1 real time report
  uint16_t asic_core_vol2;          //ASIC core voltage2 real time report
  uint16_t power_rail_vol1;         //Module power rail1 voltage report
  uint16_t power_rail_vol2;         //Module power rail2 voltage report
  /* Error Report Area        Offset 122*/
  uint8_t block_len_err_ret_area;   //Block length of Error Report Area
  uint8_t asic_error_type;          //ASIC Error Type Report
  uint8_t module_error_type;        //Module  Error Type Report
  uint8_t warning_flag;             //Warning flag
  uint8_t interrupt_flag;           //Interrupt flag
  uint8_t max_asic_temp;            //Historical max ASIC temperature
  uint8_t total_int_mem_err_count;  //Total internal memory error count
  uint8_t total_ext_mem_err_count;  //Total external memory error count
  uint8_t smbus_err;                //SMBus Error
} ssd_data;

typedef struct {
  char key[40];
  char value[40];
} t_key_value_pair;

// For Byte 1 - Status Flag.
typedef struct {
  t_key_value_pair self; //"Status Flag" and it's raw data
  t_key_value_pair read_complete;
  t_key_value_pair ready;
  t_key_value_pair functional;
  t_key_value_pair reset_required;
  t_key_value_pair port0_link;
  t_key_value_pair port1_link;
} t_status_flags;

// For Byte 2 - SMART critical warning
typedef struct{
t_key_value_pair self; // SMART Critical Warning and it's raw data
t_key_value_pair spare_space;
t_key_value_pair temp_warning;
t_key_value_pair reliability;
t_key_value_pair media_status;
t_key_value_pair backup_device;
} t_smart_warning;

// For checking nvme fileds valid or not.
enum {
  INVALID = 0,
  VALID
};

int nvme_read_byte(const char *i2c_bus, uint8_t item, uint8_t *value);
int nvme_read_word(const char *i2c_bus, uint8_t item, uint16_t *value);
int nvme_sflgs_read(const char *i2c_bus, uint8_t *value);
int nvme_smart_warning_read(const char *i2c_bus, uint8_t *value);
int nvme_temp_read(const char *i2c_bus, uint8_t *value);
int nvme_pdlu_read(const char *i2c_bus, uint8_t *value);
int nvme_vendor_read(const char *i2c_bus, uint16_t *value);
int nvme_serial_num_read(const char *i2c_bus, uint8_t *value, int size);

int check_nvme_fileds_valid(uint8_t block_len, t_key_value_pair *tmp_decoding);
int nvme_sflgs_decode(uint8_t value, t_status_flags *status_flag_decoding);
int nvme_smart_warning_decode(uint8_t value, t_smart_warning *smart_warning_decoding);
int nvme_temp_decode(uint8_t value, t_key_value_pair *temp_decoding);
int nvme_pdlu_decode(uint8_t value, t_key_value_pair *pdlu_decoding);
int nvme_vendor_decode(uint16_t value, t_key_value_pair *vendor_decoding);
int nvme_serial_num_decode(uint8_t *value, t_key_value_pair *sn_decoding);
int nvme_part_num_decode(uint8_t block_len, uint8_t *value, t_key_value_pair *pn_decoding);
int nvme_meff_decode (uint8_t block_len, uint8_t value, t_key_value_pair *meff_decoding);
int nvme_ffi_0_decode (uint8_t block_len, uint8_t value, t_key_value_pair *ffi_0_decoding);
int nvme_lower_threshold_temp_decode(uint8_t block_len, uint8_t value, t_key_value_pair *lower_thermal_temp_decoding);
int nvme_upper_threshold_temp_decode(uint8_t block_len, uint8_t value, t_key_value_pair *upper_thermal_temp_decoding);
int nvme_power_state_decode (uint8_t block_len, uint8_t value, t_key_value_pair *power_state_decoding);
int nvme_i2c_freq_decode(uint8_t block_len, uint8_t value, t_key_value_pair *i2c_freq_decoding);
int nvme_tdp_level_decode(uint8_t block_len, uint8_t value, t_key_value_pair *tdp_level_decoding);
int nvme_max_asic_temp_decode(uint8_t block_len, uint8_t value, t_key_value_pair *max_asic_temp_decoding);
int nvme_fw_version_decode(uint8_t block_len, uint8_t major_value, uint8_t minor_value, t_key_value_pair *fw_version_decoding);
int nvme_monitor_area_decode(char *key, uint8_t block_len, uint16_t value, float unit, t_key_value_pair *monitor_area_decoding);
int nvme_total_int_mem_err_count_decode(uint8_t block_len, uint8_t value, t_key_value_pair *total_int_mem_err_count_decoding);
int nvme_total_ext_mem_err_count_decode(uint8_t block_len, uint8_t value, t_key_value_pair *total_ext_mem_err_count_decoding);
int nvme_smbus_err_decode(uint8_t block_len, uint8_t value, t_key_value_pair *smbus_err_decoding);
int nvme_raw_data_prase(char *key, uint8_t block_len, uint8_t value, t_key_value_pair *raw_data_prasing);
int nvme_sinfo_0_decode (uint8_t value, t_key_value_pair *sinfo_0_decoding);

int nvme_sflgs_read_decode(const char *i2c_bus, uint8_t *value, t_status_flags *status_flag_decoding);
int nvme_smart_warning_read_decode(const char *i2c_bus, uint8_t *value, t_smart_warning *smart_warning_decoding);
int nvme_temp_read_decode(const char *i2c_bus, uint8_t *value, t_key_value_pair *temp_decoding);
int nvme_pdlu_read_decode(const char *i2c_bus, uint8_t *value, t_key_value_pair *pdlu_decoding);
int nvme_vendor_read_decode(const char *i2c_bus, uint16_t *value, t_key_value_pair *vendor_decoding);
int nvme_serial_num_read_decode(const char *i2c_bus, uint8_t *value, int size, t_key_value_pair *sn_decoding);

#endif
