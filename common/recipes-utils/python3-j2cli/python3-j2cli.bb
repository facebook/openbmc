SUMMARY = "Python support for J2CLI"
HOMEPAGE = "https://pypi.org/project/j2cli/"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e46989ee3017dd8d4373102ac88457be"
DEPENDS = "python3-pyyaml python3-jinja2"

PYPI_PACKAGE = "j2cli"
PV="0.3.10"
inherit pypi setuptools3

SRC_URI[md5sum] = "52a2d900437a318f5d99d6d3ae751e6b"
SRC_URI[sha256sum] = "6f6f643b3fa5c0f72fbe9f07e246f8e138052b9f689e14c7c64d582c59709ae4"

BBCLASSEXTEND = "native nativesdk"
