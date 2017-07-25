DESCRIPTION = "chardet module"
SECTION = "base"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a6f89e2100d9b6cdffcea4f398e37343"

SRC_URI = "https://pypi.python.org/packages/fc/bb/a5768c230f9ddb03acc9ef3f0d4a3cf93462473795d18e9535498c8f929d/chardet-3.0.4.tar.gz"
SRC_URI[md5sum] = "7dd1ba7f9c77e32351b0a0cfacf4055c"
SRC_URI[sha256sum] = "84ab92ed1c4d4f16916e05906b6b75a6c0fb5db821cc65e70cbd64a3e2a5eaae"

S = "${WORKDIR}/${PN}-${PV}"

dst="/usr/lib/python3.5/site-packages/chardet.egg-info"
dst1="/usr/lib/python3.5/site-packages/chardet"
cli="/cli"

do_install() {
  mkdir -p ${D}/${dst}
  mkdir -p ${D}/${dst1}
  mkdir -p ${D}/${dst1}/${cli}
  install chardet.egg-info/dependency_links.txt ${D}/${dst}
  install chardet.egg-info/PKG-INFO ${D}/${dst}
  install chardet.egg-info/SOURCES.txt ${D}/${dst}
  install chardet.egg-info/top_level.txt ${D}/${dst}
  install chardet.egg-info/entry_points.txt ${D}/${dst}
  install chardet/__init__.py ${D}/${dst1}
  install chardet/compat.py ${D}/${dst1}
  install chardet/version.py ${D}/${dst1}
  install chardet/universaldetector.py ${D}/${dst1}
  install chardet/utf8prober.py ${D}/${dst1}
  install chardet/charsetprober.py ${D}/${dst1}
  install chardet/charsetgroupprober.py ${D}/${dst1}
  install chardet/chardistribution.py ${D}/${dst1}
  install chardet/enums.py ${D}/${dst1}
  install chardet/escprober.py ${D}/${dst1}
  install chardet/escsm.py ${D}/${dst1}
  install chardet/codingstatemachine.py ${D}/${dst1}
  install chardet/big5freq.py ${D}/${dst1}
  install chardet/big5prober.py ${D}/${dst1}
  install chardet/cp949prober.py ${D}/${dst1}
  install chardet/eucjpprober.py ${D}/${dst1}
  install chardet/euckrfreq.py ${D}/${dst1}
  install chardet/euckrprober.py ${D}/${dst1}
  install chardet/euctwfreq.py ${D}/${dst1}
  install chardet/euctwprober.py ${D}/${dst1}
  install chardet/gb2312freq.py ${D}/${dst1}
  install chardet/gb2312prober.py ${D}/${dst1}
  install chardet/hebrewprober.py ${D}/${dst1}
  install chardet/jisfreq.py ${D}/${dst1}
  install chardet/jpcntx.py ${D}/${dst1}
  install chardet/langbulgarianmodel.py ${D}/${dst1}
  install chardet/langcyrillicmodel.py ${D}/${dst1}
  install chardet/langgreekmodel.py ${D}/${dst1}
  install chardet/langhebrewmodel.py ${D}/${dst1}
  install chardet/langhungarianmodel.py ${D}/${dst1}
  install chardet/langthaimodel.py ${D}/${dst1}
  install chardet/langturkishmodel.py ${D}/${dst1}
  install chardet/latin1prober.py ${D}/${dst1}
  install chardet/mbcharsetprober.py ${D}/${dst1}
  install chardet/mbcsgroupprober.py ${D}/${dst1}
  install chardet/mbcssm.py ${D}/${dst1}
  install chardet/sbcharsetprober.py ${D}/${dst1}
  install chardet/sbcsgroupprober.py ${D}/${dst1}
  install chardet/sjisprober.py ${D}/${dst1}
  install chardet/cli/chardetect.py ${D}/${dst1}/${cli}
  install chardet/cli/__init__.py ${D}/${dst1}/${cli}
}

do_configure() {
}

do_compile() {
}

FILES_${PN} = "${dst} ${dst1}"
