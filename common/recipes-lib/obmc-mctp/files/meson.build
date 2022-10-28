project('libobmc-mctp',
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
    'obmc-mctp.h',
]

sources = [
    'obmc-mctp.c',
]

deps = [
    cc.find_library('mctp_intel'),
    dependency('libobmc-pldm'),
]

install_headers(
    headers,
    subdir: 'openbmc',
)

mctp_lib = shared_library('obmc-mctp',
    sources,
    dependencies: deps,
    version: meson.project_version(),
    install: true,
)

pkg = import('pkgconfig')
pkg.generate(
    libraries: [ mctp_lib ],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Helper Abstraction to the Intel MCTP library'
)
