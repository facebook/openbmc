project('liblog', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers(
    'log.h',
    subdir: 'openbmc')

libs = []

srcs = files(
  'log.c',
)

# log library.
log_lib = shared_library('log', srcs,
    dependencies: libs,  
    version: meson.project_version(),
    install: true)

# pkgconfig for log library.
pkg = import('pkgconfig')
pkg.generate(libraries: [log_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Macros and functions for logging control')
