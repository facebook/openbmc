FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
            file://pal_sensors.c \
            file://pal_sensors.h \
            file://pal_health.c \
            file://pal_health.h \
            file://pal_switch.c \
            file://pal_switch.h \
            file://pal_gpu.c \
            file://pal_gpu.h \
            file://pal_calibration.h \
            "
HEADERS += "pal_calibration.h pal_health.h pal_switch.h pal_gpu.h"
SOURCES += "pal_sensors.c pal_health.c pal_switch.c pal_gpu.c"

DEPENDS += "libgpio-ctrl libobmc-i2c switchtec-user libobmc-sensors"
RDEPENDS_${PN} += " libgpio-ctrl libobmc-i2c switchtec-user libobmc-sensors"
LDFLAGS += " -lgpio-ctrl -lobmc-i2c -lswitchtec -lobmc-sensors -lssl -lcrypto"
