FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " file://0001-Revert-Do-not-automatically-set-warning-flags-if-bui.patch"
