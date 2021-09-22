LINUX_VERSION_EXTENSION = "-lightning"

COMPATIBLE_MACHINE = "lightning"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "

KERNEL_MODULE_AUTOLOAD += " \
"
