FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://init \
            file://sshd_config \
            file://backpack_ssh_config \
           "

PR .= ".1"

# If the following were do_install_append, it would get combined/concatenated
# with the do_install_append from Yocto, and not necessarily in the right
# order (last). So create a new task that comes after do_install (because
# do_install_append appears to count as do_install for task ordering purposes)
# and use fakeroot, like do_install does, to get correct permissions.
fakeroot do_install_certificates() {
  if [ -f "${AUTH_PRINCIPALS_ROOT}" ]; then
    install -m 0644 \
      ${AUTH_PRINCIPALS_ROOT} ${D}${sysconfdir}/ssh/auth_principals_root
  fi
  if [ -f "${TRUSTED_CA}" ]; then
    install -m 0644 ${TRUSTED_CA} ${D}${sysconfdir}/ssh/trusted_ca
  fi
}
addtask do_install_certificates after do_install before do_build
