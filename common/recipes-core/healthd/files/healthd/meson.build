project('healthd',
    'c',
    version: '0.2',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

srcs = [
    'healthd.c',
]

deps = [
    dependency('jansson'),
    dependency('libkv'),
    dependency('libmisc-utils'),
    dependency('libobmc-i2c'),
    dependency('libvbs'),
    dependency('libwatchdog'),
    dependency('obmc-pal'),
    dependency('threads'),
]

subdir('plat')

exe = executable(
    'healthd',
    srcs,
    dependencies: deps,
    install: true,
)

