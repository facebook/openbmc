project('attest-util', 'c', 'cpp',
    version: '0.1',
    license: 'GPL2',
    # Meson 0.40 only supports c++1z as an alias for c++17.
    default_options: [
        'cpp_std=c++1z',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

cc = meson.get_compiler('c')
deps = [
  dependency('threads'),
]

srcs = files(
    'main.cpp',
    'message_types/spdm.cpp',
    'utils.cpp',
)

attest_util_exe = executable(
    'attest-util', 
    srcs,
    dependencies: deps,
    install: true,
)

# Test deps.
test_deps = [
  cc.find_library('gmock'),
  cc.find_library('gtest'),
  cc.find_library('gtest_main')
]

cppc = meson.get_compiler('cpp')

attest_util_test = executable('test-attest-util', 'tests/test_util.cpp', srcs,
  dependencies: [deps, test_deps],
  cpp_args: ['-D__TEST__'])
test('attest-util-tests', attest_util_test)
