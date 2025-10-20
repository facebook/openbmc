FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

do_install:append() {
    rm -f ${D}${datadir}/${PN}/configurations/mtjade.json
    rm -f ${D}${datadir}/${PN}/configurations/mtjefferson_*.json
    rm -f ${D}${datadir}/${PN}/configurations/mtmitchell_*.json
}
