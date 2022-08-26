project('libasd-jtagintf', 'c',
    version: '1.4.7',
    license: 'BSD-3-Clause',
    meson_version: '>=0.40')

install_headers(
    'daemon/asd_common.h',
    'daemon/jtag_handler.h',
    subdir: 'asd')

install_headers(
    'daemon/tests/jtag.h',
    subdir: 'asd/tests')

inc = include_directories('../recipe-sysroot/usr/include/safeclib')

cc = meson.get_compiler('c')
libs = [
  dependency('libsafec'),
]

srcs = files(
  'daemon/logging.c',
  'daemon/jtag_handler.c',
  'daemon/pin_handler.c',
  'daemon/platform.c',
)

# JTAG library
jtag_lib = shared_library('asd-jtagintf', srcs,
    dependencies: libs,
    include_directories: inc,
    version: meson.project_version(),
    install: true)

# pkgconfig for JTAG library
pkg = import('pkgconfig')
pkg.generate(libraries: [jtag_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'library for ASD JTAG interface')
