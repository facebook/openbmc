LDFLAGS += " -lwedge_eeprom -lelbert_eeprom"
DEPENDS += "libwedge-eeprom libelbert-eeprom libipmi obmc-pal update-rc.d-native"
RDEPENDS_${PN} += "libipmi libkv libwedge-eeprom libelbert-eeprom"

IPMI_FEATURE_FLAGS = "-DSENSOR_DISCRETE_SEL_STATUS -DSENSOR_DISCRETE_WDT"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += "file://fruid.c"
