FILESEXTRAPATHS:prepend := "${THISDIR}/files/pal:"

LOCAL_URI += " \
    file://pal_power.c \
    file://pal_power.h \
    file://pal_sensors.c \
    file://pal_sensors.h \
    file://plat/meson.build \
    "
DEPENDS += " \
    libfbwc-fruid \
    libgpio-ctrl \
    libbic \
    libobmc-sensors \
    libncsi \
    libexp \
    "
