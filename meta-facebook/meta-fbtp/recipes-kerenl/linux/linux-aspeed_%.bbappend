LINUX_VERSION_EXTENSION = "-fbtp"

COMPATIBLE_MACHINE = "fbtp"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "

KERNEL_MODULE_AUTOLOAD += " \
"
