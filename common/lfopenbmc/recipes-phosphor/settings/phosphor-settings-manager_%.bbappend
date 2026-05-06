FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0100-mako-Add-cereal-tuple-serialization-support.patch \
"
