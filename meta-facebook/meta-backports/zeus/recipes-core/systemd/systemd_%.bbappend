FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://9990-dhcpv6-sendvendoroption.diff \
           "

FILES:${PN}-catalog-extralocales = \
            "${exec_prefix}/lib/systemd/catalog/*.*.catalog"
PACKAGES =+ "${PN}-catalog-extralocales"
