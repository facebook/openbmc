project('ipmb-util',
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
    'ipmb-util.c',
]

deps = [
    dependency('threads'),
    dependency('libipmi'),
    dependency('libipmb'),
]

exe = executable('ipmb-util',
    sources,
    dependencies: deps,
    install: true,
)
