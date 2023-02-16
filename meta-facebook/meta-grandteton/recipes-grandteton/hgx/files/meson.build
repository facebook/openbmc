project('libhgx', 'c', 'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: [
      'werror=true',
      'cpp_std=c++20',
    ],
    meson_version: '>=0.40')

install_headers(
    'hgx.h',
    subdir: 'openbmc')

cc = meson.get_compiler('cpp')
libs = [
  cc.find_library('restclient-cpp'),
  dependency('libkv'),
]

srcs = [
  'hgx.cpp',
]

hgx_lib = shared_library('hgx', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for HGX library.
pkg = import('pkgconfig')
pkg.generate(libraries: [hgx_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'library for HGX')
