FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-mmap-hack.patch"

DEPENDS += "libtatsu"
SRCREV = "4b3e847e1d9a1210049a9e3f1d1caa38650c6617"

LDFLAGS += "-lm"
