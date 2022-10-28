project('libbic', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers(
    'bic.h',
    subdir: 'facebook')

cc = meson.get_compiler('c')
libs = [
  dependency('libgpio-ctrl'),
  dependency('libipmb'),
  dependency('libipmi'),
  dependency('libkv'),
  dependency('liblog'),
  dependency('libmisc-utils'),
  dependency('libobmc-i2c'),
]

srcs = files(
  'bic.c',
  'bic_platform.c',
)

# BIC library.
bic_lib = shared_library('bic', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for BIC library.
pkg = import('pkgconfig')
pkg.generate(libraries: [bic_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'library for communicating with Bridge IC')
