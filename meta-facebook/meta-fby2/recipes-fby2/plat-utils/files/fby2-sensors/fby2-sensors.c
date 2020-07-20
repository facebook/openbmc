/*
 * fby2-sensors
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
#include <facebook/fby2_sensor.h>
#include <facebook/fby2_common.h>

int
main(int argc, char **argv) {
  int value;
  float fvalue;
  uint8_t slot_id;
  int spb_type;
  int fan_type;

  spb_type = fby2_common_get_spb_type();
  fan_type = fby2_common_get_fan_type();

  slot_id = atoi(argv[1]);

  if (fby2_sensor_read(slot_id, SP_SENSOR_INLET_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_INLET_TEMP\n");
  } else {
  	printf("SP_SENSOR_INLET_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_OUTLET_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_OUTLET_TEMP\n");
  } else {
  	printf("SP_SENSOR_OUTLET_TEMP: %.2f C\n", fvalue);
  }

  if (spb_type == TYPE_SPB_YV250) {
    if (fan_type == TYPE_DUAL_R_FAN) { // YV2.50 Dual R FAN
      if (fby2_sensor_read(slot_id, SP_SENSOR_FAN0_TACH, &value)) {
        printf("fby2_sensor_read failed: SP_SENSOR_FAN0_TACH\n");
      } else {
        printf("SP_SENSOR_FAN0_TACH: %d rpm\n", value);
      }

      if (fby2_sensor_read(slot_id, SP_SENSOR_FAN1_TACH, &value)) {
        printf("fby2_sensor_read failed: SP_SENSOR_FAN1_TACH\n");
      } else {
        printf("SP_SENSOR_FAN1_TACH: %d rpm\n", value);
      }

      if (fby2_sensor_read(slot_id, SP_SENSOR_FAN2_TACH, &value)) {
        printf("fby2_sensor_read failed: SP_SENSOR_FAN2_TACH\n");
      } else {
        printf("SP_SENSOR_FAN2_TACH: %d rpm\n", value);
      }

      if (fby2_sensor_read(slot_id, SP_SENSOR_FAN3_TACH, &value)) {
        printf("fby2_sensor_read failed: SP_SENSOR_FAN3_TACH\n");
      } else {
        printf("SP_SENSOR_FAN3_TACH: %d rpm\n", value);
      }
    } else { // YV2.50 Single R FAN
      // Workaround for FAN consistence
      if (fby2_sensor_read(slot_id, SP_SENSOR_FAN2_TACH, &value)) {
        printf("fby2_sensor_read failed: SP_SENSOR_FAN2_TACH\n");
      } else {
        printf("SP_SENSOR_FAN0_TACH: %d rpm\n", value);
      }

      if (fby2_sensor_read(slot_id, SP_SENSOR_FAN3_TACH, &value)) {
        printf("fby2_sensor_read failed: SP_SENSOR_FAN3_TACH\n");
      } else {
        printf("SP_SENSOR_FAN1_TACH: %d rpm\n", value);
      }
    }
  } else { // YV2
    if (fby2_sensor_read(slot_id, SP_SENSOR_FAN0_TACH, &value)) {
      printf("fby2_sensor_read failed: SP_SENSOR_FAN0_TACH\n");
    } else {
      printf("SP_SENSOR_FAN0_TACH: %d rpm\n", value);
    }

    if (fby2_sensor_read(slot_id, SP_SENSOR_FAN1_TACH, &value)) {
      printf("fby2_sensor_read failed: SP_SENSOR_FAN1_TACH\n");
    } else {
      printf("SP_SENSOR_FAN1_TACH: %d rpm\n", value);
    }
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_P5V, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_P5V\n");
  } else {
  	printf("SP_SENSOR_P5V: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_P12V, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_P12V\n");
  } else {
  	printf("SP_SENSOR_P12V: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_P3V3_STBY, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_P3V3_STBY\n");
  } else {
  	printf("SP_SENSOR_P3V3_STBY: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_P12V_SLOT1, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_P12V_SLOT1\n");
  } else {
  	printf("SP_SENSOR_P12V_SLOT1: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_P12V_SLOT2, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_P12V_SLOT2\n");
  } else {
  	printf("SP_SENSOR_P12V_SLOT2: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_P12V_SLOT3, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_P12V_SLOT3\n");
  } else {
  	printf("SP_SENSOR_P12V_SLOT3: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_P12V_SLOT4, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_P12V_SLOT4\n");
  } else {
  	printf("SP_SENSOR_P12V_SLOT4: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_P3V3, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_P3V3\n");
  } else {
  	printf("SP_SENSOR_P3V3: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_P1V15_BMC_STBY, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_P1V15_BMC_STBY\n");
  } else {
    printf("SP_SENSOR_P1V15_BMC_STBY: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_P1V2_BMC_STBY, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_P1V2_BMC_STBY\n");
  } else {
    printf("SP_SENSOR_P1V2_BMC_STBY: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_P2V5_BMC_STBY, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_P2V5_BMC_STBY\n");
  } else {
    printf("SP_SENSOR_P2V5_BMC_STBY: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_P1V8_STBY, &fvalue)) {
    printf("fby2_sensor_read failed: SP_P1V8_STBY\n");
  } else {
    printf("SP_P1V8_STBY: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_HSC_IN_VOLT, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_HSC_IN_VOLT\n");
  } else {
  	printf("SP_SENSOR_HSC_IN_VOLT: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_HSC_OUT_CURR, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_HSC_OUT_CURR\n");
  } else {
  	printf("SP_SENSOR_HSC_OUT_CURR: %.2f Amps\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_HSC_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_HSC_TEMP\n");
  } else {
  	printf("SP_SENSOR_P3V3: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_HSC_IN_POWER, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_HSC_IN_POWER\n");
  } else {
  	printf("SP_SENSOR_HSC_IN_POWER: %.2f Watts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_HSC_PEAK_IOUT, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_HSC_PEAK_IOUT\n");
  } else {
        printf("SP_SENSOR_HSC_PEAK_IOUT: %.2f Amps\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_HSC_PEAK_PIN, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_HSC_PEAK_PIN\n");
  } else {
        printf("SP_SENSOR_HSC_PEAK_PIN: %.2f Watts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, SP_SENSOR_BMC_HSC_PIN, &fvalue)) {
    printf("fby2_sensor_read failed: SP_SENSOR_BMC_HSC_PIN\n");
  } else {
        printf("SP_SENSOR_BMC_HSC_PIN: %.2f Watts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_MB_OUTLET_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_MB_OUTLET_TEMP\n");
  } else {
  	printf("BIC_SENSOR_MB_OUTLET_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VCCIN_VR_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VCCIN_VR_TEMP\n");
  } else {
  	printf("BIC_SENSOR_VCCIN_VR_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VCCSA_VR_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VCCSA_VR_TEMP\n");
  } else {
  	printf("BIC_SENSOR_VCCSA_VR_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_1V05_PCH_VR_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_1V05_PCH_VR_TEMP\n");
  } else {
  	printf("BIC_SENSOR_1V05_PCH_VR_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_SOC_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_SOC_TEMP\n");
  } else {
  	printf("BIC_SENSOR_SOC_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VNN_PCH_VR_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VNN_PCH_VR_TEMP\n");
  } else {
        printf("BIC_SENSOR_VNN_PCH_VR_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_MB_INLET_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_MB_INLET_TEMP\n");
  } else {
  	printf("BIC_SENSOR_MB_INLET_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_PCH_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_PCH_TEMP\n");
  } else {
  	printf("BIC_SENSOR_PCH_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_SOC_THERM_MARGIN, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_SOC_THERM_MARGIN\n");
  } else {
  	printf("BIC_SENSOR_SOC_THERM_MARGIN: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VDDR_DE_VR_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VDDR_DE_VR_TEMP\n");
  } else {
        printf("BIC_SENSOR_VDDR_DE_VR_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VDDR_AB_VR_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VDDR_AB_VR_TEMP\n");
  } else {
        printf("BIC_SENSOR_VDDR_AB_VR_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_MB_OUTLET_TEMP_BOTTOM, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_MB_OUTLET_TEMP_BOTTOM\n");
  } else {
        printf("BIC_SENSOR_MB_OUTLET_TEMP_BOTTOM: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_SOC_TJMAX, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_SOC_TJMAX\n");
  } else {
  	printf("BIC_SENSOR_SOC_TJMAX: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VCCIO_VR_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VCCIO_VR_TEMP\n");
  } else {
  	printf("BIC_SENSOR_VCCIO_VR_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_SOC_DIMMA0_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_SOC_DIMMA0_TEMP\n");
  } else {
  	printf("BIC_SENSOR_SOC_DIMMA0_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_SOC_DIMMA1_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_SOC_DIMMA1_TEMP\n");
  } else {
  	printf("BIC_SENSOR_SOC_DIMMA1_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_SOC_DIMMB0_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_SOC_DIMMB0_TEMP\n");
  } else {
  	printf("BIC_SENSOR_SOC_DIMMB0_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_SOC_DIMMB1_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_SOC_DIMMB1_TEMP\n");
  } else {
  	printf("BIC_SENSOR_SOC_DIMMB1_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_SOC_DIMMD0_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_SOC_DIMMD0_TEMP\n");
  } else {
        printf("BIC_SENSOR_SOC_DIMMD0_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_SOC_DIMMD1_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_SOC_DIMMD1_TEMP\n");
  } else {
        printf("BIC_SENSOR_SOC_DIMMD1_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_SOC_DIMME0_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_SOC_DIMME0_TEMP\n");
  } else {
        printf("BIC_SENSOR_SOC_DIMME0_TEMP: %.2f C\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_SOC_DIMME1_TEMP, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_SOC_DIMME1_TEMP\n");
  } else {
        printf("BIC_SENSOR_SOC_DIMME1_TEMP: %.2f C\n", fvalue);
  }

  // Server Current Sensors
  if (fby2_sensor_read(slot_id, BIC_SENSOR_VCCIO_VR_CURR, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VCCIO_VR_CURR\n");
  } else {
  	printf("BIC_SENSOR_VCCIO_VR_CURR: %.2f Amps\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VDDR_AB_VR_CURR, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VDDR_AB_VR_CURR\n");
  } else {
  	printf("BIC_SENSOR_VDDR_AB_VR_CURR: %.2f Amps\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VCCIN_VR_CURR, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VCCIN_VR_CURR\n");
  } else {
  	printf("BIC_SENSOR_VCCIN_VR_CURR: %.2f Amps\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VCCSA_VR_CURR, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VCCSA_VR_CURR\n");
  } else {
        printf("BIC_SENSOR_VCCSA_VR_CURR: %.2f Amps\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VNN_PCH_VR_CURR, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VNN_PCH_VR_CURR\n");
  } else {
        printf("BIC_SENSOR_VNN_PCH_VR_CURR: %.2f Amps\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VDDR_DE_VR_CURR, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VDDR_DE_VR_CURR\n");
  } else {
  	printf("BIC_SENSOR_VDDR_DE_VR_CURR: %.2f Amps\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_1V05_PCH_VR_CURR, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_1V05_PCH_VR_CURR\n");
  } else {
  	printf("BIC_SENSOR_1V05_PCH_VR_CURR: %.2f Amps\n", fvalue);
  }

  // Server Voltage Sensors
  if (fby2_sensor_read(slot_id, BIC_SENSOR_VCCIN_VR_VOL, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VCCIN_VR_VOL\n");
  } else {
  	printf("BIC_SENSOR_VCCIN_VR_VOL: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_INA230_VOL, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_INA230_VOL\n");
  } else {
        printf("BIC_SENSOR_INA230_VOL: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VDDR_DE_VR_VOL, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VDDR_DE_VR_VOL\n");
  } else {
  	printf("BIC_SENSOR_VDDR_DE_VR_VOL: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_1V05_PCH_VR_VOL, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_1V05_PCH_VR_VOL\n");
  } else {
  	printf("BIC_SENSOR_1V05_PCH_VR_VOL: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VCCSA_VR_VOL, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VCCSA_VR_VOL\n");
  } else {
        printf("BIC_SENSOR_VCCSA_VR_VOL: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VCCIO_VR_VOL, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VCCIO_VR_VOL\n");
  } else {
  	printf("BIC_SENSOR_VCCIO_VR_VOL: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VDDR_AB_VR_VOL, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VDDR_AB_VR_VOL\n");
  } else {
  	printf("BIC_SENSOR_VDDR_AB_VR_VOL: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_P3V3_MB, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_P3V3_MB\n");
  } else {
  	printf("BIC_SENSOR_P3V3_MB: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_P12V_MB, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_P12V_MB\n");
  } else {
  	printf("BIC_SENSOR_P12V_MB: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_P1V05_PCH, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_P1V05_PCH\n");
  } else {
  	printf("BIC_SENSOR_P1V05_PCH: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_P3V3_STBY_MB, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_P3V3_STBY_MB\n");
  } else {
  	printf("BIC_SENSOR_P3V3_STBY_MB: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_PVDDR_DE, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_PVDDR_DE\n");
  } else {
  	printf("BIC_SENSOR_PVDDR_DE: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_PV_BAT, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_PV_BAT\n");
  } else {
  	printf("BIC_SENSOR_PV_BAT: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_PVDDR_AB, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_PVDDR_AB\n");
  } else {
  	printf("BIC_SENSOR_PVDDR_AB: %.2f Volts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_PVNN_PCH, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_PVNN_PCH\n");
  } else {
  	printf("BIC_SENSOR_PVNN_PCH: %.2f Volts\n", fvalue);
  }

  // Server Power Sensors
  if (fby2_sensor_read(slot_id, BIC_SENSOR_VCCIN_VR_POUT, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VCCIN_VR_POUT\n");
  } else {
  	printf("BIC_SENSOR_VCCIN_VR_POUT: %.2f Watts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_INA230_POWER, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_INA230_POWER\n");
  } else {
  	printf("BIC_SENSOR_INA230_POWER: %.2f Watts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_SOC_PACKAGE_PWR, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_SOC_PACKAGE_PWR\n");
  } else {
  	printf("BIC_SENSOR_SOC_PACKAGE_PWR: %.2f Watts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VDDR_AB_VR_POUT, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VDDR_AB_VR_POUT\n");
  } else {
  	printf("BIC_SENSOR_VDDR_AB_VR_POUT: %.2f Watts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VCCSA_VR_POUT, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VCCSA_VR_POUT\n");
  } else {
  	printf("BIC_SENSOR_VCCSA_VR_POUT: %.2f Watts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VCCIO_VR_POUT, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VCCIO_VR_POUT\n");
  } else {
        printf("BIC_SENSOR_VCCIO_VR_POUT: %.2f Watts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VDDR_DE_VR_POUT, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VDDR_DE_VR_POUT\n");
  } else {
        printf("BIC_SENSOR_VDDR_DE_VR_POUT: %.2f Watts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VNN_PCH_VR_POUT, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VNN_PCH_VR_POUT\n");
  } else {
        printf("BIC_SENSOR_VNN_PCH_VR_POUT: %.2f Watts\n", fvalue);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_1V05_PCH_VR_POUT, &fvalue)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_1V05_PCH_VR_POUT\n");
  } else {
        printf("BIC_SENSOR_1V05_PCH_VR_POUT: %.2f Watts\n", fvalue);
  }

  // Discrete Sensors
  if (fby2_sensor_read(slot_id, BIC_SENSOR_SYSTEM_STATUS, &value)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_SYSTEM_STATUS\n");
  } else {
  	printf("BIC_SENSOR_SYSTEM_STATUS: 0x%X\n", value);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_PROC_FAIL, &value)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_PROC_FAIL\n");
  } else {
  	printf("BIC_SENSOR_PROC_FAIL: 0x%X\n", value);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_SYS_BOOT_STAT, &value)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_SYS_BOOT_STAT\n");
  } else {
  	printf("BIC_SENSOR_SYS_BOOT_STAT: 0x%X\n", value);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_VR_HOT, &value)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_VR_HOT\n");
  } else {
  	printf("BIC_SENSOR_VR_HOT: 0x%X\n", value);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_CPU_DIMM_HOT, &value)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_CPU_DIMM_HOT\n");
  } else {
  	printf("BIC_SENSOR_CPU_DIMM_HOT: 0x%X\n", value);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_NVME1_CTEMP, &value)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_NVME1_CTEMP\n");
  } else {
  	printf("BIC_SENSOR_NVME1_CTEMP: 0x%X\n", value);
  }

  if (fby2_sensor_read(slot_id, BIC_SENSOR_NVME2_CTEMP, &value)) {
    printf("fby2_sensor_read failed: BIC_SENSOR_NVME2_CTEMP\n");
  } else {
  	printf("BIC_SENSOR_NVME2_CTEMP: 0x%X\n", value);
  }

  return 0;
}
