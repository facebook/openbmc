FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

do_install:append() {
    rm -f ${D}${datadir}/${PN}/configurations/mtjade.json
    rm -f ${D}${datadir}/${PN}/configurations/mtjefferson_*.json
    rm -f ${D}${datadir}/${PN}/configurations/mtmitchell_*.json
}

SRC_URI:append = " \
    file://0001-Revert-entity-manager-Handle-left-over-template-vars.patch \
"
