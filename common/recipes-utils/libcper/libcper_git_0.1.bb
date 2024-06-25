SUMMARY = "OpenBMC libcper"
DESCRIPTION = "CPER JSON Representation & Conversion Library"
HOMEPAGE = "https://github.com/openbmc/libcper"

PR = "r1"
PV = "1.0+git${SRCPV}"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a832eda17114b48ae16cda6a500941c2"

SRC_URI = "git://github.com/openbmc/libcper;branch=main;protocol=https"

SRCREV = "a5b3d49b9e02ae5194f9c7dfc9fe3eb0e4bd39c1"

S = "${WORKDIR}/git"

DEPENDS:append = " json-c libb64"

EXTRA_OEMESON:append = " -Dtests=disabled"

CFLAGS:append = " -Wno-error=stringop-truncation"

inherit meson pkgconfig

# TODO: These patches are downloaded from https://gerrit.openbmc.org/q/project:openbmc/libcper 
# to support NVIDIA CPERs first and will be removed when they are merged into openbmc/libcper
SRC_URI:append = " \
    file://nvidia/0001-WIP-Add-support-for-NVIDIA-CPERs.patch \
"