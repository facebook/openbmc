The BMC shall keep a cycle counter in EEPROM or another persistent storage
location for any TI voltage regulators used for the platform.  Because TI VRs
does not store remaining allowed writes, an alternative method must be
enabled to print the remaining number of TI VR rewrites upon demand and to
update the remaining write (from base of 1000 allowed) after each firmware
write operation.
