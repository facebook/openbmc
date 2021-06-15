project('libfby35_gpio', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers('fby35_gpio.h', subdir: 'facebook')

# FBY35 gpio library.
fby35_gpio_lib = shared_library('fby35_gpio', 'fby35_gpio.c',
    version: meson.project_version(),
    install: true)

# pkgconfig for FBY35 gpio library.
pkg = import('pkgconfig')
pkg.generate(libraries: [libfby35_gpio.so],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'FBY35 gpio Library')
