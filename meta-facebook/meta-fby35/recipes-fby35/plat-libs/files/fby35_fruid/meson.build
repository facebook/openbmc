project('libfby35-fruid',
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
    'fby35_fruid.h',
]

sources = [
    'fby35_fruid.c',
]

deps = [
    dependency('libfby35-common'),
]

install_headers(
    headers,
    subdir: 'facebook',
)

fruid_lib = shared_library('fby35_fruid',
    sources,
    dependencies: deps,
    version: meson.project_version(),
    install: true,
)

pkg = import('pkgconfig')
pkg.generate(
    libraries: [ fruid_lib ],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Fruid library'
)
