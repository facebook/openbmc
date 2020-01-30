LINUX_VERSION_EXTENSION = "-elbert"

COMPATIBLE_MACHINE = "elbert"

KERNEL_MODULE_AUTOLOAD += " \
    ast_adc \
    pmbus_core \
"

KERNEL_MODULE_PROBECONF += "                    \
 i2c-mux-pca954x                                \
"
# ELBERTTODO 447409 If not needed, remove the below
module_conf_i2c-mux-pca954x = "options i2c-mux-pca954x ignore_probe=1"
