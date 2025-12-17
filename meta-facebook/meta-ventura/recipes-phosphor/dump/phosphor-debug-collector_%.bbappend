FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = "\
    file://modbus-device-dump \
"

do_install:append() {
    install -m 0755 ${UNPACKDIR}/modbus-device-dump ${S}/tools/dreport.d/plugins.d/
}
