project('libkv', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers('kv.h', subdir: 'openbmc')

# KV library.
kv_lib = shared_library('kv', 'kv.c',
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
kv_test = executable('test-kv', 'kv.c', c_args: '-D__TEST__ -DDEBUG')
test('kv-tests', kv_test)
