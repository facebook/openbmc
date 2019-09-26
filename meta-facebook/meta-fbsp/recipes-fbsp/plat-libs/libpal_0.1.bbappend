FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
            file://pal_power.c \
            file://pal_power.h \
           "

SOURCES += "pal_power.c"
HEADERS += "pal_power.h"
DEPENDS += " libgpio-ctrl"
RDEPENDS_${PN} += " libgpio-ctrl"
LDFLAGS += "-lgpio-ctrl"
