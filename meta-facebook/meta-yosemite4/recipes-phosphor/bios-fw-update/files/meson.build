project(
  'bios-fw-update',
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

dep_libs = [
    dependency('boost'),
    dependency('sdbusplus'),
    cc.find_library('usb-1.0'),
]

src = [
  'bios-update.cpp',
  'bios-usb-update.cpp',
]

executable('bios-fw-update', src, dependencies : dep_libs)