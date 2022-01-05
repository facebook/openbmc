SUMMARY = "Simple Python style checker in one Python file"
HOMEPAGE = "https://github.com/pycqa/mccabe"
LICENSE = "MIT & Python-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a8546d0e77f416fb05a26acd89c8b3bd"

PYPI_PACKAGE = "pycodestyle"

inherit pypi setuptools3

SRC_URI[sha256sum] = "eddd5847ef438ea1c7870ca7eb78a9d47ce0cdb4851a5523949f2601d0cbbe7f"

BBCLASSEXTEND = "native nativesdk"
