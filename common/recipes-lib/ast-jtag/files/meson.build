project('libast-jtag', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers(
    'ast-jtag.h',
    'jtag.h',
    subdir: 'openbmc')

cc = meson.get_compiler('c')
libs = [
  dependency('libmisc-utils'),
  dependency('threads'),
]

srcs = files(
  'ast-jtag.c',
  'ast-jtag-intf.c',
  'ast-jtag-legacy.c',
)

# ast-jtag library.
ast_jtag_lib = shared_library('ast-jtag', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for ast-jtag library.
pkg = import('pkgconfig')
pkg.generate(libraries: [ast_jtag_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Library for communicating with ASPEED JTAG controller driver (kernel 5.0.3)')
