FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://backpack_ssh_config \
           "

do_install_append() {
  cat ${WORKDIR}/backpack_ssh_config >> ${D}${sysconfdir}/ssh/ssh_config
}
