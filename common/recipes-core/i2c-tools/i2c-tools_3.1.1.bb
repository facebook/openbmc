DESCRIPTION = "i2c tools"
SECTION = "base"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=751419260aa954499f7abaabaa882bbe"

SRCREV = "f216eca1c6ebab20473070dc411cc6cb927a77db"
SRC_URI = "git://github.com/groeck/i2c-tools.git;protocol=https;branch=i2c-tools-3.1"

S = "${WORKDIR}/git"

i2ctools = "i2cdetect \
            i2cdump \
            i2cget \
            i2cset \
           "

eepromtools = "eepromer \
               eeprom \
               eeprog \
              "

do_compile() {
  make -C eepromer
  make
}

do_install() {
  mkdir -p ${D}/${bindir}
  for f in ${i2ctools}; do
    install -m 755 tools/$f ${D}/${bindir}/$f
  done
  for f in ${eepromtools}; do
    install -m 755 eepromer/$f ${D}/${bindir}/$f
  done
}

FILES_${PN} = "${bindir}"
