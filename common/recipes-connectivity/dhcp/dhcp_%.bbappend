FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://dhclient.conf \
            file://syslog_upto_notice.patch \
            file://dhclinet_ipv6_addr_missing.patch \
           "
