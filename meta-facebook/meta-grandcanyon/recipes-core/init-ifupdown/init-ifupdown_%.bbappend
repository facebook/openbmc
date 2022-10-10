FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

NETWORK_INTERFACES = "auto/lo auto/eth0"

RUN_DHC6_FILE = "run-dhc6_prefix64_ncsi.sh"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://update-ll-addr \
	"

do_install_dhcp() {
	# override it to install eth0 up update link-local addr only
	install -m 755 ${WORKDIR}/update-ll-addr \
		${D}${sysconfdir}/network/if-up.d/update-ll-addr
}