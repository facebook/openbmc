FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

LOCAL_URI += " \
    file://leakage-mond.service \
"

SYSTEMD_SERVICE:${PN} = "leakage-mond.service"
