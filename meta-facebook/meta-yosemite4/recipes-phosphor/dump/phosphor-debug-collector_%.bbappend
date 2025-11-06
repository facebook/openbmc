FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " file://terminus-dump"

do_install:append() {
    install -m 0755 ${UNPACKDIR}/terminus-dump ${S}/tools/dreport.d/plugins.d/
}
