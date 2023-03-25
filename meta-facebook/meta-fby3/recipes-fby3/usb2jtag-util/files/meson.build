project('usb2jtag-util', 'c',
    version: '0.1',
    license: 'GPL2',
    meson_version: '>=0.40',
)

cc = meson.get_compiler('c')
deps = [
    dependency('libusb-1.0'),
    dependency('libftdi1'),
    dependency('libobmc-i2c'),
    dependency('libmpsse'),
]

srcs = files('usb2jtag-util.c')

exe = executable('usb2jtag-util', srcs,
    dependencies: deps,
    install: true,
)
