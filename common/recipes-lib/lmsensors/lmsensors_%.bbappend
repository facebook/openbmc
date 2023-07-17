FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = "\
    file://support_pwm_snr.patch \
    file://support-auxiliary-bus-type.patch \
    file://0001-lmsensors-support-iio-device.patch \
    file://0002-fix-fan-enable-attribute.patch \
"
