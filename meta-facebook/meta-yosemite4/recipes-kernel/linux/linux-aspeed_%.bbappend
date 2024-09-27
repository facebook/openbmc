FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
    file://0500-add-meta-yv4-bmc-dts-setting.patch \
    file://0501-hwmon-ina233-Add-ina233-driver.patch \
    file://0502-Add-adm1281-driver.patch \
    file://0503-ARM-dts-aspeed-yosemite4-support-mux-to-cpld.patch \
    file://0504-ARM-dts-aspeed-yosemite4-Revise-gpio-name.patch \
    file://0505-remove-pincontrol-on-GPIO-U5.patch \
    file://0506-Meta-yv4-dts-add-mac-config-property.patch \
    file://0507-yosemite4-Add-EEPROMs-for-NICs-in-DTS.patch \
    file://0508-Add-ina233-and-ina238-devicetree-config-for-EVT-sche.patch \
    file://0509-Adjust-resistor-calibration-for-matching-hardware-de.patch \
    file://0510-ARM-dts-aspeed-yosemite4-Revise-i2c-duty-cycle.patch \
    file://0511-ARM-dts-aspeed-yosemite4-disable-GPIO-internal-Pull-.patch \
    file://0512-yosemite4-Change-IOE-i2c-address.patch \
    file://0513-hwmon-Driver-for-Nuvoton-NCT7363Y.patch \
    file://0514-Revise-duty-cycle-for-SMB9-and-SMB10.patch \
    file://0515-Add-nct7363-in-yosemite4-dts.patch \
    file://0516-i3c-master-add-enable-disable-hot-join-in-sys-entry.patch \
    file://0517-i3c-dw-Add-hot-join-support.patch \
    file://0518-i3c-ast2600-Validate-AST2600-I3C-for-MCTP-over-I3C.patch \
    file://0519-dt-bindings-i3c-hub-Add-Renesas-RG3MxxB12A1-I3C-HUB.patch \
    file://0520-i3c-i3c-hub-Add-Renesas-RG3MxxB12A1-I3C-HUB-driver.patch \
    file://0521-ARM-dts-aspeed-yosemite4-add-I3C-config-in-DTS.patch \
    file://0522-ARM-dts-aspeed-yosemite4-add-fan-led-config.patch \
    file://0523-Add-XDP710-in-yosemite4-dts.patch \
    file://0524-ARM-dts-aspeed-yosemite4-add-RTQ6056-support.patch \
    file://0525-net-mctp-i2c-invalidate-flows-immediately-on-TX-erro.patch \
    file://0526-meta-facebook-yosemite4-update-dts-file-to-enable-mp.patch \
    file://0527-ARM-dts-aspeed-yosemite4-adjust-mgm-cpld-ioexp.patch \
    file://0528-ARM-dts-aspeed-yosemite4-remove-mctp-driver-on-bus-1.patch \
    file://0529-ARM-dts-aspeed-yosemite4-fix-GPIO-linename-typo.patch \
    file://0530-ARM-dts-aspeed-yosemitet4-add-RTQ6056-support-on-11-.patch \
    file://0531-hwmon-new-driver-for-ISL28022-power-monitor.patch \
    file://0532-ARM-dts-aspeed-yosemite4-remove-power-sensor-0x44-on.patch \
    file://0533-Add-SQ52205-driver.patch \
"
