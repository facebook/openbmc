SUMMARY = "OpenBmc PLDM Library"
DESCRIPTION = "OpenBmc/PLDM library to provide PLDM functionality"
HOMEPAGE = "https://github.com/openbmc/libpldm"

PR = "r1"
PV = "1.0+git${SRCPV}"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

SRC_URI = "git://github.com/openbmc/libpldm;branch=main;protocol=https \
          "
SRCREV = "64764fd6debc749fd2025f2ea6e7c98c6758ccdd"

S = "${WORKDIR}/git"

EXTRA_OEMESON = " \
        -Dtests=disabled \
        -Doem-ibm=disabled \
        "
inherit meson pkgconfig
