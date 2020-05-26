FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
            file://pal_power.c \
            file://pal_power.h \
            file://pal_sensors.c \
            file://pal_sensors.h \
            file://pal_health.c \
            file://pal_health.h \
           "

SOURCES += "pal_power.c pal_sensors.c pal_health.c"
HEADERS += "pal_power.h pal_sensors.h pal_health.h"
DEPENDS += "libgpio-ctrl libobmc-sensors libbic libfby3-common libobmc-i2c libfby3-fruid libsensor-correction libncsi libnl-wrapper"
RDEPENDS_${PN} += "libgpio-ctrl libobmc-sensors libbic libfby3-common libobmc-i2c libfby3-fruid libsensor-correction libncsi libnl-wrapper"
LDFLAGS += "-lgpio-ctrl -lobmc-sensors -lbic -lfby3_common -lobmc-i2c -lfby3_fruid -lsensor-correction -lncsi -lnl-wrapper"
