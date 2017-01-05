LINUX_VERSION_EXTENSION = "-cmm"

COMPATIBLE_MACHINE = "cmm"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

# Only 4.1 will use this defconfig, the 4.7 defconfig is pulled from
# kernel source tree
SRC_URI += "file://defconfig \
           "

KERNEL_MODULE_AUTOLOAD += " \
    ast_adc \
    pmbus_core \
    pfe3000 \
"

KERNEL_MODULE_PROBECONF += "                    \
 i2c-mux-pca954x                                \
"

module_conf_i2c-mux-pca954x = "options i2c-mux-pca954x ignore_probe=1"
