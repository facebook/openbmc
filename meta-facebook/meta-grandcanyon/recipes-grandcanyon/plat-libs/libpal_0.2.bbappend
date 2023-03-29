FILESEXTRAPATHS:prepend := "${THISDIR}/files/pal:"

LOCAL_URI += " \
    file://plat/meson.build \
    file://pal_sensors.c \
    file://pal_power.c \
    file://pal_power.h \
    "
DEPENDS += " \
    libfbgc-common \
    libfbgc-fruid \
    libobmc-sensors \
    libobmc-i2c \
    libgpio-ctrl \
    libfbgc-gpio \
    libexp \
    libbic \
    libncsi \
    libnl-wrapper \
    libphymem \
    libnvme-mi \
    "
