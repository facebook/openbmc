There should be a method used for debug and manufacturing workflows to perform a
full reprogramming of each EEPROM from the SSH command line.  For EEPROMs directly
connected to the BMC, this can be performed with a `dd` call to the sysfs entry
for the EEPROM.
