project('otp', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['c_std=c99', 'werror=true'],
    meson_version: '>=0.40')

dep_libs = []
src = ['otp.c', 'sha256.c']

executable('otp', src, dependencies : dep_libs, install: true)
