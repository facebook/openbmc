project('setup-gpio', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['c_std=c99', 'werror=true'],
    meson_version: '>=0.40')

cc = meson.get_compiler('c')

dep_libs = [
    cc.find_library('gpio-ctrl'),
    cc.find_library('fbgc_gpio'),
]

executable('setup-gpio', 'setup-gpio.c', dependencies : dep_libs)
