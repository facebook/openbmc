FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://0001-configuration-Revise-CX7-NIC-card-temperature-sensor.patch \
    file://0002-configurations-Revise-the-BRCM-NIC-sensor-name.patch \
    file://0003-Add-mctp-eids-configuration-for-Yosemite-4.patch \
    file://0004-configurations-yosemite4-Add-IANA-for-sentinel-dome.patch \
    file://0005-configurations-Revise-the-Terminus-NIC-sensor-name.patch \
    file://0006-configurations-yosemite4-NIC-card-support-slot-numbe.patch \
"

