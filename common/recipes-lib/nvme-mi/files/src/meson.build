project('libnvme-mi',
    'c',
    version: '1.0',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40'
)

headers = [
    'nvme-mi.h',
]

sources = [
    'nvme-mi.c',
]

deps = [
    dependency('libobmc-i2c'),
]

install_headers(
    headers,
    subdir: 'openbmc',
)

nvme_lib = shared_library('nvme-mi',
    sources,
    dependencies: deps,
    version: meson.project_version(),
    install: true,
)

pkg = import('pkgconfig')
pkg.generate(
    libraries: [ nvme_lib ],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'NVMe-MI library'
)
