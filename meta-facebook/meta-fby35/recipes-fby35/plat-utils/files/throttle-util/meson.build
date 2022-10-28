project('throttle-util',
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
    'throttle-util.c',
]

deps = [
    dependency('libfby35-common'),
    dependency('libobmc-i2c'),
    dependency('obmc-pal'),
]

exe = executable('throttle-util',
    sources,
    dependencies: deps,
    install: true,
)
