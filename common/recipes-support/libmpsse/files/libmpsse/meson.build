project('libmpsse', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers('mpsse.h', subdir: 'facebook')

cc = meson.get_compiler('c')
libs = [
  cc.find_library('usb-1.0'),
  cc.find_library('ftdi1'),
]

srcs = files('mpsse.c')

# libmpsse lib
ags_lib = shared_library('mpsse', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for libmpsse
pkg = import('pkgconfig')
pkg.generate(libraries: [ags_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'MPSSE Library for JTAG')
