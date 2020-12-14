project('bic-cached', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['c_std=c99', 'werror=true'],
    meson_version: '>=0.40')

cc = meson.get_compiler('c')

dep_libs = [
    cc.find_library('pal'),
    cc.find_library('bic'),
    dependency('threads'),
]

executable('bic-cached', 'bic-cached.c', dependencies : dep_libs, install: true)
