FILESEXTRAPATHS:prepend := "${THISDIR}/entity-manager:"

SRC_URI += " \
    file://0001-configurations-revise-nic-temperature-name.patch \
    file://0002-configurations-harma-Update-Sparepartnumber-for-Broa.patch \
    file://0003-configuration-harma-add-PDB-power-monitor-calibratio.patch \
    file://0004-configuration-harma-set-battery-polling-rate-to-8640.patch \
    file://0005-configuration-harma-fix-swapped-fan-table-reading-an.patch \
"

