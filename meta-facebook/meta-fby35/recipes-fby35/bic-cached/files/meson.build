project('bic-cached', 'cpp',
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
    cc.find_library('fby35_common'),
    dependency('libbic'),
    dependency('obmc-pal'),
]

bic_cached_exe = executable('bic-cached',
    'bic-cached.cpp',
    dependencies: deps,
    install: true,
)
