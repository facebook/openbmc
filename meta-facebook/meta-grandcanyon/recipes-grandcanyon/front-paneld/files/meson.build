project('front-paneld', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['c_std=c99', 'werror=true'],
    meson_version: '>=0.40')

cc = meson.get_compiler('c')

dep_libs = [
    cc.find_library('pal'),
    dependency('threads'),
    dependency('libfbgc_common'),
]

executable('front-paneld', 'front-paneld.c', dependencies : dep_libs)
