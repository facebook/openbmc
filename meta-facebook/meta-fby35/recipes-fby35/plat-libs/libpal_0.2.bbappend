FILESEXTRAPATHS:prepend := "${THISDIR}/files/pal:"

LOCAL_URI += " \
    file://pal_health.c \
    file://pal_health.h \
    file://pal_power.c \
    file://pal_power.h \
    file://pal_sensors.c \
    file://pal_sensors.h \
    file://plat/meson.build \
    "

DEPENDS += " \
    libbic \
    libfby35-common \
    libfby35-fruid \
    libgpio-ctrl \
    libncsi \
    libnl-wrapper \
    libobmc-i2c \
    libobmc-sensors \
    libsensor-correction \
    libmisc-utils \
    "

CFLAGS += " -Wall -Werror "
