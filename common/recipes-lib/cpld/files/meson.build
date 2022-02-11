project('libfpga', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers(
    'altera.h',
    'cpld.h',
    subdir: 'openbmc')

cc = meson.get_compiler('c')
libs = [
  cc.find_library('ast-jtag'),
  cc.find_library('pal'),
  dependency('threads'),
]

srcs = files(
  'altera.c',
  'cpld.c',
  'lattice.c',
)

# CPLD library.
fpga_lib = shared_library('fpga', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for CPLD library.
pkg = import('pkgconfig')
pkg.generate(libraries: [fpga_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Library for communicating with CPLD')
