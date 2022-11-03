project('log-util',
    'cpp',
    version: '0.2',
    license: 'GPL2',
    # Meson 0.40 only supports c++1z as an alias for c++17.
    default_options: [
        'cpp_std=c++1z',
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

cc = meson.get_compiler('cpp')

srcs = [
    'log-util.cpp',
    'rsyslogd.cpp',
    'selformat.cpp',
    'selstream.cpp',
]

deps = [
    dependency('obmc-pal'),
    dependency('threads'),
]

common_lib = static_library(
    'log-util',
    srcs,
    dependencies: deps,
    install: false
)

exe = executable(
    'log-util',
    'main.cpp',
    link_with: common_lib,
    dependencies: deps,
    install: true,
)

test_srcs = [
    'tests/test_logutil.cpp',
    'tests/test_rsyslogd.cpp',
    'tests/test_selformat.cpp',
    'tests/test_selstream.cpp',
]

test_deps = [
    cc.find_library('gmock'),
    cc.find_library('gtest'),
    cc.find_library('gtest_main'),
]

test_exe = executable(
    'test-log-util',
    test_srcs,
    link_with: common_lib,
    dependencies: [deps, test_deps],
)
test('log-util-tests', test_exe)
