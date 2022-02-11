project('libphymem', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers(
    'phymem.h',
    subdir: 'openbmc')

srcs = files(
  'phymem.c',
)

# Physical Mem library.
phymem_lib = shared_library('phymem', srcs,
    version: meson.project_version(),
    install: true)

# pkgconfig for Physical Mem library.
pkg = import('pkgconfig')
pkg.generate(libraries: [phymem_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Phymem Operation library for applications on openbmc kernel 4.1 or higher')
