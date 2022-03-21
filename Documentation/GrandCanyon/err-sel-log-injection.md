# Grand Canyon error injection guide

## BIC health SEL
* Discription
    * healthd would check BIC health by monitoring:
    1. BIC ready pin
    1. BIC heartbeat
    1. BIC self test result
* Low level error injection
    * Pull BIC_READY_IN  to low and wait for 3min to inject BIC health fail.
    ```
    gpiocli -c aspeed-gpio -s BIC_READY_IN set-direction out
    gpiocli -c aspeed-gpio -s BIC_READY_IN set-value 0
    ```
* SEL injection
    ```
    root@bmc-oob:~# logger -s -p user.crit -t healthd "FRU 1 BIC reset by BIC health monitor due to health check failed in following order: BIC ready, BIC ready, BIC ready"
    ```
* Example SEL
    ```
    root@bmc-oob:~# log-util all --print
    0    all      2021-08-25 23:58:43    healthd          FRU 1 BIC reset by BIC health monitor due to health check failed in following order: BIC ready, BIC ready, BIC ready
    ```

## BMC DDR ECC
* Discription
    * There is a register MCRB0 that could use to inject ECC correctable error in BMC.
    * Need to unlock the register by writing 0xFC600309 to MCR00 first.
    * The base address of MCR is 0x1e6e0000.
* Low level error injection
    * Unlock SDRAM registers
    ```
    root@bmc-oob:~# devmem 0x1e6e0000 32 0xFC600309
    ```
    * Inject ECC 1st bit error at position 0
    ```
    root@bmc-oob:~# devmem 0x1e6e00B0 32 0x80
    ```
* SEL injection
    ```
    root@bmc-oob:~# logger -s -p user.warn -t healthd "ECC Recoverable Error occurred (over 0%) Counter = 2 Address of last recoverable ECC error = 0x9093a8"
    ```
* Example SEL
    ```
    root@bmc-oob:~# cat /var/log/messages | grep ECC
    2018 Mar 9 04:36:06 bmc-oob. user.warning grandcanyon-547152bee8d-dirty: healthd: ECC Recoverable Error occurred (over 0%) Counter = 2 Address of last recoverable ECC error = 0x9093a8
    ```

## Sensor UCR assertion from expander
* Discription
    * The sensors on SCC and DPB are monitoring by expander, except for SCC IOC temperature.
    * Expander firmware provides commands to modify sensor value that send to BMC.
    * Modify sensor value to exceed Upper Critical threshold so that BMC could receive UCR SEL from expander.
* Low level error injection
    * Connect to SAS smart console from OpenBMC
    ```
    root@bmc-oob:~# sol-util scc_exp_smart
    ```
    * Check sensor reading with threshold
        * td: Show threshold
        * cs: Current sensors
        * vs: Voltage sensors
        * ts: Temperature sensors
    ```
     cmd >hwmonitor cs td
    SES Element Information:
    
    Number of Current Element Device : 3
            Element  0, SCC_HSC_Current               , Current:   238 (10mA), H/L_Crit:   760 / NA, H/L_Warn:  1153 / NA, Status: 1 (OK)
            Element  1, DPB_HSC_Current               , Current:  1620 (10mA), H/L_Crit:  7000 / NA, H/L_Warn: 10617 / NA, Status: 1 (OK)
            Element  2, DPB_HSC_Current_CLIP          , Current:  4103 (10mA), H/L_Crit: 16500 / NA, H/L_Warn: 25025 / NA, Status: 1 (OK)

    ```
    * Set the value of DPB_HSC_Current to 8000 (10mA) to exceed Upper Critical threshold to assert UCR SEL.
    ```
    # assert
     cmd > wwint mock cs set 1 8000
    ```
    * Disable the setting of DPB_HSC_Current to deassert UCR SEL.
    ```
    # deassert
     cmd > wwint mock cs set 1 disable
    ```
* SEL injection
    ```
    # assert
     root@bmc-oob:~# logger -s -p user.crit -t ipmid "ASSERT: Upper Critical threshold - raised - FRU: 4, num: 0xdb curr_val: 80000 mA, thresh_val: 70000 mA, snr: DPB_HSC_Current"

    # deassert
     root@bmc-oob:~# logger -s -p user.crit -t ipmid "DEASSERT: Upper Critical threshold - raised - FRU: 4, num: 0xdb curr_val: 16250 mA, thresh_val: 70000 mA, snr: DPB_HSC_Current"

    ```
* Example SEL
    ```
    # assert
     root@bmc-oob:~# log-util all --print
     4 dpb 2018-03-09 08:32:55 ipmid ASSERT: Upper Critical threshold - raised - FRU: 4, num: 0xdb curr_val: 80000 mA, thresh_val: 70000 mA, snr: DPB_HSC_Current
    
    # deassert
     root@bmc-oob:~# log-util all --print
     4 dpb 2018-03-09 08:34:51 ipmid DEASSERT: Upper Critical threshold - settled - FRU: 4, num: 0xdb curr_val: 16250 mA, thresh_val: 70000 mA, snr: DPB_HSC_Current

    ```

## Inject SEL of PROC FAIL
* Discription
    * Use ipmi command to inject SEL of PROC FAIL:
        * FRB3
* Low level error injection
    * None
* SEL injection
    * Send command in host
    ```
    root@bmc-oob:~# sol-util server
    ...
    [root@PT5-1a ~]#
    ```
    * PROC FAIL
    ```
    # assert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x40 0x00 0x04 0x07 0x65 0x6F 0x04 0xFF 0xFF
     42 00
    
    # deassert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x40 0x00 0x04 0x07 0x65 0xEF 0x04 0xFF 0xFF
     43 00
    ```
* Example SEL
    * PROC FAIL
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:22:21    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:22:21, Sensor: PROC_FAIL (0x65), Event Data: (04FFFF) FRB3,  Assertion 
    1    server   2022-01-23 20:22:24    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:22:24, Sensor: PROC_FAIL (0x65), Event Data: (04FFFF) FRB3,  Deassertion 
    ```

## Inject SEL of ME power state
* Discription
    * Use ipmi command to inject SEL of ME power state:
        * RUNNING Assertion / Deassertion
        * POWER_OFF Assertion / Deassertion
* Low level error injection
    * None
* SEL injection
    * Send command in host
    ```
    root@bmc-oob:~# sol-util server
    ...
    [root@PT5-1a ~]#
    ```
    * RUNNING Assertion
    ```
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x16 0x00 0x00 0x00 0x00
     18 00
    ```
    * RUNNING Deassertion
    ```
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x16 0x80 0x00 0x00 0x00
     19 00
    ```
    * POWER_OFF Assertion
    ```
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x16 0x00 0x02 0x00 0x00
     1a 00
    ```
    * POWER_OFF Deassertion
    ```
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x16 0x80 0x02 0x00 0x00
     1b 00
    ```
* Example SEL
    * RUNNING Assertion
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:19:35    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:19:35, Sensor: ME_POWER_STATE (0x16), Event Data: (000000) RUNNING Assertion 
    ```
    * RUNNING Deassertion
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:19:38    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:19:38, Sensor: ME_POWER_STATE (0x16), Event Data: (000000) RUNNING Deassertion 
    ```
    * POWER_OFF Assertion
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:19:41    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:19:41, Sensor: ME_POWER_STATE (0x16), Event Data: (020000) POWER_OFF Assertion 
    ```
    * POWER_OFF Deassertion
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:19:45    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:19:45, Sensor: ME_POWER_STATE (0x16), Event Data: (020000) POWER_OFF Deassertion 
    ```

## Inject SEL of VR HOT
* Discription
    * Use ipmi command to inject SEL of VRHOT:
        * CPU VCCIN VR HOT
        * CPU VCCIO VR HOT
        * DIMM AB Memory VR HOT
        * DIMM DE Memory VR HOT
* Low level error injection
    * None
* SEL injection
    * Send command in host
    ```
    root@bmc-oob:~# sol-util server
    ...
    [root@PT5-1a ~]#
    ```
    * CPU VCCIN VR HOT
    ```
    # assert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0xB4 0x00 0x00 0xFF 0xFF
     1e 00
     
    # deassert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0xB4 0x80 0x00 0xFF 0xFF
     1f 00
    ```
    * CPU VCCIO VR HOT
    ```
    # assert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0xB4 0x00 0x01 0xFF 0xFF
     20 00
     
    # deassert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0xB4 0x80 0x01 0xFF 0xFF
     21 00
    ```
    * DIMM AB Memory VR HOT
    ```
    # assert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0xB4 0x00 0x02 0xFF 0xFF
     22 00
     
    # deassert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0xB4 0x80 0x02 0xFF 0xFF
     23 00
    ```
    * DIMM DE Memory VR HOT
    ```
    # assert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0xB4 0x00 0x03 0xFF 0xFF
     24 00
     
    # deassert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0xB4 0x80 0x03 0xFF 0xFF
     25 00
    ```
* Example SEL
    * CPU VCCIN VR HOT
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:20:08    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:20:08, Sensor: VR_HOT (0xB4), Event Data: (00FFFF) CPU VCCIN VR HOT Warning Assertion 
    1    server   2022-01-23 20:20:11    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:20:11, Sensor: VR_HOT (0xB4), Event Data: (00FFFF) CPU VCCIN VR HOT Warning Deassertion  
    ```
    * CPU VCCIO VR HOT
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:20:15    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:20:15, Sensor: VR_HOT (0xB4), Event Data: (01FFFF) CPU VCCIO VR HOT Warning Assertion 
    1    server   2022-01-23 20:20:18    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:20:18, Sensor: VR_HOT (0xB4), Event Data: (01FFFF) CPU VCCIO VR HOT Warning Deassertion 
    ```
    * DIMM AB Memory VR HOT
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:20:21    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:20:21, Sensor: VR_HOT (0xB4), Event Data: (02FFFF) DIMM AB Memory VR HOT Warning Assertion 
    1    server   2022-01-23 20:20:25    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:20:25, Sensor: VR_HOT (0xB4), Event Data: (02FFFF) DIMM AB Memory VR HOT Warning Deassertion 
    ```
    * DIMM DE Memory VR HOT
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:20:28    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:20:28, Sensor: VR_HOT (0xB4), Event Data: (03FFFF) DIMM DE Memory VR HOT Warning Assertion 
    1    server   2022-01-23 20:20:31    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:20:31, Sensor: VR_HOT (0xB4), Event Data: (03FFFF) DIMM DE Memory VR HOT Warning Deassertion  
    ```

## Inject SEL of SYSTEM STATUS
* Discription
    * Use ipmi command to inject SEL of SYSTEM STATUS:
        * System FIVR fault
        * Surge Current Warning
        * PCH prochot
        * Under Voltage Warning
        * OC Warning
        * OCP Fault Warning
        * Firmware Assertion
        * HSC fault
        * VR WDT
        * E1.S device 255 VPP Power Control
        * VCCIO fault
        * SMI stuck low over 90s
* Low level error injection
    * None
* SEL injection
    * Send command in host
    ```
    root@bmc-oob:~# sol-util server
    ...
    [root@PT5-1a ~]#
    ```
    * System FIVR fault
    ```
    # assert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x00 0x01 0xFF 0xFF
     26 00
     
    # deassert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x80 0x01 0xFF 0xFF
     27 00
    ```
    * Surge Current Warning
    ```
    # assert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x00 0x02 0xFF 0xFF
     28 00

    # deassert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x80 0x02 0xFF 0xFF
     29 00
    ```
    * PCH prochot
    ```
    # assert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x00 0x03 0xFF 0xFF
     2a 00

    # deassert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x80 0x03 0xFF 0xFF
     2b 00
    ```
    * Under Voltage Warning
    ```
    # assert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x00 0x04 0xFF 0xFF
     2c 00

    # deassert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x80 0x04 0xFF 0xFF
     2d 00
    ```
    * OC Warning
    ```
    # assert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x00 0x05 0xFF 0xFF
     2e 00

    # deassert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x80 0x05 0xFF 0xFF
     2f 00
    ```
    * OCP Fault Warning
    ```
    # assert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x00 0x06 0xFF 0xFF
     30 00

    # deassert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x80 0x06 0xFF 0xFF
     31 00
    ```
    * Firmware Assertion
    ```
    # assert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x00 0x07 0xFF 0xFF
     32 00

    # deassert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x80 0x07 0xFF 0xFF
     33 00
    ```
    * HSC fault
    ```
    # assert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x00 0x08 0xFF 0xFF
     34 00

    # deassert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x80 0x08 0xFF 0xFF
     35 00
    ```
    * VR WDT
    ```
    # assert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x00 0x0A 0xFF 0xFF
     38 00

    # deassert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x80 0x0A 0xFF 0xFF
     39 00
    ```
    * E1.S device 255 VPP Power Control
    ```
    # assert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x00 0x0B 0xFF 0xFF
     3a 00

    # deassert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x80 0x0B 0xFF 0xFF
     3b 00
    ```
    * VCCIO fault
    ```
    # assert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x00 0x0D 0xFF 0xFF
     3e 00

    # deassert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x80 0x0D 0xFF 0xFF
     3f 00
    ```
    * SMI stuck low over 90s
    ```
    # assert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x00 0x0E 0xFF 0xFF
     40 00

    # deassert
    [root@PT5-1a ~]# ipmitool raw 0x0A 0x44 0x00 0x00 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x46 0x80 0x0E 0xFF 0xFF
     41 00
    ```
* Example SEL
    * System FIVR fault
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:20:41    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:20:41, Sensor: SYSTEM_STATUS (0x46), Event Data: (01FFFF) System FIVR fault Assertion 
    1    server   2022-01-23 20:20:45    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:20:45, Sensor: SYSTEM_STATUS (0x46), Event Data: (01FFFF) System FIVR fault Deassertion
    ```
    * Surge Current Warning
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:20:48    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:20:48, Sensor: SYSTEM_STATUS (0x46), Event Data: (02FFFF) Surge Current Warning Assertion 
    1    server   2022-01-23 20:20:51    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:20:51, Sensor: SYSTEM_STATUS (0x46), Event Data: (02FFFF) Surge Current Warning Deassertion 
    ```
    * PCH prochot
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:20:55    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:20:55, Sensor: SYSTEM_STATUS (0x46), Event Data: (03FFFF) PCH prochot Assertion 
    1    server   2022-01-23 20:20:58    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:20:58, Sensor: SYSTEM_STATUS (0x46), Event Data: (03FFFF) PCH prochot Deassertion 
    ```
    * Under Voltage Warning
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:21:01    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:21:01, Sensor: SYSTEM_STATUS (0x46), Event Data: (04FFFF) Under Voltage Warning Assertion 
    1    server   2022-01-23 20:21:05    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:21:05, Sensor: SYSTEM_STATUS (0x46), Event Data: (04FFFF) Under Voltage Warning Deassertion 
    ```
    * OC Warning
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:21:08    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:21:08, Sensor: SYSTEM_STATUS (0x46), Event Data: (05FFFF) OC Warning Assertion 
    1    server   2022-01-23 20:21:11    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:21:11, Sensor: SYSTEM_STATUS (0x46), Event Data: (05FFFF) OC Warning Deassertion
    ```
    * OCP Fault Warning
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:21:15    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:21:15, Sensor: SYSTEM_STATUS (0x46), Event Data: (06FFFF) OCP Fault Warning Assertion 
    1    server   2022-01-23 20:21:18    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:21:18, Sensor: SYSTEM_STATUS (0x46), Event Data: (06FFFF) OCP Fault Warning Deassertion
    ```
    * Firmware Assertion
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:21:21    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:21:21, Sensor: SYSTEM_STATUS (0x46), Event Data: (07FFFF) Firmware Assertion 
    1    server   2022-01-23 20:21:25    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:21:25, Sensor: SYSTEM_STATUS (0x46), Event Data: (07FFFF) Firmware Deassertion
    ```
    * HSC fault
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:21:28    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:21:28, Sensor: SYSTEM_STATUS (0x46), Event Data: (08FFFF) HSC fault Assertion 
    1    server   2022-01-23 20:21:31    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:21:31, Sensor: SYSTEM_STATUS (0x46), Event Data: (08FFFF) HSC fault Deassertion 
    ```
    * VR WDT
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:21:41    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:21:41, Sensor: SYSTEM_STATUS (0x46), Event Data: (0AFFFF) VR WDT Assertion 
    1    server   2022-01-23 20:21:44    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:21:44, Sensor: SYSTEM_STATUS (0x46), Event Data: (0AFFFF) VR WDT Deassertion
    ```
    * E1.S device 255 VPP Power Control
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:21:48    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:21:48, Sensor: SYSTEM_STATUS (0x46), Event Data: (0BFFFF) E1.S device 255 VPP Power Control Assertion 
    1    server   2022-01-23 20:21:51    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:21:51, Sensor: SYSTEM_STATUS (0x46), Event Data: (0BFFFF) E1.S device 255 VPP Power Control Deassertion
    ```
    * VCCIO fault
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:22:01    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:22:01, Sensor: SYSTEM_STATUS (0x46), Event Data: (0DFFFF) VCCIO fault Assertion 
    1    server   2022-01-23 20:22:04    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:22:04, Sensor: SYSTEM_STATUS (0x46), Event Data: (0DFFFF) VCCIO fault Deassertion
    ```
    * SMI stuck low over 90s
    ```
    root@bmc-oob:~# log-util all --print
    1    server   2022-01-23 20:22:07    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:22:07, Sensor: SYSTEM_STATUS (0x46), Event Data: (0EFFFF) SMI stuck low over 90s Assertion 
    1    server   2022-01-23 20:22:11    ipmid            SEL Entry: FRU: 1, Record: Standard (0x02), Time: 2022-01-23 20:22:11, Sensor: SYSTEM_STATUS (0x46), Event Data: (0EFFFF) SMI stuck low over 90s Deassertion
    ```