FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0100-boards-ast2700_evb-bootmcu-revise-baudrate-config.patch;patchdir=zephyr \
    file://0101-boards-ast2700_evb-bootmcu-disable-PCIe-EHCI-and-PCI.patch;patchdir=zephyr \
"
