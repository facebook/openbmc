FILESEXTRAPATHS:prepend := "${THISDIR}/files/pal:"

LOCAL_URI += " \
    file://pal_calibration.h \
    file://pal_health.c \
    file://pal_health.h \
    file://pal_sensors.c \
    file://pal_sensors.h \
    file://pal_switch.c \
    file://pal_switch.h \
    file://plat/meson.build \
    "

DEPENDS += " \
    libgpio-ctrl \
    libobmc-i2c \
    libobmc-sensors \
    libasic \
    switchtec-user \
    libpex \
    "
RDEPENDS:${PN} += " \
    libasic \
    switchtec-user \
    "
