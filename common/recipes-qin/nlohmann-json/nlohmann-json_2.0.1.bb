DESCRIPTION = "nlohmann Json Parser"
SECTION = "base"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://json.hpp;beginline=1;endline=27;md5=958f9582b7b986eb8e86e2fc3f31f7ae"

SRC_URI = "file://json.hpp"
SRC_URI[md5sum] = "87a37c378bc62346a371cc81fa2dc837"
SRC_URI[sha256sum] = "ef550fcd7df572555bf068e9ec4e9d3b9e4cdd441cecb0dcea9ea7fd313f72dd"

S = "${WORKDIR}"

dst="usr/include/nlohmann"

do_install() {
  mkdir -p ${D}/${dst}
  install -m 644 json.hpp ${D}/${dst}
}
