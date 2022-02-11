LINUX_VERSION_EXTENSION = "-wedge400"

COMPATIBLE_MACHINE = "wedge400"

KERNEL_MODULE_AUTOLOAD += " \
  tpm \
  tpm_i2c_infineon \
"

KERNEL_MODULE_PROBECONF += " \
  i2c-mux-pca954x \
"

module_conf_i2c-mux-pca954x = "options i2c-mux-pca954x ignore_probe=1"
