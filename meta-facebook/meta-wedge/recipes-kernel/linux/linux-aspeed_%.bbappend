LINUX_VERSION_EXTENSION = "-wedge"

COMPATIBLE_MACHINE = "wedge"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

KERNEL_MODULE_AUTOLOAD += "                     \
    ast_adc                                     \
    lm75                                        \
"

SRC_URI += "file://defconfig \
           "
