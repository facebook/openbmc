project('iocd', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['c_std=c99', 'werror=true'],
    meson_version: '>=0.40')

cc = meson.get_compiler('c')

dep_libs = [
    dependency('libfbgc_common'),
    dependency('libobmc-i2c'),
    dependency('libipc'),
    dependency('obmc-pal'),
    dependency('threads'),
]

executable('iocd', 'iocd.c', dependencies : dep_libs, install: true)

