project('ipmbd',
    'c',
    version: '0.1',
    license: 'GPL2',
    default_options: [
      #        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

cc = meson.get_compiler('c')

sources = [
    'ipmbd.c',
]

deps = [
    cc.find_library('rt'),
    dependency('libipc'),
    dependency('libipmi'),
    dependency('libkv'),
    dependency('liblog'),
    dependency('libmisc-utils'),
    dependency('libobmc-i2c'),
    dependency('obmc-pal'),
    dependency('threads'),
]

exe = executable('ipmbd',
    sources,
    dependencies: deps,
    install: true,
)
