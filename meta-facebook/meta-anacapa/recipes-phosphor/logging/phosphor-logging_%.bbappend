FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

EXTRA_OEMESON:append = " \
    -Damd-event-log=enabled \
    -Dmax-cper-raw-binary-size=65536 \
    -Druntime-metadata-plugin=xyz.openbmc_project.Logging.Extension.CPER.Processed \
"

SRC_URI:append = " \
    file://reverse_lut.json \
"

do_configure:prepend() {
    install -m 0644 \
        ${UNPACKDIR}/reverse_lut.json \
        ${S}/extensions/amd-event-log/data/reverse_lut.json
}
