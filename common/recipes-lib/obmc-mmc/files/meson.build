project('libobmc-mmc', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'werror=true',
        'cpp_std=c++1z',
    ],
    meson_version: '>=0.40')

install_headers(
    'mmc.h',
    'mmc_int.h',
    'obmc-mmc.h',
    subdir: 'openbmc')

cc = meson.get_compiler('c')
libs = []

srcs = files(
  'lsmmc.c',
  'obmc-mmc.c',
)

# openbmc emmc library.
obmc_mmc_lib = shared_library('obmc-mmc', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

pkg = import('pkgconfig')
pkg.generate(libraries: [obmc_mmc_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Functions for eMMC device access')
