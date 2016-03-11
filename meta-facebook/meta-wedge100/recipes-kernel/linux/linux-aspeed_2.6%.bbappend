LINUX_VERSION_EXTENSION = "-wedge100"

COMPATIBLE_MACHINE = "wedge100"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

KERNEL_MODULE_AUTOLOAD += " \
    ltc4151 \
"

SRC_URI += "file://defconfig \
           "
