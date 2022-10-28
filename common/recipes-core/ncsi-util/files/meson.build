project('ncsi-util',
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
    'brcm-ncsi-util.c',
    'ncsi-util.c',
    'nvidia-ncsi-util.c',
]

deps = [
    dependency('libkv'),
    dependency('libncsi'),
    dependency('libnl-wrapper'),
    dependency('libobmc-pldm'),
    dependency('obmc-pal'),
    dependency('zlib'),
]

exe = executable('ncsi-util',
    sources,
    dependencies: deps,
    install: true,
)
