project('libnetlakenext_common', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers('netlakenext_common.h', subdir: 'facebook')

cc = meson.get_compiler('c')

netlakenext_common_deps = [
    cc.find_library('gpio-ctrl'),
    cc.find_library('m'),
    dependency('openssl'),
    dependency('libkv'),
    dependency('libobmc-i2c'),
    dependency('libipmb'),
]

# NETLAKENEXT common library.
netlakenext_common_lib = shared_library('netlakenext_common', 'netlakenext_common.c',
    dependencies: netlakenext_common_deps,
    version: meson.project_version(),
    install: true)

# pkgconfig for NETLAKENEXT common library.
pkg = import('pkgconfig')
pkg.generate(libraries: [netlakenext_common_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'NETLAKENEXT common Library')
