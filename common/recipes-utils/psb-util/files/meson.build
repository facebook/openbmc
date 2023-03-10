project('psb-util', 'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'cpp_std=c++17',
        'warning_level=3',
        'werror=true',
    ],
)

cc = meson.get_compiler('cpp')
deps = [
    cc.find_library('bic'),
    cc.find_library('pal'),
]

apml_util_exe = executable('psb-util',
    'psb-util.cpp', 'psb-platform.cpp',
    dependencies: deps,
    install: true,
)

