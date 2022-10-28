project('libfby35-common',
    'c',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40'
)

cc = meson.get_compiler('c')

headers = [
    'fby35_common.h',
]

sources = [
    'fby35_common.c',
]

deps = [
    dependency('libcrypto'),
    dependency('libgpio-ctrl'),
    dependency('libkv'),
    dependency('libobmc-i2c'),
]

install_headers(
    headers,
    subdir: 'facebook',
)

common_lib = shared_library('fby35_common',
    sources,
    dependencies: deps,
    version: meson.project_version(),
    install: true,
)

pkg = import('pkgconfig')
pkg.generate(
    libraries: [ common_lib ],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'System Common library'
)
