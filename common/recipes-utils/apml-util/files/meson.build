project('apml-util', 'cpp',
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
    dependency('libbic'),
]

sources = [
    'apml-util.cpp',
]

subdir('plat')

apml_util_exe = executable('apml-util',
    sources,
    dependencies: deps,
    install: true,
)

