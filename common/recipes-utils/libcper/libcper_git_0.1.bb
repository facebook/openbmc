SUMMARY = "OpenBMC libcper"
DESCRIPTION = "CPER JSON Representation & Conversion Library"
HOMEPAGE = "https://github.com/openbmc/libcper"

PR = "r1"
PV = "1.0+git${SRCPV}"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a832eda17114b48ae16cda6a500941c2"

SRC_URI = "git://github.com/openbmc/libcper;branch=main;protocol=https"

SRCREV = "b8b687c2e05846afd37b60222a0b4253acda81fd"

S = "${WORKDIR}/git"

DEPENDS:append = " json-c libb64"

EXTRA_OEMESON:append = " -Dtests=disabled"

CFLAGS:append = " -Wno-error=stringop-truncation"

inherit meson pkgconfig