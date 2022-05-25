project('libnetlakemtp_common', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers('netlakemtp_common.h', subdir: 'facebook')

cc = meson.get_compiler('c')

netlakemtp_common_deps = [
    cc.find_library('gpio-ctrl'),
    cc.find_library('m'),
    dependency('openssl'),
    dependency('libkv'),
    dependency('libobmc-i2c'),
]

# NETLAKEMTP common library.
netlakemtp_common_lib = shared_library('netlakemtp_common', 'netlakemtp_common.c',
    dependencies: netlakemtp_common_deps,
    version: meson.project_version(),
    install: true)

# pkgconfig for NETLAKEMTP common library.
pkg = import('pkgconfig')
pkg.generate(libraries: [netlakemtp_common_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'NETLAKEMTP common Library')
