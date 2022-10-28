project('usb2jtag-util',
    'c',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

deps = [
    dependency('libftdi1'),
    dependency('libmpsse'),
    dependency('libobmc-i2c'),
    dependency('libusb-1.0'),
]

srcs = files('usb2jtag-util.c')

exe = executable('usb2jtag-util',
    srcs,
    dependencies: deps,
    install: true,
)
