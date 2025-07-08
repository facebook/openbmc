FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-mmap-hack.patch"

DEPENDS += "libtatsu"
SRCREV = "038a49362570ac56bae330fda8a30635134fc509"

LDFLAGS += "-lm"
