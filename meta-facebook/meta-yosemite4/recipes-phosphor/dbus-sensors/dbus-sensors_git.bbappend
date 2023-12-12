FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://0100-PSUSensor-add-ina233-support.patch \
    file://0101-PSUSensor-add-adm1281-support.patch \
    file://0102-PSUSensor-Fix-error-for-decimal-part-of-scalefactor.patch \
"
