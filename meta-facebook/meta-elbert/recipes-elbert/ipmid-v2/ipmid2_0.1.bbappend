LDFLAGS += " -lelbert_eeprom"
DEPENDS += " libelbert-eeprom "
RDEPENDS:${PN} += "libipmi libkv libelbert-eeprom"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += "file://fruid.c \
              file://plat.h  \
              file://plat_support_api.c \
              file://config.json \
              "

do_install:append() {
    install -m 644 ${S}/config.json ${dst}/config.json
}

