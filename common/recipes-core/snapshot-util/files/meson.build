project('snapshot-util',
    'c',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

cc = meson.get_compiler('c')

sources = [
    'snapshot-util.c',
]

deps = [
    cc.find_library('bic'),
    dependency('obmc-pal'),
]

exe = executable('snapshot-util',
    sources,
    dependencies: deps,
    install: true,
)
