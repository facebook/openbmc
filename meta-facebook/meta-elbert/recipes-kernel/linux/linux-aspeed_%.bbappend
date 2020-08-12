LINUX_VERSION_EXTENSION = "-elbert"

COMPATIBLE_MACHINE = "elbert"

KERNEL_MODULE_AUTOLOAD += " \
    ast_adc \
    pmbus_core \
"

KERNEL_MODULE_PROBECONF += "                    \
 i2c-mux-pca954x                                \
"
