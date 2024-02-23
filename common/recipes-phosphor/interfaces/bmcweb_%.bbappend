FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-mutual-tls-Add-support-for-svc-and-host-identity-typ.patch \
"

# Enable mTLS auth + Meta certificate common name parsing
EXTRA_OEMESON:append = "-Dmutual-tls-auth='enabled' "
EXTRA_OEMESON:append = "-Dmutual-tls-common-name-parsing='meta' "
