#!/bin/bash
#
# reconfigure the CPLD<-->COMe UART mux 
# useful to force redirect COMe tx/rx data from reaching BMC UART durign COMe boot
# We could not disable the BMC UART from transmitting to COMe and hence this work around.
# usage wedge_tty_block <1:switch | 0: dont switch > [delay in seconds; default no delay]

if [ $# -eq 0 ]; then                                                           
  exit 1                                                                        
fi                                                                              
                                                                                
switch=$1                                                                       
if [ $# -gt 1 ]; then                                                           
  delay="$2"                                                                    
else                                                                            
  delay=0                                                                       
fi                                                                              
                                                                                
if [ $delay -gt 0 ]; then                                                       
  sleep "$delay"                                                                
fi                                                                              
                                                                                
val=$(i2cget -f -y 12 0x31 0x26)                                                
if [ $switch -eq 1 ]; then                                                      
  newval=$(($val|0x01))                                                         
  echo "switching COMe uart to dbg port"                                        
else                                                                            
  newval=$(($val&0xFE))                                                         
  echo "switching COMe uart to BMC UART"                                        
fi                                                                              
i2cset -f -y 12 0x31 0x26 $newval                                               

