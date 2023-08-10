FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-fansensor-add-compatible-string-for-ast2600-tach.patch \
"