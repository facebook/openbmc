project('libgpio-ctrl', 'c', 'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: [
      'werror=true',
      'cpp_std=c++1z',
    ],
    meson_version: '>=0.40')

install_headers(
    'libgpio.h', 'libgpio.hpp',
    subdir: 'openbmc')

cc = meson.get_compiler('c')
libs = [
  dependency('threads'),
  cc.find_library('misc-utils'),
  cc.find_library('obmc-i2c'),
]

srcs = files(
  'gpio.c',
  'gpio_sysfs.c',
  'gpiochip.c',
  'gpiochip_aspeed.c',
)

# GPIO Control Library
gpio_ctrl_lib = shared_library('gpio-ctrl', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

pkg = import('pkgconfig')
pkg.generate(libraries: [gpio_ctrl_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'GPIO control library')

# Test libs.
test_libs = [
  cc.find_library('gtest'),
  cc.find_library('gmock'),
  cc.find_library('gtest_main')
]

cppc = meson.get_compiler('cpp')

gpio_ctrl_test = executable('test-gpio-control', 'gpio_test.cpp', srcs,
  dependencies: [libs, test_libs],
  cpp_args: ['-D__TEST__'], c_args:['-D__TEST__'])
test('gpio-control-tests', gpio_ctrl_test)
