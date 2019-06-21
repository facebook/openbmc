FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
            file://machine_config.c \
            "

SOURCES += "machine_config.c"

DEPENDS += "plat-utils libme libvr libgpio-ctrl libsensor-correction libobmc-i2c libmisc-utils"
RDEPENDS_${PN} += " libme libvr libgpio-ctrl libsensor-correction libobmc-i2c libmisc-utils"
LDFLAGS += " -lme -lvr -lgpio-ctrl -lsensor-correction -lobmc-i2c -lmisc-utils"
