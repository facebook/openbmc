FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://backpack_ssh_config"

do_install_backpack_ssh_config () {
    # Append cmm, lc, fc ssh config
    cat ${WORKDIR}/backpack_ssh_config >> ${D}${sysconfdir}/ssh/ssh_config
}
