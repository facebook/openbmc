# OCP Debug Card

**Frame list**
- System Info
- Critical SEL
- IO Status
- Critical Sensor
- Dimm Loop
- Post Code
- User Setting
	- Boot Order
	- Power Policy

Users can view various frames using the onboard detector switch (the black button on the left-hand side), which provides four directional functions: **up, down, left, right,** and one selection function: **Select.**

Frames are displayed in the following order on the panel from left to right: Post Code->System Info->Critial Sel->Critial Sensor->DIMM Loop->IO Status->User Setting

### 5-way button operation

**Left, Right -**
Used for switching to different frames.

**Up, Down -**
Move up or down to switch pages and view more content within the current frame.

**Select -**
Press the middle of the button to select an item. This operation is used on the User Setting Frame to select the boot order or power policy.


# Descritpion

## System Info Frame
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

## Critical Sensor Frame

The Critical Sensor Frame display all critical sensors along with their corresponding names and values. The message is presented in the following format:
`${Sensor name}:${Sensor Value}${Unit}`
For example: `P0_TEMP:XXC`.

If a sensor falls outside of its designated threshold, the debug card will begin blinking and the color will invert. The following message format will be displayed: `${Sensor Name}:${Sensor Value}${Unit}/${Threshold}`. For example: `P0_TEMP:XXC/UC`.

## Critical SEL Frame

Display all the critical SELs from BMC.

## IO Status Frame

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



## Post Code Frame
The Post Code Frame displays the post codes and their descriptions after the host is powered on.

Usage:
1. Press the UART button for the host that you want to check.
2. Power on the host.
3. All post codes with their descriptions will be displayed on the panel.

## Dimm Loop

The DIMM Loop frame displays error descriptions when the BIOS detects DIMM errors.
The message is presented in the following format:
`DIMM ${dimm location} ${description}`. For example: `DIMM A0 WARN_MEMORY_BOOT_HEALTH_CHECK_MASK_FAIL`

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
