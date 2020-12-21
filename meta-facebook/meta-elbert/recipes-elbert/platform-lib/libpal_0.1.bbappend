FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://pal.c \
            file://pal.h \
            "

DEPENDS += "libgpio libsensor-correction libwedge-eeprom libelbert-eeprom"
RDEPENDS_${PN} += "libgpio libsensor-correction libwedge-eeprom libelbert-eeprom"
LDFLAGS += " -lgpio -lsensor-correction -lwedge_eeprom -lelbert_eeprom"
