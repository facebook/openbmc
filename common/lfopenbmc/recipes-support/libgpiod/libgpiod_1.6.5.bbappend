FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0002-core-gracefully-handle-disappearing-chips-during-ite.patch \
"
