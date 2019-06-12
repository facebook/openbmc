FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
            file://pal_sensors.c \
            file://pal_sensors.h \
            file://pal_health.c \ 
            file://pal_health.h \
           "

SOURCES += "pal_sensors.c pal_health.c"
HEADERS += "pal_sensors.h pal_health.h"
DEPENDS += " libgpio-ctrl"
RDEPENDS_${PN} += " libgpio-ctrl"
LDFLAGS += "-lgpio-ctrl"
