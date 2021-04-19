project('name-util', 'c', 'cpp',
    version: '0.1',
    license: 'GPL2',
    # Meson 0.40 only supports c++1z as an alias for c++17.
    default_options: [
        'cpp_std=c++1z',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

srcs = files(
    'name-util.cpp',
)

cc = meson.get_compiler('c')
deps = [
  dependency('threads'),
  cc.find_library('pal')
]

test_deps = [
  cc.find_library('gmock'),
  cc.find_library('gtest'),
  cc.find_library('gtest_main')
]

name_util_exe = executable(
    'name-util', 
    srcs,
    dependencies: deps,
    install: true,
    install_dir : 'local/bin'
)

name_util_test = executable('test-name-util', 'tests/name-test.cpp', srcs,
  dependencies:[deps, test_deps],
  cpp_args: ['-D__TEST__'],
  install_dir: 'lib/name-util/ptest')
test('name-util-tests', name_util_test)
