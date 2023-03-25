project('libhal', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers(
    'hal_fruid.h',
    subdir: 'openbmc')

cc = meson.get_compiler('c')
libs_fruid = [
  dependency('libipmi'),
  dependency('libpldm-oem'),
]

srcs_fruid = [
  'hal_fruid.c',
]

hal_fruid_lib = shared_library('hal_fruid', srcs_fruid,
    dependencies: libs_fruid,
    version: meson.project_version(),
    install: true)

# pkgconfig for HAL library.
pkg = import('pkgconfig')
pkg.generate(libraries: [hal_fruid_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'library for platform HAL')
