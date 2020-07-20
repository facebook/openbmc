FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += " \
    file://pal_calibration.h \
    file://pal_gpu.c \
    file://pal_gpu.h \
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
    switchtec-user \
    "
RDEPENDS_${PN} += " \
    libgpio-ctrl \
    libobmc-i2c \
    libobmc-sensors \
    switchtec-user \
    "
