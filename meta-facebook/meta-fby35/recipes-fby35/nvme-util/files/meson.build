project('nvme-util',
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
    'nvme-util.c',
]

deps = [
    dependency('libbic'),
    dependency('libfby35-common'),
    dependency('obmc-pal'),
]

exe = executable('nvme-util',
    sources,
    dependencies: deps,
    install: true,
)
