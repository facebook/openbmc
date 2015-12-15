include eglibc-collateral.inc

SUMMARY = "utility scripts provided by eglibc"
DESCRIPTION = "utility scripts provided by eglibc"
RDEPENDS_${PN} = "bash eglibc-mtrace"

SRC = "${STAGING_INCDIR}/eglibc-scripts-internal-${MULTIMACH_TARGET_SYS}"

bashscripts = "sotruss xtrace"

do_install() {
	install -d -m 0755 ${D}${bindir}
	for i in ${bashscripts}; do
		install -m 0755 ${SRC}/$i ${D}${bindir}/
	done
}
