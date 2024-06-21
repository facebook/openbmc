SUMMARY = "base64"
DESCRIPTION = "Fast Base64 stream encoder/decoder"
HOMEPAGE = "https://github.com/aklomp/base64"

PR = "r1"
PV = "1.0+git${SRCPV}"
LICENSE = "BSD-2-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;md5=6281c1e587393eb2099f2d9219034d24"

SRC_URI = "git://github.com/aklomp/base64;branch=master;protocol=https"

SRCREV = "8bdda2d47caf8b066999c5bd01069e55bcd0d396"

S = "${WORKDIR}/git"

inherit cmake pkgconfig