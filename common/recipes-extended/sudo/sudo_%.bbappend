PR .= ".1"

do_install_append() {
  if [ "${EXTRA_SUDOERS_CONFIG}" ]; then
    echo "
# The following lines were included via EXTRA_SUDOERS_CONFIG
${EXTRA_SUDOERS_CONFIG}" >> ${D}${sysconfdir}/sudoers
  fi
}
