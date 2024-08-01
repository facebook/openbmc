FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
    file://0500-add-meta-yv4-bmc-dts-setting.patch \
    file://0501-hwmon-ina233-Add-ina233-driver.patch \
    file://0503-Add-adm1281-driver.patch \
    file://0505-ARM-dts-aspeed-yosemite4-support-mux-to-cpld.patch \
    file://0506-ARM-dts-aspeed-yosemite4-Revise-gpio-name.patch \
    file://0507-remove-pincontrol-on-GPIO-U5.patch \
    file://0508-Meta-yv4-dts-add-mac-config-property.patch \
    file://0509-yosemite4-Add-EEPROMs-for-NICs-in-DTS.patch \
    file://0510-Add-ina233-and-ina238-devicetree-config-for-EVT-sche.patch \
    file://0511-Adjust-resistor-calibration-for-matching-hardware-de.patch \
    file://0512-ARM-dts-aspeed-yosemite4-Revise-i2c-duty-cycle.patch \
    file://0513-ARM-dts-aspeed-yosemite4-disable-GPIO-internal-Pull-.patch \
    file://0514-yosemite4-Change-IOE-i2c-address.patch \
    file://0515-hwmon-Driver-for-Nuvoton-NCT7363Y.patch \
    file://0516-Revise-duty-cycle-for-SMB9-and-SMB10.patch \
    file://0518-Add-nct7363-in-yosemite4-dts.patch \
    file://0519-i3c-master-add-enable-disable-hot-join-in-sys-entry.patch \
    file://0520-i3c-dw-Add-hot-join-support.patch \
    file://0521-ARM-dts-aspeed-g6-Add-AST2600-I3Cs.patch \
    file://0522-i3c-ast2600-Validate-AST2600-I3C-for-MCTP-over-I3C.patch \
    file://0523-dt-bindings-i3c-hub-Add-Renesas-RG3MxxB12A1-I3C-HUB.patch \
    file://0524-i3c-i3c-hub-Add-Renesas-RG3MxxB12A1-I3C-HUB-driver.patch \
    file://0525-ARM-dts-aspeed-yosemite4-add-I3C-config-in-DTS.patch \
    file://0526-ARM-dts-aspeed-yosemite4-add-fan-led-config.patch \
    file://0527-Add-XDP710-in-yosemite4-dts.patch \
    file://0528-ARM-dts-aspeed-yosemite4-add-RTQ6056-support.patch \
    file://0529-net-mctp-i2c-invalidate-flows-immediately-on-TX-erro.patch \
    file://0530-meta-facebook-yosemite4-update-dts-file-to-enable-mp.patch \
    file://0531-ARM-dts-aspeed-yosemite4-adjust-mgm-cpld-ioexp.patch \
    file://0532-ARM-dts-aspeed-yosemite4-remove-mctp-driver-on-bus-1.patch \
"
