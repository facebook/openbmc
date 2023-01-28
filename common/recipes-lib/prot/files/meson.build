
project('libprot', 'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'cpp_std=c++20',
        'werror=true'
    ],
)

install_headers(
    'ProtCommonInterface.hpp',
    'ProtDevice.hpp',
    'ProtLog.hpp',
    subdir: 'openbmc')

libs = [
    dependency('fmt'),
    dependency('libAmiSmbusInterface'),
    dependency('libobmc-i2c'),
]

srcs = files(
    'ProtDevice.cpp',
    'ProtLog.cpp',
)

prot_lib = shared_library('prot', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

pkg = import('pkgconfig')
pkg.generate(libraries: [prot_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Platform root of trust Library')
