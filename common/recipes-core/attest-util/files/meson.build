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
]

srcs = files(
    'main.cpp',
    'utils.cpp',
    'message_types/spdm.cpp',
)

attest_util_exe = executable(
    'attest-util', 
    srcs,
    dependencies: deps,
    install: true,
)
