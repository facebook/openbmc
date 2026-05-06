project(
  'cxl-coredump-collection',
  'cpp',
  version: '0.1',
  meson_version: '>=0.57.0',
  license: 'Apache-2.0',
  default_options: [
    'werror=true',
    'warning_level=3',
    'cpp_std=c++20',
  ])

cc = meson.get_compiler('cpp')

src = [
  'cxl-coredump-collection.cpp',
]

cli11_dep = dependency('CLI11')

dep_libs = [
  cli11_dep,
]
executable(
  'cxl-coredump-collection-qspi',
  src,
  dependencies: dep_libs,
  install: true,
)
