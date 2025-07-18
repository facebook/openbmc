FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://0002-configurations-nvidia_io_board-recalculate-bus-numbe.patch \
    file://0003-configurations-Revise-CX7-NIC-card-temperature-senso.patch \
    file://0004-configurations-nvidia_gb200_io_board-Use-external-se.patch \
    file://0005-configurations-cx7_ocp-Switch-to-external-sensor-for.patch \
"

SRC_URI:append:catalina = "file://0500-configurations-nvidia_hmc-add-SmbpbiVirtualEeprom-se.patch"
SRC_URI:append:clemente = "file://0500-configurations-nvidia_hmc-add-Virtual-Eeprom-sensor-.patch"
