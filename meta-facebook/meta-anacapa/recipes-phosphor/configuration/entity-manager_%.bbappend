FILESEXTRAPATHS:prepend := "${THISDIR}/entity-manager:"

SRC_URI:append = " \
    file://0001-configurations-revise-OPC-NIC-sensor-name.patch \
    file://0002-configuration-anacapa-Update-Mortaro-sensors-under-N.patch \
    file://0003-configurations-samsung-Add-PM9D3a-E1.S-naming.patch \
"
