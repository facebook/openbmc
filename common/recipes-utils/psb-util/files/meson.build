project('psb-util', 'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'cpp_std=c++17',
        'warning_level=3',
        'werror=true',
    ],
)

add_project_arguments('-Wno-psabi', language: 'cpp')

cc = meson.get_compiler('cpp')
deps = [
    dependency('libkv'),
    dependency('obmc-pal'),
    cc.find_library('bic'),
]

apml_util_exe = executable('psb-util',
    'psb-util.cpp', 'psb-platform.cpp',
    dependencies: deps,
    install: true,
)

