project('libnm', 'c',
    version: '0.1',
    license: 'LGPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

headers = [
  'nm.h',
]

sources = [
  'nm.c',
]

deps = [
  dependency('libipmb'),
  dependency('libipmi')
]

install_headers(
    headers,
    subdir: 'openbmc')

srcs = files(
  sources,
)

# nm library.
nm_lib = shared_library(
    'nm',
    srcs,
    dependencies: deps,
    version: meson.project_version(),
    install: true)

# pkgconfig for node manager library.
pkg = import('pkgconfig')
pkg.generate(libraries: [nm_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Node Manager library')

