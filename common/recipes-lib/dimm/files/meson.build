project('libdimm', 'cpp',
    version: '0.1',
    license: 'LGPL2',
    # Meson 0.40 only supports c++1z as an alias for c++17.
    default_options: ['cpp_std=c++1z', 'werror=true'],
    meson_version: '>=0.40')


headers = [
  'dimm.h',
]

sources = [
  'dimm-capacity.cpp',
  'dimm-pmic.cpp',
  'dimm-vendor.cpp',
  'dimm-base.cpp',
]

deps = [
  dependency('jansson'),
  dependency('libkv'),
  dependency('libipmi')
]

# Import the plat-specific subdirectory for platform
# headers/sources/dependencies.
subdir('plat')


install_headers(
    headers,
    subdir: 'openbmc')

srcs = files(
  sources,
)

# DIMM library.
dimm_lib = shared_library(
    'dimm',
    srcs,
    dependencies: deps,
    version: meson.project_version(),
    install: true)

# pkgconfig for DIMM library.
pkg = import('pkgconfig')
pkg.generate(libraries: [dimm_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Dimm library')

