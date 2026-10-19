project(
  'amd-cpu-manager',
  'cpp',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++23'
  ],
  license: 'Apache-2.0',
  version: '1.0',
)

cpp_args = []

cpp = meson.get_compiler('cpp')

libdir = meson.current_source_dir() + '/lib'
apml_dep = cpp.find_library('apml64', dirs : libdir)

deps = [
  # Explicitly add boost dependency as it is used for asio.
  dependency('boost', modules: ['system', 'asio']),
  dependency('systemd'),
  dependency('sdbusplus'),
  dependency('phosphor-logging'),
  apml_dep,
]

executable(
  'amd-cpu-manager',
  'src/amd-cpu-manager.cpp',
  include_directories: include_directories('inc'),
  cpp_args: cpp_args,
  dependencies: deps,
  install: true,
  install_dir: get_option('bindir'))

systemd = dependency('systemd')

install_data(
  ['service_files/amd-cpu-manager.service'],
  install_dir: systemd.get_pkgconfig_variable('systemdsystemunitdir')
)
