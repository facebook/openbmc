FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://1001-Add-NVMeMI-sensor-service.patch \
    file://1002-Fix-NVMeMI-sensor-build-error.patch \
    file://1003-nvidia-gpu-add-option-to-disable-PCIe-device-support.patch \
    file://1004-nvidia-gpu-add-support-for-SMA-leak-sensors.patch \
    file://1005-nvidia-gpu-add-State.Leak.Detector-support-for-SMA-s.patch \
    file://1006-nvidia-gpu-add-event-log-for-SMA-leak-sensor-state-c.patch \
"

DEPENDS += " libnvme"
PACKAGECONFIG[nvme-mi] = "-Dnvme-mi=enabled, -Dnvme=disabled"

PACKAGECONFIG:append = " \
    nvidia-gpu \
    nvme-mi \
"

EXTRA_OEMESON:append = " \
    -Dnvidia-gpu-pcie=disabled \
"

SYSTEMD_SERVICE:${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'nvme-mi', \
                                               'xyz.openbmc_project.nvmemisensor.service', \
                                               '', d)}"
