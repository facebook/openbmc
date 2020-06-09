
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://dhcp_vendor_info \
            file://dhcpv6_up \
            file://dhcpv6_down \
            file://setup-dhc6.sh \
            file://run-dhc6.sh \
            file://run-dhc6_prefix64.sh \
            file://run-dhc6_prefix64_ncsi.sh \
            "
RUN_DHC6_FILE = "run-dhc6_prefix64.sh"

def dhc6_run(d):
  distro = d.getVar('DISTRO_CODENAME', True)
  dhcfile = d.getVar('RUN_DHC6_FILE', True)
  if distro != 'rocko':
    return dhcfile
  else:
    return "run-dhc6.sh"

do_install_dhcp() {
  # rules to request dhcpv6
  install -d ${D}/${sysconfdir}/network/if-up.d
  install -m 755 ${WORKDIR}/dhcpv6_up ${D}${sysconfdir}/network/if-up.d/dhcpv6_up
  install -d ${D}/${sysconfdir}/network/if-down.d
  install -m 755 ${WORKDIR}/dhcpv6_down ${D}${sysconfdir}/network/if-down.d/dhcpv6_down
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/dhc6
  install -m 755 setup-dhc6.sh ${D}${sysconfdir}/init.d/setup-dhc6.sh
  install -m 755 '${@dhc6_run(d)}' ${D}${sysconfdir}/sv/dhc6/run
  update-rc.d -r ${D} setup-dhc6.sh start 03 5 .
}

do_install_dhcp_vendor_info() {
  install -d ${D}/${sysconfdir}/network/if-pre-up.d
  install -m 755 ${WORKDIR}/dhcp_vendor_info \
    ${D}${sysconfdir}/network/if-pre-up.d/dhcp_vendor_info
}

do_install_append() {
  do_install_dhcp_vendor_info
  do_install_dhcp
}

FILES_${PN} += "${sysconfdir}/network/if-up.d/dhcpv6_up \
                ${sysconfdir}/network/if-down.d/dhcpv6_down \
               "
RDEPENDS_${PN} += "bash"
DEPENDS += "update-rc.d-native"
