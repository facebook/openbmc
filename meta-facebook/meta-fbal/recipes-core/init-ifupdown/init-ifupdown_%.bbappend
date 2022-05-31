FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

NETWORK_INTERFACES = "auto/lo manual/br0 manual/br/eth0"

RUN_DHC6_FILE = "run-dhc6_prefix64_ncsi.sh"

SRC_URI += "file://run-dhc6_prefix64_ncsi.sh \
           "
