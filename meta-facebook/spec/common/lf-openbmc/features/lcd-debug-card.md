# OCP Debug Card

**Frame list**
- Frame 1: Uart Selection
- Frame 2: System Info
- Frame 3: Critical SEL
- Frame 4: Critical Sensor
- Frame 5: Post Code
- Frame 6: Dimm Loop
- Frame 7: IO Status
- User Setting
	- Boot Order
	- Power Policy

Users can view various frames using the onboard detector switch (the black button on the left-hand side), which provides four directional functions: **up, down, left, right,** and one selection function: **Select.**

Frames are displayed in the following order on the panel from left to right: Frame 1: Uart Selection ->Frame 2: System Info->Frame 3: Critical SEL->Frame 4: Critical Sensor->Frame 5: Post Code->Frame 6: Dimm Loop->Frame 7: IO Status->User Setting

### 5-way button operation

**Left, Right -**
Used for switching to different frames.

**Up, Down -**
Move up or down to switch pages and view more content within the current frame.

**Select -**
Press the middle of the button to select an item. This operation is used on the User Setting Frame to select the boot order or power policy.

# Descritpion

### Frame 1: Uart Selection

**Context:**

In the original design, the first frame combines the description of post code and uart selection. Because of HD or YV4 systems use AMD platform which sends two or four bytes postcode at a time, the original design of post code is limited by 7-seg LED can handle one byte post code only.

So, in this new design, we set the first frame for showing uart selection only and post code move to frame 5 to handle one to four post codes depending on the platform you use.

**Design:**

After the user presses the uart button, CPLD gets the current uart selection and lights up 7-seg LED as below table and then debug card sends ipmb command to get the uart selection description and shows it on the panel.

| 7-seg LED| Uart selection decode from BMC | Message show on debug card |
|--|--| --|
| 00 |BMC  |00: BMC  |
| 01 |slot1  |01: slot1  |
| 02 |slot2  |02: slot2  |
| 03 |slot3  |03: slot3  |
| 04 |slot4  |04: slot4  |
| 05 |slot5  |05: slot5  |
| 06 |slot6  |06: slot6  |
| 07 |slot7  |07: slot7  |
| 08 |slot8  |08: slot8  |


Here is the ipmb command:

| Code | Command | Request, Response Data | Description |
|--|--| --| --|
| net = 0x3C, cmd = 0x03 | Get uart selection description  |Request: <br/> Byte [0:2] - IANA ID <br/> Byte 3 - uart selection index <br/> Byte 4 - uart selection phase <br/> 01h : phase 1  <br/> 02h : phase 2 <br/> Response: <br/> Byte 0- completion code <br/> Byte [1:3] - IANA ID <br/> Byte 4 - Current uart selection index <br/> Byte 5 - next uart selection index <br/> Byte 6 - uart selection phase <br/> Byte 7 - check if it is the last one post code <br/> 00h: this is not the last one of Post code <br/> 01h: The last one available Post code <br/> Byte 8 - length (n) <br/> Byte 9: (n+1) human readable string (ASCII format) | MCU gets uart selection description from BMC. The selection decode refers to each project define.


## Frame 2: System Info
This frame displays the system information, and pressing the UART button allows you to switch to the server board that you want to check.

**Baseboard info:**
- FRU
- BMC IP (ipv4 & ipv6)
- BMC FW ver
- Board ID
- Battery charging status  and  percentage
- MCU boot loader version
- MCU FW version
- CPLD FW ver

**Server board info:**
- FRU
- Serial  Number
- Part Number
- BMC IP (ipv4 & ipv6)
- BMC FW ver
- BIOS FW ver
- ME status
- Board ID
- Battery charging status  and  percentage
- MCU boot loader version
- MCU FW version
- BIC FW ver
- CPLD FW ver


## Frame 3: Critical SEL

Display all the critical SELs from BMC.

## Frame 4: Critical Sensor

The Critical Sensor Frame display all critical sensors along with their corresponding names and values. The message is presented in the following format:
`${Sensor name}:${Sensor Value}${Unit}`
For example: `P0_TEMP:XXC`.

If a sensor falls outside of its designated threshold, the debug card will begin blinking and the color will invert. The following message format will be displayed: `${Sensor Name}:${Sensor Value}${Unit}/${Threshold}`. For example: `P0_TEMP:XXC/UC`.

## Frame 5: Post Code
This frame can handle post code one to four bytes situations depending on the platform you use (Intel or AMD). When the host power on, the message format shows on the panel as `${post code}:${post code description}` in sequence.

One byte post code example shows on panel:

	06: CPU_EARLY_INIT
	05: OEM_INIT_ENTRY
	04: SECSB_INIT
	02: MICROCODE

Four bytes post code example shows on panel:

	EA00E090: TP0x90
	EA00EA00: ABL Begin
	EA00E60C: ABL Functions execute
	EA00E0B7: ABL 1 End

Usage:
1. Press the UART button for the host that you want to check.
2. Power on the host.
3. All post codes with their descriptions will be displayed on the panel.

## Frame 6: Dimm Loop

The DIMM Loop frame displays error descriptions when the BIOS detects DIMM errors.
The message is presented in the following format:
`DIMM ${dimm location} ${description}`. For example: `DIMM A0 WARN_MEMORY_BOOT_HEALTH_CHECK_MASK_FAIL`

## Frame 7: IO Status

IO Status Frame displays the status of input pin and power button shown in the table below on platform Crater Lake:

| IO Index| Description | Message | Definition |
|--|--| --| --|
| 0x10 |RST_BTN_N  |P10: ${value}<br />RST_BTN_N  |Power button|
| 0x11 |PWR_BTN_N  |P11:${value}<br />PWR_BTN_N    |Power button|
| 0x12 |SYS_PWROK  |P12:${value}<br />SYS_PWROK  |Input pin only|
| 0x13 |RST_PLTRST  |P13:${value}<br />RST_PLTRST   |Input pin only|
| 0x14 |DSW_PWROK  |P14:${value}<br />DSW_PWROK   |Input pin only|
| 0x15 |FM_CATERR_MSMI |P15:${value}<br />FM_CATERR_MSMI  |Input pin only|
| 0x16 |FM_SLPS3  |P16:${value}<br />FM_SLPS3   |Input pin only|
| 0x17 |FM_UART_SWITCH |P17:${value}<br />FM_UART_SWITCH  |Power button |

## User Setting

### Power Policy
Usage
1. Select power policy from the user setting frame
2. Press the UART button to select the host for which you want to set the power policy.
3. The options for **Power On, Last State, and Power Off** will be displayed, and the current power policy will have a "*" symbol before it.
4. Use the 5-way button to change the power policy as required.

Below are the description of each power policy:
| Power Policy | Description |
|--|--|
| Power on | Chassis always powers up after AC/mains returns.  |
| Last state | After AC/mains returns, power is restored to the state that was in effect when AC/mains was lost.  |
| Power off |  Chassis stays powered off after AC/mains returns.|


### Boot Order

The Boot Order frame displays the BIOS boot-up sequence. Press the boot option which you want to set it as the first order.

Usage:
1. Select the boot order on the User Settings frame.
2. All the bootable devices in order will be displayed.
3. Select a boot option.
4. After selecting a boot option, it will be set at the top of the order. This boot sequence will be obtained by the BIOS and saved after the host is rebooted.
