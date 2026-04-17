FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-i2c-vr-add-preUpdateFirmware-hook-for-MPS-devices.patch \
    file://0002-i2cvr-pause-sensor-reads-during-CRC-retrieval.patch \
    file://0003-common-restore-VR-page-after-update-or-CRC-access.patch \
"
