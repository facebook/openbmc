project('libfruid', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers(
    'fruid.h',
    subdir: 'openbmc')

cc = meson.get_compiler('c')
libs = [
  dependency('libipmi'),
]

srcs = files(
  'fruid.c',
)

# FRUID library.
fruid_lib = shared_library('fruid', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for FRUID library.
pkg = import('pkgconfig')
pkg.generate(libraries: [fruid_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'library for ipmi fruid')
