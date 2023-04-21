project('libpldm-oem', 'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true', 'cpp_std=c++17'],
    meson_version: '>=0.40')

cc = meson.get_compiler('cpp')

hdrs = [
  'pldm.h',
  'oem_pldm.hpp',
  'fw_update.hpp',
]

libs = [
  dependency('libpldm')
]

srcs = [
  'pldm.cpp',
  'base.cpp',
  'platform.cpp',
  'oem.cpp',
  'oem_pldm.cpp',
  'fw_update.cpp',
  'platform_sensor.cpp',
]

subdir('plat')

install_headers(
  hdrs,
  subdir: 'libpldm-oem')

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
