project('fpc-util', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['c_std=c99', 'werror=true'],
    meson_version: '>=0.40')

cc = meson.get_compiler('c')

dep_libs = [
    cc.find_library('pal'),
    dependency('libipmi'),
    dependency('libipmb'),
]

executable('fpc-util', 'fpc-util.c', dependencies : dep_libs)
