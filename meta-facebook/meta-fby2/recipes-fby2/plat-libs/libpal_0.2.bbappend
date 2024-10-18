FILESEXTRAPATHS:prepend := "${THISDIR}/files/pal:"

LOCAL_URI += " \
    file://plat/meson.build \
    file://pal.h \
    file://pal.c \
    "

DEPENDS += " \
    jansson \
    libbic \
    libfby2-common \
    libfby2-fruid \
    libfby2-sensor \
    libfruid \
    libncsi \
    libobmc-i2c \
    libobmc-sensors \
    libgpio-ctrl \
    libnl-wrapper \
    libnvme-mi \
    "

# These shouldn't be needed but are because we aren't properly versioning the
# shared libraries contained in these recipes.
RDEPENDS:${PN} += " \
    jansson \
    libbic \
    libfby2-common \
    libfby2-fruid \
    libfby2-sensor \
    libfruid \
    libncsi \
    libobmc-i2c \
    libobmc-sensors \
    libgpio-ctrl \
    libnl-wrapper \
    libnvme-mi \
    "
