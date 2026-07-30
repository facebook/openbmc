FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://1003-nvidia-gpu-add-support-for-SMA-leak-sensors.patch \
    file://1004-nvidia-gpu-probe-SMA-sensors-via-MCTP-before-instant.patch \
"

DEPENDS += " libnvme"
PACKAGECONFIG[nvme-mi] = "-Dnvme-mi=enabled, -Dnvme=disabled"

PACKAGECONFIG:append = " \
    nvidia-gpu \
    nvme-mi \
"

SYSTEMD_SERVICE:${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'nvme-mi', \
                                               'xyz.openbmc_project.nvmemisensor.service', \
                                               '', d)}"
