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

SOURCES += "pal_sensors.c pal_health.c pal_power.c"
HEADERS += "pal_sensors.h pal_health.h pal_power.h"
DEPENDS += "libme libgpio-ctrl libnm libpeci libncsi libnl-wrapper libobmc-sensors"
RDEPENDS_${PN} += "libme libgpio-ctrl libnm libpeci libncsi libnl-wrapper libobmc-sensors"
LDFLAGS += "-lme -lgpio-ctrl -lnm -lpeci -lncsi -lnl-wrapper -lobmc-sensors"
