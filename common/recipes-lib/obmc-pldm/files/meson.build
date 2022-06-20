project('libpldm-oem', 'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true', 'cpp_std=c++17'],
    meson_version: '>=0.40')

install_headers(
    'pldm.h',
    subdir: 'libpldm-oem')

cc = meson.get_compiler('cpp')

libs = [
  dependency('libpldm')
]

srcs = files(
  'pldm.cpp',
  'base.cpp',
  'oem.cpp',
)

# obmc-pldm-mw library.
obmc_pldm_mw = shared_library('pldm_oem', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for CPLD library.
pkg = import('pkgconfig')
pkg.generate(libraries: [obmc_pldm_mw],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Middleware for OpenBMC PLDM Library')
