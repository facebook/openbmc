FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
            file://pal_sensors.c \
            file://pal_sensors.h \
            file://pal_health.c \ 
            file://pal_health.h \
            file://pal_power.c \ 
            file://pal_power.h \
           "

SOURCES += "pal_sensors.c pal_health.c pal_power.c"
HEADERS += "pal_sensors.h pal_health.h pal_power.h"
DEPENDS += " libgpio-ctrl libnm libpeci"
RDEPENDS_${PN} += " libgpio-ctrl libnm libpeci"
LDFLAGS += "-lgpio-ctrl -lnm -lpeci"
