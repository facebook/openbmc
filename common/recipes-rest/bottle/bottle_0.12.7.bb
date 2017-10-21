DESCRIPTION = "Bottle Web Framework"
SECTION = "base"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://bottle.py;beginline=1;endline=14;md5=15d806194a048a43e3a7f1d4c7574fe6"

SRC_URI = "https://pypi.python.org/packages/source/b/bottle/${PN}-${PV}.tar.gz"
SRC_URI[md5sum] = "ed0b83c9dbbdbde784e7c652d61c59f4"
SRC_URI[sha256sum] = "e3ea2191f06ca51af45bf6ca41ed2d1b2d809ceda0876466879fe205be7b2073"

S = "${WORKDIR}/${PN}-${PV}"

# TODO: Remove dst_py2 once all the platforms migrate to py3
dst_py2="/usr/lib/python2.7"
dst_py3="/usr/lib/python3.5"

do_install() {
  mkdir -p ${D}/${dst_py2}
  mkdir -p ${D}/${dst_py3}
  install -m 755 bottle.py ${D}/${dst_py2}
  install -m 755 bottle.py ${D}/${dst_py3}
}

FILES_${PN} = "${dst_py2} ${dst_py3}"
