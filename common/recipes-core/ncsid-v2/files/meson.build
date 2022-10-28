project('ncsid',
    'c',
    version: '0.2',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

cc = meson.get_compiler('c')

sources = [
    'ncsid.c',
]

deps = [
    cc.find_library('rt'),
    dependency('libkv'),
    dependency('libncsi'),
    dependency('libnl-wrapper'),
    dependency('libobmc-pldm'),
    dependency('obmc-pal'),
    dependency('threads'),
]

exe = executable('ncsid',
    sources,
    dependencies: deps,
    install: true,
)
