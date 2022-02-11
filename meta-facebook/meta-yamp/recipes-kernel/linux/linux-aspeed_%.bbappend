LINUX_VERSION_EXTENSION = "-yamp"

COMPATIBLE_MACHINE = "yamp"

# YAMPTODO: Really pfe3000 exists?
KERNEL_MODULE_AUTOLOAD += " \
    ast_adc \
    pmbus_core \
    pfe3000 \
"
# YAMPTODO: pcs954x exists?
KERNEL_MODULE_PROBECONF += "                    \
 i2c-mux-pca954x                                \
"
#YAMPTODO: If not needed, remove the below
module_conf_i2c-mux-pca954x = "options i2c-mux-pca954x ignore_probe=1"
