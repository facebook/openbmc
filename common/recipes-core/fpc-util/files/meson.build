project('fpc-util',
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
    'fpc-util.c',
]

deps = [
    dependency('obmc-pal'),
]

subdir('plat')

exe = executable('fpc-util',
    sources,
    dependencies: deps,
    install: true,
)
