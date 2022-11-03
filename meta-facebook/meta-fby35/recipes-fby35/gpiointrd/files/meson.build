project('gpiointrd',
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
    'gpiointrd.c',
]

deps = [
    dependency('libbic'),
    dependency('libfby35-common'),
    dependency('libfby35-gpio'),
    dependency('libgpio-ctrl'),
    dependency('libipmi'),
    dependency('libkv'),
    dependency('libobmc-i2c'),
    dependency('libras'),
    dependency('obmc-pal'),
    dependency('threads'),
]

exe = executable('gpiointrd',
    sources,
    dependencies: deps,
    install: true,
)
