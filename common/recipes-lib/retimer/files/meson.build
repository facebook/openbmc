project('libretimer', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=false', 'warning_level=3'],
    meson_version: '>=0.40')

install_headers(
    'include/aries_a0_reg_defines.h',
    'include/aries_api.h',
    'include/aries_api_types.h',
    'include/aries_error.h',
    'include/aries_globals.h',
    'include/aries_i2c.h',
    'include/aries_link.h',
    'include/aries_margin.h',
    'include/aries_misc.h',
    'include/aries_mpw_reg_defines.h',
    'include/astera_log.h',
    subdir: 'openbmc')

rt_deps = [
    dependency('libobmc-i2c'),
    ]

srcs = files(
    'aries_api.c',
    'aries_i2c.c',
    'aries_link.c',
    'aries_margin.c',
    'aries_misc.c',
    'astera_log.c')

# Retimer library.
rt_lib = shared_library('retimer', srcs,
    dependencies: rt_deps,
    version: meson.project_version(),
    install: true)

# pkgconfig for Retimer library.
pkg = import('pkgconfig')
pkg.generate(libraries: [rt_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Aries retimer Library')
