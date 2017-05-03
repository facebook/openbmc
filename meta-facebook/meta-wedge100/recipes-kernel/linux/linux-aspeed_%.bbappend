LINUX_VERSION_EXTENSION = "-wedge100"

COMPATIBLE_MACHINE = "wedge100"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "
KERNEL_MODULE_AUTOLOAD += " \
  tpm \
  tpm_i2c_infineon \
"
