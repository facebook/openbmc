BB_DONT_CACHE = "1"

python do_compile:prepend() {
    import datetime
    version_str = datetime.datetime.now().strftime("%Y%m%d%H%M%S")

    with open(os.path.join(d.getVar('B'), 'version'), 'w') as f:
        f.write("%s\n" % version_str)
}

do_install:append() {
    install -d ${D}${sysconfdir}
    if [ -f "${B}/version" ]; then
        install -m 0644 ${B}/version ${D}${sysconfdir}/version
    fi
}

FILES:${PN} += "${sysconfdir}/version"
