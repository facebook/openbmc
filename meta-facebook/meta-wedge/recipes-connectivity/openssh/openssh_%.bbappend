do_install_append() {
  cat ${WORKDIR}/backpack_ssh_config >> ${D}${sysconfdir}/ssh/ssh_config
}
