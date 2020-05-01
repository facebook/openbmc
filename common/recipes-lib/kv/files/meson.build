project('libkv', 'cpp',
    version: '0.1',
    license: 'GPL2',
    # Meson 0.40 only supports c++1z as an alias for c++17.
    default_options: ['cpp_std=c++1z', 'werror=true'],
    meson_version: '>=0.40')

install_headers(
    'kv.h', 'kv.hpp',
    subdir: 'openbmc')

libs = []

# GCC versions earlier than 9 require linking with stdc++fs to use
# std::filesystem functionality.
cc = meson.get_compiler('cpp')
if cc.get_id() == 'gcc' and cc.version().version_compare('<9')
    libs += [ cc.find_library('stdc++fs') ]
endif

srcs = files('kv.cpp', 'fileops.cpp')

# KV library.
kv_lib = shared_library('kv', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for KV library.
pkg = import('pkgconfig')
pkg.generate(libraries: [kv_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'KV Store Library')

# 'kv' shell script.
install_data('kv',
    install_dir: get_option('bindir'),
    install_mode: 'rwxr-xr-x')

# Test cases.
kv_test = executable('test-kv', 'test-kv.cpp', srcs,
    dependencies: libs,
    cpp_args: ['-D__TEST__', '-DDEBUG'])
test('kv-tests', kv_test)
