FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

NETWORK_INTERFACE = "auto/lo manual/br0 manual/eth0"

RUN_DHC6_FILE = "run-dhc6_prefix64_ncsi.sh"

SRC_URI += "file://run-dhc6_prefix64_ncsi.sh \
           "
