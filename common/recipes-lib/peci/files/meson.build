project('libpeci_sensors', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers(
    'peci_sensors.h',
    subdir: 'openbmc')

cc = meson.get_compiler('c')
libs = [
  cc.find_library('peci'),
]

srcs = files(
  'peci_sensors.c',
)

# peci sensors library.
peci_sensors_lib = shared_library('peci_sensors', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for peci sensors library.
pkg = import('pkgconfig')
pkg.generate(libraries: [peci_sensors_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'library for peci sensors')
