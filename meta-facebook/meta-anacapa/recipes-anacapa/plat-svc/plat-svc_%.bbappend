FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " \
    file://nic-temp-read \
"

do_install:append() {
    install -m 0755 ${UNPACKDIR}/nic-temp-read ${PLATSVC_LIBEXECDIR}
}
