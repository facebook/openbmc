LINUX_VERSION_EXTENSION = "-fbttn"

COMPATIBLE_MACHINE = "fbttn"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "

KERNEL_MODULE_AUTOLOAD += " \
"
