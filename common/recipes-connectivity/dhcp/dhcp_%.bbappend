FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://dhclient.conf \
            file://dhclinet_ipv6_addr_missing.patch \
           "
