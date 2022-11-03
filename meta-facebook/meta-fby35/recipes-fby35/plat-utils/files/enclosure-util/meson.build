project('enclosure-util',
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
    'enclosure-util.c',
]

deps = [
    dependency('libfby35-common'),
    dependency('libbic'),
    dependency('libnvme-mi'),
    dependency('obmc-pal'),
]

exe = executable('enclosure-util',
    sources,
    dependencies: deps,
    install: true,
)
