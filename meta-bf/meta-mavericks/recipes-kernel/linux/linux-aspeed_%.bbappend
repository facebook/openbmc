LINUX_VERSION_EXTENSION = "-mavericks"

COMPATIBLE_MACHINE = "mavericks"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "
KERNEL_MODULE_AUTOLOAD += " \
  tpm \
  tpm_i2c_infineon \
"
