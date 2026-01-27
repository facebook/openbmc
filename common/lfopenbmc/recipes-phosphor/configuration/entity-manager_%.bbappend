FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-configuration-schema-extend-MCTP-target-members.patch \
"

do_install:append() {
    rm -f ${D}${datadir}/${PN}/configurations/mtjade.json
    rm -f ${D}${datadir}/${PN}/configurations/mtjefferson_*.json
    rm -f ${D}${datadir}/${PN}/configurations/mtmitchell_*.json
}
