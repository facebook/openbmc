FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI_append = " file://0001-Revert-Do-not-automatically-set-warning-flags-if-bui.patch"
