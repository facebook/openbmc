## Sensors {#sensors}

### Baseboard

#### Temperature Monitoring

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| BB_INLET_TEMP_C | 0xED | C | 150 | 45 | NA | NA | NA | NA |
| BB_OUTLET_TEMP_C | 0xEE | C | 150 | 55 | NA | NA | NA | NA |
| PDB_VPDB_WW_TEMP_C | 0xB2 | C | 115 | 100 | NA | NA | NA | NA |
| BB_HSC_TEMP_C | 0xF8 | C | 125 | 55 | NA | NA | NA | NA |
| NIC_TEMP_C | 0xEF | C | 120 | 105 | NA | NA | NA | NA |

#### Voltage Monitoring

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| BB_P5V_VOLT_V | 0xF0 | V | 5.65 | 5.550  | 5.500  | 4.500  | 4.450  | 4.15 |
| BB_P12V_VOLT_V | 0xF1 | V | 14.333 | 13.320  | 13.200  | 10.800  | 10.680  | 10.091 |
| BB_P3V3_STBY_VOLT_V | 0xF2 | V | 3.729 | 3.564  | 3.531  | 3.069  | 3.036  | 2.739 |
| BB_P2V5_STBY_VOLT_V | 0xF5 | V | NA | 2.700  | 2.675  | 2.325  | 2.300  | NA |
| BB_ADC_P5V_USB_VOLT_V | 0xD7 | V | 5.65 | 5.400  | 5.350  | 4.650  | 4.600  | 4.15 |
| BB_P1V8_STBY_VOLT_V | 0xF3 | V | NA | 1.944  | 1.926  | 1.674  | 1.656  | NA |
| BB_P1V2_STBY_VOLT_V | 0xF4 | V | 1.356 | 1.296  | 1.284  | 1.116  | 1.104  | 0.996 |
| BB_ADC_P1V0_STBY_VOLT_V | 0xD4 | V | 1.13 | 1.080  | 1.070  | 0.930  | 0.920  | 0.83 |
| BB_ADC_P0V6_STBY_VOLT_V | 0xD5 | V | NA | 0.648  | 0.642  | 0.558  | 0.552  | NA |
| BB_ADC_P3V3_RGM_STBY_VOLT_V | 0xD6 | V | NA | 3.564  | 3.531  | 3.069  | 3.036  | NA |
| BB_ADC_P3V3_NIC_VOLT_V | 0xD8 | V | 3.729 | 3.630  | 3.597  | 3.003  | 2.970  | 2.95 |
| PDB_48V_WW_INPUT_VOLT_V | 0xB5 | V | 61.5 | 56.61 | 56.1 | 43.2 | 42.72 | 38.5 |
| PDB_12V_WW_OUTPUT_VOLT_V | 0xB8 | V | 13.6 | 13.875 | 13.75 | 11.25 | 11.125 | 10 |
| BB_HSC_INPUT_VOLT_V | 0xF7 | V | 14.333 | 13.320  | 13.200  | 10.800  | 10.680  | 10.091 |
| BB_ADC_NIC_P12V_VOLT_V | 0xD2 | V | 14.91 | 13.320  | 13.200  | 10.800  | 10.680  | 10.17 |
| BB_MEDUSA_INPUT_VOLT_V | 0xFD | V | 　NA | 56.61 | 56.1 | 43.2 | 42.72 | 41.1 |
| BB_MEDUSA_OUTPUT_VOLT_V | 0xF6 | V | 　NA | 56.61 | 56.1 | 43.2 | 42.72 | 41.1 |

#### Current Monitoring

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| BB_MEDUSA_CURR_A | 0xD0 | A | 　NA | 62 | NA | NA | NA | NA |
| PDB_12V_WW_OUTPUT_CURR_A | 0xBB | A | 　NA | 144 | NA | NA | NA | NA |
| BB_HSC_OUTPUT_CURR_A | 0xFA | A | 45 | 36 | NA | NA | NA | NA |
| BB_ADC_FAN_OUTPUT_CURR_A | 0xFB | A | 39.2 | 28.6 | NA | NA | NA | NA |
| BB_ADC_NIC_OUTPUT_CURR_A | 0xFC | A | 8.15 | 6.6 | NA | NA | NA | NA |

#### Power Monitoring

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| BB_MEDUSA_PWR_W | 0xD1 | W | 　NA | 1800 | NA | NA | NA | NA |
| PDB_12V_WW_PWR_W | 0xBC | W | 　NA | 1800 | NA | NA | NA | NA |
| BB_HSC_INPUT_PWR_W | 0xF9 | W | 562.5 | 450 | NA | NA | NA | NA |
| BB_NIC_PWR_W | 0xD3 | W | 101.875 | 82.5 | NA | NA | NA | NA |

#### Fan Monitoring

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| BB_FAN0_TACH_RPM | 0xE0 | RPM | NA | 16500 | 13500 | NA | 1200 | NA |
| BB_FAN1_TACH_RPM | 0xE1 | RPM | NA | 16500 | 13500 | NA | 1200 | NA |
| BB_FAN2_TACH_RPM | 0xE2 | RPM | NA | 16500 | 13500 | NA | 1200 | NA |
| BB_FAN3_TACH_RPM | 0xE3 | RPM | NA | 16500 | 13500 | NA | 1200 | NA |
| BB_FAN4_TACH_RPM | 0xE4 | RPM | NA | 16500 | 13500 | NA | 1200 | NA |
| BB_FAN5_TACH_RPM | 0xE5 | RPM | NA | 16500 | 13500 | NA | 1200 | NA |
| BB_FAN6_TACH_RPM | 0xE6 | RPM | NA | 16500 | 13500 | NA | 1200 | NA |
| BB_FAN7_TACH_RPM | 0xE7 | RPM | NA | 16500 | 13500 | NA | 1200 | NA |

#### Other

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| BB_HSC_PEAK_OUTPUT_CURR_A | 0xC8 | A | NA | NA | NA | NA | NA | NA |
| BB_HSC_PEAK_INPUT_PWR_W | 0xC9 | W | NA | NA | NA | NA | NA | NA |
| BB_MEDUSA_VDELTA_VOLT_V | 0xCF | V | NA | 0.5 | NA | NA | NA | NA |
| BB_CPU_CL_VDELTA_VOLT_V | 0xCC | V | NA | 41.8 | NA | NA | NA | NA |
| BB_CPU_PDB_VDELTA_VOLT_V | 0xCE | V | NA | 41.8 | NA | NA | NA | NA |
| BB_PMON_CURRENT_LEAKAGE_CURR_A | 0xCD | A | NA | NA | NA | NA | NA | NA |
| BB_FAN_PWR_W | 0xCA | W | 544.88 | 396.825 | NA | NA | NA | NA |

### Server board

#### Temperature Monitoring

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| MB_INLET_TEMP_C | 0x01 | C | 150 | 150 | 60 | NA | NA | NA |
| MB_OUTLET_TEMP_C | 0x02 | C | 150 | 150 | 80 | NA | NA | NA |
| FIO_FRONT_TEMP_C | 0x03 | C | 150 | 150 | 40 | NA | NA | NA |
| MB_SOC_CPU_TEMP_C | 0x04 | C | 91　 | NA | NA | NA | NA | NA |
| MB_DIMMA_TEMP_C | 0x05 | C | NA | NA | 85 | NA | NA | NA |
| MB_DIMMB_TEMP_C | 0x06 | C | NA | NA | 85 | NA | NA | NA |
| MB_DIMMC_TEMP_C | 0x07 | C | NA | NA | 85 | NA | NA | NA |
| MB_DIMMD_TEMP_C | 0x08 | C | NA | NA | 85 | NA | NA | NA |
| MB_DIMME_TEMP_C | 0x09 | C | NA | NA | 85 | NA | NA | NA |
| MB_DIMMF_TEMP_C | 0x0A | C | NA | NA | 85 | NA | NA | NA |
| MB_DIMMG_TEMP_C | 0x0B | C | NA | NA | 85 | NA | NA | NA |
| MB_DIMMH_TEMP_C | 0x0C | C | NA | NA | 85 | NA | NA | NA |
| MB_SSD0_TEMP_C | 0x0D | C | 93 | 93 | 77 | NA | NA | NA |
| MB_HSC_TEMP_C | 0x0E | C | 175 | 175 | 80 | NA | NA | NA |
| MB_VR_VCCIN_TEMP_C | 0x0F | C | 125 | 125 | 100 | NA | NA | NA |
| MB_VR_EHV_TEMP_C | 0x10 | C | 125 | 125 | 100 | NA | NA | NA |
| MB_VR_FIVRA_TEMP_C | 0x11 | C | 125 | 125 | 100 | NA | NA | NA |
| MB_VR_VCCINF_TEMP_C | 0x12 | C | 125 | 125 | 100 | NA | NA | NA |
| MB_VR_VCCD0_TEMP_C | 0x13 | C | 125 | 125 | 100 | NA | NA | NA |
| MB_VR_VCCD1_TEMP_C | 0x14 | C | 125 | 125 | 100 | NA | NA | NA |
| MB_SOC_THERMAL_MARGIN_C | 0x15 | C | NA | NA | 0 | NA | NA | NA |
| MB_SOC_TJMAX_C | 0x16 | C | NA | NA | NA | NA | NA | NA |

#### Voltage Monitoring

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| MB_ADC_P12V_STBY_VOLT_V | 0x17 | V | 14.333 | 13.3488 | 13.2192 | 10.8192 | 10.7088 | 10.091 |
| MB_ADC_P3V_BAT_VOLT_V | 0x18 | V | NA | 3.502 | 3.468 | 2.793 | 2.7645 | NA |
| MB_ADC_P3V3_STBY_VOLT_V | 0x19 | V | 3.828 | 3.56895 | 3.5343 | 3.0723 | 3.04095 | 2.64 |
| MB_ADC_P1V8_STBY_VOLT_V | 0x1A | V | 2.088 | 1.9467 | 1.9278 | 1.6758 | 1.6587 | 1.44 |
| MB_ADC_P1V05_STBY_VOLT_V | 0x1B | V | NA | 1.135575 | 1.12455 | 0.97755 | 0.96758 | NA |
| MB_HSC_INPUT_VOLT_V | 0x1C | V | 14.333 | 13.3488 | 13.2192 | 10.8192 | 10.7088 | 10.091 |
| MB_VR_VCCIN_VOLT_V | 0x1D | V | 2.28 | 1.92713 | 1.90842 | 1.48 | 1.46 | 1.43 |
| MB_VR_VCCINF_VOLT_V | 0x1E | V | 1.4 | 0.92082 | 0.91188 | 0.784 | 0.776 | 0.36 |
| MB_VR_FIVRA_VOLT_V | 0x1F | V | 2.3 | 1.95185 | 1.9329 | 1.68462 | 1.66743 | 1.4 |
| MB_VR_VCCD0_VOLT_V | 0x20 | V | 1.75 | 1.18759 | 1.17606 | 0.98392 | 0.97388 | 0.7 |
| MB_VR_VCCD1_VOLT_V | 0x21 | V | 1.75 | 1.18759 | 1.17606 | 0.98392 | 0.97388 | 0.7 |
| MB_VR_EHV_VOLT_V | 0x22 | V | 2.3 | 1.9055 | 1.887 | 1.71794 | 1.70041 | 1.4 |
| MB_ADC_VNN_VOLT_V | 0x23 | V | 1.16 | 1.09695 | 1.0863 | 0.90846 | 0.89919 | 0.8 |
| MB_ADC_P5V_STBY_VOLT_V | 0x24 | V | 5.8 | 5.665 | 5.61 | 4.655 | 4.6075 | 4 |
| MB_ADC_P12V_DIMM_VOLT_V | 0x25 | V | NA | 14.214 | 14.076 | 9.996 | 9.894 | NA |
| MB_ADC_P1V2_STBY_VOLT_V | 0x26 | V | NA | 1.2978 | 1.2852 | 1.1172 | 1.1058 | NA |
| MB_ADC_P3V3_M2_VOLT_V | 0x27 | V | NA | 3.70491 | 3.66894 | 2.94294 | 2.91291 | NA |

#### Current Monitoring

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| MB_HSC_OUTPUT_CURR_A | 0x28 | A | 68 | 60.94 | 58.17 | NA | NA | NA |
| MB_HSC_OUTPUT_CURR_A | 0x29 | A | 84 | 75.02 | 71.61 | NA | NA | NA |
| MB_VR_VCCIN_CURR_A | 0x2A | A | 306 | 290.4 | 277.2 | NA | NA | NA |
| MB_VR_EHV_CURR_A | 0x2B | A | 7 | 5.17 | 4.935 | NA | NA | NA |
| MB_VR_FIVRA_CURR_A | 0x2C | A | 69 | 62.7 | 59.85 | NA | NA | NA |
| MB_VR_VCCINF_CURR_A | 0x2D | A | 66 | 60.5 | 57.75 | NA | NA | NA |
| MB_VR_VCCD0_CURR_A | 0x2E | A | 40 | 30.25 | 28.875 | NA | NA | NA |
| MB_VR_VCCD1_CURR_A | 0x2F | A | 40 | 30.25 | 28.875 | NA | NA | NA |

#### Power Monitoring

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| MB_HSC_INPUT_PWR_W | 0x30 | W | 850 | 761.75 | 727.125 | NA | NA | NA |
| MB_HSC_INPUT_PWR_W | 0x31 | W | 1050 | 937.75 | 895.125 | NA | NA | NA |
| MB_VR_VCCIN_PWR_W | 0x32 | W | 559.98 | 522.72 | 498.96 | NA | NA | NA |
| MB_VR_EHV_PWR_W | 0x33 | W | 12.6 | 9.306 | 8.883 | NA | NA | NA |
| MB_VR_FIVRA_PWR_W | 0x34 | W | 124.2 | 112.86 | 107.73 | NA | NA | NA |
| MB_VR_VCCINF_PWR_W | 0x35 | W | 56.1 | 51.425 | 49.0875 | NA | NA | NA |
| MB_VR_VCCD0_PWR_W | 0x36 | W | 44 | 33.275 | 31.7625 | NA | NA | NA |
| MB_VR_VCCD1_PWR_W | 0x37 | W | 44 | 33.275 | 31.7625 | NA | NA | NA |
| MB_VR_DIMMA_PMIC_PWR_W | 0x38 | W | NA | 31.91623988 | 31.6002375 | NA | NA | NA |
| MB_VR_DIMMB_PMIC_PWR_W | 0x39 | W | NA | 31.91623988 | 31.6002375 | NA | NA | NA |
| MB_VR_DIMMC_PMIC_PWR_W | 0x3A | W | NA | 31.91623988 | 31.6002375 | NA | NA | NA |
| MB_VR_DIMMD_PMIC_PWR_W | 0x3B | W | NA | 31.91623988 | 31.6002375 | NA | NA | NA |
| MB_VR_DIMME_PMIC_PWR_W | 0x3C | W | NA | 31.91623988 | 31.6002375 | NA | NA | NA |
| MB_VR_DIMMF_PMIC_PWR_W | 0x3D | W | NA | 31.91623988 | 31.6002375 | NA | NA | NA |
| MB_VR_DIMMG_PMIC_PWR_W | 0x3E | W | NA | 31.91623988 | 31.6002375 | NA | NA | NA |
| MB_VR_DIMMH_PMIC_PWR_W | 0x3F | W | NA | 31.91623988 | 31.6002375 | NA | NA | NA |

### 1OU board - Niagara Falls

#### Temperature Monitoring

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| NF_1OU_BOARD_INLET_TEMP_C | 0x50 | C | NA | NA | NA | NA | NA | NA |
| NF_CXL_CNTR_TEMP_C | 0x51 | C | NA | NA | NA | NA | NA | NA |
| NF_VR_P0V85_ASIC_TEMP_C | 0x52 | C | NA | NA | NA | NA | NA | NA |
| NF_VR_PVDDQ_AB_TEMP_C | 0x53 | C | NA | NA | NA | NA | NA | NA |
| NF_VR_P0V8_ASIC_TEMP_C | 0x54 | C | NA | NA | NA | NA | NA | NA |
| NF_VR_PVDDQ_CD_TEMP_C | 0x55 | C | NA | NA | NA | NA | NA | NA |

#### Voltage Monitoring

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| NF_ADC_P1V2_STBY_VOLT_V | 0x57 | V | NA | NA | NA | NA | NA | NA |
| NF_ADC_P1V2_ASIC_VOLT_V | 0x58 | V | NA | NA | NA | NA | NA | NA |
| NF_ADC_P1V8_ASIC_VOLT_V | 0x59 | V | NA | NA | NA | NA | NA | NA |
| NF_ADC_PVPP_AB_VOLT_V | 0x5A | V | NA | NA | NA | NA | NA | NA |
| NF_ADC_PVPP_CD_VOLT_V | 0x5B | V | NA | NA | NA | NA | NA | NA |
| NF_ADC_PVTT_AB_VOLT_V | 0x5C | V | NA | NA | NA | NA | NA | NA |
| NF_ADC_PVTT_CD_VOLT_V | 0x5D | V | NA | NA | NA | NA | NA | NA |
| NF_ADC_P0V75_ASIC_VOLT_V | 0x5E | V | NA | NA | NA | NA | NA | NA |
| NF_INA233_P12V_STBY_VOLT_V | 0x5F | V | NA | NA | NA | NA | NA | NA |
| NF_INA233_P3V3_STBY_VOLT_V | 0x60 | V | NA | NA | NA | NA | NA | NA |
| NF_VR_P0V85_ASIC_VOLT_V | 0x61 | V | NA | NA | NA | NA | NA | NA |
| NF_VR_PVDDQ_AB_VOLT_V | 0x62 | V | NA | NA | NA | NA | NA | NA |
| NF_VR_P0V8_ASIC_VOLT_V | 0x63 | V | NA | NA | NA | NA | NA | NA |
| NF_VR_PVDDQ_CD_VOLT_V | 0x64 | V | NA | NA | NA | NA | NA | NA |


#### Current Monitoring

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| NF_INA233_P12V_STBY_CURR_A | 0x65 | A | NA | NA | NA | NA | NA | NA |
| NF_INA233_P3V3_STBY_CURR_A | 0x66 | A | NA | NA | NA | NA | NA | NA |
| NF_VR_P0V85_ASIC_CURR_A | 0x67 | A | NA | NA | NA | NA | NA | NA |
| NF_VR_PVDDQ_AB_CURR_A| 0x68 | A | NA | NA | NA | NA | NA | NA |
| NF_VR_P0V8_ASIC_CURR_A | 0x69 | A | NA | NA | NA | NA | NA | NA |
| NF_VR_PVDDQ_CD_CURR_A | 0x6A | A | NA | NA | NA | NA | NA | NA |


#### Power Monitoring

| Sensor Name | Sensor# | Unit | UNR | UCR | UNC | LNC | LCR | LNR |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| NF_INA233_P12V_STBY_PWR_W | 0x6B | W | NA | NA | NA | NA | NA | NA |
| NF_INA233_P3V3_STBY_PWR_W | 0x6C | W | NA | NA | NA | NA | NA | NA |
| NF_VR_P0V85_ASIC_PWR_W | 0x6D | W | NA | NA | NA | NA | NA | NA |
| NF_VR_PVDDQ_AB_PWR_W | 0x6E | W | NA | NA | NA | NA | NA | NA |
| NF_VR_P0V8_ASIC_PWR_W | 0x6F | W | NA | NA | NA | NA | NA | NA |
| NF_VR_PVDDQ_CD_PWR_W | 0x70 | W | NA | NA | NA | NA | NA | NA |
