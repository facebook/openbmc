FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://1000-TODO-nvidia-gpu-add-leak-sensors.patch \
    file://1001-Add-NVMeMI-sensor-service.patch \
    file://1002-Fix-NVMeMI-sensor-build-error.patch \
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
