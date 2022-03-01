FILESEXTRAPATHS:prepend := "${THISDIR}/files/pal:"

LOCAL_URI += " \
    file://machine_config.c \
    file://plat/meson.build \
    "

DEPENDS += " \
    libgpio-ctrl \
    libme \
    libmisc-utils \
    libobmc-i2c \
    libobmc-sensors \
    libsensor-correction \
    libvr \
    plat-utils \
    "
RDEPENDS:${PN} += " \
    libgpio-ctrl \
    libme \
    libmisc-utils \
    libobmc-i2c \
    libobmc-sensors \
    libsensor-correction \
    libvr \
    "
