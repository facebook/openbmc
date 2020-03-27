FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal_sensors.c \
            file://pal_health.c \
            file://pal_health.h \
            file://pal_power.c \
            file://pal_power.h \
            file://pal_cm.c \
            file://pal_cm.h \
            file://pal_ep.c \
            file://pal_ep.h \
           "

SOURCES += "pal_sensors.c pal_health.c pal_power.c pal_cm.c pal_ep.c"
HEADERS += "pal_health.h pal_power.h pal_cm.h pal_ep.h"
DEPENDS += "libgpio-ctrl libnm libpeci libfbal-fruid libobmc-sensors libobmc-pmbus"
RDEPENDS_${PN} += "libgpio-ctrl libnm libpeci libfbal-fruid libobmc-sensors libobmc-pmbus"
LDFLAGS += "-lgpio-ctrl -lnm -lpeci -lfbal-fruid -lobmc-sensors -lobmc-pmbus"
