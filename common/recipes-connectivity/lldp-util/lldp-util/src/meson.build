project('lldp-util',
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
    'lldp_util.c',
]

deps = [
]

exe = executable('lldp-util',
    sources,
    dependencies: deps,
    install: true,
)
