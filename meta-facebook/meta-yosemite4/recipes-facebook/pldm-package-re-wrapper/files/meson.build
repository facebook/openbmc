project(
  'pldm-package-re-wrapper',
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
  'main.cpp',
]

libpldm_dep = dependency('libpldm', fallback:['libpldm','libpldm_dep'], include_type: 'system')

dep_libs = [
  libpldm_dep,
  dependency('libpldm'),
]

executable(
  'pldm-package-re-wrapper',
  src,
  dependencies: dep_libs,
  install: true,
)
