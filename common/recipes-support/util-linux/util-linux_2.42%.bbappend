FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " \
    file://1000-Ensure-we-include-mount.h-to-fix-build.patch \
"
