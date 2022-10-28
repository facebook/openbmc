project('libmisc-utils',
    'c', 'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'cpp_std=c++1z',
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40'
)

cc = meson.get_compiler('cpp')

headers = [
    'biview.hpp',
    'misc-utils.h',
    'profile.hpp',
]

sources = [
    'device-utils.c',
    'file-utils.c',
    'path-utils.c',
    'plat-utils.c',
    'str-utils.c',
]

deps = [
]

install_headers(
    headers,
    subdir: 'openbmc',
)

misc_lib = shared_library('misc-utils',
    sources,
    dependencies: deps,
    version: meson.project_version(),
    install: true,
)

pkg = import('pkgconfig')
pkg.generate(
    libraries: [ misc_lib ],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Misc Utils library'
)

test_deps = [
    cc.find_library('gtest'),
    cc.find_library('gtest_main'),
    dependency('threads'),
]

test_c_exe = executable('test-libmisc-utils',
    'test/main.c',
    'test/test-file.c',
    'test/test-path.c',
    'test/test-str.c',
    link_with: misc_lib,
    install: true,
    install_dir: get_option('libexecdir') + '/libmisc-utils/ptest/',
)

test_cpp_exe = executable('test-cpp-libmisc-utils',
    'test/test-biview.cpp',
    'test/test-profile.cpp',
    dependencies: test_deps,
    install: true,
    install_dir: get_option('libexecdir') + '/libmisc-utils/ptest/',
)

