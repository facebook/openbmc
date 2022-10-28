project('setup-gpio', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['c_std=c99', 'werror=true'],
    meson_version: '>=0.40')

cc = meson.get_compiler('c')

dep_libs = [
    dependency('libfby35-common'),
    dependency('libfby35_gpio'),
    dependency('libgpio-ctrl'),
    dependency('libkv'),
    dependency('libphymem'),
    dependency('openssl'),
]

executable('setup-gpio', 'setup-gpio.c', dependencies : dep_libs)
