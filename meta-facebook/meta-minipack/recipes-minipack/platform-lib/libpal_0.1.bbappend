FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
            "

DEPENDS += "libbic libsensor-correction libobmc-i2c"
RDEPENDS_${PN} += " libbic libsensor-correction libobmc-i2c"
LDFLAGS += " -lbic -lsensor-correction -lm -lobmc-i2c"
