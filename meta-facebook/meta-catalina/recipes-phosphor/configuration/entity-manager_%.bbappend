FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://0001-configurations-nvidia_hmc-add-SmbpbiVirtualEeprom-se.patch \
    file://0002-configurations-nvidia_io_board-recalculate-bus-numbe.patch \
    file://0003-configurations-Revise-CX7-NIC-card-temperature-senso.patch \
    file://0004-configurations-nvidia_gb200_io_board-Use-external-se.patch \
    file://0005-configurations-cx7_ocp-Switch-to-external-sensor-for.patch \
    file://0006-configurations-add-NVIDIA-GB300-config.patch \
    file://0007-configurations-add-NVIDIA-GB300-IO-board-config.patch \
"
