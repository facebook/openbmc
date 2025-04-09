project('libretimer', 'c',
    version: '0.1.0',
    license: 'GPL2',
    default_options: ['werror=false', 'warning_level=3'],
    meson_version: '>=0.40')

headers = [
    'kb900x.h',
    'kb900x_comm.h',
    'kb900x_addresses.h',
    'kb900x_utils.h',
    'kb900x_log.h',
]

sources = [
    'kb900x.c',
    'kb900x_comm.c',
    'plat/kb900x_i2c_comm.c',
    'plat/kb900x_bic_comm.c',
    'kb900x_utils.c',
    'kb900x_log.c',
]

rt_deps = [
#    dependency('libobmc-i2c'),
]

# Import the plat-specific subdirectory for platform
# headers/sources/dependencies. FIXME: for now added manually in sources and headers
# subdir('plat')


install_headers(
    headers,
    subdir: 'openbmc'
)

srcs = files(
  sources,
)

# Retimer library.
rt_lib = shared_library(
    'libretimer', srcs,
    dependencies: rt_deps,
    version: meson.project_version(),
    install: true
)

# pkgconfig for Retimer library.
pkg = import('pkgconfig')
pkg.generate(libraries: [rt_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Regli retimer Library')
