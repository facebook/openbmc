FILESEXTRAPATHS:prepend := "${THISDIR}/files/pal:"

LOCAL_URI += " \
    file://plat/meson.build \
    file://pal_power.c \
    file://pal_power.h \
    file://pal_sensors.c \
    file://pal_sensors.h \
    file://pal_switch.c \
    file://pal_switch.h \
    file://pal_health.c \
    file://pal_health.h \
    file://pal_calibration.h \
    "

DEPENDS += " \
    libobmc-sensors \
    libmisc-utils \
    libgpio-ctrl \
    libobmc-i2c \
    switchtec-user \
    libpex \
    "
RDEPENDS:${PN} += " \
    libobmc-sensors \
    libmisc-utils \
    libgpio-ctrl \
    libobmc-i2c \
    switchtec-user \
    libpex \
    "
