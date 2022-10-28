project('libpex', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers(
    'pex88000.h',
    subdir: 'openbmc')

cc = meson.get_compiler('c')
libs = [
  dependency('libobmc-pmbus'),
  cc.find_library('misc-utils'),
  dependency('libobmc-i2c'),
]

srcs = files(
  'pex88000.c',
)

# PEX library.
pex_lib = shared_library('pex', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for PEX library.
pkg = import('pkgconfig')
pkg.generate(libraries: [pex_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'library for pex')
