project('libnl-wrapper',
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
    'nl-wrapper.h',
]

sources = [
    'nl-wrapper.c',
]

deps = [
    dependency('libnl-3.0'),
    dependency('libnl-cli-3.0'),
    dependency('libnl-genl-3.0'),
    dependency('libnl-nf-3.0'),

]

install_headers(
    headers,
    subdir: 'openbmc',
)

nl_lib = shared_library('nl-wrapper',
    sources,
    dependencies: deps,
    version: meson.project_version(),
    install: true,
)

pkg = import('pkgconfig')
pkg.generate(
    libraries: [ nl_lib ],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Netlink Wrapper library'
)
