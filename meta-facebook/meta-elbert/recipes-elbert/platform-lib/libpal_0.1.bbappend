FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

SRC_URI += "file://pal.c \
            file://pal.h \
            "
DEPENDS += "libgpio \
            libsensor-correction \
            libwedge-eeprom \
            libelbert-eeprom \
            libobmc-i2c \
            "
RDEPENDS_${PN} += "libgpio \
                   libsensor-correction \
                   libwedge-eeprom \
                   libelbert-eeprom \
                   libobmc-i2c \
                   "
LDFLAGS += " -lgpio -lsensor-correction -lwedge_eeprom -lelbert_eeprom"
