project('libvbs',
    'c',
    version: '1.0',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40')

headers = [
    'vbs.h',
]

sources = [
    'vbs.c',
]

install_headers(
    headers,
    subdir: 'openbmc',
)

vbs_lib = shared_library('vbs',
    sources,
    dependencies: [],
    version: meson.project_version(),
    install: true,
)

pkg = import('pkgconfig')
pkg.generate(
    libraries: [ vbs_lib ],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Verfied boot library'
)
