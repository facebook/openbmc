FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
            file://machine_config.c \
            "

SOURCES += "machine_config.c"

DEPENDS += "plat-utils libme libvr libgpio libsensor-correction libobmc-i2c"
RDEPENDS_${PN} += " libme libvr libsensor-correction libobmc-i2c"
LDFLAGS += " -lme -lvr -lgpio -lsensor-correction -lobmc-i2c"
