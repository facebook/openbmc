LINUX_VERSION_EXTENSION = "-cmm"

COMPATIBLE_MACHINE = "cmm"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "

KERNEL_MODULE_AUTOLOAD += " \
    ast_adc \
"
