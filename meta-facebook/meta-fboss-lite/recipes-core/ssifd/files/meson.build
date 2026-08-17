project('ssifd', 'c',
  version: '0.1',
  license: 'GPL-2.0-or-later',
  default_options: [
    'c_std=gnu99',
    'warning_level=1',
    'werror=true',
  ],
)

cc = meson.get_compiler('c')

threads_dep = dependency('threads')
ipmi_dep = cc.find_library('ipmi')
log_dep = cc.find_library('log')

executable('ssifd',
  'ssifd.c',
  dependencies: [threads_dep, ipmi_dep, log_dep],
  install: true,
  install_dir: '/usr/local/fbpackages/ssifd',
)
