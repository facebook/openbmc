project('bic-util',
    'c',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

sources = [
    'bic-util.c',
]

deps = [
    dependency('libfby35-common'),
    dependency('libbic'),
    dependency('libfby35-gpio'),
    dependency('libusb-1.0'),
    dependency('libobmc-i2c'),
    dependency('obmc-pal'),
]

exe = executable('bic-util',
    sources,
    dependencies: deps,
    install: true,
)
