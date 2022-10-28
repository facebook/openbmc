project('libncsi',
    'c',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40'
)

cc = meson.get_compiler('c')

headers = [
    'aen.h',
    'ncsi-brcm.h',
    'ncsi-nvidia.h',
    'ncsi.h',
]

sources = [
    'aen.c',
    'ncsi-brcm.c',
    'ncsi-nvidia.c',
    'ncsi.c',
]

deps = [
    dependency('libkv'),
    dependency('threads'),
]

install_headers(
    headers,
    subdir: 'openbmc',
)

ncsi_lib = shared_library('ncsi',
    sources,
    dependencies: deps,
    version: meson.project_version(),
    install: true,
)

pkg = import('pkgconfig')
pkg.generate(
    libraries: [ ncsi_lib ],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'NC-SI library',
)
