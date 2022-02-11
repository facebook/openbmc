LINUX_VERSION_EXTENSION = "-wedge100"

COMPATIBLE_MACHINE = "wedge100"

KERNEL_MODULE_AUTOLOAD += " \
  tpm \
  tpm_i2c_infineon \
  pmbus_core \
  pfe1100 \
"
