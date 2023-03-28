project('libobmc-i3c', 'c',
    version: '0.1',
    license: 'GPL2',
    # Meson 0.40 only supports c++1z as an alias for c++17.
    default_options: [
        'werror=true',
        'cpp_std=c++1z',
    ],
    meson_version: '>=0.40')

install_headers(
    'i3c_dev.h',
    subdir: 'openbmc')

cc = meson.get_compiler('c')
libs = [
]

srcs = files(
    'i3c_dev.c',
)

# OpenBMC I3C Library
obmc_i3c_lib = shared_library('obmc-i3c', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

pkg = import('pkgconfig')
pkg.generate(libraries: [obmc_i3c_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'OpenBMC I3C Library')
