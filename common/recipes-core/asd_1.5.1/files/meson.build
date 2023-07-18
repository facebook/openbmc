project('libasd-jtagintf', 'c',
    version: '1.5.1',
    license: 'BSD-3-Clause',
    meson_version: '>=0.40')

install_headers(
    'include/asd_common.h',
    'target/jtag_handler.h',
    subdir: 'asd')

install_headers(
    'target/tests/jtag.h',
    subdir: 'asd/tests')

inc = include_directories('../recipe-sysroot/usr/include/safeclib')

cc = meson.get_compiler('c')
libs = [
  dependency('libsafec'),
  dependency('libgpio-ctrl'),
]

srcs = files(
  'server/logging.c',
  'target/jtag_handler.c',
  'target/pin_handler.c',
  'target/platform.c',
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
