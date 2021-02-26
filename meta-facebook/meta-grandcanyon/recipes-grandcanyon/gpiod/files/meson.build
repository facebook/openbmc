project('gpiod', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['c_std=c99', 'werror=true'],
    meson_version: '>=0.40')

dep_libs = [
    dependency('libgpio-ctrl'),
    dependency('libfbgc_gpio'),
    dependency('libfbgc_common'),
    dependency('obmc-pal'),
    dependency('threads'),
]

executable('gpiod', 'gpiod.c', dependencies : dep_libs, install: true)
