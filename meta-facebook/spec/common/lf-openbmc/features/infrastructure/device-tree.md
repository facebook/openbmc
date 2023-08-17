A device tree for Linux should be created for the system and sent upstream to
the Linux kernel. This device tree should instantiate all kernel drivers for the
platform for all the busses connected to the BMC: i2c, i3c, gpio, PWM/Tach, ADC,
SPI, etc.
