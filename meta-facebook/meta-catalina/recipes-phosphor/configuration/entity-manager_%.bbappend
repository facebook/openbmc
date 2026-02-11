FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://0002-configurations-nvidia_io_board-recalculate-bus-numbe.patch \
    file://0003-configurations-Revise-CX7-NIC-card-temperature-senso.patch \
    file://0004-configurations-nvidia_gb200_io_board-Use-external-se.patch \
    file://0005-configurations-cx7_ocp-Switch-to-external-sensor-for.patch \
    file://0006-configurations-nvidia-Update-sensor-thresholds.patch \
"

SRC_URI:append = " \
    file://0500-configurations-nvidia_hmc-add-SmbpbiVirtualEeprom-se.patch \
"

SRC_URI:append:clemente = " \
    file://0501-Update-nvidia_hmc.json.patch \
    file://0502-Update-CX7-sensor-thresholds.patch \
    file://0503-configurations-nvidia-Fix-cable-cartridge-name-enume.patch \
    file://0504-configurations-nvidia-Use-external-senosr-for-gb300-.patch \
"
