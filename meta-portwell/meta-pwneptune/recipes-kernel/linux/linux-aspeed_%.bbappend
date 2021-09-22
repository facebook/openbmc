LINUX_VERSION_EXTENSION = "-pwneptune"

COMPATIBLE_MACHINE = "pwneptune"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "

KERNEL_MODULE_AUTOLOAD += " \
"
