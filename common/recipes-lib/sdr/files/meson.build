project('libsdr',
    'c',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40'
)

headers = [
    'sdr.h',
]

sources = [
    'sdr.c',
]

deps = [
    dependency('libipmi'),
    dependency('obmc-pal'),
]

install_headers(
    headers,
    subdir: 'openbmc',
)

sdr_lib = shared_library('sdr',
    sources,
    dependencies: deps,
    version: meson.project_version(),
    install: true,
)

pkg = import('pkgconfig')
pkg.generate(
    libraries: [ sdr_lib ],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'SDR library'
)
