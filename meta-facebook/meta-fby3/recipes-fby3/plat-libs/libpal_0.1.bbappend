FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
            file://pal_power.c \
            file://pal_power.h \
           "

SOURCES += "pal_power.c"
HEADERS += "pal_power.h"
DEPENDS += "libgpio-ctrl libobmc-sensors libbic libfby3-common libobmc-i2c"
RDEPENDS_${PN} += "libgpio-ctrl libobmc-sensors libbic libfby3-common libobmc-i2c"
LDFLAGS += "-lgpio-ctrl -lobmc-sensors -lbic -lfby3_common -lobmc-i2c"
