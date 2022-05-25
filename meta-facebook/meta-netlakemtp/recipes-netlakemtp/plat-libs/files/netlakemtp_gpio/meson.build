project('libnetlake_gpio', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers('netlakemtp_gpio.h', subdir: 'facebook')

# NETLAKEMTP gpio library.
netlakemtp_gpio_lib = shared_library('netlakemtp_gpio', 'netlakemtp_gpio.c',
    version: meson.project_version(),
    install: true)

# pkgconfig for NETLAKEMTP gpio library.
pkg = import('pkgconfig')
pkg.generate(libraries: [netlakemtp_gpio_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'NETLAKEMTP gpio Library')
