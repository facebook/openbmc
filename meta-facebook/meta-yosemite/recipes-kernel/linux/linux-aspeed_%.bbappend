LINUX_VERSION_EXTENSION = "-yosemite"

COMPATIBLE_MACHINE = "yosemite"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "

KERNEL_MODULE_AUTOLOAD += " \
"
