LINUX_VERSION_EXTENSION = "-yamp"

COMPATIBLE_MACHINE = "yamp"

FILESEXTRAPATHS:prepend := "${THISDIR}/board_config:"
SRC_URI += "file://yamp.cfg \
           "

# XXX: do we still need below options???
KERNEL_MODULE_PROBECONF += "i2c-mux-pca954x"
module_conf_i2c-mux-pca954x = "options i2c-mux-pca954x ignore_probe=1"
