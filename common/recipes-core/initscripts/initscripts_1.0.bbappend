FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += "file://hostname.sh"

do_install_prepend() {
	install -d ${D}${sysconfdir}/init.d
	install -d ${D}${sysconfdir}/rc5.d
	install -m 0755 ${WORKDIR}/hostname.sh ${D}${sysconfdir}/init.d/hostname.sh
	update-rc.d -r ${D} hostname.sh start 04 5 .
}

FILES_${PN} += "${sysconfdir}/init.d/hostname.sh"
RDEPENDS_${PN} += "bash"
DEPENDS += "update-rc.d-native"
