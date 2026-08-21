FILESEXTRAPATHS:prepend := "${THISDIR}/entity-manager:"

EXTRA_OEMESON:append = " \
     -Dconfig-prefix=yosemite4,100g_1p_ocp_mezz,200g_1p_ocp_mezz,cx7_ocp,terminus_100g_nic_tsff,bmc_storage_module \
 "

SRC_URI += " \
    file://0000-configuration-Revise-CX7-NIC-card-temperature-sensor.patch \
    file://0001-configurations-Revise-the-BRCM-NIC-sensor-name.patch \
    file://0002-Add-mctp-eids-configuration-for-Yosemite-4.patch \
    file://0003-configurations-Revise-the-Terminus-NIC-sensor-name.patch \
    file://0004-configurations-yosemite4-NIC-card-support-slot-numbe.patch \
    file://0005-entity-manager-Add-config-prefix-filtering.patch \
    file://0006-meta-facebook-yosemite4-Add-virtual-NIC-temperature-.patch \
"

