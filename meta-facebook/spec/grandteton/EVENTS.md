# Meta OpenBMC Events for Grand Teton

This document of platform specific event logs we can expect.

# Common Event
| **Event Category** | **Event** | **SEL Type** | **Comments** |
|---|---|---|---|
| Time Sync | Log event when BMC time syncs up with System RTC or NTP server when the difference is > 5 seconds |  |  |
| WDT | WDT Timer expired | MB | BIOS or Host OS can use this WDT timer "FRU: X, BIOS FRB2 Watchdog <ACTION>", "FRU: X, BIOS/POST Watchdog <ACTION>", "FRU: X, OS Load Watchdog <ACTION>", "FRU: X, SMS/OS Watchdog <ACTION>", "FRU: X, OEM watchdog <ACTION>",  <ACTION> can be one of: Timer expired Hard Reset Power Down Power Cycle |
|  | WDT expiry action taken | MB | BMC Watchdog reboot event: BMC Reboot detected \- unknown kernel flag \(0xXXXXXXXX\) BMC Reboot detected \- caused by reboot command BMC Reboot detected \- unknown reboot command flag \(0xXXXXXXXX\) BMC Reboot detected |
| Power State | Power OFF | MB | SERVER\_POWER\_OFF successful for FRU: 1 |
|  | Power ON | MB | SERVER\_POWER\_ON successful for FRU: 1 |
|  | Power Cycle | MB | SERVER\_POWER\_CYCLE successful for FRU: 1 |
|  | Power Reset | MB | SERVER\_POWER\_RESET successful for FRU: 1 |
|  | AC\-Lost | MB | SLED Powered OFF at <DATE TIME> |
|  | AC\-Recovery | MB | SLED Powered ON at <DATE TIME> |
|  | Warm Reboot | MB |  |
|  | Graceful Shutdown | MB | SERVER\_GRACEFUL\_SHUTDOWN successful for FRU: 1 |
|  | SLED Cycle | MB | power\-util       SLED\_CYCLE starting…  |
|  | Power Fail | MB | ASSERT: XXXXXX power rail fails discrete \- raised \- FRU: 1, num: 0x9C, snr: MB\_POWER\_FAIL val:0xX |
| BIOS Generated | Memory ECC Error | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), OEM data: \(<6 BYTE OEM data>\) <DESCRIPTION> See BIOS\_SEL Sheet for more details  |
|  | PCIe Error | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), OEM data: \(<6 BYTE OEM data>\) <DESCRIPTION> See BIOS\_SEL Sheet for more details  |
|  | POST Error | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), OEM data: \(<6 BYTE OEM data>\) <DESCRIPTION> See BIOS\_SEL Sheet for more details  |
|  | Other IIO Error | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), OEM data: \(<6 BYTE OEM data>\) <DESCRIPTION> See BIOS\_SEL Sheet for more details  |
|  | Machine Check Error | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), OEM data: \(<6 BYTE OEM data>\) <DESCRIPTION> See BIOS\_SEL Sheet for more details  |
|  | UPI Error Event | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), OEM data: \(<6 BYTE OEM data>\) <DESCRIPTION> See BIOS\_SEL Sheet for more details  |
|  | PPR or sPPR Event | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), OEM data: \(<6 BYTE OEM data>\) <DESCRIPTION> See BIOS\_SEL Sheet for more details  |
|  | Memory Correctable Error Log Disabled | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), OEM data: \(<6 BYTE OEM data>\) <DESCRIPTION> See BIOS\_SEL Sheet for more details  |
|  | DDR4 PPR | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), OEM data: \(<6 BYTE OEM data>\) <DESCRIPTION> See BIOS\_SEL Sheet for more details  |
|  | Software NMI | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), OEM data: \(<6 BYTE OEM data>\) <DESCRIPTION> See BIOS\_SEL Sheet for more details  |
| ME Generated | SPS FW Health | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), OEM data: \(<6 BYTE OEM data>\) <DESCRIPTION> Please refer to NM4\.0 \#550710 ch2\.6\.5 "ME Firmware Health Event" |
|  | NM Exception | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), OEM data: \(<6 BYTE OEM data>\) <DESCRIPTION> Please refer to NM4\.0 \#550710 ch3\.5\.15 |
|  | Pwr Thresh Evt | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), OEM data: \(<6 BYTE OEM data>\) <DESCRIPTION> Please refer to NM4\.0 \#550710 Table 3\-11 "Power Threshold Event" |
|  | CPU0/CPU1 Therm Status | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), OEM data: \(<6 BYTE OEM data>\) <DESCRIPTION> Please refer to NM4\.0 \#550710 Table A\-2 "CPU Thermal Status" and ch2\.10\.1 for more detail |
|  | ME Status events | MB | 1  FRU  YYYY\-MM\-DD HH:MM:SS  ipmid  SEL Entry: FRU: 1, Record: Standard \(0x02\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: ME\_POWER\_STATE \(0x16\), Event Data: \(000000\) RUNNING Assertion  1  FRU  YYYY\-MM\-DD HH:MM:SS  ipmid  SEL Entry: FRU: 1, Record: Standard \(0x02\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: ME\_POWER\_STATE \(0x16\), Event Data: \(000000\) RUNNING Deassertion   1  FRU  YYYY\-MM\-DD HH:MM:SS  ipmid  SEL Entry: FRU: 1, Record: Standard \(0x02\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: ME\_POWER\_STATE \(0x16\), Event Data: \(020000\) POWER\_OFF Assertion  1  FRU  YYYY\-MM\-DD HH:MM:SS  ipmid  SEL Entry: FRU: 1, Record: Standard \(0x02\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: ME\_POWER\_STATE \(0x16\), Event Data: \(020000\) POWER\_OFF Deassertion  |
|  | ME health | MB | ASSERT: ME Status \- Controller Unavailable on the mb DEASSERT: ME Status \- Controller Unavailable on the mb ASSERT: ME Status \- Controller Access Degraded or Unavailable on the mb DEASSERT: ME Status \- Controller Access Degraded or Unavailable on the mb |
|  | Host Partition Reset Warning | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), OEM data: \(<6 BYTE OEM data>\) <DESCRIPTION> Please refer to NM4\.0 \#550710 Table 2\-10 "Event Messages Definition" |
| Log | Log Clear | MB | Oct 19 19:57:43 log\-util: User cleared all logs |
|  |  | MB | Oct 25 21:30:10 log\-util: User cleared FRU: 1 logs  |
| Anolog Sensors | Temperature Sensors: \- Inlet, Outlet, PCH, VR, CPU core, DIMM, HSC, LAN card | MB | ASSERT: Lower Critical threshold \- raised \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME> DEASSERT: Lower Critical threshold \- settled \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME> ASSERT: Upper Critical threshold \- raised \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME>  DEASSERT: Upper Critical threshold \- settled \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME> |
|  | Power Sensors:\- VRs for CPU/DIMM, HSC, Package Power, INA230 | MB | ASSERT: Upper Critical threshold \- raised \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME>  DEASSERT: Upper Critical threshold \- settled \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME> |
|  | Current Sensors:\- VRs for CPU/DIMM, HSC, INA230 | MB | ASSERT: Upper Critical threshold \- raised \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME>  DEASSERT: Upper Critical threshold \- settled \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME> |
|  | Voltage Sensors:\- VRs for CPU/DIMM, HSC, INA230 | MB | ASSERT: Lower Critical threshold \- raised \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME> DEASSERT: Lower Critical threshold \- settled \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME> ASSERT: Upper Critical threshold \- raised \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME>  DEASSERT: Upper Critical threshold \- settled \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME> |
|  | Voltage Rail Monitor:\- P3V3, P5V, P12V, P1V05\_STBY, P1V8\_AUX, P3V3\_AUX, P5V\_AUX, P3V\_BAT | MB | ASSERT: Lower Critical threshold \- raised \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME> DEASSERT: Lower Critical threshold \- settled \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME> ASSERT: Upper Critical threshold \- raised \- TRAY: X, FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME>  DEASSERT: Upper Critical threshold \- settled \- TRAY: X, FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME> |
|  | Fan Speed | MB | ASSERT: Lower Critical threshold \- raised \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME> DEASSERT: Lower Critical threshold \- settled \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME> ASSERT: Upper Critical threshold \- raised \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME>  DEASSERT: Upper Critical threshold \- settled \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME> ASSERT: Upper Non Critical threshold \- raised \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME> DEASSERT: Upper Non Critical threshold \- settled \- FRU: X, num: 0xXX curr\_val: XX, thresh\_val: XX, snr: <SNR\_NAME> |
|  | Switch Inlet, AirFlow\- Need to findout how to monitor these sensors and report errors |  |  |
|  | FRB3 Failure | MB | ASSERT: FRB3 failure discrete \- raised \- FRU: 1 |
| User Actions | Power Button In | MB | ASSERT: GPIOP0 \- SYS\_BMC\_PWRBTN\_IN DEASSERT: GPIOP0 \- SYS\_BMC\_PWRBTN\_IN |
|  | Reset Button | MB | ASSERT: GPIOP2 \- ID\_RST\_BTN\_BMC\_IN DEASSERT: GPIOP2 \- ID\_RST\_BTN\_BMC\_IN |
|  | Debug Card Insert/Removal | MB | ASSERT: GPIOZ6 \- FM\_DEBUG\_PORT\_PRSNT\_N\_IN DEASSERT: GPIOZ6 \-FM\_DEBUG\_PORT\_PRSNT\_N\_IN Debug Card Extraction Debug Card Insertion  |
|  | Base OS Installation started | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), Event data: \(<6 BYTE OEM data>\) Base OS/Hypervisor Installation started |
|  | Base OS Installation completed | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), Event data: \(<6 BYTE OEM data>\) Base OS/Hypervisor Installation completed |
|  | Base OS Installation aborted | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), Event data: \(<6 BYTE OEM data>\) Base OS/Hypervisor Installation aborted |
|  | Base OS Installation failed | MB | SEL Entry: FRU: X, Record: <RECORD TYPE> \(0xXX\), Time: YYYY\-MM\-DD HH:MM:SS, Sensor: <NAME> \(0xXX\), Event data: \(<6 BYTE OEM data>\) Base OS/Hypervisor Installation failed |
|  | Reset BMC To Default | MB | Reset BMC data to default factory settings |
|  | Cold Reset | MB | BMC Cold Reset |
| BMC Generated | BMC Memory utilization | MB | ASSERT: BMC Memory utilization \(XX%\) exceeds the threshold \(XX%\)\. DEASSERT: BMC Memory utilization \(XX%\) is under the threshold \(XX%\)\. |
|  | BMC CPU utilization | MB | ASSERT: BMC CPU utilization \(XX%\) exceeds the threshold \(XX%\)\. DEASSERT: BMC CPU utilization \(XX%\) is under the threshold \(XX%\)\. |
|  | Verified boot status | MB | ASSERT: Verified boot failure \(XX%:XX%\) ASSERT: Verification of BMC image failed Notable errors: \(7,76\) \- TPM is not present \(3\.35\) \- CS1 image is unsigned \(9,91\) \- Rollback was prevented \(4,43\) \- U\-Boot was not verified using the intermediate keys \(3,30\) \- Invalid FDT magic number for U\-Boot FIT at 0x28080000 |
|  | BMC FW update | MB | BMC fw upgrade initiated BMC fw upgrade completed\. Version: XX\-XX ex: BMC fw upgrade completed\. Version: inspirationpoint\-v2022\.35\.1 |
|  | BIOS FW update  | MB | Component bios upgrade initiated Component bios upgrade completed |
|  | MB CPLD FW update | MB | Component mb\_cpld upgrade initiated Component mb\_cpld upgrade completed |
|  | AMD CPU VR update | MB | Component cpu0\_pvdd11 upgrade initiated Component cpu0\_pvdd11 upgrade completed Component cpu1\_pvdd11 upgrade initiated Component cpu1\_pvdd11 upgrade completed Component cpu0\_vcore0 upgrade initiated Component cpu0\_vcore0 upgrade completed Component cpu0\_vcore1 upgrade initiated Component cpu0\_vcore1 upgrade completed Component cpu1\_vcore0 upgrade initiated Component cpu1\_vcore0 upgrade completed Component cpu1\_vcore1 upgrade initiated Component cpu1\_vcore1 upgrade completed |
|  | NIC FW update | MB | Component nicX upgrade initiated Component nicX upgrade completed |
|  | SWB CPLD FW update | MB | Component swb\_cpld upgrade initiated Component swb\_cpld upgrade completed |
|  | BIC FW update | MB | Component bic upgrade initiated Component bic upgrade completed |
|  | SWB VR FW update | MB | Component pex01\_vcc upgrade initiated Component pex01\_vcc upgrade completed Component pex23\_vcc upgrade initiated Component pex23\_vcc force upgrade completed |
|  | PEX FW update | MB | Component pexX upgrade initiated Component pexX upgrade completed |
|  | Debug Card FW update | MB | Component mcu upgrade initiated Component mcu upgrade completed |
|  | Debug Card Bootloader FW update | MB | Component mcubl upgrade initiated Component mcubl upgrade completed |
|  | Reset Button | MB | Reset Button pressed |
|  | Power Button | MB | Power Button pressed |
|  | fan event | MB | XX fans failed Fan XX dead,  \-1 RPM Fan XX has recovered FRU: 1 , FAN\_BP0 not present\. FRU: 1 , FAN\_BP0 present\. FRU: 1 , FAN\_BP1 not present\. FRU: 1 , FAN\_BP1 present\. |
|  | PLDM event | MB | FRU: X NIC AEN Supported: 0x7, AEN Enable Mask=0x7 FRU: X PLDM type supported = 0x75 FRU: X PLDM type 0 version = 1\.0\.0\.0 FRU: X PLDM type 2 version = 1\.1\.1\.0 FRU: X PLDM type 4 version = 1\.0\.0\.0 FRU: X PLDM type 5 version = 1\.0\.0\.0 FRU: X PLDM type 6 version = 1\.0\.1\.0 FRU: X PLDM sensor monitoring enabled |


# GPIO Event
| **FRU** | **GPIO Name** | **GPIO Description** | **Event** |
|---|---|---|---|
| MB | GPIOP2 | FP_RST_BTN_IN_N | FRU: X ASSERT: GPIOP2- ID_RST_BTN_BMC_IN FRU: X DEASSERT: GPIOP2 - ID_RST_BTN_BMC_IN |
| MB | GPIOP0 | FP_PWR_BTN_IN_N | FRU: X ASSERT: GPIOP0 - SYS_BMC_PWRBTN_IN FRU: X DEASSERT: GPIOP0 - SYS_BMC_PWRBTN_IN |
| MB | SGPIO188 | IRQ_UV_DETECT_N | Record time delay 300ms, FRU: 1 HSC Under Voltage Warning: Assertion Record time delay 300ms, FRU: 1 HSC Under Voltage Warning: Deassertion |
| MB | SGPIO178 | IRQ_OC_DETECT_N | Record time delay 300ms, FRU: 1 HSC Surge Current Warning: Assertion  Record time delay 300ms, FRU: 1 HSC Surge Current Warning: Deassertion |
| MB | SGPIO36 | IRQ_HSC_FAULT_N | Record time delay 300ms, FRU: X ASSERT: SGPIO36 - IRQ_UV_DETECT_N Record time delay 300ms, FRU: X DEASSERT: SGPIO36 - IRQ_UV_DETECT_N |
| MB | SGPIO2 | IRQ_HSC_ALERT_N | Record time delay 300ms, FRU: 1 HSC OC Warning: Assertion  Record time delay 300ms, FRU: 1 HSC OC Warning: Deassertion |
| MB | GPIOC5 | FM_CPU_CATERR_N | ASSERT: IERR/CATERR ASSERT: MCERR/CATERR |
| MB | SGPIO202 | FM_CPU0_PROCHOT_N | FRU: X ASSERT: SGPIO202 - H_CPU0_PROCHOT_LVC1_N FRU: X DEASSERT: SGPIO202 - H_CPU0_PROCHOT_LVC1_N (reason: UV/OC) |
| MB | SGPIO186 | FM_CPU1_PROCHOT_N | FRU: X ASSERT: SGPIO186 - H_CPU1_PROCHOT_LVC1_N FRU: X DEASSERT: SGPIO186 - H_CPU1_PROCHOT_LVC1_N (reason: UV/OC) |
| MB | SGPIO136 | FM_CPU0_THERMTRIP_N | Record time delay 300ms, FRU: X ASSERT: SGPIO136 - H_CPU0_THERMTRIP_LVC1_N |
| MB | SGPIO118 | FM_CPU1_THERMTRIP_N | Record time delay 300ms, FRU: X ASSERT: SGPIO118 - H_CPU1_THERMTRIP_LVC1_N |
| MB | SGPIO144 | FM_CPU_ERR0_N | Record time delay 300ms, FRU: X ASSERT: SGPIO144 - H_CPU_ERR0_LVC2_N |
| MB | SGPIO142 | FM_CPU_ERR1_N | Record time delay 300ms, FRU: X ASSERT: SGPIO142 - H_CPU_ERR1_LVC2_N |
| MB | SGPIO140 | FM_CPU_ERR2_N | Record time delay 300ms, FRU: X ASSERT: SGPIO140 - H_CPU_ERR2_LVC2_N |
| MB | SGPIO138 | FM_CPU0_MEM_HOT_N | FRU: X ASSERT: SGPIO138 - H_CPU0_MEMHOT_OUT_LVC1_N FRU: X DEASSERT: SGPIO138 - H_CPU0_MEMHOT_OUT_LVC1_N |
| MB | SGPIO124 | FM_CPU1_MEM_HOT_N | FRU: X ASSERT: SGPIO124 - H_CPU1_MEMHOT_OUT_LVC1_N FRU: X DEASSERT: SGPIO124 - H_CPU1_MEMHOT_OUT_LVC1_N |
| MB | SGPIO122 | FM_CPU0_MEM_THERMTRIP_N | Record time delay 300ms, FRU: X ASSERT: SGPIO122 - H_CPU0_MEMTRIP_LVC1_N |
| MB | SGPIO158 | FM_CPU1_MEM_THERMTRIP_N | Record time delay 300ms, FRU: X ASSERT: SGPIO158 - H_CPU1_MEMTRIP_LVC1_N |
| MB | SGPIO20 | FM_SYS_THROTTLE_R_N | Record time delay 300ms, FRU: X ASSERT: SGPIO20 - FM_SYS_THROTTLE_R_N Record time delay 300ms, FRU: X DEASSERT: SGPIO20 - FM_SYS_THROTTLE_R_N |
| MB | SGPIO30 | FM_PCHHOT_R_N | Record time delay 300ms, FRU: X ASSERT: SGPIO30 - FM_PCHHOT_R_N Record time delay 300ms, FRU: X DEASSERT: SGPIO30 - FM_PCHHOT_R_N |
| MB | SGPIO28 | FM_PCH_BMC_THERMTRIP_N | FRU: X ASSERT: SGPIO28- FM_PCH_BMC_THERMTRIP_N FRU: X DEASSERT: SGPIO28 - FM_PCH_BMC_THERMTRIP_N |
| MB | SGPIO154 | IRQ_CPU0_VRHOT_N | Record time delay 300ms, FRU: 1 CPU0 PVCCIN VR HOT Warning: Assertion  Record time delay 300ms, FRU: 1 CPU0 PVCCIN VR HOT Warning: Deassertion |
| MB | SGPIO120 | IRQ_CPU1_VRHOT_N | Record time delay 300ms, FRU: 1 CPU1 PVCCIN VR HOT Warning: Assertion  Record time delay 300ms, FRU: 1 CPU1 PVCCIN VR HOT Warning: Deassertion |
| MB | SGPIO12 | IRQ_PVCCD_CPU0_VRHOT_LVC3_N | Record time delay 300ms, FRU: 1 CPU0 PVCCD VR HOT Warning: Assertion  Record time delay 300ms, FRU: 1 CPU0 PVCCD VR HOT Warning: Deassertion |
| MB | SGPIO18 | IRQ_PVCCD_CPU1_VRHOT_LVC3_N | Record time delay 300ms, FRU: 1 CPU1 PVCCD VR HOT Warning: Assertion  Record time delay 300ms, FRU: 1 CPU1 PVCCD VR HOT Warning: Deassertion |
| MB | SGPIO200 | RST_PLTRST_PLD_N | DEASSERT: SGPIO200 - RST_PLTRST_PLD_N ASSERT: SGPIOR200 - RST_PLTRST_PLD_N |
| MB | SGPIO116 | PWRGD_CPUPWRGD_LVC2_R1 | DEASSERT: SGPIO116 - PWRGD_CPUPWRGD_LVC2_R1 ASSERT: SGPIO116 - PWRGD_CPUPWRGD_LVC2_R1 |
| MB | SGPIO112 | FM_CPU0_SKTOCC | DEASSERT: SGPIO112 - FM_CPU0_SKTOCC_LVT3_PLD_N ASSERT: SGPIO112 - FM_CPU0_SKTOCC_LVT3_PLD_N |
| MB | SGPIO114 | FM_CPU1_SKTOCC | DEASSERT: SGPIO114 - FM_CPU1_SKTOCC_LVT3_PLD_N ASSERT: SGPIO114 - FM_CPU1_SKTOCC_LVT3_PLD_N |
| MB | SGPIO146 | FM_BIOS_POST_CMPLT_BUF_R_N | ASSERT: SGPIO146 - FM_BIOS_POST_CMPLT_BUF_R_N DEASSERT: SGPIO146 - FM_BIOS_POST_CMPLT_BUF_R_N |
|  |  |  |  |
| SWB | SGPIO32 | BIC_READY | FRU: X ASSERT: SGPIO32 - FM_SWB_BIC_READY_ISO_R_N FRU: X DEASSERT: SGPIO32 - FM_SWB_BIC_READY_ISO_R_N |
| SWB | SWB_CABLE_G | CABLE_PRSNT_G | FRU: 1 , present / not present SWB_CABLE_G |
| SWB | SWB CABLE_B | CABLE_PRSNT_B | FRU: 1 , present / not present SWB_CABLE_B |
| SWB | SWB_CABLE_C | CABLE_PRSNT_C | FRU: 1 , present / not present SWB_CABLE_C |
| SWB | SWB_CABLE_F | CABLE_PRSNT_F | FRU: 1 , present / not present SWB_CABLE_F |
|  |  |  |  |
| SCM | GPIO18A0 | FP_AC_PWR_BMC_BTN | FRU: X ASSERT: GPIO18A0 - AC_PWR_BMC_BTN_N FRU: X DEASSERT: GPIO18A0 - AC_PWR_BMC_BTN_N |
| SCM | SGPIO106 | FM_UARTSW_LSB_N | FRU: X ASSERT: SGPIO106 - FM_UARTSW_LSB_N FRU: X DEASSERT: SGPIO106 - FM_UARTSW_LSB_N |
| SCM | SGPIO108 | FM_UARTSW_MSB_N | FRU: X ASSERT: SGPIO108 - FM_UARTSW_MSB_N FRU: X DEASSERT: SGPIO108 - FM_UARTSW_MSB_N |
| SCM | GPIOZ6 | FM_POST_CARD_PRES_N | FRU: X ASSERT: GPIOZ6 - FM_DEBUG_PORT_PRSNT_N_IN FRU: X DEASSERT: GPIOZ6 - FM_DEBUG_PORT_PRSNT_N_IN |
|  |  |  |  |
| BP | FAN_BP1 | FAN_BP1_PRSNT | FRU: 1 , present / not present FAN_BP0 |
| BP | FAN_BP2 | FAN_BP2_PRSNT | FRU: 1 , present / not present FAN_BP1 |
| BP | FAN0 | FAN0_PRSNT | FRU: 1 , present / not present FAN0 |
| BP | FAN1 | FAN1_PRSNT | FRU: 1 , present / not present FAN1 |
| BP | FAN2 | FAN2_PRSNT | FRU: 1 , present / not present FAN2 |
| BP | FAN3 | FAN3_PRSNT | FRU: 1 , present / not present FAN3 |
| BP | FAN4 | FAN4_PRSNT | FRU: 1 , present / not present FAN4 |
| BP | FAN5 | FAN5_PRSNT | FRU: 1 , present / not present FAN5 |
| BP | FAN6 | FAN6_PRSNT | FRU: 1 , present / not present FAN6 |
| BP | FAN7 | FAN7_PRSNT | FRU: 1 , present / not present FAN7 |
| BP | FAN8 | FAN8_PRSNT | FRU: 1 , present / not present FAN8 |
| BP | FAN9 | FAN9_PRSNT | FRU: 1 , present / not present FAN9 |
| BP | FAN10 | FAN10_PRSNT | FRU: 1 , present / not present FAN10 |
| BP | FAN11 | FAN11_PRSNT | FRU: 1 , present / not present FAN11 |
| BP | FAN12 | FAN12_PRSNT | FRU: 1 , present / not present FAN12 |
| BP | FAN13 | FAN13_PRSNT | FRU: 1 , present / not present FAN13 |
| BP | FAN14 | FAN14_PRSNT | FRU: 1 , present / not present FAN14 |
| BP | FAN15 | FAN15_PRSNT | FRU: 1 , present / not present FAN15 |
|  |  |  |  |
| HGX | SGPIO64 | HMC_READY | FRU: %d HMC_READY - ASSERT Timed out |
| HGX | SGPIO40 | GPU_FPGA_THERM_OVERT | Record time delay 300ms, FRU: X ASSERT: SGPIO40 - GPU_FPGA_THERM_OVERT_ISO_R_N |
| HGX | SGPIO42 | GPU_FPGA_DEVIC_OVERT | Record time delay 300ms, FRU: X ASSERT: SGPIO42 - GPU_FPGA_OVERT_ISO_R_N |
| HGX | GPU_CABLE_H | CABLE_PRSNT_H | FRU: 1 , present / not present GPU_CABLE_H |
| HGX | GPU_CABLE_A | CABLE_PRSNT_A | FRU: 1 , present / not present GPU_CABLE_A |
| HGX | GPU_CABLE_D | CABLE_PRSNT_D | FRU: 1 , present / not present GPU_CABLE_D |
| HGX | GPU_CABLE_E | CABLE_PRSNT_E | FRU: 1 , present / not present GPU_CABLE_E |
| HGX | SGPIO208 | FM_SMB_1_ALERT_GPU | Record time delay 300ms, FRU: X ASSERT: SGPIO208 - FM_SMB_1_ALERT_GPU_ISO_N Record time delay 300ms, FRU: X DEASSERT: SGPIO208 - FM_SMB_1_ALERT_GPU_ISO_N |
| HGX | SGPIO210 | FM_SMB_2_ALERT_GPU | Record time delay 300ms, FRU: X ASSERT: SGPIO210 - FM_SMB_2_ALERT_GPU_ISO_N Record time delay 300ms, FRU: X DEASSERT: SGPIO210 - FM_SMB_2_ALERT_GPU_ISO_N |
