FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += " \
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
# These shouldn't be needed but are because we aren't properly versioning the
# shared libraries contained in these recipes.
RDEPENDS_${PN} += " \
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
