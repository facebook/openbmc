DESCRIPTION = "Python ply: PLY is yet another implementation of lex and yacc for Python"
HOMEPAGE = "https://pypi.python.org/pypi/ply"
SECTION = "devel/python3"
LIC_FILES_CHKSUM = "file://README.md;beginline=7;endline=30;md5=a4010c50f4f5fc4d30ee152c237c37d1"
LICENSE = "BSD-3-Clause"

PR = "r0"
SRCNAME = "ply"

SRC_URI = "https://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "94726411496c52c87c2b9429b12d5c50"
SRC_URI[sha256sum] = "e7d1bdff026beb159c9942f7a17e102c375638d9478a7ecd4cc0c76afd8de0b8"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools3
