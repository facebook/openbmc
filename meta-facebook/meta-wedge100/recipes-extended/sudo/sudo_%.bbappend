do_install:append() {
    rm -f ${D}${bindir}/visudo
    rm -f ${D}${bindir}/cvtsudoers
    rm -f ${D}${bindir}/sudoreplay
}
