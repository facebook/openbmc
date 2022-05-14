--------------------------------------------------------------------------------
OCP Debug Card introduction:
--------------------------------------------------------------------------------
The OCP Debug Card with LCD(Model: 1HY9ZZZ0869 USB3.0 connector) can used for a server system
debug.It includes:
A text‐rich user interface with an LED Panel and Display, A  Detector Switch Buttons, the
user can cycle through all frames by using Detector Switch’s left and right buttons. Some
of the frames might have multiple pages of content, and the user can go through these pages
by using up and down buttons. 
A Power Button, the user can Power ON/OFF the COMe OS.
A Reset Button, the user can Reset the COMe OS.
A Uart Select Button and A Micro USB Port & A USB Port, the user can Switch COMe Uart & BMC
Uart via USB Cable.

--------------------------------------------------------------------------------
OCP Debug Packing instructions:
--------------------------------------------------------------------------------
The user can view various frames by onboard detector switch that provides four direction 
functions: up, down, left, right and one selection function: Select.   
Left or Right 
The left or right function uses to rotate different frame information to be shown on LED 
panel: POST code ‐> System Info ‐> BMC Critical SEL ‐> Critical sensors ‐> GPIO status ‐> 
User Settings.   
Up or Down 
The up or down function uses to view multiple pages (if available) in the same frame for 
more content.   
Select 
The select function will be only useful in “User Settings” page to identify user selection. 

--------------------------------------------------------------------------------
Unit Test Framework Usage:
--------------------------------------------------------------------------------
1. Insert Debug card to Wedge400 USB Port on front-panel, Connect one Micro-USB Cable & open 
   teraterm with baud rate 57600 in PC,then press left/right/up/down button and check the OCP 
   Debug card display.
<1>. Check Post Code Page
<2>. Check Sys Info Page
<3>. Check CriSensor Page
<4>. Check IO_Status Page
<5>. Check User Settings Page

2. Press the Uart_Sel button & monitor BMC Serial screen,the Serial Screen switch from OpenBMC
   to COMe OS serial.

3. Press the reset button & monitor Serial port, the COMe OS reboot successful.

4. Press the power button & monitor Serial port, the COMe OS shutdown successful.
   (HW Limitation: Power off COMe OS, the Debug Card Power will lost, be careful do this step).
