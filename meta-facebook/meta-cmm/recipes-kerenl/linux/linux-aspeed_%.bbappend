LINUX_VERSION_EXTENSION = "-cmm"

COMPATIBLE_MACHINE = "cmm"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "

KERNEL_MODULE_AUTOLOAD += " \
    ast_adc \
"

KERNEL_MODULE_PROBECONF += "                    \
 i2c-mux-pca954x                                \
"

module_conf_i2c-mux-pca954x = "options i2c-mux-pca954x ignore_probe=1"
