project('libgpio', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true', 'warning_level=3'],
    meson_version: '>=0.40')

install_headers(
    'gpio.h',
    subdir: 'openbmc')

libs = [
    dependency('liblog'),
    dependency('threads'),
    ]

srcs = files('gpio.c', 'gpio_name.c')

# gpio library.
gpio_lib = shared_library('gpio', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for GPIO library.
pkg = import('pkgconfig')
pkg.generate(libraries: [gpio_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'GPIO Access Library')
