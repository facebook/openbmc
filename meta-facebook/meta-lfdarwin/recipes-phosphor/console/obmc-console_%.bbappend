FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://0001-Add-support-for-break.patch \
"

do_install:append() {
    install -d ${D}${localstatedir}/log
    ln -s obmc-console-host0.log ${D}${localstatedir}/log/mTerm_wedge.log
}
