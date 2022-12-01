project('gpiod',
    'c',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'werror=true',
    ],
    meson_version: '>=0.40',
)


sources = [
    'gpiod.c',
    'gpiod_cover.c'
]


cc = meson.get_compiler('c')
deps = [
    dependency('libgpio-ctrl'),
    dependency('libnm'),
    dependency('libkv'),
    dependency('libobmc-i2c'),
    dependency('libpeci_sensors'),
    dependency('threads'),
    dependency('obmc-pal'),
]

exe = executable('gpiod',
    sources,
    dependencies: deps,
    install: true,
)
