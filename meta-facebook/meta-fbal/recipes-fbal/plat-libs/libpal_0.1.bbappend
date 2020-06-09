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
            file://pal_sbmc.c \
            file://pal_sbmc.h \
           "

SOURCES += "pal_sensors.c pal_health.c pal_power.c pal_cm.c pal_ep.c pal_sbmc.c"
HEADERS += "pal_health.h pal_power.h pal_cm.h pal_ep.h pal_sbmc.h"

DEPENDS += "libgpio-ctrl \
            libnm \
            libpeci \
            libfbal-fruid \
            libobmc-sensors \
            libobmc-pmbus \
            libncsi \
            libnl-wrapper \
           "

RDEPENDS_${PN} += "libgpio-ctrl \
                   libnm \
                   libpeci \ 
                   libfbal-fruid \ 
                   libobmc-sensors \
                   libobmc-pmbus \
                   libnl-wrapper \
                   libncsi \
                  "
LDFLAGS += "-lgpio-ctrl -lnm -lpeci -lfbal-fruid -lobmc-sensors -lobmc-pmbus -lncsi -lnl-wrapper"
