project('gpiocli',
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
    'gpiocli.c',
]

deps = [
    dependency('libgpio-ctrl'),
    dependency('libmisc-utils'),
]

exe = executable('gpiocli',
    sources,
    dependencies: deps,
    install: true,
)
