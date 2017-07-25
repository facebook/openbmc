DESCRIPTION = "async-timeout module"
SECTION = "base"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e3fc50a88d0a364313df4b21ef20c29e"

SRC_URI = "https://pypi.python.org/packages/eb/a3/9fbe8bf7de4128d8f5562ca0b7b2f81d21b006085149528b937e1624e71f/async-timeout-1.2.1.tar.gz"
SRC_URI[md5sum] = "e71f9b197498f917864e29af7a5defa0"
SRC_URI[sha256sum] = "380e9bfd4c009a14931ffe487499b0906b00b3378bb743542cfd9fbb6d8e4657"

S = "${WORKDIR}/${PN}-${PV}"

dst="/usr/lib/python3.5/site-packages/async_timeout.egg-info"
dst1="/usr/lib/python3.5/site-packages/async_timeout"

do_install() {
  mkdir -p ${D}/${dst}
  mkdir -p ${D}/${dst1}
  install async_timeout.egg-info/dependency_links.txt ${D}/${dst}
  install async_timeout.egg-info/PKG-INFO ${D}/${dst}
  install async_timeout.egg-info/SOURCES.txt ${D}/${dst}
  install async_timeout.egg-info/top_level.txt ${D}/${dst}
  install async_timeout/__init__.py ${D}/${dst1}
}

do_configure () {
}

do_compile() {
}

FILES_${PN} = "${dst} ${dst1}"
