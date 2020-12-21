FILESEXTRAPATHS_prepend := "${THISDIR}/${BPN}:"

SRC_URI += " \
    file://0001-fix-gcc10-compile-errors.patch \
    file://0017-binutils-drop-redundant-program_name-definition-fno-.patch \
    "
