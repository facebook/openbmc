FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += "file://eeprom.conf \
              file://eeprom \
              "

do_install:append() {
  install -d  ${D}${sysconfdir}/weutil
  install -m 644 ${S}/eeprom.conf  ${D}${sysconfdir}/weutil/eeprom.conf
  install -m 644 ${S}/eeprom  ${D}${sysconfdir}/weutil/eeprom
}

