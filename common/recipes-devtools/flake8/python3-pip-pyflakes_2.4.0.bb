SUMMARY = "flake8 is a python tool that glues together pycodestyle, pyflakes, mccabe, and third-party plugins to check the style and quality of some python code."
HOMEPAGE = "https://github.com/pycqa/flake8"
LICENSE = "MIT & Python-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=690c2d09203dc9e07c4083fc45ea981f"

PYPI_PACKAGE = "pyflakes"

SRC_URI[sha256sum] = "05a85c2872edf37a4ed30b0cce2f6093e1d0581f8c19d7393122da7e25b2b24c"
inherit pypi setuptools3

RDEPENDS_${PN} += " \
    ${PYTHON_PN}-prettytable \
    ${PYTHON_PN}-cmd2 \
    ${PYTHON_PN}-pyparsing \
"

BBCLASSEXTEND = "native nativesdk"
