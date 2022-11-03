project('cfg-util',
    'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'cpp_std=c++1z',
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

sources = [
    'cfg-util.cpp',
]

deps = [
    dependency('libkv'),
    dependency('libmisc-utils'),
    dependency('obmc-pal'),
]

exe = executable('cfg-util',
    sources,
    dependencies: deps,
    install: true,
)
