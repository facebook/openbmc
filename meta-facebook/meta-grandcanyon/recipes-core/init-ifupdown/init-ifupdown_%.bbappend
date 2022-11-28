FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

NETWORK_INTERFACES = "auto/lo auto/eth0"

RUN_DHC6_FILE = "run-dhc6_prefix64_ncsi.sh"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://bringup-eth0.sh \
	"

do_install_dhcp() {
	# override it to install bringup-eth0.sh script
	install -m 755 bringup-eth0.sh ${D}${sysconfdir}/init.d/bringup-eth0.sh
	update-rc.d -r ${D} bringup-eth0.sh start 72 5 .
}
