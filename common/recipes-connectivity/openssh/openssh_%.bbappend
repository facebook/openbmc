FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://init"

do_install_append () {
    # Eanble root login through ssh
		sed -i -e 's:#PermitRootLogin yes:PermitRootLogin yes:' ${D}${sysconfdir}/ssh/sshd_config
}
