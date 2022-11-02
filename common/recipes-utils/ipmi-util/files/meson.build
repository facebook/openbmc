project('ipmi-util',
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
    'ipmi-util.c',
]

deps = [
    dependency('libipmi'),
    dependency('obmc-pal'),
    dependency('threads'),
]

exe = executable('ipmi-util',
    sources,
    dependencies: deps,
    install: true,
)
