FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = "\
        file://0002-Handle-build-with-older-versions-of-GCC.patch \
"
