project('libnetlake_gpio', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers('netlakenext_gpio.h', subdir: 'facebook')

# NETLAKENEXT gpio library.
netlakenext_gpio_lib = shared_library('netlakenext_gpio', 'netlakenext_gpio.c',
    version: meson.project_version(),
    install: true)

# pkgconfig for NETLAKENEXT gpio library.
pkg = import('pkgconfig')
pkg.generate(libraries: [netlakenext_gpio_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'NETLAKENEXT gpio Library')
