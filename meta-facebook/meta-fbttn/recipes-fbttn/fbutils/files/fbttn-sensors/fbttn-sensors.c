/*
 * fbttn-sensors
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <pthread.h>
#include <facebook/bic.h>
#include <openbmc/ipmi.h>
#include <facebook/fbttn_sensor.h>

int
main(int argc, char **argv) {
  int value;
  float fvalue;
  uint8_t slot_id;

  slot_id = atoi(argv[1]);

  // IOM Type VII sensors
  if (fbttn_sensor_read(slot_id, IOM_SENSOR_MEZZ_TEMP, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_MEZZ_TEMP\n");
  } else {
    printf("IOM_SENSOR_MEZZ_TEMP: %.2f C\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, IOM_SENSOR_HSC_POWER, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_HSC_POWER\n");
  } else {
    printf("IOM_SENSOR_HSC_POWER: %.2f Watts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, IOM_SENSOR_HSC_VOLT, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_HSC_VOLT\n");
  } else {
    printf("IOM_SENSOR_HSC_VOLT: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, IOM_SENSOR_HSC_CURR, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_HSC_CURR\n");
  } else {
    printf("IOM_SENSOR_HSC_CURR: %.2f Amps\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, IOM_SENSOR_ADC_12V, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_ADC_12V\n");
  } else {
    printf("IOM_SENSOR_ADC_12V: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, IOM_SENSOR_ADC_P5V_STBY, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_ADC_P5V_STBY\n");
  } else {
    printf("IOM_SENSOR_ADC_P5V_STBY: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, IOM_SENSOR_ADC_P3V3_STBY, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_ADC_P3V3_STBY\n");
  } else {
    printf("IOM_SENSOR_ADC_P3V3_STBY: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, IOM_SENSOR_ADC_P1V8_STBY, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_ADC_P1V8_STBY\n");
  } else {
    printf("IOM_SENSOR_ADC_P1V8_STBY: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, IOM_SENSOR_ADC_P2V5_STBY, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_ADC_P2V5_STBY\n");
  } else {
    printf("IOM_SENSOR_ADC_P2V5_STBY: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, IOM_SENSOR_ADC_P1V2_STBY, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_ADC_P1V2_STBY\n");
  } else {
    printf("IOM_SENSOR_ADC_P1V2_STBY: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, IOM_SENSOR_ADC_P1V15_STBY, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_ADC_P1V15_STBY\n");
  } else {
    printf("IOM_SENSOR_ADC_P1V15_STBY: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, IOM_SENSOR_ADC_P3V3, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_ADC_P3V3\n");
  } else {
    printf("IOM_SENSOR_ADC_P3V3: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, IOM_SENSOR_ADC_P1V8, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_ADC_P1V8\n");
  } else {
    printf("IOM_SENSOR_ADC_P1V8: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, IOM_SENSOR_ADC_P1V5, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_ADC_P1V5\n");
  } else {
    printf("IOM_SENSOR_ADC_P1V5v: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, IOM_SENSOR_ADC_P0V975, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_ADC_P0V975\n");
  } else {
    printf("IOM_SENSOR_ADC_P0V975: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, IOM_SENSOR_ADC_P3V3_M2, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_ADC_P3V3_M2\n");
  } else {
    printf("IOM_SENSOR_ADC_P3V3_M2: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, IOM_SENSOR_M2_AMBIENT_TEMP1, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_M2_AMBIENT_TEMP1\n");
  } else {
    printf("IOM_SENSOR_M2_AMBIENT_TEMP1: %.2f C\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, IOM_SENSOR_M2_AMBIENT_TEMP2, &fvalue)) {
    printf("fbttn_sensor_read failed: IOM_SENSOR_M2_AMBIENT_TEMP2\n");
  } else {
    printf("IOM_SENSOR_M2_AMBIENT_TEMP2: %.2f C\n", fvalue);
  }

  // DPB sensors
  if (fbttn_sensor_read(slot_id, DPB_SENSOR_FAN1_FRONT, &value)) {
    printf("fbttn_sensor_read failed: DPB_SENSOR_FAN1_FRONT\n");
  } else {
    printf("DPB_SENSOR_FAN1_FRONT: %d rpm\n", value);
  }

  if (fbttn_sensor_read(slot_id, DPB_SENSOR_FAN2_FRONT, &value)) {
    printf("fbttn_sensor_read failed: DPB_SENSOR_FAN2_FRONT\n");
  } else {
    printf("DPB_SENSOR_FAN2_FRONT: %d rpm\n", value);
  }

  if (fbttn_sensor_read(slot_id, DPB_SENSOR_FAN3_FRONT, &value)) {
    printf("fbttn_sensor_read failed: DPB_SENSOR_FAN3_FRONT\n");
  } else {
    printf("DPB_SENSOR_FAN3_FRONT: %d rpm\n", value);
  }

  if (fbttn_sensor_read(slot_id, DPB_SENSOR_FAN4_FRONT, &value)) {
    printf("fbttn_sensor_read failed: DPB_SENSOR_FAN4_FRONT\n");
  } else {
    printf("DPB_SENSOR_FAN4_FRONT: %d rpm\n", value);
  }

  if (fbttn_sensor_read(slot_id, DPB_SENSOR_FAN1_REAR, &value)) {
    printf("fbttn_sensor_read failed: DPB_SENSOR_FAN1_REAR\n");
  } else {
    printf("DPB_SENSOR_FAN1_REAR: %d rpm\n", value);
  }

  if (fbttn_sensor_read(slot_id, DPB_SENSOR_FAN2_REAR, &value)) {
    printf("fbttn_sensor_read failed: DPB_SENSOR_FAN2_REAR\n");
  } else {
    printf("DPB_SENSOR_FAN2_REAR: %d rpm\n", value);
  }

  if (fbttn_sensor_read(slot_id, DPB_SENSOR_FAN3_REAR, &value)) {
    printf("fbttn_sensor_read failed: DPB_SENSOR_FAN3_REAR\n");
  } else {
    printf("DPB_SENSOR_FAN3_REAR: %d rpm\n", value);
  }

  if (fbttn_sensor_read(slot_id, DPB_SENSOR_FAN4_REAR, &value)) {
    printf("fbttn_sensor_read failed: DPB_SENSOR_FAN4_REAR\n");
  } else {
    printf("DPB_SENSOR_FAN4_REAR: %d rpm\n", value);
  }

  if (fbttn_sensor_read(slot_id, DPB_SENSOR_HSC_POWER, &fvalue)) {
    printf("fbttn_sensor_read failed: DPB_SENSOR_HSC_POWER\n");
  } else {
    printf("DPB_SENSOR_HSC_POWER: %.2f Watts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, DPB_SENSOR_HSC_VOLT, &fvalue)) {
    printf("fbttn_sensor_read failed: DPB_SENSOR_HSC_VOLT\n");
  } else {
    printf("DPB_SENSOR_HSC_VOLT: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, DPB_SENSOR_HSC_CURR, &fvalue)) {
    printf("fbttn_sensor_read failed: DPB_SENSOR_HSC_CURR\n");
  } else {
    printf("DPB_SENSOR_HSC_CURR: %.2f Amps\n", fvalue);
  }

  // BIC sensors
  if (fbttn_sensor_read(slot_id, BIC_SENSOR_MB_OUTLET_TEMP, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_MB_OUTLET_TEMP\n");
  } else {
  	printf("BIC_SENSOR_MB_OUTLET_TEMP: %.2f C\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_VCCIN_VR_TEMP, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_VCCIN_VR_TEMP\n");
  } else {
  	printf("BIC_SENSOR_VCCIN_VR_TEMP: %.2f C\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_VCC_GBE_VR_TEMP, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_VCC_GBE_VR_TEMP\n");
  } else {
  	printf("BIC_SENSOR_VCC_GBE_VR_TEMP: %.2f C\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_1V05PCH_VR_TEMP, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_1V05PCH_VR_TEMP\n");
  } else {
  	printf("BIC_SENSOR_1V05PCH_VR_TEMP: %.2f C\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_SOC_TEMP, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_SOC_TEMP\n");
  } else {
  	printf("BIC_SENSOR_SOC_TEMP: %.2f C\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_MB_INLET_TEMP, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_MB_INLET_TEMP\n");
  } else {
  	printf("BIC_SENSOR_MB_INLET_TEMP: %.2f C\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_PCH_TEMP, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_PCH_TEMP\n");
  } else {
  	printf("BIC_SENSOR_PCH_TEMP: %.2f C\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_SOC_THERM_MARGIN, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_SOC_THERM_MARGIN\n");
  } else {
  	printf("BIC_SENSOR_SOC_THERM_MARGIN: %.2f C\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_VDDR_VR_TEMP, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_VDDR_VR_TEMP\n");
  } else {
  	printf("BIC_SENSOR_VDDR_VR_TEMP: %.2f C\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_SOC_TJMAX, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_SOC_TJMAX\n");
  } else {
  	printf("BIC_SENSOR_SOC_TJMAX: %.2f C\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_VCC_SCSUS_VR_TEMP, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_VCC_SCSUS_VR_TEMP\n");
  } else {
  	printf("BIC_SENSOR_VCC_SCSUS_VR_TEMP: %.2f C\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_SOC_DIMMA0_TEMP, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_SOC_DIMMA0_TEMP\n");
  } else {
  	printf("BIC_SENSOR_SOC_DIMMA0_TEMP: %.2f C\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_SOC_DIMMA1_TEMP, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_SOC_DIMMA1_TEMP\n");
  } else {
  	printf("BIC_SENSOR_SOC_DIMMA1_TEMP: %.2f C\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_SOC_DIMMB0_TEMP, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_SOC_DIMMB0_TEMP\n");
  } else {
  	printf("BIC_SENSOR_SOC_DIMMB0_TEMP: %.2f C\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_SOC_DIMMB1_TEMP, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_SOC_DIMMB1_TEMP\n");
  } else {
  	printf("BIC_SENSOR_SOC_DIMMB1_TEMP: %.2f C\n", fvalue);
  }

  // Monolake Current Sensors
  if (fbttn_sensor_read(slot_id, BIC_SENSOR_VCC_GBE_VR_CURR, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_VCC_GBE_VR_CURR\n");
  } else {
  	printf("BIC_SENSOR_VCC_GBE_VR_CURR: %.2f Amps\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_1V05_PCH_VR_CURR, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_1V05_PCH_VR_CURR\n");
  } else {
  	printf("BIC_SENSOR_1V05_PCH_VR_CURR: %.2f Amps\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_VCCIN_VR_CURR, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_VCCIN_VR_CURR\n");
  } else {
  	printf("BIC_SENSOR_VCCIN_VR_CURR: %.2f Amps\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_VDDR_VR_CURR, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_VDDR_VR_CURR\n");
  } else {
  	printf("BIC_SENSOR_VDDR_VR_CURR: %.2f Amps\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_VCC_SCSUS_VR_CURR, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_VCC_SCSUS_VR_CURR\n");
  } else {
  	printf("BIC_SENSOR_VCC_SCSUS_VR_CURR: %.2f Amps\n", fvalue);
  }

  // Monolake Voltage Sensors
  if (fbttn_sensor_read(slot_id, BIC_SENSOR_VCCIN_VR_VOL, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_VCCIN_VR_VOL\n");
  } else {
  	printf("BIC_SENSOR_VCCIN_VR_VOL: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_VDDR_VR_VOL, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_VDDR_VR_VOL\n");
  } else {
  	printf("BIC_SENSOR_VDDR_VR_VOL: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_VCC_SCSUS_VR_VOL, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_VCC_SCSUS_VR_VOL\n");
  } else {
  	printf("BIC_SENSOR_VCC_SCSUS_VR_VOL: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_VCC_GBE_VR_VOL, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_VCC_GBE_VR_VOL\n");
  } else {
  	printf("BIC_SENSOR_VCC_GBE_VR_VOL: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_1V05_PCH_VR_VOL, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_1V05_PCH_VR_VOL\n");
  } else {
  	printf("BIC_SENSOR_1V05_PCH_VR_VOL: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_P3V3_MB, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_P3V3_MB\n");
  } else {
  	printf("BIC_SENSOR_P3V3_MB: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_P12V_MB, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_P12V_MB\n");
  } else {
  	printf("BIC_SENSOR_P12V_MB: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_P1V05_PCH, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_P1V05_PCH\n");
  } else {
  	printf("BIC_SENSOR_P1V05_PCH: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_P3V3_STBY_MB, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_P3V3_STBY_MB\n");
  } else {
  	printf("BIC_SENSOR_P3V3_STBY_MB: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_P5V_STBY_MB, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_P5V_STBY_MB\n");
  } else {
  	printf("BIC_SENSOR_P5V_STBY_MB: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_PV_BAT, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_PV_BAT\n");
  } else {
  	printf("BIC_SENSOR_PV_BAT: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_PVDDR, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_PVDDR\n");
  } else {
  	printf("BIC_SENSOR_PVDDR: %.2f Volts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_PVCC_GBE, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_PVCC_GBE\n");
  } else {
  	printf("BIC_SENSOR_PVCC_GBE: %.2f Volts\n", fvalue);
  }

  // Monolake Power Sensors
  if (fbttn_sensor_read(slot_id, BIC_SENSOR_VCCIN_VR_POUT, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_VCCIN_VR_POUT\n");
  } else {
  	printf("BIC_SENSOR_VCCIN_VR_POUT: %.2f Watts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_INA230_POWER, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_INA230_POWER\n");
  } else {
  	printf("BIC_SENSOR_INA230_POWER: %.2f Watts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_SOC_PACKAGE_PWR, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_SOC_PACKAGE_PWR\n");
  } else {
  	printf("BIC_SENSOR_SOC_PACKAGE_PWR: %.2f Watts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_VDDR_VR_POUT, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_VDDR_VR_POUT\n");
  } else {
  	printf("BIC_SENSOR_VDDR_VR_POUT: %.2f Watts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_VCC_SCSUS_VR_POUT, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_VCC_SCSUS_VR_POUT\n");
  } else {
  	printf("BIC_SENSOR_VCC_SCSUS_VR_POUT: %.2f Watts\n", fvalue);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_VCC_GBE_VR_POUT, &fvalue)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_VCC_GBE_VR_POUT\n");
  } else {
  	printf("BIC_SENSOR_VCC_GBE_VR_POUT: %.2f Watts\n", fvalue);
  }

  // Discrete Sensors
  if (fbttn_sensor_read(slot_id, BIC_SENSOR_SYSTEM_STATUS, &value)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_SYSTEM_STATUS\n");
  } else {
  	printf("BIC_SENSOR_SYSTEM_STATUS: 0x%X\n", value);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_PROC_FAIL, &value)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_PROC_FAIL\n");
  } else {
  	printf("BIC_SENSOR_PROC_FAIL: 0x%X\n", value);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_SYS_BOOT_STAT, &value)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_SYS_BOOT_STAT\n");
  } else {
  	printf("BIC_SENSOR_SYS_BOOT_STAT: 0x%X\n", value);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_VR_HOT, &value)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_VR_HOT\n");
  } else {
  	printf("BIC_SENSOR_VR_HOT: 0x%X\n", value);
  }

  if (fbttn_sensor_read(slot_id, BIC_SENSOR_CPU_DIMM_HOT, &value)) {
    printf("fbttn_sensor_read failed: BIC_SENSOR_CPU_DIMM_HOT\n");
  } else {
  	printf("BIC_SENSOR_CPU_DIMM_HOT: 0x%X\n", value);
  }
  
  //SCC Sensors
  if (fbttn_sensor_read(slot_id, SCC_SENSOR_EXPANDER_TEMP, &fvalue)) {
    printf("fbttn_sensor_read failed: SCC_SENSOR_EXPANDER_TEMP\n");
  } else {
  	printf("SCC_SENSOR_EXPANDER_TEMP: %.2f C\n", fvalue);
  }
  
  if (fbttn_sensor_read(slot_id, SCC_SENSOR_IOC_TEMP, &fvalue)) {
    printf("fbttn_sensor_read failed: SCC_SENSOR_IOC_TEMP\n");
  } else {
  	printf("SCC_SENSOR_IOC_TEMP: %.2f C\n", fvalue);
  }
  
  if (fbttn_sensor_read(slot_id, SCC_SENSOR_HSC_POWER, &fvalue)) {
    printf("fbttn_sensor_read failed: SCC_SENSOR_HSC_POWER\n");
  } else {
  	printf("SCC_SENSOR_HSC_POWER: %.2f Watt\n", fvalue);
  }
  
  if (fbttn_sensor_read(slot_id, SCC_SENSOR_HSC_CURR, &fvalue)) {
    printf("fbttn_sensor_read failed: SCC_SENSOR_HSC_CURR\n");
  } else {
  	printf("SCC_SENSOR_HSC_CURR: %.2f Amp\n", fvalue);
  }
  
  if (fbttn_sensor_read(slot_id, SCC_SENSOR_HSC_VOLT, &fvalue)) {
    printf("fbttn_sensor_read failed: SCC_SENSOR_HSC_VOLT\n");
  } else {
  	printf("SCC_SENSOR_HSC_VOLT: %.2f Volt\n", fvalue);
  }
  
  if (fbttn_sensor_read(slot_id, SCC_SENSOR_P3V3_SENSE, &fvalue)) {
    printf("fbttn_sensor_read failed: SCC_SENSOR_P3V3_SENSE\n");
  } else {
  	printf("SCC_SENSOR_P3V3_SENSE: %.2f Volt\n", fvalue);
  }
  
  if (fbttn_sensor_read(slot_id, SCC_SENSOR_P1V8_E_SENSE, &fvalue)) {
    printf("fbttn_sensor_read failed: SCC_SENSOR_P1V8_E_SENSE\n");
  } else {
  	printf("SCC_SENSOR_P1V8_E_SENSE: %.2f Volt\n", fvalue);
  }
  
  if (fbttn_sensor_read(slot_id, SCC_SENSOR_P1V5_E_SENSE, &fvalue)) {
    printf("fbttn_sensor_read failed: SCC_SENSOR_P1V5_E_SENSE\n");
  } else {
  	printf("SCC_SENSOR_P1V5_E_SENSE: %.2f Volt\n", fvalue);
  }
  
  if (fbttn_sensor_read(slot_id, SCC_SENSOR_P0V9_SENSE, &fvalue)) {
    printf("fbttn_sensor_read failed: SCC_SENSOR_P0V9_SENSE\n");
  } else {
  	printf("SCC_SENSOR_P0V9_SENSE: %.2f Volt\n", fvalue);
  }
  
  if (fbttn_sensor_read(slot_id, SCC_SENSOR_P1V8_C_SENSE, &fvalue)) {
    printf("fbttn_sensor_read failed: SCC_SENSOR_P1V8_C_SENSE\n");
  } else {
  	printf("SCC_SENSOR_P1V8_C_SENSE: %.2f Volt\n", fvalue);
  }
  
  if (fbttn_sensor_read(slot_id, SCC_SENSOR_P1V5_C_SENSE, &fvalue)) {
    printf("fbttn_sensor_read failed: SCC_SENSOR_P1V5_C_SENSE\n");
  } else {
  	printf("SCC_SENSOR_P1V5_C_SENSE: %.2f Volt\n", fvalue);
  }
  
  if (fbttn_sensor_read(slot_id, SCC_SENSOR_P0V975_SENSE, &fvalue)) {
    printf("fbttn_sensor_read failed: SCC_SENSOR_P0V975_SENSE\n");
  } else {
  	printf("SCC_SENSOR_P0V975_SENSE: %.2f Volt\n", fvalue);
  }

  return 0;
}
