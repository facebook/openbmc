LDFLAGS += " -lwedge_eeprom -lelbert_eeprom"
DEPENDS += "libwedge-eeprom libelbert-eeprom libipmi obmc-pal update-rc.d-native"
RDEPENDS:${PN} += "libipmi libkv libwedge-eeprom libelbert-eeprom"

IPMI_FEATURE_FLAGS = "-DSENSOR_DISCRETE_SEL_STATUS -DSENSOR_DISCRETE_WDT"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
FILESEXTRAPATHS:prepend := "${THISDIR}/patches:"
LOCAL_URI += "file://fruid.c"

SRC_URI += "file://1001-In-ipmid-use-systemd-command-instead-of-sv.patch \
           "
