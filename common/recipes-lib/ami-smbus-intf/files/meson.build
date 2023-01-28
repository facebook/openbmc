
project('libAmiSmbusInterface', 'c',
    version: '0.1',
    default_options: [
        'cpp_std=c++20',
        'werror=true'
    ],
)

install_headers(
    'AmiSmbusInterfaceSrcLib.h',
    subdir: 'openbmc')

libs = [
  dependency('libkv'),
  dependency('libobmc-i2c'),
]

srcs = files(
  'AmiSmbusInterfaceSrcLib.c',
)

# AmiSmbusInterface library.
AmiSmbusInterface_lib = shared_library('AmiSmbusInterface', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for AmiSmbusInterface library.
pkg = import('pkgconfig')
pkg.generate(libraries: [AmiSmbusInterface_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'AmiSmbusInterface library')
