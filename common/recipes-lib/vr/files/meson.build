project('libvr', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers(
    'vr.h',
    subdir: 'openbmc')

cc = meson.get_compiler('c')
libs = [
  dependency('libkv'),
  dependency('libobmc-i2c'),
  dependency('libobmc-pmbus'),
  dependency('obmc-pal'),
]

srcs = files(
  'mpq8645p.c',
  'platform.c',
  'pxe1110c.c',
  'tps53688.c',
  'vr.c',
  'xdpe12284c.c',
  'xdpe152xx.c',
  'raa_gen3.c',
  'tda38640.c',
  'mp2856.c',
)

subdir('plat')

# VR library.
vr_lib = shared_library('vr', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for VR library.
pkg = import('pkgconfig')
pkg.generate(libraries: [vr_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'library for communication with the voltage regulator')
