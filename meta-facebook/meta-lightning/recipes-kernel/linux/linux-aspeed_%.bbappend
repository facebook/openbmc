LINUX_VERSION_EXTENSION = "-lightning"

COMPATIBLE_MACHINE = "lightning"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "

KERNEL_MODULE_AUTOLOAD += " \
"
