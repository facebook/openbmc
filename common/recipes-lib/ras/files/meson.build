project('libras', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers(
    'obmc-ras.h',
    'ras.h',
    subdir: 'openbmc')

cc = meson.get_compiler('c')
libs = [
  cc.find_library('gpio-ctrl'),
  dependency('threads'),
]

srcs = files(
  'obmc-ras.c',
  'ras.c',
)

# RAS library.
ras_lib = shared_library('ras', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for RAS library.
pkg = import('pkgconfig')
pkg.generate(libraries: [ras_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'RAS library. Reliability, availability and serviceability (RAS), is a computer hardware engineering term involving reliability engineering, high availability, and serviceability design')
