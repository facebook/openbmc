FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

NETWORK_INTERFACES = "auto/lo auto/eth0"

RUN_DHC6_FILE = "run-dhc6_prefix64_ncsi.sh"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://update-ll-addr \
            file://bringup-eth0.sh \
	"

do_install_dhcp() {
	# override it to install eth0 up update link-local addr only
	install -m 755 ${WORKDIR}/update-ll-addr \
		${D}${sysconfdir}/network/if-pre-up.d/update-ll-addr

	install -m 755 bringup-eth0.sh ${D}${sysconfdir}/init.d/bringup-eth0.sh
	update-rc.d -r ${D} bringup-eth0.sh start 72 5 .
}
