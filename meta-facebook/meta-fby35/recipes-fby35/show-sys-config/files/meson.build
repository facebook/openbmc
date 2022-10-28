project('show-sys-config',
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
    'show_sys_config.c',
]

deps = [
    dependency('jansson'),
    dependency('libbic'),
    dependency('libfby35-common'),
    dependency('libipmb'),
    dependency('libipmi'),
    dependency('obmc-pal'),
]

exe = executable('show-sys-config',
    sources,
    dependencies: deps,
    install: true,
)
