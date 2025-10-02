project(
  'retimer-util',
  'c', 'cpp',
  version: '0.1',
  license: 'GPL2',
  default_options: [
    'buildtype=debugoptimized',
    'cpp_std=c++23',
    'warning_level=3',
    'werror=true',
  ],
  meson_version: '>=1.1.1',
)

retimer_util = executable(
  'retimer-util',
  'retimer-util.cpp',
  dependencies: [
      dependency('libretimer'),
      dependency('libkv'),
      dependency('libmisc-utils'),
  ],
  install: true,
  install_dir : 'local/bin',
)