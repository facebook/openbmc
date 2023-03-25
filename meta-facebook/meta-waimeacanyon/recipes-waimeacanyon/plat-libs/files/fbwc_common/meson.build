project('libfbwc_common', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers('fbwc_common.h', subdir: 'facebook')

cc = meson.get_compiler('c')

fbwc_common_deps = [
    dependency('libgpio-ctrl'),
    dependency('openssl'),
    dependency('libkv'),
    dependency('libobmc-i2c'),
]

# FBWC common library.
fbwc_common_lib = shared_library('fbwc_common', 'fbwc_common.c',
    dependencies: fbwc_common_deps,
    version: meson.project_version(),
    install: true)

# pkgconfig for FBWC common library.
pkg = import('pkgconfig')
pkg.generate(libraries: [fbwc_common_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'FBWC common Library')
