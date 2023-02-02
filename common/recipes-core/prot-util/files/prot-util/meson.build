project('prot-util', 'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'cpp_std=c++20',
        'werror=true'
    ]
)

srcs = files(
    'prot-util.cpp',
)

deps = [
    dependency('fmt'),
    dependency('libAmiSmbusInterface'),
    dependency('libprot'),
    dependency('obmc-pal'),
]

prot_util_exe = executable(
    'prot-util',
    srcs,
    dependencies: deps,
    install: true,
)
