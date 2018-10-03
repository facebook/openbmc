FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
            "

DEPENDS += "liblightning-common liblightning-fruid liblightning-sensor liblightning-flash liblightning-gpio libnvme-mi obmc-i2c"
RDEPENDS_${PN} += " liblightning-common liblightning-fruid liblightning-sensor liblightning-flash liblightning-gpio libnvme-mi"
LDFLAGS += " -llightning_common -llightning_fruid -llightning_sensor -llightning_flash -llightning_gpio -lnvme-mi"
