SUMMARY = "flake8 is a python tool that glues together pycodestyle, pyflakes, mccabe, and third-party plugins to check the style and quality of some python code."
HOMEPAGE = "https://github.com/pycqa/flake8"
LICENSE = "MIT & Python-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=75b26781f1adf1aa310bda6098937878"

PYPI_PACKAGE = "flake8"

inherit pypi setuptools3

SRC_URI[sha256sum] = "806e034dda44114815e23c16ef92f95c91e4c71100ff52813adf7132a6ad870d"

BBCLASSEXTEND = "native nativesdk"

RDEPENDS:${PN} += " \
    ${PYTHON_PN}-pip-pyflakes \
    ${PYTHON_PN}-pip-pycodestyle \
    ${PYTHON_PN}-pip-mccabe \
"
