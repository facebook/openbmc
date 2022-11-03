project('front-paneld',
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
    'front-paneld.c',
]

deps = [
    dependency('libfby35-common'),
    dependency('libkv'),
    dependency('obmc-pal'),
]

exe = executable('front-paneld',
    sources,
    dependencies: deps,
    install: true,
)
