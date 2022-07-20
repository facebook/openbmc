FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://dhcp_vendor_info \
            file://dhcpv6_up \
            file://dhcpv6_down \
            file://setup-dhc6.sh \
            file://run-dhc6.sh \
            file://run-dhc6_ncsi.sh \
            file://run-dhc6_prefix64.sh \
            file://run-dhc6_prefix64_ncsi.sh \
            file://interfaces.d \
            file://if-down.d \
            "

NETWORK_INTERFACES ?= "auto/lo auto/eth0 static/usb0"

do_configure:append() {
    if [ -n "${NETWORK_INTERFACES}" ]; then
        echo -n > ${S}/interfaces
        for intf in ${NETWORK_INTERFACES}; do
            cat ${S}/interfaces.d/$intf >> ${S}/interfaces
        done
    fi
}

do_install_interfaces() {
    if [ -n "${NETWORK_INTERFACES}" ]; then
        for intf in ${NETWORK_INTERFACES}; do
            if [ -e ${S}/if-down.d/${intf} ]; then
                install -d ${D}${sysconfdir}/network/if-down.d
                install -m 755 ${S}/if-down.d/${intf} \
                    ${D}${sysconfdir}/network/if-down.d/$(basename ${intf})
            fi
        done
    fi
}


def dhc6_run(d):
  distro = d.getVar('DISTRO_CODENAME', True)
  dhcfile = d.getVar('RUN_DHC6_FILE', True)
  if dhcfile != None:
    return dhcfile
  elif distro != 'rocko':
    return "run-dhc6_prefix64.sh"
  else:
    return "run-dhc6.sh"

do_install_dhcp() {
    # rules to request dhcpv6 - doesn't apply to images based on
    # systemd
    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'false', 'true', d)}; then
        install -d ${D}/${sysconfdir}/network/if-up.d
        install -m 755 ${WORKDIR}/dhcpv6_up ${D}${sysconfdir}/network/if-up.d/dhcpv6_up
        install -d ${D}/${sysconfdir}/network/if-down.d
        install -m 755 ${WORKDIR}/dhcpv6_down ${D}${sysconfdir}/network/if-down.d/dhcpv6_down
        install -d ${D}${sysconfdir}/sv
        install -d ${D}${sysconfdir}/sv/dhc6
        install -m 755 setup-dhc6.sh ${D}${sysconfdir}/init.d/setup-dhc6.sh
        install -m 755 '${@dhc6_run(d)}' ${D}${sysconfdir}/sv/dhc6/run
        update-rc.d -r ${D} setup-dhc6.sh start 03 5 .
    fi
}

do_install_dhcp_vendor_info() {
  install -d ${D}/${sysconfdir}/network/if-pre-up.d
  install -m 755 ${WORKDIR}/dhcp_vendor_info \
    ${D}${sysconfdir}/network/if-pre-up.d/dhcp_vendor_info
}

do_install:append() {
  do_install_dhcp_vendor_info
  do_install_dhcp
  do_install_interfaces
}

FILES:${PN} += "${sysconfdir}/network/if-up.d/dhcpv6_up \
                ${sysconfdir}/network/if-down.d/dhcpv6_down \
               "
RDEPENDS:${PN} += "bash"
DEPENDS += "update-rc.d-native"
