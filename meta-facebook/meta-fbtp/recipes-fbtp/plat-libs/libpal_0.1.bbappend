FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
            file://machine_config.c \
            "

SOURCES += "machine_config.c"

DEPENDS += "plat-utils libme libvr libgpio libsensor-correction"
RDEPENDS_${PN} += " libme libvr libsensor-correction"
LDFLAGS += " -lme -lvr -lgpio -lsensor-correction"
