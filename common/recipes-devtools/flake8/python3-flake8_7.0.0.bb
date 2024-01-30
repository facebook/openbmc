SUMMARY = "flake8 is a python tool that glues together pycodestyle, pyflakes, mccabe, and third-party plugins to check the style and quality of some python code."
HOMEPAGE = "https://github.com/pycqa/flake8"
LICENSE = "MIT & Python-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=75b26781f1adf1aa310bda6098937878"

PYPI_PACKAGE = "flake8"

inherit pypi setuptools3

SRC_URI[sha256sum] = "33f96621059e65eec474169085dc92bf26e7b2d47366b70be2f67ab80dc25132"

BBCLASSEXTEND = "native nativesdk"

RDEPENDS:${PN} += " \
    ${PYTHON_PN}-pyflakes \
    ${PYTHON_PN}-pycodestyle \
    ${PYTHON_PN}-mccabe \
"
