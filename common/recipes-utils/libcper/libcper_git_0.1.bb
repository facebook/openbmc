SUMMARY = "libCper"
DESCRIPTION = "CPER JSON Representation & Conversion Library"
HOMEPAGE = "https://gitlab.arm.com/server_management/libcper"

PR = "r1"
PV = "1.0+git${SRCPV}"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a832eda17114b48ae16cda6a500941c2"

SRC_URI = "git://gitlab.arm.com/server_management/libcper;branch=master;protocol=https \
           file://0001-Only-build-cper-convert.patch \
           file://0002-Fix-incorrect-GUID-string-length.patch \
          "

SRCREV = "94153499f4c8b9a763d09bafabdc6b1b454f6e35"

DEPENDS = "json-c b64c"

S = "${WORKDIR}/git"

CFLAGS += " -I${STAGING_INCDIR}/json-c -I${STAGING_INCDIR}/b64-c -Wno-unused-result"

inherit cmake pkgconfig