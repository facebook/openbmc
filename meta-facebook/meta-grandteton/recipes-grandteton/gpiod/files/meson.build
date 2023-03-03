project('gpiod',
    'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'werror=true',
    ],
    meson_version: '>=0.40',
)


sources = [
    'gpiod.cpp',
    'gpiod_cover.cpp'
]


cc = meson.get_compiler('cpp')
deps = [
    dependency('libgpio-ctrl'),
    dependency('libnm'),
    dependency('libkv'),
    dependency('libobmc-i2c'),
    dependency('libpeci_sensors'),
    dependency('threads'),
    dependency('obmc-pal'),
    dependency('libpldm-oem'),
    dependency('libhgx'),
]

exe = executable('gpiod',
    sources,
    dependencies: deps,
    install: true,
)
