project('libmcu',
    'c',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40'
)

headers = [
    'mcu.h',
]

sources = [
    'mcu.c',
]

deps = [
    dependency('libipmb'),
    dependency('libkv'),
    dependency('libobmc-i2c'),
    dependency('obmc-pal'),
]

install_headers(
    headers,
    subdir: 'openbmc',
)

mcu_lib = shared_library('mcu',
    sources,
    dependencies: deps,
    version: meson.project_version(),
    install: true,
)

pkg = import('pkgconfig')
pkg.generate(
    libraries: [ mcu_lib ],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'MCU library'
)
