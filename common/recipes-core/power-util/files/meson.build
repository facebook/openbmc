project('power-util',
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
    'power-util.c',
]

deps = [
    dependency('obmc-pal'),
]

exe = executable('power-util',
    sources,
    dependencies: deps,
    install: true,
)
