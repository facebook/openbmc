project('expander-util', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['c_std=c99', 'werror=true'],
    meson_version: '>=0.40')

dep_libs = [
    dependency('libipmb'),
    ]

executable('expander-util', 'expander-util.c', dependencies : dep_libs)
