project('me-util', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['c_std=c99', 'werror=true'],
    meson_version: '>=0.40')

dep_libs = [
    dependency('obmc-pal'),
    dependency('libbic'),
    dependency('libfbgc_common'),
    dependency('libipmi'),
    dependency('libipmb'),
]

executable('me-util', 'me-util.c', dependencies : dep_libs, install: true)
