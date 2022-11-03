project('guid-util',
    'c',
    version: '0.2',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

sources = [
    'guid-util.c',
]

deps = [
    dependency('obmc-pal'),
]

exe = executable('guid-util',
    sources,
    dependencies: deps,
    install: true,
)
