project('libfbgc_gpio', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers('fbgc_gpio.h', subdir: 'facebook')

# FBGC gpio library.
fbgc_gpio_lib = shared_library('fbgc_gpio', 'fbgc_gpio.c',
    version: meson.project_version(),
    install: true)

# pkgconfig for FBGC gpio library.
pkg = import('pkgconfig')
pkg.generate(libraries: [fbgc_gpio_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'FBGC gpio Library')
