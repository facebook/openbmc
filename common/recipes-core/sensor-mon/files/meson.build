project('sensor-mon',
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
    'sensord.c',
]

deps = [
    dependency('libaggregate-sensor'),
    dependency('libkv'),
    dependency('libsdr'),
    dependency('obmc-pal'),
    dependency('threads'),
]

exe = executable('sensord',
    sources,
    dependencies: deps,
    install: true,
)
