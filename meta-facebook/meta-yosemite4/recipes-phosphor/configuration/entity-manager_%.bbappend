FILESEXTRAPATHS:prepend := "${THISDIR}/entity-manager:"

EXTRA_OEMESON:append = " \
     -Dconfig-prefix=yosemite4,brcm,cx7,terminus,bmc \
 "

SRC_URI += " \
    file://0001-configuration-Revise-CX7-NIC-card-temperature-sensor.patch \
    file://0002-configurations-Revise-the-BRCM-NIC-sensor-name.patch \
    file://0003-Add-mctp-eids-configuration-for-Yosemite-4.patch \
    file://0004-configurations-Revise-the-Terminus-NIC-sensor-name.patch \
    file://0005-configurations-yosemite4-NIC-card-support-slot-numbe.patch \
    file://0006-entity-manager-Add-config-prefix-filtering.patch \
    file://0007-entity-manager-Add-virtual-NIC-temperature.patch \
"

