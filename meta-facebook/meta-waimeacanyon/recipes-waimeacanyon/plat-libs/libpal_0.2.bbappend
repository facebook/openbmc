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

# These shouldn't be needed but are because we aren't properly versioning the
# shared libraries contained in these recipes.
RDEPENDS:${PN} += " \
    libfbwc-fruid \
    libgpio-ctrl \
    libbic \
    libobmc-sensors \
    libncsi \
    libexp \
    "
