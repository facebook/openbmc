FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-configurations-rainier-Support-for-a-second-source-M.patch \
"
