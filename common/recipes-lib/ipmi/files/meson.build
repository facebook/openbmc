project('libipmi', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers(
    'ipmi.h',
    subdir: 'openbmc')

libs = [
  dependency('libipc')
]

srcs = files(
  'ipmi.c', 
)

# IPMI library.
ipmi_lib = shared_library('ipmi', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for IPMI library.
pkg = import('pkgconfig')
pkg.generate(libraries: [ipmi_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'IPMI Client Library')
