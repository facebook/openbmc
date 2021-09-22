FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
DEPENDS += "libwedge-eeprom"
RDEPENDS:${PN} += "libwedge-eeprom"
