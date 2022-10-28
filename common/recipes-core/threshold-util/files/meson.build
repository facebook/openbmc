project('threshold-util',
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
    'threshold-util.c',
]

deps = [
    dependency('libsdr'),
    dependency('obmc-pal'),
]

exe = executable('threshold-util',
    sources,
    dependencies: deps,
    install: true,
)
