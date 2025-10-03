FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

NETWORK_INTERFACES = "auto/lo auto/eth0"

RUN_DHC6_FILE = "run-dhc6_prefix64_ncsi.sh"

SRC_URI += "file://eth1 \
            file://bringup-eth0.sh \
          "

do_configure:append() {
	cat ${S}/eth1 >> ${S}/interfaces
}

do_install_dhcp:append() {
    install -m 755 bringup-eth0.sh ${D}${sysconfdir}/init.d/bringup-eth0.sh
    update-rc.d -r ${D} bringup-eth0.sh start 72 5 .
}
