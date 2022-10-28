project('libbic',
    'c', 'cpp',
    version: '0.1',
    license: 'LGPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40')

cc = meson.get_compiler('cpp')

# OpenSSL3, which is present in Kirkstone and above, deprecates some
# SHA hash functions we are using.  Turn off the warning until we can
# port to OpenSSL3.
add_global_arguments('-Wno-error=deprecated-declarations', language: 'c')

headers = [
    'bic.h',
]

sources = [
    'bic.c',
]

deps = [
    cc.find_library('fby2_common'),
    cc.find_library('misc-utils'),
    dependency('libcrypto'),
    dependency('libgpio-ctrl'),
    dependency('libkv'),
    dependency('libobmc-i2c'),
    dependency('libipmb'),
]

install_headers(
    headers,
    subdir: 'facebook',
)

bic_lib = shared_library('bic',
    sources,
    dependencies: deps,
    version: meson.project_version(),
    install: true,
)

pkg = import('pkgconfig')
pkg.generate(
    libraries: [ bic_lib ],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Bridge IC library'
)
