DESCRIPTION = "multidict module"
SECTION = "base"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e74c98abe0de8f798ca609137f9cef4a"

SRC_URI = "https://pypi.python.org/packages/e5/04/b272ba67ed896c5e9ac05cac916da6508855352524aae00f99938bdf9dc6/multidict-2.1.6.tar.gz"
SRC_URI[md5sum] = "7837353cf71008484e99737ccf55a4ea"
SRC_URI[sha256sum] = "9ec33a1da4d2096949e29ddd66a352aae57fad6b5483087d54566a2f6345ae10"

S = "${WORKDIR}/${PN}-${PV}"

dst="/usr/lib/python3.5/site-packages/multidict.egg-info"
dst1="/usr/lib/python3.5/site-packages/multidict"

do_install() {
  mkdir -p ${D}/${dst}
  mkdir -p ${D}/${dst1}
  install multidict.egg-info/dependency_links.txt ${D}/${dst}
  install multidict.egg-info/PKG-INFO ${D}/${dst}
  install multidict.egg-info/SOURCES.txt ${D}/${dst}
  install multidict.egg-info/top_level.txt ${D}/${dst}
  install multidict/__init__.py ${D}/${dst1}
  install multidict/__init__.pyi ${D}/${dst1}
  install multidict/_multidict_py.py ${D}/${dst1}
  install multidict/_multidict.pyx ${D}/${dst1}
  install multidict/_multidict.c ${D}/${dst1}
}

do_configure () {
}

do_compile() {
}

FILES_${PN} = "${dst} ${dst1}"
