project('libobmc-pmbus',
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
    'obmc-pmbus.h',
    'pmbus.h',
]

sources = [
    'obmc-pmbus.c',
]

deps = [
    dependency('libobmc-i2c'),
]

install_headers(
    headers,
    subdir: 'openbmc',
)

pmbus_lib = shared_library('obmc-pmbus',
    sources,
    dependencies: deps,
    version: meson.project_version(),
    install: true,
)

pkg = import('pkgconfig')
pkg.generate(
    libraries: [ pmbus_lib ],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'PMBus Device Access Library'
)
