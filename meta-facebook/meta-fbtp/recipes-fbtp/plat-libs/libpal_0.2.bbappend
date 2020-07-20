FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += " \
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
RDEPENDS_${PN} += " \
    libgpio-ctrl \
    libme \
    libmisc-utils \
    libobmc-i2c \
    libobmc-sensors \
    libsensor-correction \
    libvr \
    "
