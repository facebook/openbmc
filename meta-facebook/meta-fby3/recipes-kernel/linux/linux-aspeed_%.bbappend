LINUX_VERSION_EXTENSION = "-fby3"

COMPATIBLE_MACHINE = "fby3"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "

KERNEL_MODULE_AUTOLOAD += " \
"
