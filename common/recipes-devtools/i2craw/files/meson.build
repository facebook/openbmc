project('i2craw',
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
    'i2craw.c',
]

deps = [
    dependency('liblog'),
    dependency('libobmc-i2c'),
]

exe = executable('i2craw',
    sources,
    dependencies: deps,
    install: true,
)
