FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

LOCAL_URI += " \
    file://plat/meson.build \
    file://pal_sensors.c \
    file://pal_sensors.h \
    file://pal_guid.c \
    file://pal_guid.h \
    file://pal_power.c \
    file://pal_power.h \
    "
DEPENDS += " \
    libnetlakemtp-fruid \
    libnetlakemtp-common \
    libobmc-i2c \
    libobmc-sensors \
    libgpio-ctrl \
    libphymem \
    "
# These shouldn't be needed but are because we aren't properly versioning the
# shared libraries contained in these recipes.
RDEPENDS_${PN} += " \
    libnetlakemtp-fruid \
    libnetlakemtp-common \
    libobmc-i2c \
    libobmc-sensors \
    libgpio-ctrl \
    libphymem \
    "
