FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = "\
    file://anacapa-info \
    file://ncsi-thor2 \
"

# For online debug, BMC OOB is unreachable (FBA-862). and ncsi-thor2 script
# Since the NCSI core dump binary is around 30MB, and about 2.xMB after compression,
# the default values are too small. Therefore, these settings must be increased.
EXTRA_OEMESON:append = " -DBMC_DUMP_TOTAL_SIZE=12288"
EXTRA_OEMESON:append = " -DBMC_DUMP_MAX_SIZE=3072"
EXTRA_OEMESON:append = " -DBMC_DUMP_MIN_SPACE_REQD=3072"

do_install:append() {
    install -m 0755 ${UNPACKDIR}/anacapa-info ${S}/tools/dreport.d/plugins.d/
    install -m 0755 ${UNPACKDIR}/ncsi-thor2 ${S}/tools/dreport.d/plugins.d/
}
