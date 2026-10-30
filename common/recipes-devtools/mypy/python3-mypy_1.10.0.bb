SUMMARY = "Optional static typing for Python 3 and 2 (PEP 484)"
HOMEPAGE = "https://github.com/python/mypy"
LICENSE = "MIT AND Python-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=8d62fd8f8648cb018e52857347e340b9"

def inherit_setuptools(d):
    distro = d.getVar('DISTRO_CODENAME', True)
    if distro in [ 'rocko', 'dunfell' ]:
        return "setuptools3"
    return "python_setuptools_build_meta"

inherit pypi
inherit ${@inherit_setuptools(d)}

SRC_URI[sha256sum] = "3d087fcbec056c4ee34974da493a826ce316947485cef3901f511848e687c131"

BBCLASSEXTEND = "native"

DEPENDS += " \
    python3-mypy-extensions-native \
    python3-types-psutil-native \
    python3-types-setuptools-native \
    python3-typing-extensions-native \
    python3-wheel-native \
"

RDEPENDS:${PN} += " \
    python3-modules \
    python3-mypy-extensions \
    python3-typing-extensions \
"
