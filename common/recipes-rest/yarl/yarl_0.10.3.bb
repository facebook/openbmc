DESCRIPTION = "yarl framework"
SECTION = "base"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e3fc50a88d0a364313df4b21ef20c29e"

SRC_URI = "https://pypi.python.org/packages/e6/fc/9b3820c47aa66924c38841f1766bf3b4857161b3c53e8548dd7a6dc0b226/yarl-0.10.3.tar.gz"
SRC_URI[md5sum] = "c10a2eb1e3474c0f84994d537313d246"
SRC_URI[sha256sum] = "27b24ba3ef3cb8475aea1a655a1750bb11918ba139278af21db5846ee9643138"

S = "${WORKDIR}/${PN}-${PV}"

dst="/usr/lib/python3.5/site-packages/yarl.egg-info"
dst1="/usr/lib/python3.5/site-packages/yarl"

do_install () {
  mkdir -p ${D}/${dst}
  mkdir -p ${D}/${dst1}
  install yarl.egg-info/dependency_links.txt ${D}/${dst}
  install yarl.egg-info/PKG-INFO ${D}/${dst}
  install yarl.egg-info/requires.txt ${D}/${dst}
  install yarl.egg-info/SOURCES.txt ${D}/${dst}
  install yarl.egg-info/top_level.txt ${D}/${dst}
  install yarl/__init__.py ${D}/${dst1}
  install yarl/__init__.pyi ${D}/${dst1}
  install yarl/quoting.py ${D}/${dst1}
  install yarl/_quoting.pyx ${D}/${dst1}
  install yarl/_quoting.c ${D}/${dst1}
}

do_configure () {
}

do_compile () {
}

FILES_${PN} = "${dst} ${dst1}"
