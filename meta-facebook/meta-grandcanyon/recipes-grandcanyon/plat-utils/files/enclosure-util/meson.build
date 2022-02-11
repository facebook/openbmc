project('enclosure-util', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['c_std=c99', 'werror=true'],
    meson_version: '>=0.40')

dep_libs = [
    dependency('obmc-pal'),
    dependency('libexp'),
    dependency('libfbgc_common'),
    dependency('libipmb'),
    ]

executable('enclosure-util', 'enclosure-util.c', dependencies : dep_libs, install: true)
