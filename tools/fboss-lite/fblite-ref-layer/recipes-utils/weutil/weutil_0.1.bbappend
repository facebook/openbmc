FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += "file://eeprom.conf \
              file://meta_eeprom_v4_sample.bin \
              "

do_install:append() {
  install -d  ${D}${sysconfdir}/weutil
  install -m 644 ${S}/eeprom.conf  ${D}${sysconfdir}/weutil/eeprom.conf
  install -m 644 ${S}/meta_eeprom_v4_sample.bin  ${D}${sysconfdir}/weutil/meta_eeprom_v4_sample.bin
}

