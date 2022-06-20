FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

NETWORK_INTERFACES = "auto/lo manual/eth0"

SRC_URI += "file://eth1 \
          "

do_configure:append() {
	cat ${S}/eth1 >> ${S}/interfaces
}

RUN_DHC6_FILE = "run-dhc6_prefix64_ncsi.sh"
