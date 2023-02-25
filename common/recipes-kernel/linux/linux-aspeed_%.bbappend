FILESEXTRAPATHS:prepend := "${THISDIR}/6.1:"

SRC_URI:append:openbmc-fb-lf = " \
    file://0100-dt-bindings-Add-bindings-for-aspeed-pwm-tach.patch \
    file://0101-pwm-Add-Aspeed-ast2600-PWM-support.patch \
    file://0102-Porting-tach-driver-for-ast26xx-from-Aspeed-linux.patch \
    file://0103-aspeed-i2c-add-clock-duty-cycle-property.patch \
    file://0104-dt-bindings-aspeed-i2c-add-properties-for-setting-i2.patch \
    file://0105-drivers-misc-Add-Aspeed-OTP-control-register.patch \
    file://0106-Add-jtag-driver.patch \
    file://0107-Add-JTAG-bindings-to-AST2600-device-tree.patch \
"
