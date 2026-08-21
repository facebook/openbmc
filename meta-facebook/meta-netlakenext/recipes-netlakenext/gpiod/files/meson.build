project('gpiod', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['c_std=c99', 'werror=true'],
    meson_version: '>=0.40')

cc = meson.get_compiler('c')

dep_libs = [
    dependency('libgpio-ctrl'),
    dependency('obmc-pal'),
    dependency('threads'),
    dependency('libkv'),
    dependency('libobmc-sensors'),
    cc.find_library('apml64'),
    cc.find_library('netlakenext_common'),
]

executable('gpiod', 'gpiod.c', dependencies : dep_libs, install: true)
