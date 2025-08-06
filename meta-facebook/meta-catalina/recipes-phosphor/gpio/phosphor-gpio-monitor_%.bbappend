RDEPENDS:${PN}:append = " fw-versions"

SYSTEMD_LINK:${PN}-monitor:append = " ../fw-versions-hmc-hgx-fw-cpu-0@.service:platform-host-ready.target.wants/fw-versions-hmc-hgx-fw-cpu-0@0.service"
SYSTEMD_LINK:${PN}-monitor:append = " ../fw-versions-hmc-hgx-fw-cpu-1@.service:platform-host-ready.target.wants/fw-versions-hmc-hgx-fw-cpu-1@0.service"
SYSTEMD_LINK:${PN}-monitor:append = " ../fw-versions-hmc-hgx-fw-gpu-0@.service:platform-host-ready.target.wants/fw-versions-hmc-hgx-fw-gpu-0@0.service"
SYSTEMD_LINK:${PN}-monitor:append = " ../fw-versions-hmc-hgx-fw-gpu-1@.service:platform-host-ready.target.wants/fw-versions-hmc-hgx-fw-gpu-1@0.service"
SYSTEMD_LINK:${PN}-monitor:append = " ../fw-versions-hmc-hgx-inforom-gpu-0@.service:platform-host-ready.target.wants/fw-versions-hmc-hgx-inforom-gpu-0@0.service"
SYSTEMD_LINK:${PN}-monitor:append = " ../fw-versions-hmc-hgx-inforom-gpu-1@.service:platform-host-ready.target.wants/fw-versions-hmc-hgx-inforom-gpu-1@0.service"
