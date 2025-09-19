RDEPENDS:${PN}:append = " fw-versions mctp"

SYSTEMD_LINK:${PN}-monitor:append = " ../mctp_setup@.service:platform-host-ready.target.wants/mctp_setup@swb_PCIe_switch_mctp.service"
SYSTEMD_LINK:${PN}-monitor:append = " ../fw-versions-pcie-switch@.service:platform-host-ready.target.wants/fw-versions-pcie-switch@0.service"
