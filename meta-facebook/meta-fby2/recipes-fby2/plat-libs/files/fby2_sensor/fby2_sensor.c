/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <time.h>
#include <openbmc/obmc-i2c.h>
#include "fby2_sensor.h"
#include <openbmc/nvme-mi.h>

#define LARGEST_DEVICE_NAME 120

#define MEZZ_TEMP_DEVICE "/sys/class/i2c-adapter/i2c-11/11-001f/hwmon/hwmon*"
#define GPIO_VAL "/sys/class/gpio/gpio%d/value"

#define I2C_BUS_1_DIR "/sys/class/i2c-adapter/i2c-1/"
#define I2C_BUS_5_DIR "/sys/class/i2c-adapter/i2c-5/"
#define I2C_BUS_9_DIR "/sys/class/i2c-adapter/i2c-9/"

#define TACH_DIR "/sys/devices/platform/ast_pwm_tacho.0"
#define ADC_DIR "/sys/devices/platform/ast_adc.0"

#define SP_INLET_TEMP_DEVICE I2C_BUS_9_DIR "9-004e/hwmon/hwmon*"
#define SP_OUTLET_TEMP_DEVICE I2C_BUS_9_DIR "9-004f/hwmon/hwmon*"

#define DC_SLOT1_INLET_TEMP_DEVICE I2C_BUS_1_DIR "1-004d/hwmon/hwmon*"
#define DC_SLOT1_OUTLET_TEMP_DEVICE I2C_BUS_1_DIR "1-004e/hwmon/hwmon*"

#define DC_SLOT3_INLET_TEMP_DEVICE I2C_BUS_5_DIR "5-004d/hwmon/hwmon*"
#define DC_SLOT3_OUTLET_TEMP_DEVICE I2C_BUS_5_DIR "5-004e/hwmon/hwmon*"

#define FAN_TACH_RPM "tacho%d_rpm"
#define ADC_VALUE "adc%d_value"

#define UNIT_DIV 1000

#define I2C_DEV_DC_1 "/dev/i2c-1"
#define I2C_DEV_DC_3 "/dev/i2c-5"
#define I2C_DC_INA_ADDR 0x40
#define I2C_DC_MUX_ADDR 0x71
#define DC_INA230_DEFAULT_CALIBRATION 0x000A

#define I2C_DEV_HSC "/dev/i2c-10"
#define I2C_HSC_ADDR 0x80  // 8-bit
#define EIN_ROLLOVER_CNT 0x10000
#define EIN_SAMPLE_CNT 0x1000000
#define EIN_ENERGY_CNT 0x800000
#define PIN_COEF (0.0163318634656214)  // X = 1/m * (Y * 10^(-R) - b) = 1/6123 * (Y * 100)
#define ADM1278_R_SENSE 0.3

#define I2C_DEV_NIC "/dev/i2c-11"
#define I2C_NIC_ADDR 0x3e  // 8-bit
#define I2C_NIC_SENSOR_TEMP_REG 0x01

#define BIC_SENSOR_READ_NA 0x20

#define MAX_SENSOR_NUM 0xFF
#define ALL_BYTES 0xFF
#define LAST_REC_ID 0xFFFF

#define FBY2_SDR_PATH "/tmp/sdr_%s.bin"
#define SLOT_FILE "/tmp/slot%d.bin"

#define TOTAL_M2_CH_ON_GP 6
#define MAX_POS_READING_MARGIN 127

static float hsc_r_sense = ADM1278_R_SENSE;

/* Error codes returned */
#define ERR_UNKNOWN_FRU -1
#define ERR_SENSOR_NA   -2
#define ERR_FAILURE     -3

// List of BIC sensors which need to do negative reading handle
const uint8_t bic_neg_reading_sensor_support_list[] = {
  /* Temperature sensors*/
  BIC_SENSOR_MB_OUTLET_TEMP,
  BIC_SENSOR_MB_OUTLET_TEMP_BOTTOM,
  BIC_SENSOR_MB_INLET_TEMP,
  BIC_SENSOR_PCH_TEMP,
  BIC_SENSOR_SOC_TEMP,
  BIC_SENSOR_SOC_DIMMA0_TEMP,
  BIC_SENSOR_SOC_DIMMA1_TEMP,
  BIC_SENSOR_SOC_DIMMB0_TEMP,
  BIC_SENSOR_SOC_DIMMB1_TEMP,
  BIC_SENSOR_SOC_DIMMD0_TEMP,
  BIC_SENSOR_SOC_DIMMD1_TEMP,
  BIC_SENSOR_SOC_DIMME0_TEMP,
  BIC_SENSOR_SOC_DIMME1_TEMP,
  BIC_SENSOR_NVME1_CTEMP,
  BIC_SENSOR_NVME2_CTEMP,
  BIC_SENSOR_VCCIO_VR_CURR,
  BIC_SENSOR_VCCIN_VR_CURR,
  BIC_SENSOR_VDDR_AB_VR_CURR,
  BIC_SENSOR_VDDR_DE_VR_CURR,
  BIC_SENSOR_1V05_PCH_VR_CURR,
  BIC_SENSOR_VCCSA_VR_CURR,
  BIC_SENSOR_VNN_PCH_VR_CURR,
};

#ifdef CONFIG_FBY2_GPV2
const uint8_t bic_gpv2_neg_reading_sensor_support_list[] = {
  GPV2_SENSOR_INLET_TEMP,
  GPV2_SENSOR_OUTLET_TEMP,
  GPV2_SENSOR_PCIE_SW_TEMP,
  // VR
  GPV2_SENSOR_3V3_VR_Temp,
  GPV2_SENSOR_0V92_VR_Temp,
  //M.2 A
  GPV2_SENSOR_M2A_Temp,
  //M.2 B
  GPV2_SENSOR_M2B_Temp,
  //M.2 C
  GPV2_SENSOR_M2C_Temp,
  //M.2 D
  GPV2_SENSOR_M2D_Temp,
  //M.2 E
  GPV2_SENSOR_M2E_Temp,
  //M.2 F
  GPV2_SENSOR_M2F_Temp,
  //M.2 G
  GPV2_SENSOR_M2G_Temp,
  //M.2 H
  GPV2_SENSOR_M2H_Temp,
  //M.2 I
  GPV2_SENSOR_M2I_Temp,
  //M.2 J
  GPV2_SENSOR_M2J_Temp,
  //M.2 K
  GPV2_SENSOR_M2K_Temp,
  //M.2 L
  GPV2_SENSOR_M2L_Temp,
};
#endif

const uint8_t bic_sdr_accuracy_sensor_support_list[] = {
  BIC_SENSOR_VCCIN_VR_POUT,
  BIC_SENSOR_INA230_POWER,
  BIC_SENSOR_SOC_PACKAGE_PWR,
};

// List of BIC sensors (Twinlake) to be monitored
const uint8_t bic_sensor_list[] = {
  /* Threshold sensors */
  BIC_SENSOR_MB_OUTLET_TEMP,
  BIC_SENSOR_MB_OUTLET_TEMP_BOTTOM,
  BIC_SENSOR_MB_INLET_TEMP,
  BIC_SENSOR_PCH_TEMP,
  BIC_SENSOR_SOC_TEMP,
  BIC_SENSOR_SOC_THERM_MARGIN,
  BIC_SENSOR_SOC_TJMAX,
  BIC_SENSOR_SOC_DIMMA0_TEMP,
  BIC_SENSOR_SOC_DIMMA1_TEMP,
  BIC_SENSOR_SOC_DIMMB0_TEMP,
  BIC_SENSOR_SOC_DIMMB1_TEMP,
  BIC_SENSOR_SOC_DIMMD0_TEMP,
  BIC_SENSOR_SOC_DIMMD1_TEMP,
  BIC_SENSOR_SOC_DIMME0_TEMP,
  BIC_SENSOR_SOC_DIMME1_TEMP,
  BIC_SENSOR_SOC_PACKAGE_PWR,
  BIC_SENSOR_VCCIN_VR_TEMP,
  BIC_SENSOR_VCCIO_VR_TEMP,
  BIC_SENSOR_NVME1_CTEMP,
  BIC_SENSOR_NVME2_CTEMP,
  BIC_SENSOR_1V05_PCH_VR_TEMP,
  BIC_SENSOR_VNN_PCH_VR_TEMP,
  BIC_SENSOR_VDDR_AB_VR_TEMP,
  BIC_SENSOR_VDDR_DE_VR_TEMP,
  BIC_SENSOR_VCCSA_VR_TEMP,
  BIC_SENSOR_VCCIN_VR_VOL,
  BIC_SENSOR_VCCIO_VR_VOL,
  BIC_SENSOR_1V05_PCH_VR_VOL,
  BIC_SENSOR_VDDR_AB_VR_VOL,
  BIC_SENSOR_VDDR_DE_VR_VOL,
  BIC_SENSOR_VCCSA_VR_VOL,
  BIC_SENSOR_VCCIN_VR_CURR,
  BIC_SENSOR_VCCIO_VR_CURR,
  BIC_SENSOR_1V05_PCH_VR_CURR,
  BIC_SENSOR_VNN_PCH_VR_CURR,
  BIC_SENSOR_VDDR_AB_VR_CURR,
  BIC_SENSOR_VDDR_DE_VR_CURR,
  BIC_SENSOR_VCCSA_VR_CURR,
  BIC_SENSOR_VCCIN_VR_POUT,
  BIC_SENSOR_VCCIO_VR_POUT,
  BIC_SENSOR_1V05_PCH_VR_POUT,
  BIC_SENSOR_VNN_PCH_VR_POUT,
  BIC_SENSOR_VDDR_AB_VR_POUT,
  BIC_SENSOR_VDDR_DE_VR_POUT,
  BIC_SENSOR_VCCSA_VR_POUT,
  BIC_SENSOR_INA230_POWER,
  BIC_SENSOR_INA230_VOL,
  BIC_SENSOR_P3V3_MB,
  BIC_SENSOR_P12V_MB,
  BIC_SENSOR_P1V05_PCH,
  BIC_SENSOR_P3V3_STBY_MB,
  BIC_SENSOR_PV_BAT,
  BIC_SENSOR_PVDDR_AB,
  BIC_SENSOR_PVDDR_DE,
  BIC_SENSOR_PVNN_PCH,
};

const uint8_t bic_discrete_list[] = {
  /* Discrete sensors */
  BIC_SENSOR_SYSTEM_STATUS,
  BIC_SENSOR_PROC_FAIL,
};

// List of BIC (RC) sensors to be monitored
const uint8_t bic_rc_sensor_list[] = {
  BIC_RC_SENSOR_MB_OUTLET_TEMP,
  BIC_RC_SENSOR_MB_INLET_TEMP,
  BIC_RC_SENSOR_SENSOR_TEMP,
  BIC_RC_SENSOR_SOC_TEMP_DIODE,
  BIC_RC_SENSOR_NVME1_CTEMP,
  BIC_RC_SENSOR_P12V_MB,
  BIC_RC_SENSOR_P3V3_STBY_MB,
  BIC_RC_SENSOR_P3V2_MB,
  BIC_RC_SENSOR_PV_BAT,
  BIC_RC_SENSOR_PVDDQ_423,
  BIC_RC_SENSOR_PVDDQ_510,
  BIC_RC_SENSOR_SOC_TEMP_IMC,
  BIC_RC_SENSOR_PMF2432_TEMP,
  BIC_RC_SENSOR_PMF2344_TEMP,
  BIC_RC_SENSOR_CVR_APC_TEMP,
  BIC_RC_SENSOR_CVR_CBF_TEMP,
  BIC_RC_SENSOR_SOC_DIMMB_TEMP,
  BIC_RC_SENSOR_SOC_DIMMA_TEMP,
  BIC_RC_SENSOR_SOC_DIMMC_TEMP,
  BIC_RC_SENSOR_SOC_DIMMD_TEMP,
  BIC_RC_SENSOR_SOC_PWR,
  BIC_RC_SENSOR_PVDDQ_423_VR_TEMP,
  BIC_RC_SENSOR_PVDDQ_510_VR_TEMP,
  BIC_RC_SENSOR_PVDDQ_423_VR_VOL,
  BIC_RC_SENSOR_PVDDQ_510_VR_VOL,
  BIC_RC_SENSOR_CVR_APC_VOL,
  BIC_RC_SENSOR_CVR_CBF_VOL,
  BIC_RC_SENSOR_PVDDQ_423_VR_CURR,
  BIC_RC_SENSOR_PVDDQ_510_VR_CURR,
  BIC_RC_SENSOR_CVR_APC_CURR,
  BIC_RC_SENSOR_CVR_CBF_CURR,
  BIC_RC_SENSOR_PVDDQ_423_VR_POUT,
  BIC_RC_SENSOR_PVDDQ_510_VR_POUT,
  BIC_RC_SENSOR_CVR_APC_POUT,
  BIC_RC_SENSOR_CVR_CBF_POUT,
  BIC_RC_SENSOR_INA230_VOL,
  BIC_RC_SENSOR_INA230_POWER,
};

const uint8_t bic_rc_discrete_list[] = {
  /* RC discrete sensors */
  BIC_RC_SENSOR_SYSTEM_STATUS ,
  BIC_RC_SENSOR_VR_HOT ,
  BIC_RC_SENSOR_PROC_FAIL,
};

#ifdef CONFIG_FBY2_EP
// List of BIC (EP) sensors to be monitored
const uint8_t bic_ep_sensor_list[] = {
  BIC_EP_SENSOR_MB_OUTLET_TEMP,
  BIC_EP_SENSOR_MB_OUTLET_TEMP_BOTTOM,
  BIC_EP_SENSOR_MB_INLET_TEMP,
  BIC_EP_SENSOR_NVME1_CTEMP,
  BIC_EP_SENSOR_NVME2_CTEMP,
  BIC_EP_SENSOR_SOC_TEMP,
  BIC_EP_SENSOR_SOC_DIMMA_TEMP,
  BIC_EP_SENSOR_SOC_DIMMB_TEMP,
  BIC_EP_SENSOR_SOC_DIMMC_TEMP,
  BIC_EP_SENSOR_SOC_DIMMD_TEMP,
  BIC_EP_SENSOR_SOC_PACKAGE_PWR,
  BIC_EP_SENSOR_VDD_CORE_VR_TEMP,
  BIC_EP_SENSOR_VDD_SRAM_VR_TEMP,
  BIC_EP_SENSOR_VDD_MEM_VR_TEMP,
  BIC_EP_SENSOR_VDD_SOC_VR_TEMP,
  BIC_EP_SENSOR_VDD_CORE_VR_VOL,
  BIC_EP_SENSOR_VDD_SRAM_VR_VOL,
  BIC_EP_SENSOR_VDD_MEM_VR_VOL,
  BIC_EP_SENSOR_VDD_SOC_VR_VOL,
  BIC_EP_SENSOR_VDD_CORE_VR_CURR,
  BIC_EP_SENSOR_VDD_SRAM_VR_CURR,
  BIC_EP_SENSOR_VDD_MEM_VR_CURR,
  BIC_EP_SENSOR_VDD_SOC_VR_CURR,
  BIC_EP_SENSOR_VDD_CORE_VR_POUT,
  BIC_EP_SENSOR_VDD_SRAM_VR_POUT,
  BIC_EP_SENSOR_VDD_MEM_VR_POUT,
  BIC_EP_SENSOR_VDD_SOC_VR_POUT,
  BIC_EP_SENSOR_P3V3_MB,
  BIC_EP_SENSOR_P12V_STBY_MB,
  BIC_EP_SENSOR_P1V8_VDD,
  BIC_EP_SENSOR_P3V3_STBY_MB,
  BIC_EP_SENSOR_PVDDQ_AB,
  BIC_EP_SENSOR_PVDDQ_CD,
  BIC_EP_SENSOR_PV_BAT,
  BIC_EP_SENSOR_P0V8_VDD,
  BIC_EP_SENSOR_INA230_POWER,
  BIC_EP_SENSOR_INA230_VOL,
};

const uint8_t bic_ep_discrete_list[] = {
  /* EP Discrete sensors */
  BIC_EP_SENSOR_SYSTEM_STATUS,
  BIC_EP_SENSOR_PROC_FAIL,
};
#endif



// List of SPB sensors to be monitored
const uint8_t spb_sensor_list[] = {
  SP_SENSOR_INLET_TEMP,
  SP_SENSOR_OUTLET_TEMP,
  //SP_SENSOR_MEZZ_TEMP
  SP_SENSOR_FAN0_TACH,
  SP_SENSOR_FAN1_TACH,
  //SP_SENSOR_AIR_FLOW,
  SP_SENSOR_P5V,
  SP_SENSOR_P12V,
  SP_SENSOR_P3V3_STBY,
  SP_SENSOR_P12V_SLOT1,
  SP_SENSOR_P12V_SLOT2,
  SP_SENSOR_P12V_SLOT3,
  SP_SENSOR_P12V_SLOT4,
  SP_SENSOR_P3V3,
  SP_SENSOR_P1V15_BMC_STBY,
  SP_SENSOR_P1V2_BMC_STBY,
  SP_SENSOR_P2V5_BMC_STBY,
  SP_P1V8_STBY,
  SP_SENSOR_HSC_IN_VOLT,
  SP_SENSOR_HSC_OUT_CURR,
  SP_SENSOR_HSC_TEMP,
  SP_SENSOR_HSC_IN_POWER,
  SP_SENSOR_HSC_PEAK_IOUT,
  SP_SENSOR_HSC_PEAK_PIN,
};

const uint8_t dc_sensor_list[] = {
  DC_SENSOR_OUTLET_TEMP,
  DC_SENSOR_INLET_TEMP,
  DC_SENSOR_INA230_VOLT,
  DC_SENSOR_INA230_POWER,
  DC_SENSOR_NVMe1_CTEMP,
  DC_SENSOR_NVMe2_CTEMP,
  DC_SENSOR_NVMe3_CTEMP,
  DC_SENSOR_NVMe4_CTEMP,
  DC_SENSOR_NVMe5_CTEMP,
  DC_SENSOR_NVMe6_CTEMP,
};

// List of CF sensors to be monitored
const uint8_t dc_cf_sensor_list[] = {
  DC_CF_SENSOR_OUTLET_TEMP,
  DC_CF_SENSOR_INLET_TEMP,
  DC_CF_SENSOR_INA230_VOLT,
  DC_CF_SENSOR_INA230_POWER,
};

// List of NIC sensors to be monitored
const uint8_t nic_sensor_list[] = {
  MEZZ_SENSOR_TEMP,
};

#ifdef CONFIG_FBY2_GPV2
const uint8_t gpv2_sensor_list[] = {
  GPV2_SENSOR_INLET_TEMP,
  GPV2_SENSOR_OUTLET_TEMP,
  GPV2_SENSOR_P12V_BIC_SCALED,
  GPV2_SENSOR_P3V3_STBY_BIC_SCALED,
  GPV2_SENSOR_P0V92_BIC_SCALED,
  GPV2_SENSOR_P1V8_BIC_SCALED,
  GPV2_SENSOR_INA230_POWER,
  GPV2_SENSOR_INA230_VOLT,
  GPV2_SENSOR_PCIE_SW_TEMP,
  // VR
  GPV2_SENSOR_3V3_VR_Vol,
  GPV2_SENSOR_0V92_VR_Vol,
  GPV2_SENSOR_3V3_VR_Curr,
  GPV2_SENSOR_0V92_VR_Curr,
  GPV2_SENSOR_3V3_VR_Pwr,
  GPV2_SENSOR_0V92_VR_Pwr,
  GPV2_SENSOR_3V3_VR_Temp,
  GPV2_SENSOR_0V92_VR_Temp,
  //M.2 A
  GPV2_SENSOR_M2A_INA231_PW,
  GPV2_SENSOR_M2A_INA231_VOL,
  GPV2_SENSOR_M2A_FPGA_PW,
  GPV2_SENSOR_M2A_FPGA_VOL,
  GPV2_SENSOR_M2A_DDR_PW,
  GPV2_SENSOR_M2A_DDR_VOL,
  GPV2_SENSOR_M2A_Temp,
  //M.2 B
  GPV2_SENSOR_M2B_INA231_PW,
  GPV2_SENSOR_M2B_INA231_VOL,
  GPV2_SENSOR_M2B_FPGA_PW,
  GPV2_SENSOR_M2B_FPGA_VOL,
  GPV2_SENSOR_M2B_DDR_PW,
  GPV2_SENSOR_M2B_DDR_VOL,
  GPV2_SENSOR_M2B_Temp,
  //M.2 C
  GPV2_SENSOR_M2C_INA231_PW,
  GPV2_SENSOR_M2C_INA231_VOL,
  GPV2_SENSOR_M2C_FPGA_PW,
  GPV2_SENSOR_M2C_FPGA_VOL,
  GPV2_SENSOR_M2C_DDR_PW,
  GPV2_SENSOR_M2C_DDR_VOL,
  GPV2_SENSOR_M2C_Temp,
  //M.2 D
  GPV2_SENSOR_M2D_INA231_PW,
  GPV2_SENSOR_M2D_INA231_VOL,
  GPV2_SENSOR_M2D_FPGA_PW,
  GPV2_SENSOR_M2D_FPGA_VOL,
  GPV2_SENSOR_M2D_DDR_PW,
  GPV2_SENSOR_M2D_DDR_VOL,
  GPV2_SENSOR_M2D_Temp,
  //M.2 E
  GPV2_SENSOR_M2E_INA231_PW,
  GPV2_SENSOR_M2E_INA231_VOL,
  GPV2_SENSOR_M2E_FPGA_PW,
  GPV2_SENSOR_M2E_FPGA_VOL,
  GPV2_SENSOR_M2E_DDR_PW,
  GPV2_SENSOR_M2E_DDR_VOL,
  GPV2_SENSOR_M2E_Temp,
  //M.2 F
  GPV2_SENSOR_M2F_INA231_PW,
  GPV2_SENSOR_M2F_INA231_VOL,
  GPV2_SENSOR_M2F_FPGA_PW,
  GPV2_SENSOR_M2F_FPGA_VOL,
  GPV2_SENSOR_M2F_DDR_PW,
  GPV2_SENSOR_M2F_DDR_VOL,
  GPV2_SENSOR_M2F_Temp,
  //M.2 G
  GPV2_SENSOR_M2G_INA231_PW,
  GPV2_SENSOR_M2G_INA231_VOL,
  GPV2_SENSOR_M2G_FPGA_PW,
  GPV2_SENSOR_M2G_FPGA_VOL,
  GPV2_SENSOR_M2G_DDR_PW,
  GPV2_SENSOR_M2G_DDR_VOL,
  GPV2_SENSOR_M2G_Temp,
  //M.2 H
  GPV2_SENSOR_M2H_INA231_PW,
  GPV2_SENSOR_M2H_INA231_VOL,
  GPV2_SENSOR_M2H_FPGA_PW,
  GPV2_SENSOR_M2H_FPGA_VOL,
  GPV2_SENSOR_M2H_DDR_PW,
  GPV2_SENSOR_M2H_DDR_VOL,
  GPV2_SENSOR_M2H_Temp,
  //M.2 I
  GPV2_SENSOR_M2I_INA231_PW,
  GPV2_SENSOR_M2I_INA231_VOL,
  GPV2_SENSOR_M2I_FPGA_PW,
  GPV2_SENSOR_M2I_FPGA_VOL,
  GPV2_SENSOR_M2I_DDR_PW,
  GPV2_SENSOR_M2I_DDR_VOL,
  GPV2_SENSOR_M2I_Temp,
  //M.2 J
  GPV2_SENSOR_M2J_INA231_PW,
  GPV2_SENSOR_M2J_INA231_VOL,
  GPV2_SENSOR_M2J_FPGA_PW,
  GPV2_SENSOR_M2J_FPGA_VOL,
  GPV2_SENSOR_M2J_DDR_PW,
  GPV2_SENSOR_M2J_DDR_VOL,
  GPV2_SENSOR_M2J_Temp,
  //M.2 K
  GPV2_SENSOR_M2K_INA231_PW,
  GPV2_SENSOR_M2K_INA231_VOL,
  GPV2_SENSOR_M2K_FPGA_PW,
  GPV2_SENSOR_M2K_FPGA_VOL,
  GPV2_SENSOR_M2K_DDR_PW,
  GPV2_SENSOR_M2K_DDR_VOL,
  GPV2_SENSOR_M2K_Temp,
  //M.2 L
  GPV2_SENSOR_M2L_INA231_PW,
  GPV2_SENSOR_M2L_INA231_VOL,
  GPV2_SENSOR_M2L_FPGA_PW,
  GPV2_SENSOR_M2L_FPGA_VOL,
  GPV2_SENSOR_M2L_DDR_PW,
  GPV2_SENSOR_M2L_DDR_VOL,
  GPV2_SENSOR_M2L_Temp,
};
#endif

float spb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float dc_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float nic_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float dc_cf_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};

static void
sensor_thresh_array_init() {
  static bool init_done = false;

  if (init_done)
    return;

  spb_sensor_threshold[SP_SENSOR_INLET_TEMP][UCR_THRESH] = 40;
  spb_sensor_threshold[SP_SENSOR_OUTLET_TEMP][UCR_THRESH] = 70;
  spb_sensor_threshold[SP_SENSOR_FAN0_TACH][UCR_THRESH] = 11500;
  spb_sensor_threshold[SP_SENSOR_FAN0_TACH][UNC_THRESH] = 8500;
  spb_sensor_threshold[SP_SENSOR_FAN0_TACH][LCR_THRESH] = 500;
  spb_sensor_threshold[SP_SENSOR_FAN1_TACH][UCR_THRESH] = 11500;
  spb_sensor_threshold[SP_SENSOR_FAN1_TACH][UNC_THRESH] = 8500;
  spb_sensor_threshold[SP_SENSOR_FAN1_TACH][LCR_THRESH] = 500;
  //spb_sensor_threshold[SP_SENSOR_AIR_FLOW][UCR_THRESH] =  {75.0, 0, 0, 0, 0, 0, 0, 0};
  spb_sensor_threshold[SP_SENSOR_P5V][UCR_THRESH] = 5.5;
  spb_sensor_threshold[SP_SENSOR_P5V][LCR_THRESH] = 4.5;
  spb_sensor_threshold[SP_SENSOR_P12V][UCR_THRESH] = 13.75;
  spb_sensor_threshold[SP_SENSOR_P12V][LCR_THRESH] = 11.25;
  spb_sensor_threshold[SP_SENSOR_P3V3_STBY][UCR_THRESH] = 3.63;
  spb_sensor_threshold[SP_SENSOR_P3V3_STBY][LCR_THRESH] = 2.97;
  spb_sensor_threshold[SP_SENSOR_P12V_SLOT1][UCR_THRESH] = 13.75;
  spb_sensor_threshold[SP_SENSOR_P12V_SLOT1][LCR_THRESH] = 11.25;
  spb_sensor_threshold[SP_SENSOR_P12V_SLOT2][UCR_THRESH] = 13.75;
  spb_sensor_threshold[SP_SENSOR_P12V_SLOT2][LCR_THRESH] = 11.25;
  spb_sensor_threshold[SP_SENSOR_P12V_SLOT3][UCR_THRESH] = 13.75;
  spb_sensor_threshold[SP_SENSOR_P12V_SLOT3][LCR_THRESH] = 11.25;
  spb_sensor_threshold[SP_SENSOR_P12V_SLOT4][UCR_THRESH] = 13.75;
  spb_sensor_threshold[SP_SENSOR_P12V_SLOT4][LCR_THRESH] = 11.25;
  spb_sensor_threshold[SP_SENSOR_P3V3][UCR_THRESH] = 3.63;
  spb_sensor_threshold[SP_SENSOR_P3V3][LCR_THRESH] = 2.97;
  spb_sensor_threshold[SP_SENSOR_P1V15_BMC_STBY][UCR_THRESH] = 1.265;
  spb_sensor_threshold[SP_SENSOR_P1V15_BMC_STBY][LCR_THRESH] = 1.035;
  spb_sensor_threshold[SP_SENSOR_P1V2_BMC_STBY][UCR_THRESH] = 1.32;
  spb_sensor_threshold[SP_SENSOR_P1V2_BMC_STBY][LCR_THRESH] = 1.08;
  spb_sensor_threshold[SP_SENSOR_P2V5_BMC_STBY][UCR_THRESH] = 2.75;
  spb_sensor_threshold[SP_SENSOR_P2V5_BMC_STBY][LCR_THRESH] = 2.25;
  spb_sensor_threshold[SP_P1V8_STBY][UCR_THRESH] = 1.98;
  spb_sensor_threshold[SP_P1V8_STBY][LCR_THRESH] = 1.62;
  spb_sensor_threshold[SP_SENSOR_HSC_IN_VOLT][UCR_THRESH] = 13.75;
  spb_sensor_threshold[SP_SENSOR_HSC_IN_VOLT][LCR_THRESH] = 11.25;
  spb_sensor_threshold[SP_SENSOR_HSC_OUT_CURR][UCR_THRESH] = 52;
  spb_sensor_threshold[SP_SENSOR_HSC_TEMP][UCR_THRESH] = 120;
  spb_sensor_threshold[SP_SENSOR_HSC_IN_POWER][UCR_THRESH] = 625;

  //DC
  dc_sensor_threshold[DC_SENSOR_OUTLET_TEMP][UCR_THRESH] = 70;
  dc_sensor_threshold[DC_SENSOR_INLET_TEMP][UCR_THRESH] = 40;
  dc_sensor_threshold[DC_SENSOR_INA230_VOLT][UCR_THRESH] = 13.75;
  dc_sensor_threshold[DC_SENSOR_INA230_VOLT][LCR_THRESH] = 11.25;
  dc_sensor_threshold[DC_SENSOR_INA230_POWER][UCR_THRESH] = 80;
  dc_sensor_threshold[DC_SENSOR_NVMe1_CTEMP][UCR_THRESH] = 75;
  dc_sensor_threshold[DC_SENSOR_NVMe2_CTEMP][UCR_THRESH] = 75;
  dc_sensor_threshold[DC_SENSOR_NVMe3_CTEMP][UCR_THRESH] = 75;
  dc_sensor_threshold[DC_SENSOR_NVMe4_CTEMP][UCR_THRESH] = 75;
  dc_sensor_threshold[DC_SENSOR_NVMe5_CTEMP][UCR_THRESH] = 75;
  dc_sensor_threshold[DC_SENSOR_NVMe6_CTEMP][UCR_THRESH] = 75;

  dc_cf_sensor_threshold[DC_CF_SENSOR_OUTLET_TEMP][UCR_THRESH] = 70;
  dc_cf_sensor_threshold[DC_CF_SENSOR_INLET_TEMP][UCR_THRESH] = 40;
  dc_cf_sensor_threshold[DC_CF_SENSOR_INA230_VOLT][UCR_THRESH] = 13.75;
  dc_cf_sensor_threshold[DC_CF_SENSOR_INA230_VOLT][LCR_THRESH] = 11.25;
  dc_cf_sensor_threshold[DC_CF_SENSOR_INA230_POWER][UCR_THRESH] = 70;

  // MEZZ

  // MLX NIC will auto shutdown at 120C
  // BMC NIC will auto shutdown at 110C -
  // setting this to 120 for BMC loggintg purpose
  // (also in case older NCI FW is used that doesn't have auto-shutdown feature)
  nic_sensor_threshold[MEZZ_SENSOR_TEMP][UNR_THRESH] = 120;
  nic_sensor_threshold[MEZZ_SENSOR_TEMP][UCR_THRESH] = 105;
  nic_sensor_threshold[MEZZ_SENSOR_TEMP][UNC_THRESH] = 95; // for logging purpose
  init_done = true;
}

size_t bic_sensor_cnt = sizeof(bic_sensor_list)/sizeof(uint8_t);
size_t bic_discrete_cnt = sizeof(bic_discrete_list)/sizeof(uint8_t);

size_t bic_rc_sensor_cnt = sizeof(bic_rc_sensor_list)/sizeof(uint8_t);
size_t bic_rc_discrete_cnt = sizeof(bic_rc_discrete_list)/sizeof(uint8_t);

#ifdef CONFIG_FBY2_EP
size_t bic_ep_sensor_cnt = sizeof(bic_ep_sensor_list)/sizeof(uint8_t);
size_t bic_ep_discrete_cnt = sizeof(bic_ep_discrete_list)/sizeof(uint8_t);
#endif

size_t spb_sensor_cnt = sizeof(spb_sensor_list)/sizeof(uint8_t);

size_t nic_sensor_cnt = sizeof(nic_sensor_list)/sizeof(uint8_t);

size_t dc_sensor_cnt = sizeof(dc_sensor_list)/sizeof(uint8_t);

size_t dc_cf_sensor_cnt = sizeof(dc_cf_sensor_list)/sizeof(uint8_t);

#ifdef CONFIG_FBY2_GPV2
size_t gpv2_sensor_cnt = sizeof(gpv2_sensor_list)/sizeof(uint8_t);
#endif

enum {
  FAN0 = 0,
  FAN1,
};

enum {
  ADC_PIN0 = 0,
  ADC_PIN1,
  ADC_PIN2,
  ADC_PIN3,
  ADC_PIN4,
  ADC_PIN5,
  ADC_PIN6,
  ADC_PIN7,
  ADC_PIN8,
  ADC_PIN9,
  ADC_PIN10,
  ADC_PIN11,
};

enum ina_register {
  INA230_VOLT = 0x02,
  INA230_POWER = 0x03,
  INA230_CALIBRATION = 0x05,
};

rc_dimm_location_info rc_dimm_location_list[] = {
  // {dimm_location_file, dimm_sensor_number}
  {SYS_CONFIG_PATH "fru%u_dimm0_location", BIC_RC_SENSOR_SOC_DIMMB_TEMP},
  {SYS_CONFIG_PATH "fru%u_dimm1_location", BIC_RC_SENSOR_SOC_DIMMA_TEMP},
  {SYS_CONFIG_PATH "fru%u_dimm2_location", BIC_RC_SENSOR_SOC_DIMMC_TEMP},
  {SYS_CONFIG_PATH "fru%u_dimm3_location", BIC_RC_SENSOR_SOC_DIMMD_TEMP},
};

static sensor_info_t g_sinfo[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};
static ipmi_sensor_reading_t g_sread[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};

void
msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

int
flock_retry(int fd)
{
  int ret = 0;
  int retry_count = 0;

  ret = flock(fd, LOCK_EX | LOCK_NB);
  while (ret && (retry_count < 3)) {
    retry_count++;
    msleep(100);
    ret = flock(fd, LOCK_EX | LOCK_NB);
  }
  if (ret) {
    return -1;
  }

  return 0;
}

int
unflock_retry(int fd)
{
  int ret = 0;
  int retry_count = 0;

  ret = flock(fd, LOCK_UN);
  while (ret && (retry_count < 3)) {
    retry_count++;
    msleep(100);
    ret = flock(fd, LOCK_UN);
  }
  if (ret) {
    return -1;
  }

  return 0;
}

static int
read_device(const char *device, int *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;

#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%d", value);
  fclose(fp);

  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int
read_device_float(const char *device, float *value) {
  FILE *fp;
  int rc;
  char tmp[10];

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%s", tmp);
  fclose(fp);

  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  }

  *value = atof(tmp);

  return 0;
}

int
fby2_get_server_type(uint8_t fru, uint8_t *type) {
  return bic_get_server_type(fru, type);
}

int
fby2_get_server_type_directly(uint8_t fru, uint8_t *type) {
  int ret;
  uint8_t rbuf[64] = {0};
  ipmi_dev_id_t *id = (ipmi_dev_id_t *)rbuf;

  ret = bic_get_dev_id(fru, id);
  if (!ret) {
    // Use product ID to identify the server type
    if (id->prod_id[0] == 0x39 && id->prod_id[1] == 0x30) {
      *type = SERVER_TYPE_TL;
    } else if (id->prod_id[0] == 0x43 && id->prod_id[1] == 0x52) {
      *type = SERVER_TYPE_RC;
    } else if (id->prod_id[0] == 0x50 && id->prod_id[1] == 0x45) {
      *type = SERVER_TYPE_EP;
    } else {
      *type = SERVER_TYPE_NONE;
    }
  }

  return ret;
}

int
fby2_mux_control(char *device, uint8_t addr, uint8_t channel) {          //PCA9848
  int dev;
  int ret;
  uint8_t reg;
  int retry = 0;

  dev = open(device, O_RDWR);
  if (dev < 0) {
    syslog(LOG_ERR, "%s: open() failed", __func__);
    return -1;
  }

  /* Assign the i2c device address */
  ret = ioctl(dev, I2C_SLAVE, addr);
  if (ret < 0) {
    syslog(LOG_ERR, "%s: ioctl() assigning i2c addr failed", __func__);
    close(dev);
    return -1;
  }

  if (channel < TOTAL_M2_CH_ON_GP)       //total 6 pcs M.2 on GP
    reg = 0x01 << channel;
  else
    reg = 0x00; // close all channels

  ret = i2c_smbus_write_byte(dev, reg);
  retry = 0;
  while ((retry < 5) && (ret < 0)) {
    msleep(100);
    ret = i2c_smbus_write_byte(dev, reg);
    if (ret < 0)
      retry++;
    else
      break;
  }
  if (ret < 0) {
    close(dev);
    syslog(LOG_ERR, "%s: i2c_smbus_write_byte failed", __func__);
    return EER_READ_NA;
  }

  close(dev);
  return 0;
}

static int
read_nvme_temp(char *device, uint8_t *temp) {
  int dev, ret, retry = 3;
  uint8_t wbuf[4], rbuf[4];

  dev = open(device, O_RDWR);
  if (dev < 0) {
    return -1;
  }

  while ((--retry) >= 0) {
    wbuf[0] = 0x03;
    ret = i2c_rdwr_msg_transfer(dev, 0xD4, wbuf, 1, rbuf, 1);
    if (!ret)
      break;
    if (retry)
      msleep(10);
  }
  close(dev);
  if (ret) {
    return -1;
  }

  *temp = rbuf[0];
  return 0;
}

static int
read_m2_temp_on_gp(char *device, uint8_t sensor_num, float *value) {
  int ret;
  uint8_t mux_channel;
  uint8_t temp;

  switch (sensor_num) {
    case DC_SENSOR_NVMe1_CTEMP:
      mux_channel = MUX_CH_1;
      break;
    case DC_SENSOR_NVMe2_CTEMP:
      mux_channel = MUX_CH_0;
      break;
    case DC_SENSOR_NVMe3_CTEMP:
      mux_channel = MUX_CH_4;
      break;
    case DC_SENSOR_NVMe4_CTEMP:
      mux_channel = MUX_CH_3;
      break;
    case DC_SENSOR_NVMe5_CTEMP:
      mux_channel = MUX_CH_2;
      break;
    case DC_SENSOR_NVMe6_CTEMP:
      mux_channel = MUX_CH_5;
      break;
  }

  // control I2C multiplexer on GP to target channel
  ret = fby2_mux_control(device, I2C_DC_MUX_ADDR, mux_channel);
  if (ret < 0) {
     syslog(LOG_ERR, "%s: fby2_mux_control failed", __func__);
     return ret;
  }

  ret = read_nvme_temp(device, &temp);
  if (ret < 0) {
#ifdef DEBUG
     syslog(LOG_ERR, "%s: nvme_temp_read failed", __func__);
#endif
     return EER_READ_NA;
  }

  if ((temp >= 0x80) && (temp <= 0xC3)) {
    return EER_READ_NA;
  }
  *value = (temp <= 0x7F) ? (float)temp : -(float)(0x100 - temp);

  return 0;
}

static int
get_current_dir(const char *device, char *dir_name) {
  char cmd[LARGEST_DEVICE_NAME + 1];
  FILE *fp;
  int ret=-1;
  int size;

  // Get current working directory
  snprintf(
      cmd, LARGEST_DEVICE_NAME, "cd %s;pwd", device);

  fp = popen(cmd, "r");
  if(NULL == fp)
     return -1;
  fgets(dir_name, LARGEST_DEVICE_NAME, fp);

  ret = pclose(fp);
  if(-1 == ret)
     syslog(LOG_ERR, "$s pclose() fail ", __func__);

  // Remove the newline character at the end
  size = strlen(dir_name);
  dir_name[size-1] = '\0';

  return 0;
}


static int
read_temp_attr(const char *device, const char *attr, float *value) {
  char full_dir_name[LARGEST_DEVICE_NAME + 1];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  int tmp;

  // Get current working directory
  if (get_current_dir(device, dir_name))
  {
    return -1;
  }
  snprintf(
      full_dir_name, LARGEST_DEVICE_NAME, "%s/%s", dir_name, attr);


  if (read_device(full_dir_name, &tmp)) {
     return -1;
  }

  *value = ((float)tmp)/UNIT_DIV;

  return 0;
}

static int
read_temp(const char *device, float *value) {
  return read_temp_attr(device, "temp1_input", value);
}

static int
read_fan_value(const int fan, const char *device, float *value) {
  char device_name[LARGEST_DEVICE_NAME];
  char full_name[LARGEST_DEVICE_NAME];

  snprintf(device_name, LARGEST_DEVICE_NAME, device, fan);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", TACH_DIR, device_name);
  return read_device_float(full_name, value);
}

static int
read_adc_value(const int pin, const char *device, float *value) {
  char device_name[LARGEST_DEVICE_NAME];
  char full_name[LARGEST_DEVICE_NAME];

  snprintf(device_name, LARGEST_DEVICE_NAME, device, pin);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", ADC_DIR, device_name);
  return read_device_float(full_name, value);
}

static int
read_hsc_reg(uint8_t reg, uint8_t *rbuf, uint8_t len) {
  int dev, ret, retry = 2;
  uint8_t wbuf[4] = {0};

  dev = open(I2C_DEV_HSC, O_RDWR);
  if (dev < 0) {
    return -1;
  }

  while ((--retry) >= 0) {
    wbuf[0] = reg;
    ret = i2c_rdwr_msg_transfer(dev, I2C_HSC_ADDR, wbuf, 1, rbuf, len);
    if (!ret)
      break;
    if (retry)
      msleep(10);
  }
  close(dev);
  if (ret) {
    return -1;
  }

  return 0;
}

static float
direct2real(uint16_t direct, float m, int b, int R) {
  if (m == 0) {
    return 0;
  }
  return (((float)direct * pow(10, -R) - b) / m);  // X = 1/m * (Y * 10^-R - b)
}

static int
read_hsc_value(uint8_t reg, float m, int b, int R, float *value) {
  uint16_t data;

  if (read_hsc_reg(reg, (uint8_t *)&data, 2)) {
    return -1;
  }

  *value = direct2real(data, m, b, R);
  return 0;
}

static int
read_hsc_ein(float r_sense, float *value) {
  int ret;
  uint8_t rbuf[12] = {0};
  uint32_t energy, rollover, sample;
  uint32_t pre_energy, pre_rollover, pre_sample;
  uint32_t sample_diff;
  double energy_diff;
  static uint32_t last_energy, last_rollover, last_sample;
  static uint8_t pre_ein = 0;

  // read READ_EIN_EXT
  ret = read_hsc_reg(0xdc, rbuf, 9);
  if (ret || (rbuf[0] != 8)) {  // length = 8 bytes
    return -1;
  }

  pre_energy = last_energy;
  pre_rollover = last_rollover;
  pre_sample = last_sample;

  last_energy = energy = (rbuf[3]<<16) | (rbuf[2]<<8) | rbuf[1];
  last_rollover = rollover = (rbuf[5]<<8) | rbuf[4];
  last_sample = sample = (rbuf[8]<<16) | (rbuf[7]<<8) | rbuf[6];

  if (!pre_ein) {
    pre_ein = 1;
    return -1;
  }

  if ((pre_rollover > rollover) || ((pre_rollover == rollover) && (pre_energy > energy))) {
    rollover += EIN_ROLLOVER_CNT;
  }
  if (pre_sample > sample) {
    sample += EIN_SAMPLE_CNT;
  }

  energy_diff = (double)(rollover-pre_rollover)*EIN_ENERGY_CNT + (double)energy - (double)pre_energy;
  if (energy_diff < 0) {
    return -1;
  }
  sample_diff = sample - pre_sample;
  if (sample_diff == 0) {
    return -1;
  }
  *value = (float)((energy_diff/sample_diff/256) * PIN_COEF/r_sense);

  return 0;
}

static int
read_ina230_value(uint8_t reg, char *device, uint8_t addr, float *value) {
  int dev;
  int ret;
  int32_t res;
  int retry = 4;

  dev = open(device, O_RDWR);
  if (dev < 0) {
    syslog(LOG_ERR, "%s: open() failed", __func__);
    return -1;
  }

  /* Assign the i2c device address */
  ret = ioctl(dev, I2C_SLAVE, addr);
  if (ret < 0) {
    syslog(LOG_ERR, "%s: ioctl() assigning i2c addr failed", __func__);
    close(dev);
    return -1;
  }

  // Get INA230 Calibration
  do {
    res = i2c_smbus_read_word_data(dev, INA230_CALIBRATION);
    if (res < 0) {
      syslog(LOG_ERR, "%s: i2c_smbus_read_word_data failed", __func__);
      close(dev);
      return -1;
    }

    if (0 == res) {
      /* Write the config in the Calibration register */
      ret = i2c_smbus_write_word_data(dev, INA230_CALIBRATION, DC_INA230_DEFAULT_CALIBRATION);
      if (ret < 0) {
        syslog(LOG_ERR, "%s: i2c_smbus_write_word_data failed", __func__);
        close(dev);
        return -1;
      }
      /* Wait for the conversion to finish */
      msleep(50);
      retry--;
    } else {
      break;
    }
  } while(retry);

  res = i2c_smbus_read_word_data(dev, reg);
  if (res < 0) {
    syslog(LOG_ERR, "%s: i2c_smbus_read_word_data failed", __func__);
    close(dev);
    return -1;
  }

  switch (reg) {
    case INA230_VOLT:
      res = ((res & 0x007F) << 8) | ((res & 0xFF00) >> 8);
      *value = ((float) res) / 800;
      break;
    case INA230_POWER:
      res = ((res & 0x00FF) << 8) | ((res & 0xFF00) >> 8);
      *value = ((float) res) / 40;
      break;
  }

  close(dev);

  return 0;
}

static int
read_nic_temp(const char *device, uint8_t addr, float *value) {
  int dev, ret, res;
  uint8_t wbuf[4] = {I2C_NIC_SENSOR_TEMP_REG};

  dev = open(device, O_RDWR);
  if (dev < 0) {
    return -1;
  }

  ret = i2c_rdwr_msg_transfer(dev, addr, wbuf, 1, (uint8_t *)&res, 1);
  close(dev);
  if (ret) {
    return -1;
  }
  *value = (float)(res & 0xFF);
  if (*value > MAX_POS_READING_MARGIN)
    *value -= THERMAL_CONSTANT;

  return 0;
}

static int
rc_dimm_present_check(uint8_t fru, int index, uint8_t sensor_num) {

  int fd;
  char path[64] = {0};
  uint8_t dimm_present_status;
  int dimm_present_data_size = 1;

  sprintf(path, rc_dimm_location_list[index].dimm_location_file, fru);

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    return -1;     //DIMM location file doesn't exist
  }

  if (read(fd, &dimm_present_status, dimm_present_data_size) != dimm_present_data_size) {
    close(fd);
    return -1;
  }

  switch(dimm_present_status) {
    case 0x01:     //DIMM present
      break;
    case 0xFF:     //DIMM not present
    default:
      close(fd);
      return -1;
  }

  close(fd);
  return 0;
}

static int
bic_read_sensor_wrapper(uint8_t fru, uint8_t sensor_num, bool discrete,
    void *value) {

  int ret;
  int i;
  int index;
  sdr_full_t *sdr;
  ipmi_sensor_reading_t *sensor = &g_sread[fru-1][sensor_num];
  ipmi_accuracy_sensor_reading_t acsensor;
  bool is_accuracy_sensor = false;
  uint8_t server_type = 0xFF;
  int slot_type = 3;

  slot_type = fby2_get_slot_type(fru);

  if (slot_type == 0) { //Server
    ret = fby2_get_server_type(fru, &server_type);
    if (ret) {
      syslog(LOG_ERR, "%s, Get server type failed", __func__);
    }

#if defined(CONFIG_FBY2_RC) || defined(CONFIG_FBY2_EP)
    switch(server_type){
      case SERVER_TYPE_EP:
      case SERVER_TYPE_RC:
        break;
      case SERVER_TYPE_TL:
        for (i=0; i < sizeof(bic_sdr_accuracy_sensor_support_list)/sizeof(uint8_t); i++) {
          if (bic_sdr_accuracy_sensor_support_list[i] == sensor_num)
            is_accuracy_sensor = true;
        }
        break;
      default:
        return -1;
    }
#else
    for (i=0; i < sizeof(bic_sdr_accuracy_sensor_support_list)/sizeof(uint8_t); i++) {
      if (bic_sdr_accuracy_sensor_support_list[i] == sensor_num)
        is_accuracy_sensor = true;
    }
#endif
    // accuracy sensor VCCIN_VR_POUT, INA230_POWER and SOC_PACKAGE_PWR
    if (is_accuracy_sensor) {
      ret = bic_read_accuracy_sensor(fru, sensor_num, &acsensor);
      sensor->flags = acsensor.flags;
    } else {
      ret = bic_read_sensor(fru, sensor_num, sensor);
    }

    if (ret)
      return ret;

    msleep(1);  // a little delay to reduce CPU utilization

    if(server_type == SERVER_TYPE_RC) {
      for(index = 0; index < sizeof(rc_dimm_location_list)/sizeof(rc_dimm_location_info); index++) {
        if(sensor_num == rc_dimm_location_list[index].dimm_sensor_num) {
          if(rc_dimm_present_check(fru, index, sensor_num))
            return EER_READ_NA;
        }
      }
    }
  } else if (slot_type == SLOT_TYPE_GPV2) {
    uint8_t dev_id = DEV_NONE;
    switch (sensor_num) {
      case GPV2_SENSOR_INLET_TEMP:
      case GPV2_SENSOR_OUTLET_TEMP:
      case GPV2_SENSOR_P12V_BIC_SCALED:
      case GPV2_SENSOR_P3V3_STBY_BIC_SCALED:
      case GPV2_SENSOR_P0V92_BIC_SCALED:
      case GPV2_SENSOR_P1V8_BIC_SCALED:
      case GPV2_SENSOR_INA230_POWER:
      case GPV2_SENSOR_INA230_VOLT:
      case GPV2_SENSOR_3V3_VR_Vol:
      case GPV2_SENSOR_0V92_VR_Vol:
      case GPV2_SENSOR_3V3_VR_Curr:
      case GPV2_SENSOR_0V92_VR_Curr:
      case GPV2_SENSOR_3V3_VR_Pwr:
      case GPV2_SENSOR_0V92_VR_Pwr:
      case GPV2_SENSOR_3V3_VR_Temp:
      case GPV2_SENSOR_0V92_VR_Temp:
      case GPV2_SENSOR_PCIE_SW_TEMP:
        dev_id = DEV_NONE;
        ret = bic_read_sensor(fru, sensor_num, sensor);
        if (ret)
          return ret;
        break;
      case GPV2_SENSOR_M2A_INA231_PW:
        dev_id = 1;
        break;
      case GPV2_SENSOR_M2B_INA231_PW:
        dev_id = 2;
        break;
      case GPV2_SENSOR_M2C_INA231_PW:
        dev_id = 3;
        break;
      case GPV2_SENSOR_M2D_INA231_PW:
        dev_id = 4;
        break;
      case GPV2_SENSOR_M2E_INA231_PW:
        dev_id = 5;
        break;
      case GPV2_SENSOR_M2F_INA231_PW:
        dev_id = 6;
        break;
      case GPV2_SENSOR_M2G_INA231_PW:
        dev_id = 7;
        break;
      case GPV2_SENSOR_M2H_INA231_PW:
        dev_id = 8;
        break;
      case GPV2_SENSOR_M2I_INA231_PW:
        dev_id = 9;
        break;
      case GPV2_SENSOR_M2J_INA231_PW:
        dev_id = 10;
        break;
      case GPV2_SENSOR_M2K_INA231_PW:
        dev_id = 11;
        break;
      case GPV2_SENSOR_M2L_INA231_PW:
        dev_id = 12;
        break;
      default:
        dev_id = DEV_NONE;
        break;
    }

    if (dev_id != DEV_NONE) {
      uint8_t rbuf[256] = {0x00};
      uint8_t rlen = 0;
      uint8_t dev_sensor_num = sensor_num;
      int num = 0;
      ipmi_device_sensor_t sdata = {0};
      ipmi_device_sensor_reading_t *dev_sensor = (ipmi_device_sensor_reading_t *) rbuf;
      ret = bic_read_device_sensors(fru, dev_id, dev_sensor,&rlen);

      if (ret) {
        return ret;
      } else if (rlen == 0) {
        return -1;
      }

      msleep(1);  // a little delay to reduce CPU utilization

      num = rlen/5;
      if (num > MAX_NUM_DEV_SENSORS)
        num = MAX_NUM_DEV_SENSORS;

      for (i=0;i<num;i++) {
        sdata = dev_sensor->data[i];
        dev_sensor_num = sdata.sensor_num;
        g_sread[fru-1][dev_sensor_num].value = sdata.value;
        g_sread[fru-1][dev_sensor_num].flags = sdata.flags;
        g_sread[fru-1][dev_sensor_num].status = sdata.status;
        g_sread[fru-1][dev_sensor_num].ext_status = sdata.ext_status;
      }
    }
  }

  if (sensor->flags & BIC_SENSOR_READ_NA) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_read_sensor_wrapper: Reading Not Available");
    syslog(LOG_ERR, "bic_read_sensor_wrapper: sensor_num: 0x%X, flag: 0x%X",
        sensor_num, sensor.flags);
#endif
    return EER_READ_NA;
  }

  if (discrete) {
    *(float *) value = (float) sensor->status;
    return 0;
  }

  sdr = &g_sinfo[fru-1][sensor_num].sdr;

  // If the SDR is not type1, no need for conversion
  if (sdr->type !=1) {
    *(float *) value = sensor->value;
    return 0;
  }

  if (is_accuracy_sensor) {
    *(float *) value = ((float)(acsensor.int_value*100 + acsensor.dec_value))/100;
    return 0;
  }

  // y = (mx + b * 10^b_exp) * 10^r_exp
  uint8_t x;
  uint8_t m_lsb, m_msb;
  uint16_t m = 0;
  uint8_t b_lsb, b_msb;
  uint16_t b = 0;
  int8_t b_exp, r_exp;

  x = sensor->value;

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

  //printf("m:%d, x:%d, b:%d, b_exp:%d, r_exp:%d\n", m, x, b, b_exp, r_exp);

  * (float *) value = ((m * x) + (b * pow(10, b_exp))) * (pow(10, r_exp));

  if ((sensor_num == BIC_SENSOR_SOC_THERM_MARGIN) && (* (float *) value > 0)) {
   * (float *) value -= (float) THERMAL_CONSTANT;
  }

  if (*(float *) value > MAX_POS_READING_MARGIN) {     //Negative reading handle
    if (slot_type == SLOT_TYPE_SERVER) { //Server
      if (server_type == SERVER_TYPE_TL) {
        for(i=0;i<sizeof(bic_neg_reading_sensor_support_list)/sizeof(uint8_t);i++) {
          if (sensor_num == bic_neg_reading_sensor_support_list[i]) {
            * (float *) value -= (float) THERMAL_CONSTANT;
          }
        }
      }
    }
#ifdef CONFIG_FBY2_GPV2
    else if (slot_type == SLOT_TYPE_GPV2) {
      for(i=0;i<sizeof(bic_gpv2_neg_reading_sensor_support_list)/sizeof(uint8_t);i++) {
        if (sensor_num == bic_gpv2_neg_reading_sensor_support_list[i]) {
          * (float *) value -= (float) THERMAL_CONSTANT;
        }
      }
    }
#endif
  }

  return 0;
}

int
fby2_sensor_sdr_path(uint8_t fru, char *path) {

char fru_name[16] = {0};

switch(fru) {
  case FRU_SLOT1:
    sprintf(fru_name, "%s", "slot1");
    break;
  case FRU_SLOT2:
    sprintf(fru_name, "%s", "slot2");
    break;
  case FRU_SLOT3:
    sprintf(fru_name, "%s", "slot3");
    break;
  case FRU_SLOT4:
    sprintf(fru_name, "%s", "slot4");
    break;
  case FRU_SPB:
    sprintf(fru_name, "%s", "spb");
    break;
  case FRU_NIC:
    sprintf(fru_name, "%s", "nic");
    break;
  default:
#ifdef DEBUG
    syslog(LOG_WARNING, "fby2_sensor_sdr_path: Wrong Slot ID\n");
#endif
    return -1;
}

sprintf(path, FBY2_SDR_PATH, fru_name);

if (access(path, F_OK) == -1) {
  return -1;
}

return 0;
}

/* Populates all sensor_info_t struct using the path to SDR dump */
static int
_sdr_init(char *path, sensor_info_t *sinfo) {
  int fd;
  int ret = 0;
  uint8_t buf[MAX_SDR_LEN] = {0};
  uint8_t bytes_rd = 0;
  uint8_t snr_num = 0;
  sdr_full_t *sdr;

  while (access(path, F_OK) == -1) {
    sleep(5);
  }

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "%s: open failed for %s\n", __func__, path);
    return -1;
  }

  ret = flock_retry(fd);
  if (ret == -1) {
   syslog(LOG_WARNING, "%s: failed to flock on %s", __func__, path);
   close(fd);
   return -1;
  }

  while ((bytes_rd = read(fd, buf, sizeof(sdr_full_t))) > 0) {
    if (bytes_rd != sizeof(sdr_full_t)) {
      syslog(LOG_ERR, "%s: read returns %d bytes\n", __func__, bytes_rd);
      unflock_retry(fd);
      close(fd);
      return -1;
    }

    sdr = (sdr_full_t *) buf;
    snr_num = sdr->sensor_num;
    sinfo[snr_num].valid = true;
    memcpy(&sinfo[snr_num].sdr, sdr, sizeof(sdr_full_t));
  }

  ret = unflock_retry(fd);
  if (ret == -1) {
    syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

int
fby2_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  char path[64] = {0};
  int retry = 0;

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      switch(fby2_get_slot_type(fru))
      {
        case SLOT_TYPE_SERVER:
        case SLOT_TYPE_GPV2:
              if (fby2_sensor_sdr_path(fru, path) < 0) {
#ifdef DEBUG
                syslog(LOG_WARNING, "fby2_sensor_sdr_init: get_fru_sdr_path failed\n");
#endif
                return ERR_NOT_READY;
            }
            while (retry <= 3) {
              if (_sdr_init(path, sinfo) < 0) {
                 if (retry == 3) { //if the third retry still failed, return -1
#ifdef DEBUG
                   syslog(LOG_ERR, "fby2_sensor_sdr_init: sdr_init failed for FRU %d", fru);
#endif
                   return -1;
                 }
                 retry++;
                 sleep(1);
              } else {
                break;
              }
            }
        break;
        case SLOT_TYPE_CF:
        case SLOT_TYPE_GP:
        case SLOT_TYPE_NULL:
            return -1;
        break;
      }
      break;

    case FRU_SPB:
    case FRU_NIC:
      return -1;
      break;
  }

  return 0;
}

static int
fby2_sdr_init(uint8_t fru) {

  static bool init_done[MAX_NUM_FRUS] = {false};

  if (!init_done[fru - 1]) {

    sensor_info_t *sinfo = g_sinfo[fru-1];

    if (fby2_sensor_sdr_init(fru, sinfo) < 0)
      return ERR_NOT_READY;

    init_done[fru - 1] = true;
  }

  return 0;
}

static bool
is_server_prsnt(uint8_t fru) {
  uint8_t gpio_prim, gpio_ext;
  int val, val_prim, val_ext;
  char path[64] = {0};

  switch(fru) {
  case 1:
    gpio_prim = GPIO_SLOT1_PRSNT_N;
    gpio_ext = GPIO_SLOT1_PRSNT_B_N;
    break;
  case 2:
    gpio_prim = GPIO_SLOT2_PRSNT_N;
    gpio_ext = GPIO_SLOT2_PRSNT_B_N;
    break;
  case 3:
    gpio_prim = GPIO_SLOT3_PRSNT_N;
    gpio_ext = GPIO_SLOT3_PRSNT_B_N;
    break;
  case 4:
    gpio_prim = GPIO_SLOT4_PRSNT_N;
    gpio_ext = GPIO_SLOT4_PRSNT_B_N;
    break;
  default:
    return 0;
  }

  sprintf(path, GPIO_VAL, gpio_prim);
  if (read_device(path, &val_prim)) {
    return -1;
  }

  sprintf(path, GPIO_VAL, gpio_ext);
  if (read_device(path, &val_ext)) {
    return -1;
  }

  val = (val_prim || val_ext);

  if (val == 0x0) {
    return 1;
  } else {
    return 0;
  }
}

/*
 * Get SLOT type
 * PAL_TYPE = 0(TwinLake), 1(Crace Flat), 2(Glacier Point), 3(Empty Slot), 4(Glacier Point V2)
 */
int
fby2_get_slot_type(uint8_t fru) {
  int type = 3;   //set default to 3(Empty Slot)
  int retry = 3;
  char key[MAX_KEY_LEN] = {0};

  if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4))
    return type;

  sprintf(key, SLOT_FILE, fru);
  do {
    if (read_device(key, &type) == 0)
      break;
    syslog(LOG_WARNING,"fby2_get_slot_type failed");
    retry++;
    msleep(10);
  } while (--retry);

  return type;
}

/* Get the units for the sensor */
int
fby2_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  uint8_t op, modifier;
  sensor_info_t *sinfo;

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      switch(fby2_get_slot_type(fru))
      {
         case SLOT_TYPE_SERVER:
         case SLOT_TYPE_GPV2:
           if (is_server_prsnt(fru) && (fby2_sdr_init(fru) != 0)) {
              return -1;
           }
           sprintf(units, "");
           break;
         case SLOT_TYPE_CF:
           switch(sensor_num) {
             case DC_CF_SENSOR_OUTLET_TEMP:
             case DC_CF_SENSOR_INLET_TEMP:
               sprintf(units, "C");
               break;
             case DC_CF_SENSOR_INA230_VOLT:
               sprintf(units, "Volts");
               break;
             case DC_CF_SENSOR_INA230_POWER:
               sprintf(units, "Watts");
               break;
           }
           break;
         case SLOT_TYPE_GP:
           switch(sensor_num) {
             case DC_SENSOR_OUTLET_TEMP:
             case DC_SENSOR_INLET_TEMP:
             case DC_SENSOR_NVMe1_CTEMP:
             case DC_SENSOR_NVMe2_CTEMP:
             case DC_SENSOR_NVMe3_CTEMP:
             case DC_SENSOR_NVMe4_CTEMP:
             case DC_SENSOR_NVMe5_CTEMP:
             case DC_SENSOR_NVMe6_CTEMP:
               sprintf(units, "C");
               break;
             case DC_SENSOR_INA230_VOLT:
               sprintf(units, "Volts");
               break;
             case DC_SENSOR_INA230_POWER:
               sprintf(units, "Watts");
               break;
           }
           break;
      }
      break;
    case FRU_SPB:
      switch(sensor_num) {
        case SP_SENSOR_INLET_TEMP:
          sprintf(units, "C");
          break;
        case SP_SENSOR_OUTLET_TEMP:
          sprintf(units, "C");
          break;
        case SP_SENSOR_MEZZ_TEMP:
          sprintf(units, "C");
          break;
        case SP_SENSOR_FAN0_TACH:
          sprintf(units, "RPM");
          break;
        case SP_SENSOR_FAN1_TACH:
          sprintf(units, "RPM");
          break;
        case SP_SENSOR_AIR_FLOW:
          sprintf(units, "");
          break;
        case SP_SENSOR_P5V:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_P12V:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_P3V3_STBY:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_P12V_SLOT1:
        case SP_SENSOR_P12V_SLOT2:
        case SP_SENSOR_P12V_SLOT3:
        case SP_SENSOR_P12V_SLOT4:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_P3V3:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_P1V15_BMC_STBY:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_P1V2_BMC_STBY:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_P2V5_BMC_STBY:
          sprintf(units, "Volts");
          break;
       case SP_P1V8_STBY:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_HSC_IN_VOLT:
          sprintf(units, "Volts");
          break;
        case SP_SENSOR_HSC_OUT_CURR:
          sprintf(units, "Amps");
          break;
        case SP_SENSOR_HSC_TEMP:
          sprintf(units, "C");
          break;
        case SP_SENSOR_HSC_IN_POWER:
          sprintf(units, "Watts");
          break;
        case SP_SENSOR_HSC_PEAK_IOUT:
          sprintf(units, "Amps");
          break;
        case SP_SENSOR_HSC_PEAK_PIN:
          sprintf(units, "Watts");
          break;
      }
      break;
    case FRU_NIC:
      switch(sensor_num) {
        case MEZZ_SENSOR_TEMP:
          sprintf(units, "C");
          break;
      }
      break;
  }
  return 0;
}

int
fby2_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, float *value) {

  sensor_thresh_array_init();

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      switch(fby2_get_slot_type(fru))
      {
        case SLOT_TYPE_SERVER:
        case SLOT_TYPE_GPV2:
           break;
        case SLOT_TYPE_CF:
           *value = dc_cf_sensor_threshold[sensor_num][thresh];
           break;
        case SLOT_TYPE_GP:
           *value = dc_sensor_threshold[sensor_num][thresh];
           break;
        case SLOT_TYPE_NULL:
           break;
      }
      break;
    case FRU_SPB:
      *value = spb_sensor_threshold[sensor_num][thresh];
      break;
    case FRU_NIC:
      *value = nic_sensor_threshold[sensor_num][thresh];
      break;
  }
  return 0;
}

/* Get the name for the sensor */
int
fby2_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {

  switch(fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      switch(fby2_get_slot_type(fru))
      {
        case SLOT_TYPE_SERVER:
          switch(sensor_num) {
            case BIC_SENSOR_SYSTEM_STATUS:
              sprintf(name, "SYSTEM_STATUS");
              break;
            case BIC_SENSOR_SYS_BOOT_STAT:
              sprintf(name, "SYS_BOOT_STAT");
              break;
            case BIC_SENSOR_CPU_DIMM_HOT:
              sprintf(name, "CPU_DIMM_HOT");
              break;
            case BIC_SENSOR_PROC_FAIL:
              sprintf(name, "PROC_FAIL");
              break;
            case BIC_SENSOR_VR_HOT:
              sprintf(name, "VR_HOT");
              break;
#ifdef CONFIG_FBY2_EP
            case NBU_ERROR:
              sprintf(name, "NBU_ERROR");
              break;
#endif
            default:
              sprintf(name, "");
              return -1;
          }
          break;
        case SLOT_TYPE_GPV2:
          sprintf(name, "");
          break;
        case SLOT_TYPE_CF:
          switch(sensor_num) {
            case DC_CF_SENSOR_OUTLET_TEMP:
              sprintf(name, "DC_CF_OUTLET_TEMP");
              break;
            case DC_CF_SENSOR_INLET_TEMP:
              sprintf(name, "DC_CF_INLET_TEMP");
              break;
            case DC_CF_SENSOR_INA230_VOLT:
              sprintf(name, "DC_CF_INA230_VOLT");
              break;
            case DC_CF_SENSOR_INA230_POWER:
              sprintf(name, "DC_CF_INA230_POWER");
              break;
            default:
              sprintf(name, "");
              break;
          }
          break;
        case SLOT_TYPE_GP:
          switch(sensor_num) {
            case DC_SENSOR_OUTLET_TEMP:
              sprintf(name, "DC_OUTLET_TEMP");
              break;
            case DC_SENSOR_INLET_TEMP:
              sprintf(name, "DC_INLET_TEMP");
              break;
            case DC_SENSOR_INA230_VOLT:
              sprintf(name, "DC_INA230_VOLT");
              break;
            case DC_SENSOR_INA230_POWER:
              sprintf(name, "DC_INA230_POWER");
              break;
            case DC_SENSOR_NVMe1_CTEMP:
              sprintf(name, "DC_NVMe1_CTEMP");
              break;
            case DC_SENSOR_NVMe2_CTEMP:
              sprintf(name, "DC_NVMe2_CTEMP");
              break;
            case DC_SENSOR_NVMe3_CTEMP:
              sprintf(name, "DC_NVMe3_CTEMP");
              break;
            case DC_SENSOR_NVMe4_CTEMP:
              sprintf(name, "DC_NVMe4_CTEMP");
              break;
            case DC_SENSOR_NVMe5_CTEMP:
              sprintf(name, "DC_NVMe5_CTEMP");
              break;
            case DC_SENSOR_NVMe6_CTEMP:
              sprintf(name, "DC_NVMe6_CTEMP");
              break;
            default:
              sprintf(name, "");
              break;
          }
          break;
      }
      break;
    case FRU_SPB:
      switch(sensor_num) {
        case SP_SENSOR_INLET_TEMP:
          sprintf(name, "SP_INLET_TEMP");
          break;
        case SP_SENSOR_OUTLET_TEMP:
          sprintf(name, "SP_OUTLET_TEMP");
          break;
        case SP_SENSOR_MEZZ_TEMP:
          sprintf(name, "SP_MEZZ_TEMP");
          break;
        case SP_SENSOR_FAN0_TACH:
          sprintf(name, "SP_FAN0_TACH");
          break;
        case SP_SENSOR_FAN1_TACH:
          sprintf(name, "SP_FAN1_TACH");
          break;
        case SP_SENSOR_AIR_FLOW:
          sprintf(name, "SP_AIR_FLOW");
          break;
        case SP_SENSOR_P5V:
          sprintf(name, "SP_P5V");
          break;
        case SP_SENSOR_P12V:
          sprintf(name, "SP_P12V");
          break;
        case SP_SENSOR_P3V3_STBY:
          sprintf(name, "SP_P3V3_STBY");
          break;
        case SP_SENSOR_P12V_SLOT1:
          sprintf(name, "SP_P12V_SLOT1");
          break;
        case SP_SENSOR_P12V_SLOT2:
          sprintf(name, "SP_P12V_SLOT2");
          break;
        case SP_SENSOR_P12V_SLOT3:
          sprintf(name, "SP_P12V_SLOT3");
          break;
        case SP_SENSOR_P12V_SLOT4:
          sprintf(name, "SP_P12V_SLOT4");
          break;
        case SP_SENSOR_P3V3:
          sprintf(name, "SP_P3V3");
          break;
        case SP_SENSOR_P1V15_BMC_STBY:
          sprintf(name, "SP_SENSOR_P1V15_BMC_STBY");
          break;
        case SP_SENSOR_P1V2_BMC_STBY:
          sprintf(name, "SP_SENSOR_P1V2_BMC_STBY");
          break;
        case SP_SENSOR_P2V5_BMC_STBY:
          sprintf(name, "SP_SENSOR_P2V5_BMC_STBY");
          break;
        case SP_P1V8_STBY:
          sprintf(name, "SP_P1V8_STBY");
          break;
        case SP_SENSOR_HSC_IN_VOLT:
          sprintf(name, "SP_HSC_IN_VOLT");
          break;
        case SP_SENSOR_HSC_OUT_CURR:
          sprintf(name, "SP_HSC_OUT_CURR");
          break;
        case SP_SENSOR_HSC_TEMP:
          sprintf(name, "SP_HSC_TEMP");
          break;
        case SP_SENSOR_HSC_IN_POWER:
          sprintf(name, "SP_HSC_IN_POWER");
          break;
        case SP_SENSOR_HSC_PEAK_IOUT:
          sprintf(name, "SP_HSC_PEAK_IOUT");
          break;
        case SP_SENSOR_HSC_PEAK_PIN:
          sprintf(name, "SP_HSC_PEAK_PIN");
          break;
      }
      break;
    case FRU_NIC:
      switch(sensor_num) {
        case MEZZ_SENSOR_TEMP:
          sprintf(name, "MEZZ_SENSOR_TEMP");
          break;
      }
      break;
  }
  return 0;
}

int
fby2_disable_gp_m2_monior(uint8_t slot_id, uint8_t dis) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN];

  sprintf(key, "slot%u_dis_gp_m2_mon", slot_id);
  sprintf(value, "%u", dis);
  if (kv_set(key, value, 0, 0) < 0) {
    return -1;
  }

  return 0;
}

static bool
check_gp_m2_monior(uint8_t slot_id) {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};

  sprintf(key, "slot%u_dis_gp_m2_mon", slot_id);
  if (kv_get(key, value, NULL, 0)) {
    return true;
  }

  if (strtoul(value, NULL, 10) == 0)
    return true;

  return false;
}

int
fby2_sensor_read(uint8_t fru, uint8_t sensor_num, void *value) {

  float volt;
  float curr;
  int ret;
  bool discrete;
  int i;
  char path[LARGEST_DEVICE_NAME];
  uint8_t status;
  uint8_t server_type = 0xFF;

  switch (fru) {
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:

      switch(fby2_get_slot_type(fru))
      {
        case SLOT_TYPE_SERVER:
          if (!(is_server_prsnt(fru))) {
            return -1;
          }

          ret = fby2_sdr_init(fru);
          if (ret < 0) {
            return ret;
          }

          discrete = false;

          i = 0;
#if defined(CONFIG_FBY2_RC)
          ret = fby2_get_server_type(fru, &server_type);
          if (ret) {
            syslog(LOG_ERR, "%s, Get server type failed", __func__);
          }
          switch (server_type) {
            case SERVER_TYPE_RC:
              while (i < bic_rc_discrete_cnt) {
                if (sensor_num == bic_rc_discrete_list[i++]) {
                  discrete = true;
                  break;
                }
              }
              break;
            case SERVER_TYPE_TL:
              while (i < bic_discrete_cnt) {
                if (sensor_num == bic_discrete_list[i++]) {
                  discrete = true;
                  break;
                }
              }
              break;
            default:
              syslog(LOG_ERR, "%s, Undefined server type", __func__);
              return -1;
          }
#else
          while (i < bic_discrete_cnt) {
            if (sensor_num == bic_discrete_list[i++]) {
              discrete = true;
              break;
            }
          }
#endif
          return bic_read_sensor_wrapper(fru, sensor_num, discrete, value);
#ifdef CONFIG_FBY2_GPV2
        case SLOT_TYPE_GPV2:
          if (!(is_server_prsnt(fru))) {
            return -1;
          }

          ret = fby2_sdr_init(fru);
          if (ret < 0) {
            return ret;
          }

          //Glacier Point V2
          /* Check whether the system is 12V off or on */
          if (!bic_is_slot_12v_on(fru))
            return EER_READ_NA;

          return bic_read_sensor_wrapper(fru, sensor_num, false, value);
#endif
        case SLOT_TYPE_CF:
          //Crane Flat
          /* Check whether the system is 12V off or on */
          if (!bic_is_slot_12v_on(fru))
            return EER_READ_NA;

          switch(sensor_num) {
            case DC_CF_SENSOR_OUTLET_TEMP:
              if(fru == FRU_SLOT1)
                return read_temp(DC_SLOT1_OUTLET_TEMP_DEVICE, (float*) value);
              else
                return read_temp(DC_SLOT3_OUTLET_TEMP_DEVICE, (float*) value);
            case DC_CF_SENSOR_INLET_TEMP:
              if(fru == FRU_SLOT1)
                return read_temp(DC_SLOT1_INLET_TEMP_DEVICE, (float*) value);
              else
                return read_temp(DC_SLOT3_INLET_TEMP_DEVICE, (float*) value);
            case DC_CF_SENSOR_INA230_VOLT:
              if (fru == FRU_SLOT1)
                snprintf(path, LARGEST_DEVICE_NAME, "%s", I2C_DEV_DC_1);
              else
                snprintf(path, LARGEST_DEVICE_NAME, "%s", I2C_DEV_DC_3);
              return read_ina230_value(INA230_VOLT, path, I2C_DC_INA_ADDR, (float*) value);
            case DC_CF_SENSOR_INA230_POWER:
              if (fru == FRU_SLOT1)
                snprintf(path, LARGEST_DEVICE_NAME, "%s", I2C_DEV_DC_1);
              else
                snprintf(path, LARGEST_DEVICE_NAME, "%s", I2C_DEV_DC_3);
              return read_ina230_value(INA230_POWER, path, I2C_DC_INA_ADDR, (float*) value);
            default:
              return -1;
          }
          break;
        case SLOT_TYPE_GP:
          //Glacier Point
          /* Check whether the system is 12V off or on */
          if (!bic_is_slot_12v_on(fru))
            return EER_READ_NA;

          switch(sensor_num) {
            case DC_SENSOR_OUTLET_TEMP:
              if(fru == FRU_SLOT1)
                return read_temp(DC_SLOT1_OUTLET_TEMP_DEVICE, (float*) value);
              else
                return read_temp(DC_SLOT3_OUTLET_TEMP_DEVICE, (float*) value);
            case DC_SENSOR_INLET_TEMP:
              if(fru == FRU_SLOT1)
                return read_temp(DC_SLOT1_INLET_TEMP_DEVICE, (float*) value);
              else
                return read_temp(DC_SLOT3_INLET_TEMP_DEVICE, (float*) value);
            case DC_SENSOR_INA230_VOLT:
              if (fru == FRU_SLOT1)
                snprintf(path, LARGEST_DEVICE_NAME, "%s", I2C_DEV_DC_1);
              else
                snprintf(path, LARGEST_DEVICE_NAME, "%s", I2C_DEV_DC_3);
              return read_ina230_value(INA230_VOLT, path, I2C_DC_INA_ADDR, (float*) value);
            case DC_SENSOR_INA230_POWER:
              if (fru == FRU_SLOT1)
                snprintf(path, LARGEST_DEVICE_NAME, "%s", I2C_DEV_DC_1);
              else
                snprintf(path, LARGEST_DEVICE_NAME, "%s", I2C_DEV_DC_3);
              return read_ina230_value(INA230_POWER, path, I2C_DC_INA_ADDR, (float*) value);
            case DC_SENSOR_NVMe1_CTEMP:
            case DC_SENSOR_NVMe2_CTEMP:
            case DC_SENSOR_NVMe3_CTEMP:
            case DC_SENSOR_NVMe4_CTEMP:
            case DC_SENSOR_NVMe5_CTEMP:
            case DC_SENSOR_NVMe6_CTEMP:
              if (!bic_is_slot_power_en(fru))
                return EER_READ_NA;
              if (check_gp_m2_monior(fru) == false)
                return EER_READ_NA;

              if (fru == FRU_SLOT1)
                snprintf(path, LARGEST_DEVICE_NAME, "%s", I2C_DEV_DC_1);
              else
                snprintf(path, LARGEST_DEVICE_NAME, "%s", I2C_DEV_DC_3);
              return read_m2_temp_on_gp(path, sensor_num, (float*) value);
            default:
              return -1;
        }
        case SLOT_TYPE_NULL:
          // do nothing
          break;
      }
      break;
    case FRU_SPB:
      switch(sensor_num) {

        // Inlet, Outlet Temp
        case SP_SENSOR_INLET_TEMP:
          return read_temp(SP_INLET_TEMP_DEVICE, (float*) value);
        case SP_SENSOR_OUTLET_TEMP:
          return read_temp(SP_OUTLET_TEMP_DEVICE, (float*) value);

        // Fan Tach Values
        case SP_SENSOR_FAN0_TACH:
          return read_fan_value(FAN0, FAN_TACH_RPM, (float*) value);
        case SP_SENSOR_FAN1_TACH:
          return read_fan_value(FAN1, FAN_TACH_RPM, (float*) value);

        // Various Voltages
        case SP_SENSOR_P5V:
          return read_adc_value(ADC_PIN0, ADC_VALUE, (float*) value);
        case SP_SENSOR_P12V:
          return read_adc_value(ADC_PIN1, ADC_VALUE, (float*) value);
        case SP_SENSOR_P3V3_STBY:
          return read_adc_value(ADC_PIN2, ADC_VALUE, (float*) value);
        case SP_SENSOR_P12V_SLOT1:
          return read_adc_value(ADC_PIN3, ADC_VALUE, (float*) value);
        case SP_SENSOR_P12V_SLOT2:
          return read_adc_value(ADC_PIN4, ADC_VALUE, (float*) value);
        case SP_SENSOR_P12V_SLOT3:
          return read_adc_value(ADC_PIN5, ADC_VALUE, (float*) value);
        case SP_SENSOR_P12V_SLOT4:
          return read_adc_value(ADC_PIN6, ADC_VALUE, (float*) value);
        case SP_SENSOR_P3V3:
          return read_adc_value(ADC_PIN7, ADC_VALUE, (float*) value);
        case SP_SENSOR_P1V15_BMC_STBY:
          return read_adc_value(ADC_PIN8, ADC_VALUE, (float*) value);
        case SP_SENSOR_P1V2_BMC_STBY:
          return read_adc_value(ADC_PIN9, ADC_VALUE, (float*) value);
        case SP_SENSOR_P2V5_BMC_STBY:
          return read_adc_value(ADC_PIN10, ADC_VALUE, (float*) value);
        case SP_P1V8_STBY:
          return read_adc_value(ADC_PIN11, ADC_VALUE, (float*) value);

        // Hot Swap Controller
        case SP_SENSOR_HSC_IN_VOLT:
          return read_hsc_value(0x88, 19599, 0, -2, (float *)value);
        case SP_SENSOR_HSC_OUT_CURR:
          return read_hsc_value(0x8c, (800*hsc_r_sense), 20475, -1, (float *)value);
        case SP_SENSOR_HSC_TEMP:
          return read_hsc_value(0x8d, 42, 31880, -1, (float *)value);
        case SP_SENSOR_HSC_IN_POWER:
          return read_hsc_ein(hsc_r_sense, (float *)value);
        case SP_SENSOR_HSC_PEAK_IOUT:
          return read_hsc_value(0xd0, (800*hsc_r_sense), 20475, -1, (float *)value);
        case SP_SENSOR_HSC_PEAK_PIN:
          return read_hsc_value(0xda, (6123*hsc_r_sense), 0, -2, (float *)value);
      }
      break;

    case FRU_NIC:
      switch(sensor_num) {
        // Mezz Temp
        case MEZZ_SENSOR_TEMP:
          return read_nic_temp(I2C_DEV_NIC, I2C_NIC_ADDR, (float*) value);
      }
      break;
  }
}

static int
check_hsc_status(uint8_t reg, uint8_t len, uint16_t mask) {
  uint8_t rbuf[4] = {0};
  uint16_t sts;

  if (read_hsc_reg(reg, rbuf, len)) {
    return -1;
  }

  sts = (len == 1) ? rbuf[0] : (rbuf[1] << 8)|rbuf[0];
  if ((sts & mask) != 0) {
    return 1;
  }

  return 0;
}

static int
clear_hsc_fault(void) {
  return read_hsc_reg(0x03, NULL, 0);  // CLEAR_FAULTS
}

int
fby2_check_hsc_sts_iout(uint8_t mask) {
  return check_hsc_status(0x7b, 1, mask);  // STATUS_IOUT
}

int
fby2_check_hsc_fault(void) {
  if (check_hsc_status(0x79, 2, 0xBFFE))  // STATUS_WORD
    return -1;
  if (check_hsc_status(0x7a, 1, 0xFF))    // STATUS_VOUT
    return -1;
  if (check_hsc_status(0x7b, 1, 0xDF))    // STATUS_IOUT
    return -1;
  if (check_hsc_status(0x7c, 1, 0xFF))    // STATUS_INPUT
    return -1;
  if (check_hsc_status(0x7d, 1, 0xFF))    // STATUS_TEMPERATURE
    return -1;
  if (check_hsc_status(0x80, 1, 0xFF))    // STATUS_MFR_SPECIFIC
    return -1;

  clear_hsc_fault();
  return 0;
}
