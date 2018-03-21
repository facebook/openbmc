LINUX_VERSION_EXTENSION = "-minipack"

COMPATIBLE_MACHINE = "minipack"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "
KERNEL_MODULE_AUTOLOAD += " \
  tpm \
  tpm_i2c_infineon \
"

KERNEL_MODULE_PROBECONF += " \
  i2c-mux-pca954x \
"

module_conf_i2c-mux-pca954x = "options i2c-mux-pca954x ignore_probe=1"
