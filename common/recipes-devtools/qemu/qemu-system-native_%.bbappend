FILESEXTRAPATHS:prepend := "${THISDIR}/${BPN}:"

SRC_URI += " \
    file://0500-net-slirp-Add-mfr-id-and-oob-eth-addr-parameters.patch \
    file://0501-aspeed-Zero-extend-flash-files-to-128MB.patch \
    file://0502-hw-aspeed_vic-Add-heartbeat-LED-registers.patch \
    file://0503-hw-misc-Add-i2c-netdev-device.patch \
    file://0504-hw-misc-Add-byte-by-byte-i2c-network-device.patch \
    file://0505-hw-intc-Add-a-property-to-allow-GIC-to-reset-into-no.patch \
    file://0506-hw-m25p80-Add-BP-and-TB-bits-to-n25q00.patch \
    file://0507-hw-i2c-pca954x-Add-method-to-get-channels.patch \
    file://0508-hw-arm-aspeed-Add-fb_machine_class_init.patch \
    file://0509-fbttn-Add-I2C-SCL-timeout-property.patch \
    file://0510-hw-i2c-aspeed-Add-bus-ID-to-all-trace-events.patch \
    file://0511-hw-i2c-aspeed-Add-slave-event-traces.patch \
    file://0512-hw-i2c-aspeed-Fix-bus-derivation-for-slave-events.patch \
    file://0513-tests-Rename-aspeed_i2c-test-to-i2c-netdev2-test.patch \
    file://0514-hw-i2c-aspeed-Fix-interrupt-status-flag-names.patch \
    file://0515-tests-Create-qtest-for-Aspeed-I2C-controller.patch \
    file://0516-scripts-Add-redact-util.patch \
    file://0517-aspeed-Expose-i2c-buses-to-machine.patch \
    file://0518-aspeed-fby35-Add-bmc-bb-and-nic-FRU-s.patch \
    file://0519-hw-misc-aspeed-Add-fby35-sb-cpld.patch \
    file://0520-hw-misc-aspeed-Add-intel-me.patch \
    file://0521-hw-misc-aspeed-Add-fby35-server-board-bridge-IC.patch \
    file://0522-hw-arm-aspeed-Switch-fby35-grandcanyon-to-n25q00.patch \
    file://0523-fby35-Add-CPLD-and-BIC-as-I2C-devices.patch \
    file://0524-fby35-Setup-I2C-devices-and-GPIO-s.patch \
    file://0525-fby35-Add-motherboard-fru-EEPROM-to-BIC.patch \
    file://0526-qemu-Add-i2c-devices-to-oby35-cl.patch \
    file://0527-hw-arm-aspeed-Don-t-initialize-fby35-bmc-GPIO-s.patch \
    file://0528-aspeed-Add-grandteton-bmc.patch \
    file://0529-aspeed-Add-greatlakes-bmc.patch \
    file://0530-aspeed-Add-Sandia.patch \
    file://0531-aspeed-add-montblanc-bmc-reference-from-fuji.patch \
    file://0532-aspeed-add-janga-bmc.patch \
    "

PACKAGECONFIG:remove = "alsa"
PACKAGECONFIG:remove = "kvm"
PACKAGECONFIG:remove = "png"
