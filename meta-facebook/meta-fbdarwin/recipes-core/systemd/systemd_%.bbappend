do_install:append() {
    # Darwin has no usb0 connection to the uServer
    sed -i 's@ExecStart.*@\0 --ignore=usb0@' ${D}${systemd_unitdir}/system/systemd-networkd-wait-online.service
}