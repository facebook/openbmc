# Facebook OpenBMC Sensor List for waimeacanyon

This document of sensor lists for platform waimeacanyon.

## IOM Sensors

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| ----------- | ------- | ---- | --- | --- | --- | --- | --- | --- |
| IOM_TEMP_C       | 0x3 | C | NA | 40 | NA | NA | NA | NA |
| IOM_INA_VOLT_V   | 0x2F | V | NA | NA | NA | NA | NA | NA |
| IOM_INA_CURR_A   | 0x47 | A | NA | NA | NA | NA | NA | NA |
| IOM_INA_PWR_W    | 0x6B | W | NA | NA | NA | NA | NA | NA |


## MB Sensors

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| ----------- | ------- | ---- | --- | --- | --- | --- | --- | --- |
| MB_MB_INLET_TEMP_C    | 0x1 | C | 150 | 48 | NA | NA | NA | NA |
| MB_MB_OUTLET_TEMP_C   | 0x2 | C | 150 | 93 | NA | NA | NA | NA |
| MB_PCH_TEMP_C         | 0x4 | C | NA | 74 | NA | NA | NA | NA |
| MB_CPU_TEMP_C         | 0x5 | C | NA | 84 | NA | NA | NA | NA |
| MB_SOC_THERM_MARGIN_C | 0x6 | C | 125 | NA | NA | NA | NA | NA |
| MB_CPU_TJMAX_C        | 0x7 | C | 125 | NA | NA | NA | NA | NA |
| MB_SSD0_TEMP_C        | 0x8 | C | NA | 75 | NA | NA | NA | NA |
| MB_HSC_TEMP_C         | 0x9 | C | 125 | 86 | NA | NA | NA | NA |
| MB_DIMMA_TEMP_C       | 0xA | C | NA | 85 | NA | NA | NA | NA |
| MB_DIMMC_TEMP_C       | 0xB | C | NA | 85 | NA | NA | NA | NA |
| MB_DIMME_TEMP_C       | 0xC | C | NA | 85 | NA | NA | NA | NA |
| MB_DIMMG_TEMP_C       | 0xD | C | NA | 85 | NA | NA | NA | NA |
| MB_VCCIN_SPS_TEMP_C   | 0xE | C | 125 | 100 | NA | NA | NA | NA |
| MB_FIVRA_SPS_TEMP_C   | 0xF | C | 125 | 100 | NA | NA | NA | NA |
| MB_EHV_SPS_TEMP_C     | 0x10 | C | 125 | 100 | NA | NA | NA | NA |
| MB_VCCD_SPS_TEMP_C    | 0x11 | C | 125 | 100 | NA | NA | NA | NA |
| MB_FAON_SPS_TEMP_C    | 0x12 | C | 125 | 100 | NA | NA | NA | NA |
| MB_P12V_STBY_VOLT_V   | 0x20 | V | 14.333 | 13.3488 | 13.2192 | 10.8192 | 10.7088 | 10.091 |
| MB_P3V_BAT_VOLT_V     | 0x21 | V | NA | 3.502 | 3.468 | 2.793 | 2.7645 | NA |
| MB_P3V3_STBY_VOLT_V   | 0x22 | V | 3.993 | 3.56895 | 3.5343 | 3.0723 | 3.04095 | 2.31 |
| MB_P1V8_STBY_VOLT_V   | 0x23 | V | 2.088 | 1.91889 | 1.90026 | 1.69344 | 1.67616 | 1.44 |
| MB_P1V05_PCH_VOLT_V   | 0x24 | V | 1.218 | 1.11395 | 1.10313 | 0.97755 | 0.96758 | 0.84 |
| MB_P5V_STBY_VOLT_V    | 0x25 | V | 5.8 | 5.4075 | 5.355 | 4.655 | 4.6075 | 4 |
| MB_P12V_DIMM_VOLT_V   | 0x26 | V | NA | 14.214 | 14.076 | 9.996 | 9.894 | NA |
| MB_P1V2_STBY_VOLT_V   | 0x27 | V | 1.392 | 1.2978 | 1.2852 | 1.1172 | 1.1058 | 0.96 |
| MB_P3V3_M2_VOLT_V     | 0x28 | V | NA | 3.70491 | 3.66894 | 2.94294 | 2.91291 | NA |
| MB_HSC_INPUT_VOLT_V   | 0x29 | V | 14.333 | 13.3488 | 13.2192 | 10.8192 | 10.7088 | 10.091 |
| MB_VCCIN_VR_VOLT_V    | 0x2A | V | 2.2 | 1.92713 | 1.90842 | 1.65424 | 1.63736 | 0.4 |
| MB_FIVRA_VR_VOLT_V    | 0x2B | V | 2.2 | 1.91477 | 1.89618 | 1.70422 | 1.68683 | 0.4 |
| MB_EHV_VR_VOLT_V      | 0x2C | V | 2.2 | 1.89623 | 1.87782 | 1.71402 | 1.69653 | 0.4 |
| MB_VCCD_VR_VOLT_V     | 0x2D | V | 1.5 | 1.22467 | 1.90842 | 1.65424 | 1.63736 | 0.4 |
| MB_FAON_VR_VOLT_V     | 0x2E | V | 1.4 | 1.0815 | 1.071 | 0.9114 | 0.9021 | 0.4 |
| MB_HSC_OUTPUT_CURR_A  | 0x40 | A | 40 | 32 | 28.8 | NA | NA | NA |
| MB_VCCIN_VR_CURR_A    | 0x41 | A | 180 | 141 | 119 | NA | NA | NA |
| MB_FIVRA_VR_CURR_A    | 0x42 | A | 71 | 53 | 48 | NA | NA | NA |
| MB_EHV_VR_CURR_A      | 0x43 | A | 18 | 6.25 | 5 | NA | NA | NA |
| MB_VCCD_VR_CURR_A     | 0x44 | A | 43 | 30 | 27 | NA | NA | NA |
| MB_VCCD_VR_CURR_IN_A  | 0x45 | A | 43 | 30 | 27 | NA | NA | NA |
| MB_FAON_VR_CURR_A     | 0x46 | A | 60 | 46 | 42 | NA | NA | NA |
| MB_CPU_PWR_W          | 0x60 | W | NA | NA | NA | NA | NA | NA |
| MB_HSC_INPUT_PWR_W    | 0x61 | W | 500 | 400 | 360 | NA | NA | NA |
| MB_DIMMA_PMIC_PWR_W   | 0x62 | W | NA | 31.9162 | 31.6002 | NA | NA | NA |
| MB_DIMMC_PMIC_PWR_W   | 0x63 | W | NA | 31.9162 | 31.6002 | NA | NA | NA |
| MB_DIMME_PMIC_PWR_W   | 0x64 | W | NA | 31.9162 | 31.6002 | NA | NA | NA |
| MB_DIMMG_PMIC_PWR_W   | 0x65 | W | NA | 31.9162 | 31.6002 | NA | NA | NA |
| MB_VCCIN_VR_PWR_W     | 0x66 | W | 324 | 253.8 | 214.2 | NA | NA | NA |
| MB_FIVRA_VR_PWR_W     | 0x67 | W | 127.8 | 95.4 | 86.4 | NA | NA | NA |
| MB_EHV_VR_PWR_W       | 0x68 | W | 32.4 | 11.25 | 9 | NA | NA | NA |
| MB_VCCD_VR_PWR_W      | 0x69 | W | 49.235 | 34.35 | 30.915 | NA | NA | NA |
| MB_FAON_VR_PWR_W      | 0x6A | W | 64.2 | 49.22 | 44.94 | NA | NA | NA |


## SCB Sensors

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| ----------- | ------- | ---- | --- | --- | --- | --- | --- | --- |
| SCB_0_TEMP_C            | 0x7D | C | NA | 70 | NA | NA | 10 | NA |
| SCB_0_POWER_CURR_A      | 0x7E | A | NA | 5.25 | 3.85 | NA | NA | NA |
| SCB_0_EXP_P0V9_VOLT_V   | 0x7F | V | NA | 1 | 0.95 | 0.85 | 0.81 | NA |
| SCB_0_IOC_P0V865_VOLT_V | 0x80 | V | NA | 0.96 | 0.91 | 0.82 | 0.78 | NA |
| SCB_0_IOC_TEMP_C        | 0x81 | C | NA | 115 | NA | NA | 10 | NA |
| SCB_0_EXP_TEMP_C        | 0x82 | C | NA | 115 | NA | NA | 10 | NA |
| SCB_1_TEMP_C            | 0x83 | C | NA | 70 | NA | NA | 10 | NA |
| SCB_1_POWER_CURR_A      | 0x84 | A | NA | 5.25 | 3.85 | NA | NA | NA |
| SCB_1_EXP_P0V9_VOLT_V   | 0x85 | V | NA | 1 | 0.95 | 0.85 | 0.81 | NA |
| SCB_1_IOC_P0V865_VOLT_V | 0x86 | V | NA | 0.96 | 0.91 | 0.82 | 0.78 | NA |
| SCB_1_IOC_TEMP_C        | 0x87 | C | NA | 115 | NA | NA | 10 | NA |
| SCB_1_EXP_TEMP_C        | 0x88 | C | NA | 115 | NA | NA | 10 | NA |


## SCM Sensors

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| ----------- | ------- | ---- | --- | --- | --- | --- | --- | --- |
| SCM_P12V_V           | 0x1 | V | NA | 13.32 | 12.61 | 11.39 | 10.79 | NA |
| SCM_P5V_V            | 0x2 | V | NA | 5.55 | 5.26 | 4.75 | 4.5 | NA |
| SCM_P3V3_V           | 0x3 | V | NA | 3.66 | 3.47 | 3.13 | 2.97 | NA |
| SCM_P2V5_V           | 0x4 | V | NA | 2.78 | 2.63 | 2.37 | 2.25 | NA |
| SCM_P1V8_V           | 0x5 | V | NA | 2 | 1.89 | 1.71 | 1.62 | NA |
| SCM_P1V2_V           | 0x6 | V | NA | 1.33 | 1.26 | 1.14 | 1.08 | NA |
| SCM_P1V0_V           | 0x7 | V | NA | 1.11 | 1.05 | 0.95 | 0.9 | NA |


## HDBP Sensors

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| ----------- | ------- | ---- | --- | --- | --- | --- | --- | --- |
| HDBP_SYSTEM_INLET_0_TEMP_C    | 0x1 | C | NA | 40 | NA | NA | 10 | NA |
| HDBP_SYSTEM_INLET_1_TEMP_C    | 0x2 | C | NA | 40 | NA | NA | 10 | NA |
| HDBP_HDD-L_REAR_TEMP_C        | 0x97 | C | NA | 70 | NA | NA | 10 | NA |
| HDBP_HDD-L_HSC_CURR_A         | 0x98 | A | NA | 100 | 90 | NA | NA | NA |
| HDBP_HDD-L_HSC_VOLT_V         | 0x99 | V | NA | 13.32 | 12.61 | 11.39 | 10.79 | NA |
| HDBP_HDD-L_HSC_PWR_W          | 0x9A | W | NA | 1200 | 1080 | NA | NA | NA |
| HDBP_HDD-L_0_TEMP_C           | 0x9B | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_1_TEMP_C           | 0x9C | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_2_TEMP_C           | 0x9D | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_3_TEMP_C           | 0x9E | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_4_TEMP_C           | 0x9F | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_5_TEMP_C           | 0xA0 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_6_TEMP_C           | 0xA1 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_7_TEMP_C           | 0xA2 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_8_TEMP_C           | 0xA3 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_9_TEMP_C           | 0xA4 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_10_TEMP_C          | 0xA5 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_11_TEMP_C          | 0xA6 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_12_TEMP_C          | 0xA7 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_13_TEMP_C          | 0xA8 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_14_TEMP_C          | 0xA9 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_15_TEMP_C          | 0xAA | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_16_TEMP_C          | 0xAB | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_17_TEMP_C          | 0xAC | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_18_TEMP_C          | 0xAD | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_19_TEMP_C          | 0xAE | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_20_TEMP_C          | 0xAF | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_21_TEMP_C          | 0xB0 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_22_TEMP_C          | 0xB1 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_23_TEMP_C          | 0xB2 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_24_TEMP_C          | 0xB3 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_25_TEMP_C          | 0xB4 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_26_TEMP_C          | 0xB5 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_27_TEMP_C          | 0xB6 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_28_TEMP_C          | 0xB7 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_29_TEMP_C          | 0xB8 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_30_TEMP_C          | 0xB9 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_31_TEMP_C          | 0xBA | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_32_TEMP_C          | 0xBB | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_33_TEMP_C          | 0xBC | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_34_TEMP_C          | 0xBD | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-L_35_TEMP_C          | 0xBE | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_REAR_TEMP_C        | 0xBF | C | NA | 70 | NA | NA | 10 | NA |
| HDBP_HDD-R_HSC_POWER_CURR_A   | 0xC0 | A | NA | 100 | 90 | NA | NA | NA |
| HDBP_HDD-R_HSC_VOLT_V         | 0xC1 | V | NA | 13.32 | 12.61 | 11.39 | 10.79 | NA |
| HDBP_HDD-R_HSC_PWR_W          | 0xC2 | W | NA | 1200 | 1080 | NA | NA | NA |
| HDBP_HDD-R_0_TEMP_C           | 0xC3 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_1_TEMP_C           | 0xC4 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_2_TEMP_C           | 0xC5 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_3_TEMP_C           | 0xC6 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_4_TEMP_C           | 0xC7 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_5_TEMP_C           | 0xC8 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_6_TEMP_C           | 0xC9 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_7_TEMP_C           | 0xCA | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_8_TEMP_C           | 0xCB | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_9_TEMP_C           | 0xCC | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_10_TEMP_C          | 0xCD | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_11_TEMP_C          | 0xCE | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_12_TEMP_C          | 0xCF | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_13_TEMP_C          | 0xD0 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_14_TEMP_C          | 0xD1 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_15_TEMP_C          | 0xD2 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_16_TEMP_C          | 0xD3 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_17_TEMP_C          | 0xD4 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_18_TEMP_C          | 0xD5 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_19_TEMP_C          | 0xD6 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_20_TEMP_C          | 0xD7 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_21_TEMP_C          | 0xD8 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_22_TEMP_C          | 0xD9 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_23_TEMP_C          | 0xDA | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_24_TEMP_C          | 0xDB | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_25_TEMP_C          | 0xDC | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_26_TEMP_C          | 0xDD | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_27_TEMP_C          | 0xDE | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_28_TEMP_C          | 0xDF | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_29_TEMP_C          | 0xE0 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_30_TEMP_C          | 0xE1 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_31_TEMP_C          | 0xE2 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_32_TEMP_C          | 0xE3 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_33_TEMP_C          | 0xE4 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_34_TEMP_C          | 0xE5 | C | NA | 60 | NA | NA | 10 | NA |
| HDBP_HDD-R_35_TEMP_C          | 0xE6 | C | NA | 60 | NA | NA | 10 | NA |


## PDB Sensors

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| ----------- | ------- | ---- | --- | --- | --- | --- | --- | --- |
| PDB_SYSTEM_OUTLET_0_TEMP_C    | 0x3 | C | NA | 70 | NA | NA | 10 | NA |
| PDB_SYSTEM_OUTLET_1_TEMP_C    | 0x4 | C | NA | 70 | NA | NA | 10 | NA |
| PDB_SYSTEM_HSC_INPUT_VOLT_V   | 0x5 | V | NA | 13.32 | 12.61 | 11.39 | 10.79 | NA |
| PDB_SYSTEM_HSC_OUTPUT_VOLT_V  | 0x6 | V | NA | 13.32 | 12.61 | 11.39 | 10.79 | NA |
| PDB_SYSTEM_HSC_CURR_A         | 0x7 | A | NA | 160 | 155 | NA | NA | NA |
| PDB_SYSTEM_HSC_PWR_W          | 0x8 | W | NA | 2000 | 1900 | NA | NA | NA |
| PDB_FAN0_INLET_TACH_RPM       | 0x9 | RPM | NA | 13000 | NA | NA | 700 | NA |
| PDB_FAN0_OUTLET_TACH_RPM      | 0xA | RPM | NA | 12000 | NA | NA | 650 | NA |
| PDB_FAN0_POWER_CURR_A         | 0xB | A | NA | 4 | 3 | NA | NA | NA |
| PDB_FAN1_INLET_TACH_RPM       | 0xC | RPM | NA | 13000 | NA | NA | 700 | NA |
| PDB_FAN1_OUTLET_TACH_RPM      | 0xD | RPM | NA | 12000 | NA | NA | 650 | NA |
| PDB_FAN1_POWER_CURR_A         | 0xE | A | NA | 4 | 3 | NA | NA | NA |
| PDB_FAN2_INLET_TACH_RPM       | 0xF | RPM | NA | 13000 | NA | NA | 700 | NA |
| PDB_FAN2_OUTLET_TACH_RPM      | 0x10 | RPM | NA | 12000 | NA | NA | 650 | NA |
| PDB_FAN2_POWER_CURR_A         | 0x11 | A | NA | 4 | 3 | NA | NA | NA |
| PDB_FAN3_INLET_TACH_RPM       | 0x12 | RPM | NA | 13000 | NA | NA | 700 | NA |
| PDB_FAN3_OUTLET_TACH_RPM      | 0x13 | RPM | NA | 12000 | NA | NA | 650 | NA |
| PDB_FAN3_POWER_CURR_A         | 0x14 | A | NA | 4 | 3 | NA | NA | NA |
