## OCP Debug Card {#debugcard}

### GPIO Status

| Bit | Net name       | Direction (In the perspective of PCA9555) |
| --- | -------------- | ----------------------------------------- |
| P10 | FM_DBG_RST_BTN | Output                                    |
| P11 | FM_PWR_BTN     | Output                                    |
| P12 | SYS_PWROK      | Input                                     |
| P13 | RST_PLTRST     | Input                                     |
| P14 | DSW_PWROK      | Input                                     |
| P15 | FM_CATERR_MSMI | Input                                     |
| P16 | FM_SLPS3       | Input                                     |
| P17 | FM_UART_SWITCH | Input                                     |

### Critical Sensor

| Sensor Name        | Messages       |
| ------------------ | -------------- |
| Virtual_Inlet_Temp | INLET_TEMP:XXC |

### Critical SEL

Not support
