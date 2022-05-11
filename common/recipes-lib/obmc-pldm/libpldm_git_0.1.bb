SUMMARY = "OpenBmc PLDM Library"
DESCRIPTION = "OpenBmc/PLDM library to provide PLDM functionality"
HOMEPAGE = "https://github.com/openbmc/pldm/"

PR = "r1"
PV = "1.0+git${SRCPV}"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

SRC_URI = "git://github.com/openbmc/pldm;protocol=git;branch=master \
          "
SRCREV = "357b72dcca2c7aab08c8f8c660c7393c412ac260"

S = "${WORKDIR}/git"

EXTRA_OEMESON = " \
        -Dtests=disabled \
        -Doem-ibm=disabled \
        -Dlibpldm-only=enabled \
        "
inherit meson pkgconfig
