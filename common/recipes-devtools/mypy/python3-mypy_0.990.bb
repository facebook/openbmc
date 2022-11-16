SUMMARY = "Optional static typing for Python 3 and 2 (PEP 484)"
HOMEPAGE = "https://github.com/python/mypy"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=6ba8ec528da02073b7e1f4124c0f836f"

PYPI_PACKAGE = "mypy"

inherit pypi setuptools3

SRC_URI[sha256sum] = "72382cb609142dba3f04140d016c94b4092bc7b4d98ca718740dc989e5271b8d"

BBCLASSEXTEND = "native"

RDEPENDS:${PN} += " \
    ${PYTHON_PN}-mypy-extensions \
    ${PYTHON_PN}-typed-ast \
    ${PYTHON_PN}-typing-extensions \
    ${PYTHON_PN}-json \
    ${PYTHON_PN}-compression \
    ${PYTHON_PN}-pprint \
    ${PYTHON_PN}-difflib \
    ${PYTHON_PN}-tomli \
"
