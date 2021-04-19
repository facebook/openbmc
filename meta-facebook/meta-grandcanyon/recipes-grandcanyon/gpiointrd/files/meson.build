project('gpiointrd', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['c_std=c99', 'werror=true'],
    meson_version: '>=0.40')

cc = meson.get_compiler('c')

dep_libs = [
    cc.find_library('gpio-ctrl'),
    cc.find_library('fbgc_gpio'),
    cc.find_library('fbgc_common'),
    cc.find_library('pal'),
    dependency('threads'),
]

executable('gpiointrd', 'gpiointrd.c', dependencies : dep_libs, install: true)
