FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

EXTRA_OEMESON:append:openbmc-fb-lf = " -Doem=meta"

SRC_URI:append = " \
    file://0001-transport-af-mctp-Fix-TID-lookup-with-MCTP_NET_ANY.patch \
"
