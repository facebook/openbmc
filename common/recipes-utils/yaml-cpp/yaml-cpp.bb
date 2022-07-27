SUMMARY = "yaml-cpp is a YAML parser and emitter in C++ matching the YAML 1.2 spec."
DESCRIPTION = "yaml-cpp is a YAML parser and emitter in C++ matching the YAML 1.2 spec."
HOMEPAGE = "https://yaml-cpp.docsforge.com/"
SECTION = "libs/devel"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=6a8aaf0595c2efc1a9c2e0913e9c1a2c"

PV = "0.6.0"
SRC_URI = "https://github.com/jbeder/yaml-cpp/archive/refs/tags/yaml-cpp-${PV}.tar.gz"
SRC_URI[md5sum] = "8adc0ae6c2698a61ab086606cc7cf562"
SRC_URI[sha256sum] = "e643119f1d629a77605f02096cc3ac211922d48e3db12249b06a3db810dd8756"

# Yes the tar file really has this messed up top level directory name
S = "${WORKDIR}/yaml-cpp-yaml-cpp-${PV}"

inherit cmake

EXTRA_OECMAKE += "-DYAML_BUILD_SHARED_LIBS=ON"

RDEPENDS:${PN}-dev = ""

BBCLASSEXTEND = "native nativesdk"
