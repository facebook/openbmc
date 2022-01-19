SUMMARY = "Simple Python style checker in one Python file"
HOMEPAGE = "https://github.com/pycqa/mccabe"
LICENSE = "MIT & Python-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a489dc62bacbdad3335c0f160a974f0f"

PYPI_PACKAGE = "mccabe"

inherit pypi setuptools3
SRC_URI:append = " \
    file://0001-python-mccabe-remove-unnecessary-setup_requires-pyte.patch \
"
SRC_URI[sha256sum] = "dd8d182285a0fe56bace7f45b5e7d1a6ebcbf524e8f3bd87eb0f125271b8831f"

BBCLASSEXTEND = "native nativesdk"
