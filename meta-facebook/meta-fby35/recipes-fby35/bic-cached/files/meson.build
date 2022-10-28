project('bic-cached', 'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'cpp_std=c++17',
        'warning_level=3',
        'werror=true',
    ],
)

deps = [
    dependency('libbic'),
    dependency('libfby35-common'),
    dependency('obmc-pal'),
]

bic_cached_exe = executable('bic-cached',
    'bic-cached.cpp',
    dependencies: deps,
    install: true,
)
