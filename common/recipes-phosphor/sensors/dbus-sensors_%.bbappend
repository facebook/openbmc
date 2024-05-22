FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-psusensor-simplify-the-labelMatch-table.patch \
"
