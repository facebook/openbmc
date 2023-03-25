project('libfbgc_common', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers('fbgc_common.h', subdir: 'facebook')

cc = meson.get_compiler('c')

fbgc_common_deps = [
    dependency('libgpio-ctrl'),
    dependency('libfbgc_gpio'),
    dependency('openssl'),
    dependency('libkv'),
    dependency('libobmc-i2c'),
]

# FBGC common library.
fbgc_common_lib = shared_library('fbgc_common', 'fbgc_common.c',
    dependencies: fbgc_common_deps,
    version: meson.project_version(),
    install: true)

# pkgconfig for FBGC common library.
pkg = import('pkgconfig')
pkg.generate(libraries: [fbgc_common_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'FBGC common Library')
