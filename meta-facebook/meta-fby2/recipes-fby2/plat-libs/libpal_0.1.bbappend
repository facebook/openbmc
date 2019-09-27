FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
            "

DEPENDS += "libbic libfruid libfby2-common libfby2-fruid libfby2-sensor libncsi libobmc-i2c jansson"
RDEPENDS_${PN} += " libfruid libfby2-common libfby2-fruid libfby2-sensor libncsi libobmc-i2c jansson"
LDFLAGS += " -lbic -lfruid -lfby2_common -lfby2_fruid -lfby2_sensor -lncsi -lobmc-i2c -ljansson"
