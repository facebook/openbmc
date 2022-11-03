project('gpiod',
    'c',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

sources = [
    'gpiod.c',
]

deps = [
    dependency('libbic'),
    dependency('libfby35-common'),
    dependency('libfby35-gpio'),
    dependency('libfruid'),
    dependency('libkv'),
    dependency('obmc-pal'),
]

exe = executable('gpiod',
    sources,
    dependencies: deps,
    install: true,
)
