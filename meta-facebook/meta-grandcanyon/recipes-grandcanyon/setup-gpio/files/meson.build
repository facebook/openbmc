project('setup-gpio', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['c_std=c99', 'werror=true'],
    meson_version: '>=0.40')

cc = meson.get_compiler('c')

dep_libs = [
    dependency('libgpio-ctrl'),
    dependency('libfbgc_gpio'),
    dependency('libphymem'),
]

executable('setup-gpio', 'setup-gpio.c', dependencies : dep_libs)
