FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

LOCAL_URI += " \
    file://ssd-mond.service \
"

SYSTEMD_SERVICE:${PN} = "ssd-mond.service"
