LINUX_VERSION_EXTENSION = "-wedge"

COMPATIBLE_MACHINE = "wedge"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "

KERNEL_MODULE_AUTOLOAD += " \
    pmbus_core \
    pfe1100 \
"
