DESCRIPTION = "json-log-formatter module"
SECTION = "base"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "https://files.pythonhosted.org/packages/2a/51/603c3cedc789617556248b51bdc941e967fc068902e754c1475774161d7a/JSON-log-formatter-0.3.0.tar.gz"
SRC_URI[md5sum] = "b757c635dc55cf02371c01552c6068f5"
SRC_URI[sha256sum] = "ee187c9a80936cbf1259f73573973450fc24b84a4fb54e53eb0dcff86ea1e759"
BPN = "JSON-log-formatter"
S = "${WORKDIR}/${BPN}-${PV}"

inherit python3-dir
dst="${PYTHON_SITEPACKAGES_DIR}/json_log_formatter"

do_install() {
  mkdir -p ${D}/${dst}
  install json_log_formatter/__init__.py ${D}/${dst}
}


FILES_${PN} = "${dst}"
BBCLASSEXTEND += "native nativesdk"
