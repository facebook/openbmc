RDEPENDS:${PN} += "libipmi libkv"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += "file://config.json  \
              "

do_install:append() {
    dst="${D}/usr/local/fbpackages/${pkgdir}"
    install -d $dst
    install -m 644 ${S}/config.json ${dst}/config.json
}

pkgdir = "ipmid"
