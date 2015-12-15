
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://dhcpv6_up \
            file://dhcpv6_down \
           "

do_install_append() {
  # rules to request dhcpv6
  install -d ${D}/${sysconfdir}/network/if-up.d
  install -m 755 ${WORKDIR}/dhcpv6_up ${D}${sysconfdir}/network/if-up.d/dhcpv6_up
  install -d ${D}/${sysconfdir}/network/if-down.d
  install -m 755 ${WORKDIR}/dhcpv6_down ${D}${sysconfdir}/network/if-down.d/dhcpv6_down
}

FILES_${PN} += "${sysconfdir}/network/if-up.d/dhcpv6_up \
                ${sysconfdir}/network/if-down.d/dhcpv6_down \
               "
