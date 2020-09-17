project('libaggregate-sensor', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers(
    'aggregate-sensor.h',
    subdir: 'openbmc')

cc = meson.get_compiler('c')
libs = [
  cc.find_library('jansson'),
  cc.find_library('pal'),
  cc.find_library('sdr'),
  dependency('libkv')
]

srcs = files('aggregate-sensor.c', 'aggregate-sensor-json.c', 'math_expression.c')

# KV library.
ags_lib = shared_library('aggregate-sensor', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for KV library.
pkg = import('pkgconfig')
pkg.generate(libraries: [ags_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Aggregate Sensor Library')

# Test cases.
test_libs = [cc.find_library('jansson')]
ags_test = executable('test-aggregate-sensor', 'test/aggregate-sensor-test.c', srcs,
    dependencies: test_libs,
    c_args: ['-D__TEST__'])
test('aggregate-sensor-tests', ags_test)
